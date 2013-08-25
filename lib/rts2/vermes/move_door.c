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
#include <signal.h>
#endif

#include <door_vermes.h>
#include <vermes/move_door.h>
#include <vermes/ssd650v_comm.h>

int doorEvent= EVNT_DOOR_CMD_DO_NOTHING ;
int doorState= DS_UNDEF ; 

// wildi must not necessarily go aay
#define TEST_SSD 1
#define NO_TEST_SSD 0
int test_ssd= NO_TEST_SSD ;
// 
#define AngularSpeed 90./180. *M_PI / 120. 
#define POLLSEC          (time_t)   1 // make it variable
#define SLEEP_TEST_SSD   (time_t)   1 // seconds!
#define SLEEP_OPEN_DOOR  (time_t) 130 // 
#define SLEEP_CLOSE_DOOR (time_t) 130 // 

void off_zero();


void sigHandler( int signum ) {
    fprintf( stderr, "received signal %d\n", signum) ; 
    off_zero();
}

void off_zero()
{
  int ret ;
  struct timespec rep_slv ;
  struct timespec rep_rsl ;
  rep_slv.tv_sec= 0 ;
  rep_slv.tv_nsec= REPEAT_RATE_NANO_SEC ;

  while(( ret= motor_off()) != SSD650V_MS_STOPPED) {
    fprintf(stderr, "move_door: motor_off != SSD650V_MS_STOPPED\n") ;
    errno= 0;
    ret= nanosleep( &rep_slv, &rep_rsl) ;
    if((errno== EFAULT) || ( errno== EINTR)|| ( errno== EINVAL ))  {
      fprintf( stderr, "off_zero: signal, or error in nanosleep %d\n", ret) ;
    }
  }
  // set setpoint
  while(( ret= set_setpoint(SETPOINT_ZERO)) != SSD650V_MS_OK) {
    fprintf(stderr, "move_door: set_setpoint != SSD650V_MS_OK\n") ;
    errno= 0;
    ret= nanosleep( &rep_slv, &rep_rsl) ;
    if((errno== EFAULT) || ( errno== EINTR)|| ( errno== EINVAL ))  {
      fprintf( stderr, "off_zero: signal, or error in nanosleep %d\n", ret) ;
    }
  }
  fprintf(stderr, "off_zero: motor_off == SSD650V_MS_STOPPED\n") ;
}

void sleep_while_moving()
{
  int    ret ;
  struct timespec slv ;
  struct timespec rsl ;

  fprintf( stderr, "move_door: testRun == TEST_SSD\n") ;
  // sleep
  slv.tv_sec= SLEEP_TEST_SSD ;
  slv.tv_nsec= 0 ;
  errno= 0;
  ret= nanosleep( &slv, &rsl) ;
  if((errno== EFAULT) || ( errno== EINTR)|| ( errno== EINVAL ))  {
    fprintf( stderr, "sleep_while_moving: error in nanosleep %d\n", ret) ;
  }
  off_zero() ;
}
int open_close( float setpoint, float setpoint_slow, int targetState, int testRun, time_t seconds)
{
  int    ret ;
  struct timespec slv ;
  struct timespec rsl ;

  // set setpoint
  if(( ret= set_setpoint( setpoint)) != SSD650V_MS_OK) {
    fprintf(stderr, "move_door: set_setpoint != SSD650V_MS_OK\n") ;
    return -1 ;
  } else {
    // motor_on
    if(( ret= motor_on()) != SSD650V_MS_RUNNING) {
      fprintf(stderr, "move_door: motor_on != SSD650V_MS_RUNNING\n") ;
      return -1 ;
    } else {
      doorState= targetState;
      fprintf( stderr, "move_door: motor running doorState== DS_RUNNING_(OPEN|CLOSE)\n") ;
      // begin test section
      if( testRun == TEST_SSD) {
	sleep_while_moving() ;
	return 0 ;
	// end test section
      } else {
	// after SLEEP_(CLOSE|OPEN_DOOR) seconds sleep drive slowly until OAK digin changes state
	slv.tv_sec= seconds ;
	slv.tv_nsec= 0 ;
	errno= 0;
	ret= nanosleep( &slv, &rsl) ;

	if( ret == -1 ) {
	  if( errno== EINTR)  {
	    fprintf( stderr, "move_door: signal received in nanosleep: %d, motor is presumably stopped\n", ret) ;
	  } else if((errno== EFAULT) ||( errno== EINVAL ))  {
	    fprintf( stderr, "move_door: error in nanosleep: %d\n", ret) ;
	  }
	} else { // ret== 0

	  struct timespec rep_slv ;
	  struct timespec rep_rsl ;

	  rep_slv.tv_sec= 0 ;
	  rep_slv.tv_nsec= REPEAT_RATE_NANO_SEC ;

	  // set setpoint is now low door moves slowly until the end switch is reached (oak_digin_thread)
	  while(( ret= set_setpoint(setpoint_slow)) != SSD650V_MS_OK) {
	    fprintf(stderr, "move_door: set_setpoint != SSD650V_MS_OK\n") ;
	    errno= 0;
	    ret= nanosleep( &rep_slv, &rep_rsl) ;
	    if((errno== EFAULT) || ( errno== EINTR)|| ( errno== EINVAL ))  {
	      fprintf( stderr, "open_close: error in nanosleep %d\n", ret) ;
	    }
	  }
	  fprintf(stderr, "move_door: creeping slowly towards end switch, waiting for oak_digin_thread to switch off\n") ;
	  return 0 ;
	}
      }
    }
  }
  return -1 ;
}

int close_undefined( float setpoint, int targetState, int testRun)
{
  int    ret ;
  // set setpoint
  if(( ret= set_setpoint(setpoint)) != SSD650V_MS_OK) {
    fprintf(stderr, "move_door: set_setpoint != SSD650V_MS_OK\n") ;
    return -1 ;
  } else {
    // motor_on
    if(( ret= motor_on()) != SSD650V_MS_RUNNING) {
      fprintf(stderr, "move_door: motor_on != SSD650V_MS_RUNNING\n") ;
      return -1 ;
    } else {
      doorState= targetState ;
      fprintf( stderr, "move_door: motor running doorState== DS_RUNNING_UNDEF\n") ;

      // begin test section
      if( testRun == TEST_SSD) {
	sleep_while_moving() ;
	return 0 ;
	// end test section
      } else {
	fprintf(stderr, "move_door: creeping slowly towards end switch, waiting for oak_digin_thread to switch off\n") ;
	return 0 ;
      }
    }
  }
  return -1 ;
}
int move_manual( float setpoint)
{
  fprintf(stderr, "move_door: going on-----------------------------------------------------\n") ;
  off_zero();

  if( setpoint ==0.){
    return 0 ;
  }
  fprintf(stderr, "move_door: going on\n") ;
 
  int    ret ;
  // set setpoint
  if(( ret= set_setpoint(setpoint)) != SSD650V_MS_OK) {
    fprintf(stderr, "move_door: set_setpoint != SSD650V_MS_OK\n") ;
    return -1 ;
  } else {
    // motor_on
    if(( ret= motor_on()) != SSD650V_MS_RUNNING) {
      fprintf(stderr, "move_door: motor_on != SSD650V_MS_RUNNING\n") ;
      return -1 ;
    } else {
      fprintf(stderr, "move_door: moving, waiting for oak_digin_thread to switch off\n") ;
    }
    return 0 ;
  }
  return -1 ;
}
void *move_door( void *value)
{
  int    ret ;
  struct timespec slv ;
  struct timespec rsl ;

  if( test_ssd == TEST_SSD) {
    fprintf(stderr, "move_door: test_ssd == TEST_SSD, %ld\n", SLEEP_TEST_SSD) ;
  } 
  signal( SIGUSR2, sigHandler );

  while( 1==1) {
    
    // wildi ToDo MUTEX
    while( doorEvent== EVNT_DOOR_CMD_DO_NOTHING) {
      slv.tv_sec= POLLSEC ;
      slv.tv_nsec= 0 ;
      errno= 0 ;
      ret= nanosleep( &slv, &rsl) ;
      if((errno== EFAULT) || ( errno== EINTR)||( errno== EINVAL ))  {
	switch( errno) {
	case EFAULT:
	  fprintf( stderr, "move_door: error in nanosleep EFAULT=%d\n", errno) ;
	  break ;
	case EINTR:
	  fprintf( stderr, "move_door: error in nanosleep EINTR=%d\n",errno) ;
	  break ;
	case EINVAL:
	  fprintf( stderr, "move_door: error in nanosleep EINVAL=%d\n",errno) ;
	  break ;
	default:
	  fprintf( stderr, "move_door: error in nanosleep NONE of these\n") ;
	}
	fprintf( stderr, "move_door: signal received in nanosleep: %d, motor is presumably stopped\n", ret) ;
      }      
    }
    if(doorEvent== EVNT_DOOR_CMD_OPEN) {
      fprintf(stderr, "move_door: opening door\n") ;
      // check the state
      if( doorState != DS_STOPPED_CLOSED) {
	fprintf(stderr, "move_door: close door first see CLOSE_UDFD, doorState != DS_STOPPED_CLOSED\n") ;
      } else {

	if( (ret= open_close( SETPOINT_OPEN_DOOR, SETPOINT_OPEN_DOOR_SLOW, DS_RUNNING_OPEN, test_ssd, SLEEP_OPEN_DOOR)) == 0) {
	  fprintf(stderr, "move_door: DONE EVNT_DOOR_CMD_OPEN\n") ;
	} else {
	  fprintf(stderr, "move_door: ERROR open_close( SETPOINT_OPEN_DOOR, SETPOINT_OPEN_DOOR_SLOW, DS_RUNNING_OPEN, test_ssd, SLEEP_OPEN_DOOR)\n") ;
	}
      }
      // Now, it is up to oak_digin_thread to say DS_STOPPED_OPEN or what is approprate
    } else if(doorEvent== EVNT_DOOR_CMD_CLOSE){
      fprintf(stderr, "move_door: closing door\n") ;
      // check the state
      if( doorState != DS_STOPPED_OPENED) {
      	fprintf(stderr, "move_door: doorState != DS_STOPPED_OPENED, doing nothing\n") ;
      } else {

	if( (ret= open_close( SETPOINT_CLOSE_DOOR, SETPOINT_CLOSE_DOOR_SLOW, DS_RUNNING_CLOSE, test_ssd, SLEEP_CLOSE_DOOR)) == 0) {
	  fprintf(stderr, "move_door: DONE EVNT_DOOR_CMD_CLOSE\n") ;
	} else {
	  fprintf(stderr, "move_door: ERROR open_close( SETPOINT_CLOSE_DOOR, SETPOINT_CLOSE_DOOR_SLOW, DS_RUNNING_CLOSE, test_ssd, SLEEP_CLOSE_DOOR)\n") ;
	}
      }
      // Now, it is up to oak_digin_thread to say DS_STOPPED_CLOSED  or what is approprate
    } else if(doorEvent== EVNT_DOOR_CMD_CLOSE_IF_UNDEFINED_STATE){
      fprintf(stderr, "move_door: closing door after door state is DS_UNDEF resp. != DS_STOPPED_UNDEF\n") ;
      // check the state
      if( doorState == DS_STOPPED_CLOSED) {
	fprintf(stderr, "move_door: doorState == DS_STOPPED_CLOSED, doing nothing\n") ;
      } else {

	if( (ret=  close_undefined(SETPOINT_UNDEFINED_POSITION, DS_RUNNING_CLOSE_UNDEFINED, test_ssd)) == 0) {
	  fprintf(stderr, "move_door: DONE EVNT_DOOR_CMD_CLOSE_IF_UNDEFINED_STATE\n") ;
	} else {
	  fprintf(stderr, "move_door: ERROR close_undefined(SETPOINT_UNDEFINED_POSITION, DS_RUNNING_CLOSE_UNDEFINED, test_ssd)\n") ;
	}
      }
      // Now, it is up to oak_digin_thread to say DS_STOPPED_CLOSED or what is approprate
    } else if(doorEvent== EVNT_DOOR_CMD_DO_NOTHING){

      fprintf(stderr, "move_door: doing nothing doorState=%d, setpoint=%4.1f\n", doorState, get_setpoint()) ;
    } else {
      fprintf(stderr, "move_door: undefined command %d\n", doorEvent) ;
    }
    // reset doorEvent
    doorEvent= EVNT_DOOR_CMD_DO_NOTHING ;
  }
  return NULL ;
}
