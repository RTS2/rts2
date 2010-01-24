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
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#endif

#include "vermes.h"
#include "move-door_vermes.h"
#include "ssd650v_comm_vermes.h"

extern char *oak_bits ;
int doorEvent       = EVNT_DS_CMD_DO_NOTHING ;
int doorState       = DS_UNDEF ; 
// wildi must not necessarily go aay
#define TEST_SSD 1
#define NO_TEST_SSD 0
int test_ssd= NO_TEST_SSD ;
// 
#define AngularSpeed 90./180. *M_PI / 120. 
#define POLLSEC (time_t) 1       // make it variable
#define SLEEP_TEST_SSD (time_t) 11 // seconds!
#define SLEEP_OPEN_DOOR (time_t)  100 // (time_t) 100
#define SLEEP_CLOSE_DOOR (time_t) 100 // (time_t) 100
//
void *move_door( void *value)
{
  int    ret ;
  struct timespec slv ;
  struct timespec rsl ;

  static int lastDoorState= -1 ;
  static char last_oak_bits[32] ;
  strcpy(  last_oak_bits, "01234567" ) ; // make the first oak state visible

  if( test_ssd == TEST_SSD) {
    fprintf(stderr, "move_door: test_ssd == TEST_SSD, %d\n", SLEEP_TEST_SSD) ;
  } else {
    fprintf(stderr, "move_door: not in TEST modus\n") ;
  }

  while( 1==1) {
    if(doorEvent== EVNT_DS_CMD_OPEN) {
      fprintf(stderr, "move_door: opening door\n") ;
      // check the state
      //if( doorState != DS_STOPPED_CLOSED) {
      //fprintf(stderr, "move_door: doorState != DS_STOPPED_CLOSED\n") ;
      if(1) { 
	//} else {
	// set setpoint
	if(( ret= set_setpoint(SETPOINT_OPEN_DOOR)) != SSD650V_MS_OK) {
	  fprintf(stderr, "move_door: set_setpoint != SSD650V_MS_OK\n") ;
	} else {
	  doorState= DS_START_OPEN;
	  // motor_on
	  if(( ret= motor_on()) != SSD650V_MS_RUNNING) {
	    fprintf(stderr, "move_door: motor_on != SSD650V_MS_RUNNING\n") ;
	  } else {
	    doorState= DS_RUNNING_OPEN;
	    fprintf( stderr, "move_door: motor running doorState== DS_RUNNING_OPEN\n") ;
	    // begin test section
	    if( test_ssd == TEST_SSD) {
	      fprintf( stderr, "move_door: test_ssd == TEST_SSD\n") ;
	      // sleep
	      slv.tv_sec= SLEEP_TEST_SSD ;
	      slv.tv_nsec= 0 ;
	      ret= nanosleep( &slv, &rsl) ;
	      if((ret== EFAULT) || ( ret== EINTR)|| ( ret== EINVAL ))  {
		fprintf( stderr, "Error in nanosleep %d\n", ret) ;
	      }
	      while(( ret= motor_off()) != SSD650V_MS_STOPPED) {
		fprintf(stderr, "move_door: motor_on != SSD650V_MS_STOPPED\n") ;
		ret= nanosleep( &slv, &rsl) ;
		if((ret== EFAULT) || ( ret== EINTR)|| ( ret== EINVAL ))  {
		    fprintf( stderr, "Error in nanosleep %d\n", ret) ;
		}
	      }
	      // set setpoint
	      if(( ret= set_setpoint(SETPOINT_ZERO)) != SSD650V_MS_OK) {
		fprintf(stderr, "move_door: door stopped, set_setpoint != SSD650V_MS_OK\n") ;
	      } else {
		fprintf(stderr, "move_door: door stopped, setpoint = SETPOINT_ZERO\n") ;
	      }
	      // end test section
	    } else {
	      // after SLEEP_OPEN_DOOR seconds sleep drive slowly until OAK digin changes state
	      slv.tv_sec= SLEEP_OPEN_DOOR ;
	      slv.tv_nsec= 0 ;
	      ret= nanosleep( &slv, &rsl) ;
	      if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
		switch( ret) {
		case EFAULT:
		  fprintf( stderr, "move_door: error in nanosleep EFAULT=%d\n", ret) ;
		  break ;
		case EINTR:
		  fprintf( stderr, "move_door: error in nanosleep EINTR=%d\n",ret) ;
		  break ;
		case EINVAL:
		  fprintf( stderr, "move_door: error in nanosleep EINVAL=%d\n",ret) ;
		  break ;
		default:
		  fprintf( stderr, "move_door: error in nanosleep NONE of these\n") ;
		}
	      }
	      // set setpoint is now low door moves slowly until the end switch is reached (oak_digin_thread)
	      while(( ret= set_setpoint(SETPOINT_OPEN_DOOR_SLOW)) != SSD650V_MS_OK) {
		fprintf(stderr, "move_door: set_setpoint != SSD650V_MS_OK\n") ;
		ret= nanosleep( &slv, &rsl) ;
		if((ret== EFAULT) || ( ret== EINTR)|| ( ret== EINVAL ))  {
		  fprintf( stderr, "Error in nanosleep %d\n", ret) ;
		}
	      }
	      fprintf(stderr, "move_door: creeping slowly towards end switch, waiting for oak_digin_thread to switch off\n") ;
	    }
	  }
	}
      }
      // Now, it is up to oak_digin_thread to say DS_STOPPED_OPEN or what is approprate
    } else if(doorEvent== EVNT_DS_CMD_CLOSE){

      fprintf(stderr, "move_door: closing door\n") ;
      // check the state
      //if( doorState != DS_STOPPED_OPEN) {
      //	fprintf(stderr, "move_door: doorState != DS_STOPPED_OPEN\n") ;
      //    } else {
      if(1) {
	// set setpoint
	if(( ret= set_setpoint(SETPOINT_CLOSE_DOOR)) != SSD650V_MS_OK) {
	  fprintf(stderr, "move_door: set_setpoint != SSD650V_MS_OK\n") ;
	} else {
	  doorState= DS_START_CLOSE ;
	  // motor_on
	  if(( ret= motor_on()) != SSD650V_MS_RUNNING) {
	    fprintf(stderr, "move_door: motor_on != SSD650V_MS_RUNNING\n") ;
	  } else {
	    doorState= DS_RUNNING_CLOSE ;
	    fprintf( stderr, "move_door: motor running doorState== DS_RUNNING_CLOSE\n") ;
	    // begin test section
	    if( test_ssd == TEST_SSD) {
	      fprintf( stderr, "move_door: test_ssd == TEST_SSD\n") ;
	      // sleep
	      slv.tv_sec= SLEEP_TEST_SSD ;
	      slv.tv_nsec= 0  ;
	      ret= nanosleep( &slv, &rsl) ;
	      if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
		  fprintf( stderr, "move_door: error in nanosleep %d\n", ret) ;
	      }
	      while(( ret= motor_off()) != SSD650V_MS_STOPPED) {
		fprintf(stderr, "move_door: motor_on != SSD650V_MS_STOPPED\n") ;
		ret= nanosleep( &slv, &rsl) ;
		if((ret== EFAULT) || ( ret== EINTR)|| ( ret== EINVAL ))  {
		    fprintf( stderr, "Error in nanosleep %d\n", ret) ;
		}
	      }
	      // set setpoint
	      if(( ret= set_setpoint(SETPOINT_ZERO)) != SSD650V_MS_OK) {
		fprintf(stderr, "move_door: door stopped, set_setpoint != SSD650V_MS_OK\n") ;
	      } else {
		fprintf(stderr, "move_door: door stopped, setpoint = SETPOINT_ZERO\n") ;
	      }
	      // end test section
	    } else {
	      // after SLEEP_CLOSE_DOOR seconds sleep drive slowly until OAK digin changes state
	      slv.tv_sec= SLEEP_CLOSE_DOOR;
	      slv.tv_nsec= 0 ;
	      ret= nanosleep( &slv, &rsl) ;
	      if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
		fprintf( stderr, "move_door: error in nanosleep\n") ;
	      }
	      // set setpoint is now low door moves slowly until the end switch is reached (oak_digin_thread)
	      while(( ret= set_setpoint(SETPOINT_CLOSE_DOOR_SLOW)) != SSD650V_MS_OK) {
		fprintf(stderr, "move_door: set_setpoint != SSD650V_MS_OK\n") ;
		ret= nanosleep( &slv, &rsl) ;
		if((ret== EFAULT) || ( ret== EINTR)|| ( ret== EINVAL ))  {
		  fprintf( stderr, "Error in nanosleep %d\n", ret) ;
		}
	      }
	      fprintf(stderr, "move_door: creeping slowly towards end switch, waiting for oak_digin_thread to switch off\n") ;
	    }
	  }
	}
      }
      // Now, it is up to oak_digin_thread to say DS_STOPPED_CLOSED  or what is approprate
    } else if(doorEvent== EVNT_DS_CMD_CLOSE_IF_UNDEFINED_STATE){

      if( doorState == DS_STOPPED_CLOSED) {
	fprintf(stderr, "move_door: doorState == DS_STOPPED_CLOSED, doing nothing\n") ;
	lastDoorState= doorState ;
	strcpy( last_oak_bits, oak_bits) ;
      } else {

	fprintf(stderr, "move_door: closing door after door state is UNDEFINED resp. != DS_STOPPED_CLOSED\n") ;
	// do not chack state, since it is be definition DS_UNDEF
	// set setpoint
	if(( ret= set_setpoint(SETPOINT_UNDEFINED_POSITION)) != SSD650V_MS_OK) {
	  fprintf(stderr, "move_door: set_setpoint != SSD650V_MS_OK\n") ;
	} else {
	  doorState= DS_START_UNDEF ;
	  // motor_on
	  if(( ret= motor_on()) != SSD650V_MS_RUNNING) {
	    fprintf(stderr, "move_door: motor_on != SSD650V_MS_RUNNING\n") ;
	  } else {
	    doorState= DS_RUNNING_UNDEF ;
	    fprintf( stderr, "move_door: motor running doorState== DS_RUNNING_UNDEF\n") ;
	    // sleep
	    // begin test section, wildi ToDo must go away
	    if( test_ssd == TEST_SSD) {
	      slv.tv_sec= SLEEP_TEST_SSD ;
	      slv.tv_nsec= 0 ;
	      ret= nanosleep( &slv, &rsl) ;
	      if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
		fprintf( stderr, "move_door: error in nanosleep\n") ;
		ret= nanosleep( &slv, &rsl) ;
		if((ret== EFAULT) || ( ret== EINTR)|| ( ret== EINVAL ))  {
		  fprintf( stderr, "Error in nanosleep %d\n", ret) ;
		}
	      }
	      while( ( ret= motor_off()) != SSD650V_MS_STOPPED) {
		fprintf(stderr, "move_door: motor_on != SSD650V_MS_STOPPED\n") ;
		ret= nanosleep( &slv, &rsl) ;
		if((ret== EFAULT) || ( ret== EINTR)|| ( ret== EINVAL ))  {
		  fprintf( stderr, "Error in nanosleep %d\n", ret) ;
		}
	      }
	      // set setpoint
	      while(( ret= set_setpoint(SETPOINT_ZERO)) != SSD650V_MS_OK) {
		fprintf(stderr, "move_door: set_setpoint != SSD650V_MS_OK\n") ;
		ret= nanosleep( &slv, &rsl) ;
		if((ret== EFAULT) || ( ret== EINTR)|| ( ret== EINVAL ))  {
		  fprintf( stderr, "Error in nanosleep %d\n", ret) ;
		}
	      }
	    }
	    // end test section
	  }
	}
	// Now, it is up to oak_digin_thread to say DS_STOPPED_CLOSED or what is approprate
      }
    } else if(doorEvent== EVNT_DS_CMD_DO_NOTHING){
      if(( doorState != lastDoorState) || ( strcmp(oak_bits, last_oak_bits))) {
	fprintf(stderr, "move_door: doing nothing doorState=%d, setpoint=%4.1f, bits= %s\n", doorState, get_setpoint(), oak_bits) ;
	lastDoorState= doorState ;
	strcpy( last_oak_bits, oak_bits) ;
      }
    } else {
      fprintf(stderr, "move_door: undefined command %d\n", doorEvent) ;
    }
    // wildi ToDo MUTEX
    // reset doorEvent
    doorEvent       = EVNT_DS_CMD_DO_NOTHING ;
    //fprintf( stderr, "move_door: read from device setpoint=%f\n",  get_setpoint()) ;
    slv.tv_sec= POLLSEC ;
    slv.tv_nsec= 0 ;
    ret= nanosleep( &slv, &rsl) ;
    if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
      switch( ret) {
      case EFAULT:
	fprintf( stderr, "move_door: error in nanosleep EFAULT=%d\n", ret) ;
	break ;
      case EINTR:
	fprintf( stderr, "move_door: error in nanosleep EINTR=%d\n",ret) ;
	break ;
      case EINVAL:
	fprintf( stderr, "move_door: error in nanosleep EINVAL=%d\n",ret) ;
	break ;
      default:
	fprintf( stderr, "move_door: error in nanosleep NONE of these\n") ;
      }
    } 
  }
  return NULL ;
}
