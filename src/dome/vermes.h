/* 
 * Obs. Vermes cupola driver.
 * Copyright (C) 2010 Markus Wildi <markus.wildi@one-arcsec.org>
 * based on Petr Kubanek's dummy_cup.cpp
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

#define SSD650V_CONNECT 0
#define SSD650V_DISCONNECT 1


#define SSD650V_CONNECTION_OK                    0
#define SSD650V_CONNECTION_NOK                   1
#define SSD650V_CONNECTION_FAILED                2
#define SSD650V_MAJOR_STATE_FAILED               3
#define SSD650V_LAST_ERROR_FAILED                4
#define SSD650V_GETTING_SET_POINT_FAILED         5
#define SSD650V_SETTING_SET_POINT_FAILED         6
#define SSD650V_GETTING_ACCELERATION_TIME_FAILED 7
#define SSD650V_GETTING_DECELERATION_TIME_FAILED 8
#define SSD650V_GETTING_MOTOR_COMMAND_FAILED     9
#define SSD650V_RUNNING                         10
#define SSD650V_STOPPED                         11


#define SSD650V_GENERAL_FAILURE                200
