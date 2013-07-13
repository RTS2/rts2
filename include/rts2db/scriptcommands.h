/* 
 * Commands which can be used in scripts
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __RTS2_SCRIPTCOMMANDS__
#define __RTS2_SCRIPTCOMMANDS__

/**
 * @file This file list commands which can be introduced to script.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */

#define COMMAND_SEQUENCE        "SEQUENCE"
#define COMMAND_EXPOSURE        "E"
#define COMMAND_DARK            "D"
#define COMMAND_CHANGE          "C"
#define COMMAND_BOX             "BOX"
#define COMMAND_CENTER          "CENTER"
// must be paired with COMMAND_CHANGE
#define COMMAND_WAIT            "W"
#define COMMAND_ACQUIRE         "A"
#define COMMAND_WAIT_ACQUIRE    "Aw"
#define COMMAND_PHOTOMETER      "P"
#define COMMAND_BLOCK_WAITSIG   "block_waitsig"
#define COMMAND_BLOCK_ACQ       "ifacq"
#define COMMAND_BLOCK_ELSE      "else"
#define COMMAND_BLOCK_FOR       "for"
#define COMMAND_BLOCK_WHILE     "while"
#define COMMAND_BLOCK_DO        "do"
#define COMMAND_BLOCK_ONCE      "once"
#define COMMAND_WAIT_SOD        "waitsod"
#define COMMAND_WAITFOR         "waitfor"
#define COMMAND_SLEEP           "sleep"
#define COMMAND_WAIT_FOR_IDLE   "wait_idle"
#define COMMAND_EXE             "exe"
#define COMMAND_COMMAND         "cmd"

// hex pattern
#define COMMAND_HEX             "hex"
// 5x5 pattern
#define COMMAND_FXF             "fxf"

// signal handling..
#define COMMAND_SEND_SIGNAL     "SS"
#define COMMAND_WAIT_SIGNAL     "SW"

// target operations
#define COMMAND_TARGET_DISABLE  "tardisable"
#define COMMAND_TAR_TEMP_DISAB  "tempdisable"
#define COMMAND_TARGET_BOOST    "tarboost"
#endif							 /* !__RTS2_SCRIPTCOMMANDS__ */
