/*
 *
 * SSD650v inverter communication
 * Copyright (C) 2010 Markus Wildi, version for RTS2
 * Copyright (C) 2009 Lukas Zimmermann, Basel, Switzerland
 *
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
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>

#include <vermes/bisync.h>
#include <vermes/util.h>
#include <vermes/serial.h>

/* the control characters relevant in the ei bisync protocol */
#define ASCII_STX '\x02'
#define ASCII_ETX '\x03'
#define ASCII_EOT '\x04'
#define ASCII_ENQ '\x05'
#define ASCII_ACK '\x06'
#define ASCII_NAK '\x15'

#define DEVICE_RESPONSE_TIMEOUT 160    /* SSD650V should reply within 160ms  */
//#define DEVICE_RESPONSE_TIMEOUT 999
#define MAX_REREAD_TRIES 3             /* upto 3 times a read from the serial
                                        * is done when an incomplete reply from
                                        * SSD650V is received */
#define ENQ_CONNECTED_TO 7             /* the `connected' state of the SSD650V
                                        * seems to timeout after 10s */
#define FRAME_BUF_SIZE 32              /* the SSD650V frames mustn't and don't
                                        * exceed this size */


enum BISYNC_PROT_ST {                  /* the states of the ei bisync protocol
                                        * machine */
  BISYNC_PS_UNDEF,
  BISYNC_PS_BASIC,
  BISYNC_PS_ENQ_CONNECTED,
  BISYNC_PS_ENQ_RPLY_STARTED,
  BISYNC_PS_ENQ_RPLY_ETX,
  BISYNC_PS_ENQ_RPLY_BCC,
  BISYNC_PS_SETPARAM_CONNECTED
};

enum BISYNC_PROT_ST bisync_pstate = BISYNC_PS_UNDEF;

struct timeval time_last_enq = { 0, 0 }; /* time of last invoked message
                                          * exchange with SSD650V */
extern int debug;

/******************************************************************************
 * bisync_calc_bcc(...)
 * Calculates and returns the BCC (Block Check Character) of the given byte
 * buffer.
 *****************************************************************************/
byte
bisync_calc_bcc(const byte *buf, int len)
{
  byte bcc = 0;
  int i;

  for (i = 0; i < len; i++) {
    bcc ^= buf[i];
  }

  return bcc;
}

/******************************************************************************
 * bisync_check_bcc(...)
 * Takes a string and compares the calculated BCC (Block Check Character) with
 * the given one. When calculated and given BCC are equal, -1 is returned,
 * otherwise the calculated BCC is returned.
 *****************************************************************************/
int
bisync_check_bcc(const byte *buf, int len, byte bcc)
{
  byte bcc_calc = bisync_calc_bcc(buf, len);

  if (debug > 3) {
    fprintf(stderr, "Checking bcc starting with 0x%.2x: (%d bytes).\n",
                    buf[0], len);
  }

  if (bcc != bcc_calc) {
    return bcc_calc;

  } else {
    return -1;
  }
}

/******************************************************************************
 * bisync_errstr(..)
 *****************************************************************************/
char *
bisync_errstr(int err) {
  switch (err) {
    case BISYNC_OK:
      return "no error";
      break;
    case BISYNC_ERR:
      return "inspecific error";
      break;
    case BISYNC_ERR_READ:
      return "error while reading";
      break;
    case BISYNC_ERR_WRITE:
      return "error while writing";
      break;
    case BISYNC_ERR_SELECT:
      return "error in select()";
      break;
    case BISYNC_TIME_OUT:
      return "timeout";
      break;
    case BISYNC_ERR_EOT:
      return "rxed unexpected EOT";
      break;
    case BISYNC_ERR_BCC:
      return "block check character mismatch";
      break;
    case BISYNC_ERR_FRAME:
      return "error in framing";
      break;
    case BISYNC_ERR_NAK:
      return "rxed NAK";
      break;
    case BISYNC_ERR_FRAME_INCMPL:
      return "incomplete frame";
      break;
    case BISYNC_ERR_STX:
      return "no frame start found";
      break;
    default:
      return "undefined error";
  }
}

/******************************************************************************
 * bisync_init(..)
 *****************************************************************************/
void
bisync_init()
{
  bisync_pstate = BISYNC_PS_UNDEF;
}

/******************************************************************************
 * bisync_read_frame(..)
 * Reads a max of FRAME_BUF_SIZE bytes from serial port into a newly allocated
 * buffer. If no complete and correctly framed reply has arrived yet, a 2nd 
 * or even more tries to read more bytes are made after a short delay.
 * Return:
 *   return BISYNC_OK, if complete and correctly framed reply could be read
 *     and return pointer to allocated frame buffer in `frame' argument.
 *     (don't forget to free() the buffer when not needed anymore)
 *   BISYNC_ERROR error code otherwise.
 *****************************************************************************/
int
bisync_read_frame(int sd, char ** frame)
{
  int bytes_read;
  int bytes_tot = 0;
  int scan_pos = 0;
  byte * buf;
  int bcc_check;
  int read_rpt_cnt;
  char * c2h_buf;

  buf = (byte *)realloc(NULL, FRAME_BUF_SIZE + 1);
  *frame = NULL;
  for (read_rpt_cnt = 0; ; read_rpt_cnt++) {
    bytes_read = read(sd, &buf[bytes_tot], FRAME_BUF_SIZE - bytes_tot);
    if (bytes_read < 0 ) {
      if (debug > 3) {
        buf[bytes_tot] = '\0';
        c2h_buf = ctrl2hex((char *)buf);
        fprintf(stderr, "Read %d bytes from SSD650V upto here, but now "
                        "read() returned %d: %s\n",
                        bytes_tot, bytes_read, strerror(errno));
        free(c2h_buf);
      }
      free(buf);
      if (errno == EAGAIN) {
        /* TODO:
         *   Think on possible cases where this might occur and what could be
         *   done to get a more robust behaviour. Maybe instead of sleeping at
         *   the end of the outter for-loop it would be better to do a
         *   select() call. */
        /* It seems that a return value < 0 from read() for the case of no
         * characters are ready for reading only occurs when the terminal had
         * been opened with O_NONBLOCK flag. Without O_NONBLOCK and with
         * termios.c_cc[VMIN] and termios.c_cc[VTIME] both of 0, read()
         * should return immediately and return 0 when no characters are
         * ready to be read. */
        if (read_rpt_cnt > MAX_REREAD_TRIES) {
          free(buf);
          return BISYNC_ERR_FRAME_INCMPL;
        }
        millisleep(10);
      } else {
        return BISYNC_ERR_READ;
      }

    } else if (bytes_read == 0) {
      /* nothing to be read upto now, wait and retry */
      if (read_rpt_cnt > MAX_REREAD_TRIES) {
        if (debug > 1) {
          buf[bytes_tot] = '\0';
          c2h_buf = ctrl2hex((char *)buf);
          fprintf(stderr, "bisync_read_frame() rpt > %d: \"%s\"\n", 3, c2h_buf);
          free(c2h_buf);
        }
        free(buf);
        return BISYNC_ERR_FRAME_INCMPL;
      }
      millisleep(10);
    } else {
      /* got some new characters, going to scan them */
      bytes_tot += bytes_read;
    }

    while (scan_pos < bytes_tot) {
      switch (bisync_pstate) {
        case BISYNC_PS_ENQ_CONNECTED:
          /* look for ASCII_STX */
          if (buf[scan_pos] == ASCII_STX) {
            bisync_pstate = BISYNC_PS_ENQ_RPLY_STARTED;
            scan_pos++;
          } else if (buf[scan_pos] == ASCII_EOT) {
            /* device replied with ASCII_EOT: discard everything from the current
             * frame and return to give the chance to restart a message transfer */
            bisync_pstate = BISYNC_PS_UNDEF;
            tcflush(sd, TCIOFLUSH); /* clear all buffers */
            free(buf);
            return BISYNC_ERR_EOT;
          } else {
            bisync_pstate = BISYNC_PS_UNDEF;
            free(buf);
            return BISYNC_ERR_STX;
          }
          break;
        case BISYNC_PS_ENQ_RPLY_STARTED:
          /* look for ASCII_ETX */
          if (buf[scan_pos] == ASCII_ETX) {
            bisync_pstate = BISYNC_PS_ENQ_RPLY_ETX;
          } else if (buf[scan_pos] == ASCII_EOT) {
            /* device replied with ASCII_EOT: discard everything from the current
             * frame and return to give the chance to restart a message transfer */
            bisync_pstate = BISYNC_PS_UNDEF;
            tcflush(sd, TCIOFLUSH); /* clear all buffers */
            free(buf);
            return BISYNC_ERR_EOT;
          }
          scan_pos++;
          break;
        case BISYNC_PS_ENQ_RPLY_ETX:
          /* look for BCC */
          /* a complete response frame was received from SSD device */
          /* check BCC not including ASCII_STX upto and including ASCII_ETX */
          bcc_check = bisync_check_bcc(&buf[1], scan_pos - 1, buf[scan_pos]);
          buf[scan_pos + 1] = 0;
          if (bcc_check >= 0) {
            fprintf(stderr, "bisync_read_frame: bcc check failed "
                            "calculated: 0x%2x, received: 0x%2x)\n",
                             bcc_check, buf[scan_pos]);
            bisync_pstate = BISYNC_PS_UNDEF;
            free(buf);
            return BISYNC_ERR_BCC;
          } else {
            buf[scan_pos - 1] = 0;
            bisync_pstate = BISYNC_PS_ENQ_CONNECTED;
            *frame = (char *)&buf[0];
            return BISYNC_OK;
          }
          break;
        default:
          free(buf);
          return BISYNC_ERR;
      } // switch (bisync_pstate)
    } // while (scan_pos < bytes_tot)

    /* not a complete response frame received yet! */
    /* as soon as a reply from the SSD device is started, it should take */
    /* less than 10ms to be completely transmit, so wait that time, then */
    /* read more. */
    millisleep(10);
  } // for (;; read_rpt_cnt++)

  bisync_pstate = BISYNC_PS_UNDEF;
  free(buf);
  if (debug > 1) {
    buf[bytes_tot] = '\0';
    c2h_buf = ctrl2hex((char *)buf);
    fprintf(stderr, "bisync_read_frame(): \"%s\"\n", c2h_buf);
    free(c2h_buf);
  }
  return BISYNC_ERR_FRAME_INCMPL;
}


/******************************************************************************
 * bisync_enquiry(...)
 *****************************************************************************/
int
bisync_enquiry(int sd, byte group_id, byte unit_id, char * cmd,
               char ** response_frame, enum ENQUIRY_STYLE style)
{
  int i = 0;
  byte send_buf[16];
  char * resp_frame = NULL;

  /* It seems that the "BISYNC_ENQUIRY_CONNECTED" state of the SSD650V
   * timeouts after a while so we make bookkeeping on the times of last
   * enquiry and change to BISYNC_PS_UNDEF if it's too long since. */
  struct timeval time_enq;
  gettimeofday(&time_enq, NULL);
  if (difftime(time_enq.tv_sec, time_last_enq.tv_sec) > ENQ_CONNECTED_TO) {
    bisync_pstate = BISYNC_PS_UNDEF;
  }
  time_last_enq = time_enq;

  if ((bisync_pstate == BISYNC_PS_UNDEF) ||
      (bisync_pstate == BISYNC_PS_SETPARAM_CONNECTED))
  {
    send_buf[i] = ASCII_EOT;
    i++;
  }
  group_id += '0';
  unit_id += '0';
  send_buf[i] = group_id;
  i++;
  send_buf[i] = group_id;
  i++;
  send_buf[i] = unit_id;
  i++;
  send_buf[i] = unit_id;
  i++;
  send_buf[i] = (byte)cmd[0];
  i++;
  send_buf[i] = (byte)cmd[1];
  i++;
  send_buf[i] = ASCII_ENQ;
  i++;
  send_buf[i] = 0;
  //bisync_p_state = BISYNC_ENQUIRY_CONNECTED;
  bisync_pstate = BISYNC_PS_ENQ_CONNECTED;
  tcflush(sd, TCIOFLUSH); /* clear all buffers */
  int r = serial_write(sd, send_buf, i);
  if (r != TTY_OK) {
    //bisync_p_state = BISYNC_UNDEF;
    bisync_pstate = BISYNC_PS_UNDEF;
    return BISYNC_ERR_WRITE;
  }
  char * c2h_buf;
  if (debug > 1) {
    c2h_buf = ctrl2hex((char *)send_buf);
    fprintf(stderr, "Sending enquiry \"%s\" to SSD650V\n", c2h_buf);
    free(c2h_buf);
    if (debug > 3) {
      fprintf(stderr, "freed memory for hexed send buffer.\n");
    }
  }

  r = serial_timeout(sd, DEVICE_RESPONSE_TIMEOUT);
  switch (r) {
    case TTY_TIME_OUT:
      serial_write_byte(sd, ASCII_EOT);
      //bisync_p_state = BISYNC_BASIC_STATE;
      bisync_pstate = BISYNC_PS_BASIC;
      tcflush(sd, TCIOFLUSH); /* clear all buffers */
      return BISYNC_TIME_OUT;
      break;

    case TTY_SELECT_ERROR:
      serial_write_byte(sd, ASCII_EOT);
      //bisync_p_state = BISYNC_BASIC_STATE;
      bisync_pstate = BISYNC_PS_BASIC;
      tcflush(sd, TCIOFLUSH); /* clear all buffers */
      return BISYNC_ERR_SELECT;
      break;

    case TTY_OK:
      /* TODO: SSD650v could reply with an ASCII_EOT */
      r = bisync_read_frame(sd, &resp_frame);
      if ((debug > 1) && (r != BISYNC_OK)) {
        fprintf(stderr, "bisync_read_frame() returned error (%d: %s)\n",
                        r, bisync_errstr(r));
      }
      if (r == BISYNC_OK) {
        *response_frame = resp_frame;
        if (debug > 3) {
          c2h_buf = ctrl2hex((char *)resp_frame);
          fprintf(stderr, "Got reply \"%s\" from SSD650V\n", c2h_buf);
          free(c2h_buf);
        }
        switch (style) {
          case ENQUIRY_SINGLE:
            r = serial_write_byte(sd, ASCII_EOT);
            //bisync_p_state = BISYNC_BASIC_STATE;
            bisync_pstate = BISYNC_PS_BASIC;
            return r;
            break;

          /* TODO:
           *   correct implementation of read same and read next parameter
           *   protocol handling: we should not reply at all until next call
           *   to this function. */
          case ENQUIRY_SAME:
            //r = serial_write_byte(sd, ASCII_NAK);
            //bisync_p_state = BISYNC_UNDEF;
            bisync_pstate = BISYNC_PS_ENQ_CONNECTED;
            return BISYNC_ERR;
            break;
          case ENQUIRY_NEXT:
            //r = serial_write_byte(sd, ASCII_ACK);
            //bisync_p_state = BISYNC_UNDEF;
            bisync_pstate = BISYNC_PS_ENQ_CONNECTED;
            return BISYNC_ERR;
            break;
          default:
            fprintf(stderr, "bisync_enquiry(): bad ENQUIRY_STYLE parameter "
                            "%d.\n", style);
            //bisync_p_state = BISYNC_UNDEF;
            bisync_pstate = BISYNC_PS_UNDEF;
            return BISYNC_ERR;
        } // switch (style)
      } else {
        //bisync_p_state = BISYNC_UNDEF;
        bisync_pstate = BISYNC_PS_UNDEF;
        return r;  
      }
      break;

    default:
      fprintf(stderr, "bisync_enquiry: internal error.\n");
      //bisync_p_state = BISYNC_UNDEF;
      bisync_pstate = BISYNC_PS_UNDEF;
      return   BISYNC_ERR;
  } // switch (r)
}

/******************************************************************************
 * bisync_setparam(...)
 *****************************************************************************/
int
bisync_setparam(int sd, byte group_id, byte unit_id, char * cmd, char * data,
                enum SETPARAM_STYLE style)
{
  int i = 0;
  int r;
  byte send_buf[20];

  /* It seems that the "BISYNC_ENQUIRY_CONNECTED" state of the SSD650V
   * timeouts after a while so we make bookkeeping on the times of last
   * enquiry and change to BISYNC_PS_UNDEF if it's too long since. */
  struct timeval time_enq;
  gettimeofday(&time_enq, NULL);
  if (difftime(time_enq.tv_sec, time_last_enq.tv_sec) > ENQ_CONNECTED_TO) {
    bisync_pstate = BISYNC_PS_UNDEF;
  }
  time_last_enq = time_enq;

  if ((bisync_pstate == BISYNC_PS_UNDEF) ||
      (bisync_pstate == BISYNC_PS_BASIC) ||
      (bisync_pstate == BISYNC_PS_ENQ_CONNECTED))
  {
    if (bisync_pstate != BISYNC_PS_BASIC) {
      send_buf[i] = ASCII_EOT;
      i++;
    }
    group_id += '0';
    unit_id += '0';
    send_buf[i] = group_id;
    i++;
    send_buf[i] = group_id;
    i++;
    send_buf[i] = unit_id;
    i++;
    send_buf[i] = unit_id;
    i++;
  }
  send_buf[i] = ASCII_STX;
  i++;
  int bcc_start = i;
  send_buf[i] = (byte)cmd[0];
  i++;
  send_buf[i] = (byte)cmd[1];
  i++;
  int sl = strlen(data);
  strncpy((char *)&send_buf[i], data, sl);
  i += sl;
  send_buf[i] = ASCII_ETX;
  i++;
  send_buf[i] = bisync_calc_bcc(&send_buf[bcc_start], sl + 3);
  if (debug > 1) {
    send_buf[i + 1] = 0;
    char * c2h_buf = ctrl2hex((char *)send_buf);
    fprintf(stderr, "Send Set Parameter frame \"%s\" to SSD650V\n", c2h_buf);
    free(c2h_buf);
  }

  tcflush(sd, TCIOFLUSH); /* clear all buffers */
  r = serial_write(sd, send_buf, i + 2);
  if (r != TTY_OK) {
    sys_debug_log(2, "Error in serial_write(): %d", r);
    return BISYNC_ERR_WRITE;
  }

  r = serial_timeout(sd, DEVICE_RESPONSE_TIMEOUT);
  byte buf;
  int bytes_read;
  switch (r) {
    case TTY_TIME_OUT:
      serial_write_byte(sd, ASCII_EOT);
      bisync_pstate = BISYNC_PS_BASIC;
      return BISYNC_TIME_OUT;
      break;
    case TTY_SELECT_ERROR:
      serial_write_byte(sd, ASCII_EOT);
      bisync_pstate = BISYNC_PS_BASIC;
      return BISYNC_ERR_SELECT;
      break;
    case TTY_OK:
      bytes_read = read(sd, &buf, 1);
      if (bytes_read == 1) {
        bisync_pstate = BISYNC_PS_SETPARAM_CONNECTED;
        if ((char)buf == ASCII_ACK) {
          return BISYNC_OK;
        } else if ((char)buf == ASCII_NAK) {
          bisync_pstate = BISYNC_PS_UNDEF;
          return BISYNC_ERR_NAK;
        } else {
          bisync_pstate = BISYNC_PS_UNDEF;
          return BISYNC_ERR_FRAME;
        }
 
      } else if (bytes_read == 0) {
        /* this case should not occur as it is handled by the select timeout */
        bisync_pstate = BISYNC_PS_UNDEF;
        return BISYNC_ERR_READ;

      } else if (bytes_read < 0) {
        bisync_pstate = BISYNC_PS_UNDEF;
        return BISYNC_ERR_READ;

      } else {
        bisync_pstate = BISYNC_PS_UNDEF;
        return BISYNC_ERR_READ;
      }
      break;
    default:
      fprintf(stderr, "bisync_setparam: internal error.\n");
      bisync_pstate = BISYNC_PS_UNDEF;
      return BISYNC_ERR;
  }
}

