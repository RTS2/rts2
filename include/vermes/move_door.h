/* 
 * Obs. Vermes door driver.
 * Copyright (C) 2010 Markus Wildi <markus.wildi@one-arcsec.org>
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

#ifndef  __RTS_MOVE_DOOR_VERMES__
#define __RTS_MOVE_DOOR_VERMES__

#ifdef __cplusplus
extern "C"
{
#endif
void *move_door( void *value);
int move_manual( float setpoint);
#ifdef __cplusplus
}
#endif
// set point used here and in oak_digin_thread 
#define SETPOINT_ZERO                 0.
#define SETPOINT_OPEN_DOOR          100.
#define SETPOINT_CLOSE_DOOR        -100.
#define SETPOINT_OPEN_DOOR_SLOW      40.
#define SETPOINT_CLOSE_DOOR_SLOW    -50.
#define SETPOINT_UNDEFINED_POSITION -50. // manual intervention: reach a defined state by closing the door until the end switch is reached 


// door state
#define DS_UNDEF                       0
#define DS_STOPPED_OPENED              1
#define DS_STOPPED_CLOSED              2
#define DS_STOPPED_UNDEF               3
#define DS_RUNNING_OPEN                4
#define DS_RUNNING_CLOSE               5
#define DS_RUNNING_CLOSE_UNDEFINED     6
#define DS_RUNNING_UNDEF               7
#define DS_EMERGENCY_ENDSWITCH         8

// Oak sensor state
#define EVNT_DS_OPEN_SENSOR           20
#define EVNT_DS_CLOSE_SENSOR          21
#define EVNT_DS_NO_SENSOR             22
#define EVNT_DS_EMERGENCY_ENDSWITCH   23

// door commands
#define EVNT_DOOR_CMD_OPEN              30
#define EVNT_DOOR_CMD_CLOSE             31
#define EVNT_DOOR_CMD_CLOSE_IF_UNDEFINED_STATE 32
#define EVNT_DOOR_CMD_MOVE_MANUAL 33
#define EVNT_DOOR_CMD_DO_NOTHING        34

// comment to be defined
#define ERR_DOOR_BAD_STATE            50
#define ERR_DOOR_EMERGENCY_ENDSWITCH  51
#define ERR_DOOR_UNEXP_EVENT          52

// Manual mode
#define IGNORE_END_SWITCH 1
#define NOT_IGNORE_END_SWITCH 0


#endif //  __RTS_MOVE_DOOR_VERMES__
