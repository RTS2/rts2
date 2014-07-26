/*
 * Barcode reader communication
 * Copyright (C) 2010 Markus Wildi, version for RTS2
 * Copyright (C) 2007 Lukas Zimmermann, Basel, Switzerland
 *
 * Documentation:
 * https://azug.minpet.unibas.ch/wikiobsvermes/index.php/Robotic_ObsVermes#Dome_Azimuth_Control
 * (yes it is https)
 *
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Or visit http://www.gnu.org/licenses/gpl.html.

 *
 *
 * TODO:
 *   - better analysation of reception data
 *   - protocol state machine
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <syslog.h>
#include <math.h>

#include <vermes/serial.h>
#include <vermes/util.h>
#include <vermes/barcodereader.h>

extern int barcodereader_state ;
extern double barcodereader_az ;
extern double barcodereader_dome_azimut_offset ;

//extern int debug;
int debug ; // check that 
#define ASCII_ACK         '\x06'
#define ASCII_NAK         '\x15'
#define ASCII_CR          '\r'
#define ASCII_LF          '\n'

// barcode reader commands
#define CMD_TRIGGER       "\x16T\r"
#define CMD_UNTRIGGER     "\x16U\r"
#define MENU_CMD_PREFIX   "\x16M\r"

// barcode reader config storage selectors
#define BC_STOR_VOLATILE     0
#define BC_STOR_PERMANENT    1

// barcode reader menu command tags/subtags
#define CMD_ADD_PREFIX       "PREBK2"
#define CMD_ALL_SYMBOLOGIES  "ALLENA"
#define CMD_SYMB_128         "128ENA"
#define CMD_BEEPER_VOL       "BEPLVL"
#define CMD_BEEPER           "BEPBIP"
#define CMD_TRIGGER_MODE     "TRGMOD"

// barcode reader comm protocol states
#define PSTAT_IDLE          0
#define PSTAT_TX_CMD_START  1
#define PSTAT_TX_CMD_SENT   2
#define PSTAT_RX_WAIT       3
#define PSTAT_RX_COMPLETE   4
#define PSTAT_TX_CMD_SEND_FAIL 5


PORT_STAT sport1_stat;
PORT_STAT sport2_stat;

pthread_t bcr_sending_th_id;
pthread_t bcr_receive_th_id;

POS_DETECT_STATE pos_detect_state;
POS_DETECT_STATE pos_detect_state_buffered;


POS_CHG_CBF *callback_function ; // wildi 1= NULL;

pthread_mutex_t pos_data_mutex;

extern char * log_string;
int syslog_opened ; //wildi = 0;
extern int sys_log_mask ; // wildi 1= 0;

/******************************************************************************
 * sys_setlogmask(...)
 * Sets the mask used by syslog() to decide whether a certain priority is
 * going to be logged (if preprocessor define USE_SYSLOG).
 *****************************************************************************/
void
sys_setlogmask(int mask)
{
  sys_log_mask = mask;
#if USE_SYSLOG
  int msk = (mask & ILOG_DEBUG)   ? LOG_MASK(LOG_DEBUG)   : 0 |
            (mask & ILOG_INFO)    ? LOG_MASK(LOG_INFO)    : 0 |
            (mask & ILOG_NOTICE)  ? LOG_MASK(LOG_NOTICE)  : 0 |
            (mask & ILOG_WARNING) ? LOG_MASK(LOG_WARNING) : 0 |
            (mask & ILOG_ERR)     ? LOG_MASK(LOG_ERR)     : 0 |
            (mask & ILOG_CRIT)    ? LOG_MASK(LOG_CRIT)    : 0 |
            (mask & ILOG_ALERT)   ? LOG_MASK(LOG_ALERT)   : 0 |
            (mask & ILOG_EMERG)   ? LOG_MASK(LOG_EMERG)   : 0;
  setlogmask(msk);
#endif
}

/******************************************************************************
 * sprintf_log(...)
 * Creates a logging string from the variadic arguments in the way of sprintf()
 * but allocates the memory for it if needed.
 *****************************************************************************/
char *
sprintf_log(char * str, char * format, va_list ap)
{
  va_list cp_ap;
  size_t new_len;
  size_t s_len =
    str == NULL ? 0 : sizeof(&str) / sizeof(char);

#ifdef __va_copy
  __va_copy (cp_ap, ap);
#else
  cp_ap = ap;
#endif
  /* check out whether the current string buffer size is sufficent */
  new_len = vsnprintf(str, s_len, format, ap);

  if (new_len >= s_len) {
    /* Reallocate buffer, now that we know how much space is needed. */
    new_len += 1;
    str = (char *) realloc(str, new_len);

    if (str != NULL) {
      /* print again. */
      vsnprintf(str, new_len, format, cp_ap);
    }
  }

  return str;
}


/******************************************************************************
 * cleanup(...)
 *****************************************************************************/
void
cleanup(void)
{
  if (sport1_stat.fd) {
    // wildi was shutdown_serial(sport1_stat.fd);
    serial_shutdown(sport1_stat.fd);
    sys_debug_log(3, "%s closed", sport1_stat.dev_name);
  }

  if (sport2_stat.fd) {
    // wildi was shutdown_serial(sport2_stat.fd);
    serial_shutdown(sport2_stat.fd);
    sys_debug_log(3, "%s closed", sport2_stat.dev_name);
  }
}

/******************************************************************************
 * init_sport(...)
 * Initializes the serial port and the PORT_STAT structure representing a
 * barcode reader.
 *****************************************************************************/
int
init_sport(PORT_STAT *sport_stat, int sd, char *dev_name)
{
  sport_stat->fd = sd;
  sport_stat->dev_name = dev_name;
  sport_stat->head = 0;
  sport_stat->tail = 0;
  sport_stat->rx_cnt = 0;
  sport_stat->rxstr[0] = '\0';
  pthread_mutex_init(&sport_stat->mutex_rx, NULL);
  sport_stat->protocol_stat = PSTAT_IDLE;
  return 1;
}

/******************************************************************************
 * sendCmd(...)
 * Sends the given string to the given open serial port, retries sending when
 * not completely done and returns with an error when send was not completed
 * after MAX_TX_RETRY retries or 0 when everything is sent.
 *****************************************************************************/
int
sendCmd(PORT_STAT *pstat, const char *data)
{
  int retval = 0;
  int len = strlen(data);
  int i = 0;
  int retry_cnt = 0;

  pthread_mutex_lock(&pstat->mutex_rx);
  pstat->protocol_stat = PSTAT_TX_CMD_START;
  pstat->rx_cnt = 0;
  pstat->tail = pstat->head;
  pstat->rxstr[0] = '\0';
  pthread_mutex_unlock(&pstat->mutex_rx);

  do {
    retval = write(pstat->fd, &(data[i]), len);
    if (retval != len && retval > 0) {
      //device output buffer full
      //wait some ms then try to send the remaining bytes
      sys_debug_log(3, "tx congestion on serial port %s.",
                        pstat->dev_name);
      millisleep(SAMPLE_MS);
      len -= retval;
      i += retval;
      retry_cnt++;
    }
  } while ((retval != len && retval > 0) && (retry_cnt < MAX_TX_RETRY));

  if  (retry_cnt > MAX_TX_RETRY) {
    sys_log(ILOG_ERR, "tx congestion on serial port %s after %i retries.",
                       pstat->dev_name, MAX_TX_RETRY);
    pstat->protocol_stat = PSTAT_TX_CMD_SEND_FAIL;
    return -1;
  }

  pstat->protocol_stat = PSTAT_TX_CMD_SENT;
  return 0;
}

/******************************************************************************
 * sendMenuCmd(...)
 * Composes a "Menu Command" for the barcode reader, assembled from
 * MENU_CMD_PREFIX, tag, subtag, data and storage type and sends it to the
 * barcode reader by calling sendCmd(...).
 * Returns the result of the sendCmd(...) call.
 *****************************************************************************/
int
sendMenuCmd(PORT_STAT *pstat, const char *tag, const char *subtag,
                              const char *data, int storage)
{
  size_t totlen = 5 + strlen(tag) + strlen(subtag) + strlen(data);
  sys_debug_log(4, "sendMenuCmd(): allocated %ld chars.", totlen);
  char *tx_str = malloc(totlen);
  tx_str[0] = '\0';
  strcat(tx_str, MENU_CMD_PREFIX);
  strcat(tx_str, tag);
  strcat(tx_str, subtag);
  strcat(tx_str, data);
  if (storage == BC_STOR_VOLATILE) {
    strcat(tx_str, "!");
  } else if (storage == BC_STOR_PERMANENT) {
    strcat(tx_str, ".");
  }
  //tx_str[totlen - 1] = '\0';
  if (strlen(tx_str) != totlen - 1) {
    sys_log(ILOG_ERR, "sendMenuCmd(): tx_str has length %ld instead of %ld.",
                       strlen(tx_str), totlen - 1);
  }
  if (debug > 4) {
    char *hx_str = ctrl2hex(tx_str);
    sys_debug_log(4, "sendMenuCmd(%s): sending \"%s\".",
                      pstat->dev_name, hx_str);
    free(hx_str);
  }
  int res = sendCmd(pstat, tx_str);
  free(tx_str);
  return res;
}

/******************************************************************************
 * check_rxed(...)
 * Checks contents of PORT_STAT's rx_ring buffer for a proper reply to a
 * command and clears the rx_ring buffer if it is not empty.
 * Returns -1 if buffer is empty.
 *         -2 if no ACK nor NAK character is found.
 *          0 expected reply string in rx_ring buffer.
 *****************************************************************************/
int
check_rxed(PORT_STAT *pstat)
{
  int cnt, i;
  char ack_char = ' ';
  pthread_mutex_lock(&pstat->mutex_rx);
  cnt = pstat->rx_cnt;
  if (cnt <= 0) {
    pthread_mutex_unlock(&pstat->mutex_rx);
    return -1;
  }

  // search for an ACK or a NAK character or for a string of 3 digits
  // terminated with cr/lf.
  int rx_idx = pstat->tail;
  int ack = 0;
  for (i = 0; i < cnt; i++) {
    pstat->rxstr[i] = pstat->rx_ring[rx_idx];
    if ((pstat->rx_ring[rx_idx] == ASCII_ACK)
         || (pstat->rx_ring[rx_idx] == ASCII_NAK))
    {
      pstat->rxstr[i + 1] = '\0';
      pstat->protocol_stat = PSTAT_RX_COMPLETE;
      ack_char = pstat->rx_ring[rx_idx];
      ack = 1;
      break;
    } else if (i == 4
               && pstat->rxstr[0] >= '0' && pstat->rxstr[0] <= '9'
               && pstat->rxstr[1] >= '0' && pstat->rxstr[1] <= '9'
               && pstat->rxstr[2] >= '0' && pstat->rxstr[2] <= '9'
               && pstat->rxstr[3] >= ASCII_CR && pstat->rxstr[4] <= ASCII_LF)
    {
      pstat->rxstr[3] = '\0';
      ack_char = ASCII_LF;
      ack = 1;
      break;
    }
    if (++rx_idx >= PSTAT_RINGBUF_SIZE) rx_idx = 0;
  } // for (i = 0; i < cnt; i++)
  if (debug > 3) {
    char *s;
    switch (ack_char) {
      case ASCII_ACK:
        s = "ACK";
        break;
      case ASCII_NAK:
        s = "NAK";
        break;
      case ASCII_LF:
        s = "CRLF";
        break;
      default:
        s = "";
    }
    sys_debug_log(3, "check_rxed(%s): %s<%s>",
                      pstat->dev_name, pstat->rxstr, s);
  }

  if (!ack) {
    pthread_mutex_unlock(&pstat->mutex_rx);
    return -2;
  }
  // got a complete valid response: clear the rx_ring buffer
  pstat->rx_cnt = 0;
  pstat->tail = pstat->head;
  pthread_mutex_unlock(&pstat->mutex_rx);

  return 0;
}


/******************************************************************************
 * init_barcodereader(...)
 * Initializes the barcode reader.
 *****************************************************************************/
#define MAX_REPT 5
#define CMD_WAIT_DELAY 30
int
init_barcodereader(PORT_STAT *sport_stat)
{
  int rx_stat, i;

  if (sendMenuCmd(sport_stat, CMD_ALL_SYMBOLOGIES, "",
                              "0", BC_STOR_VOLATILE))
  {
    sys_log(ILOG_ERR, "sendCmd(%s) to %s failed.",
                       CMD_ALL_SYMBOLOGIES, sport_stat->dev_name);
    return -1;
  }
  for (i = 0; i < MAX_REPT; i++) {
    millisleep(CMD_WAIT_DELAY);
    rx_stat = check_rxed(sport_stat);
    if (!rx_stat) break;
  }
  if (rx_stat) {
    sys_log(ILOG_ERR, "no good reply from check_rxed(%s): %d",
                       sport_stat->dev_name, rx_stat);
  }

  if (sendMenuCmd(sport_stat, CMD_SYMB_128, "", "1", BC_STOR_VOLATILE)) {
    sys_log(ILOG_ERR, "sendCmd(%s) to %s failed.",
                       CMD_SYMB_128, sport_stat->dev_name);
    return -1;
  }
  for (i = 0; i < MAX_REPT; i++) {
    millisleep(CMD_WAIT_DELAY);
    rx_stat = check_rxed(sport_stat);
    if (!rx_stat) break;
  }
  if (rx_stat) {
    sys_log(ILOG_ERR, "no good reply from check_rxed(%s): %d",
                       sport_stat->dev_name, rx_stat);
  }

  if (sendMenuCmd(sport_stat, CMD_BEEPER_VOL, "", "0", BC_STOR_VOLATILE)) {
    sys_log(ILOG_ERR, "sendCmd(%s) to %s failed.",
                       CMD_BEEPER_VOL, sport_stat->dev_name);
    return -1;
  }
  for (i = 0; i < MAX_REPT; i++) {
    millisleep(CMD_WAIT_DELAY);
    rx_stat = check_rxed(sport_stat);
    if (!rx_stat) break;
  }
  if (rx_stat) {
    sys_log(ILOG_ERR, "no good reply from check_rxed(%s): %d",
                       sport_stat->dev_name, rx_stat);
  }
  if (sendMenuCmd(sport_stat, CMD_BEEPER, "", "1", BC_STOR_VOLATILE)) {
    sys_log(ILOG_ERR, "sendCmd(%s) to %s failed.",
                       CMD_BEEPER, sport_stat->dev_name);
    return -1;
  }
  for (i = 0; i < MAX_REPT; i++) {
    millisleep(CMD_WAIT_DELAY);
    rx_stat = check_rxed(sport_stat);
    if (!rx_stat) break;
  }
  if (rx_stat) {
    sys_log(ILOG_ERR, "no good reply from check_rxed(%s): %d",
                       sport_stat->dev_name, rx_stat);
  }

  if (sendMenuCmd(sport_stat, CMD_TRIGGER_MODE, "", "0", BC_STOR_VOLATILE)) {
    sys_log(ILOG_ERR, "sendCmd(%s) to %s failed.",
                       CMD_TRIGGER_MODE, sport_stat->dev_name);
    return -1;
  }
  for (i = 0; i < MAX_REPT; i++) {
    millisleep(CMD_WAIT_DELAY);
    rx_stat = check_rxed(sport_stat);
    if (!rx_stat) break;
  }
  if (rx_stat) {
    sys_log(ILOG_ERR, "no good reply from check_rxed(%s): %d",
                       sport_stat->dev_name, rx_stat);
  }

  sys_debug_log(3, "barcode reader on %s initialized.",
                    sport_stat->dev_name);

  return 0;
}

/******************************************************************************
 * handle_rx(...)
 * Enters the characters received by the receive_thread into the ring buffer of
 * the PORT_STAT structure representing a barcode reader.
 * Access to PORT_STAT structure is protected by a mutex.
 * Returns 0 if all went fine, -1 when a overflow of the ring buffer occured.
 *****************************************************************************/
int
handle_rx(PORT_STAT *pstat, char *data, size_t count)
{
  size_t i;
  pthread_mutex_lock(&pstat->mutex_rx);
  for (i = 0; i < count; i++) {
    if (pstat->rx_cnt < PSTAT_RINGBUF_SIZE) {
      pstat->rx_ring[pstat->head++] = data[i];
      //pstat->head++;
      if (pstat->head >= PSTAT_RINGBUF_SIZE) pstat->head = 0;
      pstat->rx_cnt++;
      /*
      if ((data[i] == ASCII_ACK) || (data[i] == ASCII_NAK)) {
        pstat->protocol_stat = PSTAT_RX_COMPLETE;
      }
      */
    } else {
      pthread_mutex_unlock(&pstat->mutex_rx);
      return -1;
    }
  }
  if (debug > 4) {
    data[count] = '\0';
    char *hx_data = ctrl2hex(data);

    sys_debug_log(4, "handle_rx(%s): %d char in ringbuf: \"%s\".",
                      pstat->dev_name, pstat->rx_cnt, hx_data);
    free(hx_data);
  }
  pthread_mutex_unlock(&pstat->mutex_rx);
  return 0;
}

/******************************************************************************
 * readerCodesToPosition(...)
 * Returns a position value in the range from 0 to 4 * HIGHEST_BARCODE or a
 * negative value indicating an error, where
 * -1 means that none of the reader position values are valid and
 * -2 means that the values of reader1 and reader2 are not in sequence.
 *****************************************************************************/
int
readerCodesToPosition(int rdr1_pos, int rdr2_pos)
{
  int comb_pos;
  if (rdr1_pos >= 0) {
    // valid reading on reader 1
    comb_pos = 4 * rdr1_pos;
    if (rdr2_pos >= 0) {
      // valid reading on reader 2
      if ((rdr2_pos != rdr1_pos)
          && (rdr2_pos != rdr1_pos - 1)
          && !((rdr1_pos == 0) && (rdr2_pos == HIGHEST_BARCODE)))
      {
        // reader2 reading is not equal nor 1 smaller than reader1
        // nor (reader1 has 0 and reader2 has HIGHEST_BARCODE)
        comb_pos = -2;
      } else if (rdr2_pos == rdr1_pos) {
        comb_pos += 2;
      }
    } else {
      comb_pos += 1;
    }
  } else if (rdr2_pos >= 0) {
    // valid reading on reader 2 only
    comb_pos = 4 * rdr2_pos + 3;

  } else {
    // neither reader1 nor reader2 detected a barcode
    comb_pos = -1;
  }
  return comb_pos;
}

/******************************************************************************
 * bcr_receive_thread()
 * This function is started as a thread. It waits in a select() for input from
 * the two serial interfaces where the barcode readers are connected.
 * When any of the barcode reader sent some data, the received characters are
 * entered into a ringbuffer which is a member of the PORT_STAT structure
 * representing the barcode reader.
 *****************************************************************************/
void * bcr_receive_thread(void* args)
{
  fd_set rfds;
  int ready_fd;
  char rxbuf[100];
  int read_stat;
  int res;

  while (1) {
    FD_ZERO(&rfds);
    FD_SET(sport1_stat.fd, &rfds);    // add serial port 1 file descriptor to set
    FD_SET(sport2_stat.fd, &rfds);    // add serial port 2 file descriptor to set
    //ready_fd = select(FD_SETSIZE, &rfds, NULL, NULL, &timeout);
    ready_fd = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
    if (ready_fd < 0) {
      /* select returns an error */
      sys_log(ILOG_ERR, "select(): %s", strerror(errno));
    } else if (ready_fd == 0) {
      /* select timed out */
      sys_log(ILOG_WARNING, "timeout in select()");
    } else {

      if (FD_ISSET(sport1_stat.fd, &rfds)) {
        if ((read_stat = read(sport1_stat.fd, &rxbuf, 100)) < 0) {
          /* there was an error while reading */
          sys_log(ILOG_ERR, "error while reading %s: %s"
                            ,sport1_stat.dev_name, strerror(errno));
        } else if (read_stat == 0) {
          /* no character to read, empty buffer */
        } else {
          /* character successfully read */
          rxbuf[read_stat] = '\0';
          sys_debug_log(4, "RXed %d chars on %s: %s",
                            read_stat, sport1_stat.dev_name, rxbuf);
          if ((res = handle_rx(&sport1_stat, rxbuf, read_stat))) {
            sys_log(ILOG_ERR, "rx buffer overflow on port %s",
                               sport1_stat.dev_name);
          } else {
            sys_debug_log(4, "handle_rx(%s) returned %d",
                              sport1_stat.dev_name, res);
          }
        }
      }

      if (FD_ISSET(sport2_stat.fd, &rfds)) {
        if ((read_stat = read(sport2_stat.fd, &rxbuf, 100)) < 0) {
          /* there was an error while reading */
          sys_log(ILOG_ERR, "error while reading %s: %s",
                             sport2_stat.dev_name, strerror(errno));
        } else if (read_stat == 0) {
          /* no character read, empty buffer */
        } else {
          /* character successfully read */
          rxbuf[read_stat] = '\0';
          if (debug > 4) {
            sys_debug_log(4, "RXed %d chars on %s: %s",
                              read_stat, sport2_stat.dev_name, rxbuf);
          }
          if ((res = handle_rx(&sport2_stat, rxbuf, read_stat))) {
            sys_log(ILOG_ERR, "rx buffer overflow on port %s",
                               sport2_stat.dev_name);
          } else {
            sys_debug_log(4, "handle_rx(%s) returned %d",
                              sport2_stat.dev_name, res);
          }
        }
      }
    }
  } /* while (1) */

  sys_debug_log(3, "exiting receive_thread()");

  return NULL;
}

/******************************************************************************
 * init_pos_detect_state(...)
 * Initialize the contents of the POS_DETECT_STATE structure at the location
 * given as argument.
 *****************************************************************************/
void
init_pos_detect_state(POS_DETECT_STATE * pos_state)
{
  pthread_mutex_lock(&pos_data_mutex);
    pos_state->current_pos         = 0;
    pos_state->last_current_pos    = 0;
    pos_state->pos_changed         = 0;
    pos_state->azimut_propstate_changed = 0;
    pos_state->pos_invalid         = 1;
  pthread_mutex_unlock(&pos_data_mutex);
}

/******************************************************************************
 * get_pos_detect_state()
 * Copy the contents "pos_detect_state" structure to the location given as
 * argument.
 *****************************************************************************/
void
get_pos_detect_state(POS_DETECT_STATE * pos_state)
{
  pthread_mutex_lock(&pos_data_mutex);
    pos_state->current_pos         = pos_detect_state.current_pos;
    pos_state->last_current_pos    = pos_detect_state.last_current_pos;
    pos_state->pos_changed         = pos_detect_state.pos_changed;
    pos_state->azimut_propstate_changed
                                   = pos_detect_state.azimut_propstate_changed;
    pos_state->pos_invalid         = pos_detect_state.pos_invalid;
  pthread_mutex_unlock(&pos_data_mutex);
}

/******************************************************************************
 * bcr_sending_thread(...)
 * Thread running in an endless loop, sending trigger commands to the two
 * barcode readers. If after a fixed delay no barcode reading is received
 * the barcode reader gets sent an untrigger command. Otherwise the barcode
 * read results of the two readers are combined to a position reading.
 * The position reading, the reading validity, and change of both are stored
 * to a global structure "pos_detect_state".
 * If position or validity state changed, the "pos_detect_state" structure
 * gets copied and handed to indi driver via callback function (if set).
 * TODO:
 *   only call callback when position or validity changed.
 *****************************************************************************/
void * bcr_sending_thread(void* args)
{
  int rx1_stat = 0;
  int rx2_stat = 0;
  int rdr1_pos;
  int rdr2_pos;
  int bc_pos;
  int do_call_callback;

  while (1) {
    // sending trigger command to both readers
    if (sendCmd(&sport1_stat, CMD_TRIGGER)) {
      sys_log(ILOG_ERR, "sendCmd(%s) failed.", CMD_TRIGGER);
    }
    if (sendCmd(&sport2_stat, CMD_TRIGGER)) {
      sys_log(ILOG_ERR, "sendCmd(%s) failed.", CMD_TRIGGER);
    }

    // give time for response
    millisleep(90);

    // check whether reader 1 detected a valid barcode, send
    // untrigger command if not.
    rx1_stat = check_rxed(&sport1_stat);
    if (rx1_stat != 0) {
      if (sendCmd(&sport1_stat, CMD_UNTRIGGER)) {
        sys_log(ILOG_ERR, "sendCmd(%s) failed.", CMD_UNTRIGGER);
      }
      rdr1_pos = -1;
    } else {
      sys_debug_log(4, "%s: %s",
                        sport1_stat.dev_name, sport1_stat.rxstr);
      sscanf(sport1_stat.rxstr, "%d", &rdr1_pos);
    }

    // check whether reader 2 detected a valid barcode, send
    // untrigger command if not.
    rx2_stat = check_rxed(&sport2_stat);
    if (rx2_stat != 0) {
      if (sendCmd(&sport2_stat, CMD_UNTRIGGER)) {
        sys_log(ILOG_ERR, "sendCmd(%s) failed.", CMD_UNTRIGGER);
      }
      rdr2_pos = -1;
    } else {
      sys_debug_log(4, "%s: %s", sport2_stat.dev_name, sport2_stat.rxstr);
      sscanf(sport2_stat.rxstr, "%d", &rdr2_pos);
    }

    // create an azimut value from the combination of the detections of
    // both barcode readers.
    pos_detect_state.pos_changed = 0;
    pos_detect_state.azimut_propstate_changed = 0;
    bc_pos = readerCodesToPosition(rdr1_pos, rdr2_pos);
    if (bc_pos >= 0) {
      sys_debug_log(3, "barcode combined reading: %d", bc_pos);
    }
    pthread_mutex_lock(&pos_data_mutex);
    if (bc_pos >= 0) {
      // Got a valid position code reading
      if (pos_detect_state.pos_invalid) {
        // last position reading was invalid
        pos_detect_state.azimut_propstate_changed = 1;
      }
      if (bc_pos != pos_detect_state.current_pos) {
        // position reading changed since last one
        pos_detect_state.last_current_pos = pos_detect_state.current_pos;
        pos_detect_state.current_pos = bc_pos;
        pos_detect_state.pos_changed = 1;
      }
      pos_detect_state.pos_invalid = 0;

    } else { // if (bc_pos >= 0)
      // NO valid position code reading
      sys_debug_log(3,
               "bcr_sending_thread(): no valid reading (%d), previous %s",
               bc_pos, pos_detect_state.pos_invalid ? "also" : "was valid");
      if (!pos_detect_state.pos_invalid) {
        // position code reading was valid before
        pos_detect_state.azimut_propstate_changed = 1;

      } else {
        if (pos_detect_state.pos_invalid != bc_pos) {
          pos_detect_state.azimut_propstate_changed = 1;
        }
      }
      pos_detect_state.pos_invalid = bc_pos;

    } // else, if (bc_pos >= 0)
    do_call_callback = pos_detect_state.azimut_propstate_changed || 
                       pos_detect_state.pos_changed;
    pthread_mutex_unlock(&pos_data_mutex);

    if (bc_pos < 0) {
      if (bc_pos == -1) {
        sys_log(ILOG_WARNING, "no barcode decoded");
      } else if (bc_pos == -2) {
        sys_log(ILOG_WARNING, "wrong barcode sequence");
      } else {
        sys_log(ILOG_ERR,
                 "barcode detection failed, unexpected return: %d", bc_pos);
      }
    }

    if (callback_function && do_call_callback) {
      get_pos_detect_state(&pos_detect_state_buffered);
      sys_debug_log(3, "buffered current_pos = %d",
                        pos_detect_state_buffered.current_pos);
      (*callback_function)(&pos_detect_state_buffered);
    }

    millisleep(10);
  } // while (keep_going)

  return NULL;
}

/******************************************************************************
 * start_bcr_comm()
 * Sets up the serial ports to communicate with barcode readers, initializes
 * the barcode readers and creates two threads to handle the serial
 * communication.
 *****************************************************************************/
int start_bcr_comm()
{
  char *SerialDev1 = DEFAULT_BCR_SERPORT_DEV1;
  char *SerialDev2 = DEFAULT_BCR_SERPORT_DEV2;
  int sd1;         // file descriptor for 1st serial device
  int sd2;         // file descriptor for 2nd serial device

  int bitrate  = DEFAULT_BCR_BITRATE;
  int databits = DEFAULT_BCR_DATABITS;
  int parity   = DEFAULT_BCR_PARITY;
  int stopbits = DEFAULT_BCR_STOPBITS;

  int res = 0;

  sys_debug_log(1, "Barcode reader 1 on %s, barcode reader 2 on %s",
                    SerialDev1, SerialDev2);

  // open serial ports
  // wildi if ((sd1 = init_serial(SerialDev1, bitrate, databits, parity, stopbits)) < 0)
  if ((sd1 = serial_init(SerialDev1, bitrate, databits, parity, stopbits)) < 0)
  {
    sys_log(ILOG_ERR, "open of %s failed.", SerialDev1);
    return -1;
  }
  init_sport(&sport1_stat, sd1, SerialDev1);

  //wildi if ((sd2 = init_serial(SerialDev2, bitrate, databits, parity, stopbits)) < 0)
  if ((sd2 = serial_init(SerialDev2, bitrate, databits, parity, stopbits)) < 0)
  {
    sys_log(ILOG_ERR, "open of %s failed.", SerialDev2);
    // wildi was shutdown_serial(sd1);
    serial_shutdown(sd1);
    return -2;
  }
  init_sport(&sport2_stat, sd2, SerialDev2);

  pthread_mutex_init(&pos_data_mutex, NULL);

  // create receive communication thread
  pthread_create(&bcr_receive_th_id, NULL, &bcr_receive_thread, NULL);

  // initialize/setup barcode readers
  init_barcodereader(&sport1_stat);
  init_barcodereader(&sport2_stat);

  millisleep(200);

  init_pos_detect_state(&pos_detect_state);



  openlog ("rts2 threads", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
     
  syslog (LOG_NOTICE, "Program started by User %d", getuid ());


  // create communication thread which talks to barcode readers
  int ret= pthread_create(&bcr_sending_th_id, NULL, &bcr_sending_thread, NULL);

  syslog (LOG_NOTICE, "State was  %d", ret);
  return res;
}

/******************************************************************************
 * stop_bcr_comm()
 * Stop serial receive and send threads and close serial ports.
 *****************************************************************************/
int
stop_bcr_comm()
{
  int res = 0;

  pthread_cancel(bcr_sending_th_id);
  pthread_cancel(bcr_receive_th_id);

  if (sendCmd(&sport1_stat, CMD_UNTRIGGER)) {
    sys_log(ILOG_ERR, "sendCmd(%s) failed.", CMD_UNTRIGGER);
  }
  if (sendCmd(&sport2_stat, CMD_UNTRIGGER)) {
    sys_log(ILOG_ERR, "sendCmd(%s) failed.", CMD_UNTRIGGER);
  }
  cleanup();

  return res;
}

/******************************************************************************
 * register_pos_change_callback(...)
 * The function given as argument gets called whenever the dome azimut
 * property changes.
 *****************************************************************************/
void
register_pos_change_callback(POS_CHG_CBF * callback_func)
{
  callback_function = callback_func;
}

/******************************************************************************
 * position_update_callback(...)
 *****************************************************************************/
void
position_update_callback(POS_DETECT_STATE * pos_detct_state)
{

  int pos = pos_detct_state->current_pos;
  sys_debug_log(4, "position_update_callback() called.");

  if (pos_detct_state->pos_changed) {

    barcodereader_state = pos_detct_state->pos_invalid ;
    barcodereader_az = 360.0 / (4.0 * (HIGHEST_BARCODE + 1.0)) * pos + barcodereader_dome_azimut_offset;

    if( barcodereader_az < 0. ) barcodereader_az += 360. ;

    barcodereader_az= fmod( barcodereader_az, 360.0) ;
    sys_debug_log(1, "position_update_callback(): value changed to %3.1f", barcodereader_az);
  } else if (pos_detct_state->azimut_propstate_changed) {
    barcodereader_state = pos_detct_state->pos_invalid ;
    sys_log(1, "position_update_callback(): state changed to %s", barcodereader_state  ? "ALERT" : "OK");
  }
}
