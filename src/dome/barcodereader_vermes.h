/*
 *
 * Copyright (C) 2010 Markus Wildi, version for RTS2, minor adaptions
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

#ifndef __barcodereader_h__
#define __barcodereader_h__

// default serial port devices and their default settings
#define DEFAULT_SERPORT_DEV1     "/dev/bcreader_lft"
#define DEFAULT_SERPORT_DEV2     "/dev/bcreader_rgt"
#define DEFAULT_BITRATE          38400
#define DEFAULT_DATABITS         8
#define DEFAULT_PARITY           PARITY_NONE
#define DEFAULT_STOPBITS         1

/* The number of the highest barcode used */
#define HIGHEST_BARCODE  104 
//#define HIGHEST_BARCODE  22

/* Constants defining indi_log priorities */
#define ILOG_DEBUG      1
#define ILOG_INFO       2
#define ILOG_NOTICE     4
#define ILOG_WARNING    8
#define ILOG_ERR       16
#define ILOG_CRIT      32
#define ILOG_ALERT     64
#define ILOG_EMERG    128

#define SAMPLE_MS 10
#define MAX_TX_RETRY 3

#define PSTAT_RINGBUF_SIZE  100    /* serial reception ring buffer size */
/** \typedef PORT_STAT
    \brief Structure for holding the state of the serial communication with
           a barcode reader.
*/
typedef struct _PORT_STAT {
  int fd;                           // file descriptor of serial port
  char *dev_name;                   // device file name of serial port
  int protocol_stat;                // protocol state of serial communication
  char rxstr[32];                   // storage for a complete serial reply
  char rx_ring[PSTAT_RINGBUF_SIZE]; // ring buffer for serial reception (fifo)
  int head;                         // ring buffer head pointer (where next
                                    //  rxed char gets stored)
  int tail;                         // ring buffer tail pointer (where next
                                    //  char should be taken out)
  int rx_cnt;                       // fill count of ring buffer
  pthread_mutex_t mutex_rx;         // mutex for ring buffer access synchronisation
} PORT_STAT;

/** \typedef POS_DETECT_STATE
    \brief Structure for storing position detector status.
*/
typedef struct _POS_DETECT_STATE {
  int current_pos;                  // the latest valid position reading
  int last_current_pos;             // the previous valid position reading
  int pos_invalid;                  // true, when position detection was invalid
  int pos_changed;                  // true, when the detected position has
                                    // changed
  int azimut_propstate_changed;     // true, when indi client's property
                                    // status indicator needs to be changed.
} POS_DETECT_STATE;

/** \typedef POS_CHG_CBF
    \brief Signature of a position change callback function.
*/
typedef void (POS_CHG_CBF) (POS_DETECT_STATE * );


/** \brief Takes the given string and replaces all non-printing characters by
           a hexadecimal representation. The created string gets dynamically
           allocated and returned. The allocated memory must be explicitly
           freed when the returned string is not needed anymore.
    \param str The string to analyse.
    \return A new allocated string with unprintable characters replaced.
*/
char * ctrl2hex(char *str);

/** \brief Sets up the serial ports to communicate with barcode readers,
           initializes the barcode readers and creates two threads to handle
           the serial communication.
    \return 0
*/
#ifdef __cplusplus
extern "C"
{
#endif
int start_bcr_comm();
#ifdef __cplusplus
}
#endif

/** \brief Stops all communication with barcode readers.
    \return Always returns 0.
*/
#ifdef __cplusplus
extern "C"
{
#endif
int stop_bcr_comm();
#ifdef __cplusplus
}
#endif

/** \brief Registers the given function to get called when the dome azimut
           property changes.
    \param callback_func The callback function.
*/
#ifdef __cplusplus
extern "C"
{
#endif
void register_pos_change_callback(POS_CHG_CBF * callback_func);
#ifdef __cplusplus
}
#endif


/** \brief Handles value change.
    \return void.
*/
#ifdef __cplusplus 
extern "C" 
{ 
#endif 
void position_update_callback(POS_DETECT_STATE * pos_detct_state); 
#ifdef __cplusplus 
} 
#endif 



/** \brief Creates syslog entries (if preprocessor define USE_SYSLOG) with the
           given priority and prints the message to stderr.
    \param prio Priority of the log message. Will get translated to a syslog
                priority.
    \param format Format string like those used with printf() and friends.
    \param ...
*/
void indi_log(int prio, char * format, ...);

/** \brief Creates syslog entries for debugging (if preprocessor define
           USE_SYSLOG)* and prints the message to stderr depending on the
           global debug variable.
    \param level Importance level of message. Increased level is more verbose.
    \param format Format string like those used with printf() and friends.
    \param ... 
*/
void indi_debug_log(int level, char * format, ...);

/** \brief Sets the mask used by syslog() to decide whether a certain priority
           is going to be logged (if preprocessor define USE_SYSLOG).
    \param mask Bit mask using the defined ILOG_ values.
*/
void indi_setlogmask(int mask);



#endif   // #ifndef __barcodereader_h__

