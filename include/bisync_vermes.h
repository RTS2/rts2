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

#ifndef __bisync_h__
#define __bisync_h__

#include "serial_vermes.h"


enum ENQUIRY_STYLE {
  ENQUIRY_SINGLE,         /* go back to disconnected state after the enquiry */
  ENQUIRY_SAME,           /* stay connected after the enquiry to query the same
                             parameter by simply sending ASCII_NAK */
  ENQUIRY_NEXT            /* stay connected after the enquiry to query the next
                             parameter by simply sending ASCII_ACK */
};

enum SETPARAM_STYLE {
  SETPARAM_SINGLE
};

enum BISYNC_ERROR {
  BISYNC_OK = 0,
  BISYNC_ERR,
  BISYNC_ERR_READ,
  BISYNC_ERR_WRITE,
  BISYNC_ERR_SELECT,
  BISYNC_TIME_OUT,
  BISYNC_ERR_EOT,
  BISYNC_ERR_BCC,
  BISYNC_ERR_FRAME,
  BISYNC_ERR_NAK,
  BISYNC_ERR_FRAME_INCMPL,
  BISYNC_ERR_STX
};


void bisync_init();

int bisync_enquiry(int sd, byte group_id, byte unit_id, char * cmd,
                   char ** response_frame, enum ENQUIRY_STYLE style);

int bisync_setparam(int sd, byte group_id, byte unit_id, char * cmd,
                    char * data, enum SETPARAM_STYLE style);

int bisync_read_frame(int sd, char ** frame);

char * bisync_errstr(int err);

#endif   // #ifndef __bisync_h__

