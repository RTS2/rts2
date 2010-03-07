/*
 * RS 232 communication
 * Copyright (C) 2010 Markus Wildi, version for RTS2, minor adaptions
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

/* TODO: implement locking of the serial device when opened.
 */

/* asprintf(..) is a GNU extension */ 
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "serial_vermes.h"
#include "util_vermes.h"


/* place to store dynamically allocated serial devices descriptions */
tser_dev* serial_devices[SER_DEVICES_MAX];
int serial_devices_cnt = 0;


/******************************************************************************
 * serial_write_byte(...)
 *****************************************************************************/
int
serial_write_byte(int sd, byte b)
{
  int bytes_w = write(sd, &b, 1);
  if (bytes_w < 0)
    return TTY_WRITE_ERROR;

  return TTY_OK;
}

/******************************************************************************
 * serial_write(...)
 *****************************************************************************/
int
serial_write(int sd, byte *buf, int nbytes)
{
  int bytes_w = 0;

  while (nbytes > 0) {
    bytes_w = write(sd, buf, nbytes);
    if (bytes_w < 0)
      return TTY_WRITE_ERROR;

    buf += bytes_w;
    nbytes -= bytes_w;
  }

  return TTY_OK;
}

/******************************************************************************
 * serial_timeout(..)
 ******************************************************************************
 * Wait until a character is received via serial port sd or a time of timeout
 * milliseconds has expired.
 * Arguments:
 *   sd: open serial port's file descriptor.
 *   timeout_ms: timeout in milli seconds.
 * Return:
 *   one of the TTY_ERROR error codes TTY_OK (0), TTY_SELECT_ERROR, TTY_TIME_OUT
 *****************************************************************************/
int
serial_timeout(int sd, int timeout_ms)
{
  struct timeval tv;
  fd_set readout;
  int retval;

  FD_ZERO(&readout);
  FD_SET(sd, &readout);

  /* wait for 'timeout' seconds */
  tv.tv_sec = 0;
  tv.tv_usec = 1000 * timeout_ms;

  /* Wait till we have a change in the fd status */
  retval = select(sd+1, &readout, NULL, NULL, &tv);

  /* Return 0 on character ready to be read */
  if (retval > 0)
    return TTY_OK;
  /* Return -1 due to an error */
  else if (retval == -1)
    return TTY_SELECT_ERROR;
  /* Return -2 if time expires before anything interesting happens */
  else 
    return TTY_TIME_OUT;
}

/******************************************************************************
 * get_ser_port_descriptor(..)
 ******************************************************************************
 * search for the entry with given file descriptor in the list of open serial
 * port descriptions and return the pointer to that struct. Return NULL when
 * not found. 
 *****************************************************************************/
tser_dev*
get_ser_port_descriptor(int sd)
{
  int i = 0;
  while ((i < SER_DEVICES_MAX)) {
    if (i > SER_DEVICES_MAX) return NULL;
    if (sd == serial_devices[i]->file_descriptor) break;
    i++;
  }
  return serial_devices[i];
}

/******************************************************************************
 * serial_shutdown(..)
 ******************************************************************************
 * Restores terminal settings of open serial port device and close the file.
 * Arguments:
 *   sd: open serial port's file descriptor.
 *****************************************************************************/
int
serial_shutdown(int sd)
{
  int res = 0;
  tser_dev* ser_dev = get_ser_port_descriptor(sd);
  if (sd > 0) {
    ioctl(sd, TIOCMSET, &ser_dev->modem_ctrl_bits);
    if (tcsetattr(sd, TCSANOW, &ser_dev->orig_tty_setting) < 0) {
      perror("serial_shutdown: can't restore serial device's terminal settings.");
    }
    res = close(sd);
  } else {
    fprintf(stderr, "serial_shutdown: application bug, tried to close fd <= 0!\n");
    res = -2;
  }
  return res;
}

/******************************************************************************
 * serial_init(..)
 ******************************************************************************
 * Opens and initializes a serial device and returns it's file descriptor.
 * Arguments:
 *   device_name : device name string of the device to open (/dev/ttyS0, ...)
 *   bit_rate    : bit rate
 *   word_size   : number of data bits, 7 or 8, USE 8 DATA BITS with modbus
 *   parity      : 0=no parity, 1=parity EVEN, 2=parity ODD
 *   stop_bits   : number of stop bits : 1 or 2
 * Return:
 *   file descriptor  of successfully opened serial device
 *   or -1 in case of error.
 *****************************************************************************/
int
serial_init(char *device_name, int bit_rate, int word_size,
            int parity, int stop_bits)
{
  int fd;
  char *msg;

  /* init the elements of the array of opened serial port's descriptions */
  int i;
  if (serial_devices_cnt == 0) {
    for (i = 0; i < SER_DEVICES_MAX; i++) {
      serial_devices[i] = NULL;
    }
  }

  if (serial_devices_cnt >= SER_DEVICES_MAX) {
    fprintf(stderr, "FATAL: already used %d serial devices which is the "
                    "maximum possible!\n", serial_devices_cnt);
    exit(1);
  }

  /* open serial device */
  fd = open(device_name, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    if (asprintf(&msg, "serial_init: open %s failed", device_name) < 0)
      perror(NULL);
    else {
      perror(msg);
      free(msg);
    }
    return -1;
  }
  if (!isatty(fd)) {
    fprintf(stderr,
            "serial_init: %s is not a terminal device.\n", device_name);
    return -1;
  }

  /* save current tty settings */
  tser_dev *ser_d = xrealloc(NULL, sizeof(tser_dev));
  serial_devices[serial_devices_cnt] = ser_d;
  serial_devices[serial_devices_cnt]->file_descriptor = fd;
  serial_devices_cnt++;
  if (tcgetattr(fd, &ser_d->orig_tty_setting) < 0) {
    perror("serial_init: can't get terminal parameters.");
    return -1;
  }

  // Control Modes
  // Set bps rate
  int bps;
  switch (bit_rate) {
    case 0:
      bps = B0;
      break;
    case 50:
      bps = B50;
      break;
    case 75:
      bps = B75;
      break;
    case 110:
      bps = B110;
      break;
    case 134:
      bps = B134;
      break;
    case 150:
      bps = B150;
      break;
    case 200:
      bps = B200;
      break;
    case 300:
      bps = B300;
      break;
    case 600:
      bps = B600;
      break;
    case 1200:
      bps = B1200;
      break;
    case 1800:
      bps = B1800;
      break;
    case 2400:
      bps = B2400;
      break;
    case 4800:
      bps = B4800;
      break;
    case 9600:
      bps = B9600;
      break;
    case 19200:
      bps = B19200;
      break;
    case 38400:
      bps = B38400;
      break;
    case 57600:
      bps = B57600;
      break;
    case 115200:
      bps = B115200;
      break;
    case 230400:
      bps = B230400;
      break;
    default:
      fprintf(stderr, "serial_init: %d is not a valid bit rate.\n", bit_rate);
      return -1;
  }
  if ((cfsetispeed(&ser_d->tty_setting, bps) < 0) ||
      (cfsetospeed(&ser_d->tty_setting, bps) < 0))
  {
    perror("serial_init: failed setting bit rate.");
    return -1;
  }

  // Control Modes
  // set no flow control word size, parity and stop bits.
  // Also don't hangup automatically and ignore modem status.
  // Finally enable receiving characters.
  ser_d->tty_setting.c_cflag &=
          ~(CSIZE | CSTOPB | PARENB | PARODD | HUPCL | CRTSCTS);
  #ifdef CBAUDEX
  ser_d->tty_setting.c_cflag &= ~(CBAUDEX);
  #endif
  #ifdef CBAUDEXT
  ser_d->tty_setting.c_cflag &= ~(CBAUDEXT);
  #endif
  ser_d->tty_setting.c_cflag |= (CLOCAL | CREAD);

  // word size
  switch (word_size) {
    case 5:
      ser_d->tty_setting.c_cflag |= CS5;
      break;
    case 6:
      ser_d->tty_setting.c_cflag |= CS6;
      break;
    case 7:
      ser_d->tty_setting.c_cflag |= CS7;
      break;
    case 8:
      ser_d->tty_setting.c_cflag |= CS8;
      break;
    default:
      fprintf(stderr,
              "serial_init: %d is not a valid data bit count.\n", word_size);
      return -1;
  }
  // parity
  switch (parity) {
    case PARITY_NONE:
      break;
    case PARITY_EVEN:
      ser_d->tty_setting.c_cflag |= PARENB;
      break;
    case PARITY_ODD:
      ser_d->tty_setting.c_cflag |= PARENB | PARODD;
      break;
    default:
      fprintf(stderr, "serial_init: %d is not a valid parity selection "
                      "value.\n", parity);
      return -1;
  }

  // stop_bits
  switch (stop_bits) {
    case 1:
      break;
    case 2:
      ser_d->tty_setting.c_cflag |= CSTOPB;
      break;
    default:
      fprintf(stderr,
              "serial_init: %d is not a valid stop bit count.", stop_bits);
      return -1;
  }
  // Control Modes complete

  // Ignore bytes with parity errors and make terminal raw and dumb.
  ser_d->tty_setting.c_iflag &=
          ~(PARMRK | ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON | IXANY);
  ser_d->tty_setting.c_iflag |= INPCK | IGNPAR | IGNBRK;

  // Raw output.
  ser_d->tty_setting.c_oflag &= ~(OPOST | ONLCR);

  // Local Modes
  // Don't echo characters. Don't generate signals.
  // Don't process any characters.
  ser_d->tty_setting.c_lflag &=
          ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN | NOFLSH | TOSTOP);
  ser_d->tty_setting.c_lflag |=  NOFLSH;

  // non blocking read 
  ser_d->tty_setting.c_cc[VMIN]  = 0;
  ser_d->tty_setting.c_cc[VTIME] = 0;

  // now clear input and output buffers and activate the new terminal settings
  if (tcflush(fd, TCIOFLUSH)) {
    perror("serial_init: could not flush buffers");
    return -1;
  }
  if (tcsetattr(fd, TCSANOW, &ser_d->tty_setting)) {
    perror("serial_init: failed setting attributes on serial port.");
    serial_shutdown(fd);
    return -1;
  }

  int ctl;
  ioctl(fd, TIOCMGET, &ctl);
  ser_d->modem_ctrl_bits = ctl;
  ctl &= ~TIOCM_DTR & ~TIOCM_RTS;
  ioctl(fd, TIOCMSET, &ctl);
  tcflush(fd, TCIOFLUSH); /* clear all buffers */
  return fd;

}


