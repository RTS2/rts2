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
float get_setpoint() ;
int set_setpoint( float setpoint) ;
int motor_on() ;
int motor_off() ;
int connectDevice( int power_state) ;
#ifdef __cplusplus
}
#endif

// SSD650V states
#define SSD650V_ON    0 // active
#define SSD650V_OFF   1 // inactive
#define SSD650V_IDLE  2 
#define SSD650V_OK    3 
#define SSD650V_BUSY  4 
#define SSD650V_ALERT 5 


#endif   // #ifndef __ssd650v_comm_h__

