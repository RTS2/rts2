/*
 *
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

#ifndef __ssd650v_comm_h__
#define __ssd650v_comm_h__

#ifdef __cplusplus
extern "C"
{
#endif
double get_current() ;
double get_current_percentage() ;
float get_setpoint() ;
int set_setpoint( float setpoint) ;
int motor_on() ;
int motor_off() ;
int connectSSD650vDevice( int power_state) ;
#ifdef __cplusplus
}
#endif
#define REPEAT_RATE_NANO_SEC (long) (200. * 1000. * 1000.) // [1^-9 sec] frequency while getting incorrect answer
#define CURRENT_MAX_PERCENT 80. // do not exceed this percentage of the maximum current (ssd650v dome 0.8 A)
// SSD650V commands
#define SSD650V_CMD_ON         1  // starts motor
#define SSD650V_CMD_OFF        2  // stopps motor
#define SSD650V_CMD_CONNECT    3  // RS232
#define SSD650V_CMD_DISCONNECT 4
// SSD650V states
#define SSD650V_MS_OK                                99
#define SSD650V_MS_UNDEFINED                        100
#define SSD650V_MS_RUNNING                          101
#define SSD650V_MS_STOPPED                          102
#define SSD650V_MS_CONNECTION_OK                    103
#define SSD650V_MS_CONNECTION_FAILED                104
#define SSD650V_MS_IDENTITY_FAILED                  105
#define SSD650V_MS_MAJOR_STATE_FAILED               106
#define SSD650V_MS_LAST_ERROR_FAILED                107
#define SSD650V_MS_GETTING_SET_POINT_FAILED         108
#define SSD650V_MS_SETTING_SET_POINT_FAILED         110
#define SSD650V_MS_GETTING_ACCELERATION_TIME_FAILED 111
#define SSD650V_MS_GETTING_DECELERATION_TIME_FAILED 112
#define SSD650V_MS_GETTING_MOTOR_COMMAND_FAILED     113
#define SSD650V_MS_GENERAL_FAILURE                  114

// wildi ToDo
#define SSD650V_IS_ON       0 
#define SSD650V_IS_OFF      1 
#define SSD650V_IS_IDLE     2 
#define SSD650V_IS_OK       3 
#define SSD650V_IS_BUSY     4
#define SSD650V_IS_ALERT    5 


#endif   // #ifndef __ssd650v_comm_h__

