/* Thread to move the door of cupola Obs. Vermes*/
/* Copyright (C) 2010, Markus Wildi */

/* This library is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU Lesser General Public */
/* License as published by the Free Software Foundation; either */
/* version 2.1 of the License, or (at your option) any later version. */

/* This library is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU */
/* Lesser General Public License for more details. */

/* You should have received a copy of the GNU Lesser General Public */
/* License along with this library; if not, write to the Free Software */
/* Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#endif

#include "vermes.h"
#include "move-door_vermes.h"

// 
#define AngularSpeed 90./180. *M_PI / 120. 
#define POLLMICROS 0.1 * 1000. * 1000. // make it variable

void *move_door( void *value)
{
  double ret = -1 ;
  double curMaxSetPoint= 80. ;
  double curMinSetPoint= 40. ;
  static double lastSetPoint=0., curSetPoint=0. ;
  static double lastSetPointSign=0., curSetPointSign=0. ;
  double tmpSetPoint= 0. ;

  while( 1==1) {
    
   
    usleep(POLLMICROS) ;
  }
  return NULL ;
}
