/*
 *
 * Copyright (C) 2009, Lukas Zimmermann, Basel, Switzerland.
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
*/

#ifndef __serial_h__
#define __serial_h__

#include <termios.h>

#define PARITY_NONE    0
#define PARITY_EVEN    1
#define PARITY_ODD     2

/* the maximum number of serial devices that can be used simultanously */
#define SER_DEVICES_MAX 16

/* TTY Error Codes */
enum TTY_ERROR {
  TTY_OK=0,
  TTY_READ_ERROR=-1,
  TTY_WRITE_ERROR=-2,
  TTY_SELECT_ERROR=-3,
  TTY_TIME_OUT=-4,
  TTY_PORT_FAILURE=-5,
  TTY_PARAM_ERROR=-6,
  TTY_ERRNO = -7
};

typedef unsigned char byte;         /* define byte type */

/* define a struct type to store a description of an open serial device */
typedef struct tser_dev {
  char * device_name;
  int file_descriptor;
  struct termios orig_tty_setting;
  struct termios tty_setting;
  int modem_ctrl_bits;
} tser_dev;


int serial_write_byte(int sd, byte b);

int serial_write(int sd, byte *buf, int nbytes);


/* Opens and initializes a serial device and returns it's file descriptor. */
int serial_init(char *device_name, int bit_rate, int word_size,
                int parity, int stop_bits);

/* Restores terminal settings of open serial port device and close the file. */
int serial_shutdown(int fd);

/*  */
int serial_timeout(int sd, int timeout_ms);

/*  */
//tser_dev * get_ser_port_descriptor(int sd);

/* */
void shutdown_serial(int fd);

#endif    // #ifndef __serial_h__
