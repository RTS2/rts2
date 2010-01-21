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
#endif

#include "vermes.h"
#include "move-door_vermes.h"
#include "ssd650v_comm_vermes.h"

int motor_on_off_state_door= SSD650V_MS_UNDEFINED ;
int evt_door               = EVNT_DS_CMD_DO_NOTHING ;
int doorState              = DS_UNDEF ; // wildi INCORRECT
// 
#define AngularSpeed 90./180. *M_PI / 120. 
#define POLLMICROS (useconds_t)(1. * 1000. * 1000.) // make it variable
#define SLEEP_TEST (useconds_t)(1. * 1000. * 1000.)
#define SLEEP_OPEN_DOOR (useconds_t)(100. * 1000. * 1000.)
#define SLEEP_CLOSE_DOOR (useconds_t)(100. * 1000. * 1000.)
//
void *move_door( void *value)
{
  double ret = -1 ;

  while( 1==1) {
      if(evt_door== EVNT_DS_CMD_OPEN) {
	  fprintf(stderr, "move_door: opening door\n") ;
	  // check the state
	  if( doorState== DS_STOPPED_CLOSED) {
	  } else {
	      fprintf(stderr, "move_door: doorState != DS_STOPPED_CLOSED\n") ;
	  }
	  // set setpoint
	  if(( ret= set_setpoint(SETPOINT_OPEN_DOOR)) != 0) {
	      fprintf(stderr, "move_door: set_setpoint != 0\n") ;
	  }
	  // motor_on
	  if(( ret= motor_on()) != SSD650V_MS_RUNNING) {
	      fprintf(stderr, "move_door: motor_on != SSD650V_MS_RUNNING\n") ;
	  }
// begin test section
	  // sleep
	  if( usleep( SLEEP_TEST) !=0)
	  {
	      fprintf(stderr, "move_door: sleep  != 0, failed\n") ;   
	  }
          // section for initial testing
	  if(( ret= motor_off()) != SSD650V_MS_STOPPED) {
	      fprintf(stderr, "move_door: motor_on != SSD650V_MS_STOPPED\n") ;
	  }
	  // set setpoint
	  if(( ret= set_setpoint(SETPOINT_ZERO)) != 0) {
	      fprintf(stderr, "move_door: set_setpoint != 0\n") ;
	  }
	  break ;
// end test section
	  // after SLEEP_OPEN_DOOR seconds sleep
	  // set setpoint
	  if(( ret= set_setpoint(SETPOINT_UNDEFINED_POSITION)) != 0) {
	      fprintf(stderr, "move_door: set_setpoint != 0\n") ;
	  }
	  // Now, it is up to oak_digin_thread to say DS_STOPPED_FULLY_OPEN or what is approprate

      } else if(evt_door== EVNT_DS_CMD_CLOSE){

	  fprintf(stderr, "move_door: closing\n") ;
	  // check the state
	  if( doorState== DS_STOPPED_FULLY_OPEN) {
	  } else {
	      fprintf(stderr, "move_door: doorState != DS_STOPPED_FULLY_OPEN\n") ;
	  }
	  // set setpoint
	  if(( ret= set_setpoint(SETPOINT_CLOSE_DOOR)) != 0) {
	      fprintf(stderr, "move_door: set_setpoint != 0\n") ;
	  }
	  // motor_on
	  if(( ret= motor_on()) != SSD650V_MS_RUNNING) {
	      fprintf(stderr, "move_door: motor_on != SSD650V_MS_RUNNING\n") ;
	  }
// begin test section
	  // sleep
	  if( usleep( SLEEP_TEST) !=0)
	  {
	      fprintf(stderr, "move_door: sleep  != 0, failed\n") ;   
	  }
          // section for initial testing
	  if(( ret= motor_off()) != SSD650V_MS_STOPPED) {
	      fprintf(stderr, "move_door: motor_on != SSD650V_MS_STOPPED\n") ;
	  }
	  // set setpoint
	  if(( ret= set_setpoint(SETPOINT_ZERO)) != 0) {
	      fprintf(stderr, "move_door: set_setpoint != 0\n") ;
	  }
	  break ;
// end test section
	  // after SLEEP_OPEN_DOOR seconds sleep
	  // set setpoint
	  if(( ret= set_setpoint(SETPOINT_UNDEFINED_POSITION)) != 0) {
	      fprintf(stderr, "move_door: set_setpoint != 0\n") ;
	  }
	  // Now, it is up to oak_digin_thread to say DS_STOPPED_FULLY_OPEN or what is approprate

      } else if(evt_door== EVNT_DS_CMD_CLOSE_IF_UNDEFINED_STATE){

	  fprintf(stderr, "move_door: closing door after door state is UNDEFINED\n") ;
	  // check the state
	  if( doorState== DS_UNDEF ) {
	  } else {
	      fprintf(stderr, "move_door: doorState != DS_UNDEF \n") ;
	  }
	  // set setpoint
	  if(( ret= set_setpoint(SETPOINT_UNDEFINED_POSITION)) != 0) {
	      fprintf(stderr, "move_door: set_setpoint != 0\n") ;
	  }
	  // motor_on
	  if(( ret= motor_on()) != SSD650V_MS_RUNNING) {
	      fprintf(stderr, "move_door: motor_on != SSD650V_MS_RUNNING\n") ;
	  }
// begin test section
	  // sleep
	  if( usleep( SLEEP_TEST) !=0)
	  {
	      fprintf(stderr, "move_door: sleep  != 0, failed\n") ;   
	  }
          // section for initial testing
	  if(( ret= motor_off()) != SSD650V_MS_STOPPED) {
	      fprintf(stderr, "move_door: motor_on != SSD650V_MS_STOPPED\n") ;
	  }
	  // set setpoint
	  if(( ret= set_setpoint(SETPOINT_ZERO)) != 0) {
	      fprintf(stderr, "move_door: set_setpoint != 0\n") ;
	  }
	  break ;
// end test section
	  // Now, it is up to oak_digin_thread to say DS_STOPPED_FULLY_OPEN or what is approprate
      } else if(evt_door== EVNT_DS_CMD_DO_NOTHING){
	  fprintf(stderr, "move_door: doing nothing\n") ;
      } else {
	  fprintf(stderr, "move_door: undefined command %d\n", evt_door) ;
      }
      fprintf( stderr, "move_door: setpoint=%f\n",  get_setpoint()) ;
    usleep(POLLMICROS) ;
  }
  return NULL ;
}
