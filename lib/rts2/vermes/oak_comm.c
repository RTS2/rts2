/*
 * oak digital input communication
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
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#endif

#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <time.h>
#include <errno.h>
#include <libnova/libnova.h>
// wildi ToDo: must go aay
#define TEST_OAK 1
#define NO_TEST_OAK 0
int test_oak= NO_TEST_OAK ;
#define SLEEP_TEST_OAK_MILLISECONDS 10500 // !milli seconds!
// wildi ToDo TEMP, must go away
time_t difftimeval(struct timeval *t1, struct timeval *t0) ;

extern int doorState ; 
extern int motorState ; 
extern int debug;
// Manual mode
int ignoreEndswitch;

#include <OakHidBase.h>
#include <OakFeatureReports.h>
int checkOakCall(EOakStatus status, const char *file, int line, const char* function) ;
/// \brief wrapper function for error report that add file,
///        line and function in which an error occured
#define CHECK_OAK_CALL(status) checkOakCall(status, __FILE__, __LINE__, __FUNCTION__)

/* GNU libc dynamic memory allocation tracer. Only needed for debugging */
/* Set environment variable MALLOC_TRACE to a writeable file name. */
//#include <mcheck.h>

#include <vermes.h>
#include <door_vermes.h>
#include <vermes/oak_comm.h>
#include <vermes/util.h>
#include <vermes/move_door.h>
#include <vermes/ssd650v_comm.h>


char date_time[256] ;
char *lastMotorStop_str= date_time ;

int oak_thread_state= THREAD_STATE_UNDEFINED ;
int oakDiginHandle;
DeviceInfo devInfo;      // structure holding oak digital input device informations
pthread_t oak_digin_th_id;
pthread_mutex_t oak_th_mutex;
int oak_digin_thread_heart_beat ;

void *oak_digin_thread(void * args);
int oak_digin_setup(int* deviceHandle, char* oakDiginDevice) ;

#define OAK_MASK_CLOSED            0x01
#define OAK_MASK_OPENED            0x02 
#define OAK_MASK_PRE_CLOSED        0x04  // wildi  ToDo: not yet used here
#define OAK_MASK_END_SWITCHES      0x08 

/******************************************************************************
 * connectOakDevice(int connecting)
 *****************************************************************************/
int
connectOakDiginDevice(int connecting)
{
  int oak_stat;
  int thread_stat= -1; // if not set to 0 main program dies
  char oakDiginDevice[]="/dev/door_switch" ;

  if (connecting== OAKDIGIN_CMD_CONNECT) {

    strcpy( date_time, "undefined") ;
    oak_stat = oak_digin_setup(&oakDiginHandle, oakDiginDevice);
    
    if (oak_stat == 0) {
      fprintf( stderr,  "connectOakDiginDevice: opened %s:\n", oakDiginDevice);
      
      // Set the LED Mode to OFF
      CHECK_OAK_CALL(setLedMode(oakDiginHandle, eLedModeOn, false));

      // wildi ToDo int* values = xmalloc(devInfo.numberOfChannels * sizeof(int));
      int* values = malloc( 64 * sizeof(int));
      int rd_stat;

      oak_digin_thread_heart_beat= 0 ;
      rd_stat = readInterruptReport(oakDiginHandle, values);
      if ((rd_stat < 0) && (errno == EINTR)) {
	fprintf( stderr, "connectOakDiginDevice: no connection to OAK device") ;
	oak_thread_state= THREAD_STATE_UNDEFINED ;
      } else {
	oak_thread_state= THREAD_STATE_RUNNING ;
      }
      int bits = values[1] & 0xff;
      // it is the duty of connectSSD650vDevice() call to set the state first
      // setting the initial DS_ state
      if( motorState== SSD650V_MS_STOPPED) { // 
	fprintf(stderr, "connectOakDiginDevice: motorState == SSD650V_MS_STOPPED\n") ;
	if( bits & OAK_MASK_CLOSED) { // true if OAK sees the door
	  doorState= DS_STOPPED_CLOSED ;
	  fprintf( stderr, "connectOakDiginDevice: state is DS_STOPPED_CLOSED\n") ;
	} else if( bits & OAK_MASK_OPENED) { 
	  doorState= DS_STOPPED_OPENED ;
	  fprintf( stderr, "connectOakDiginDevice: state is DS_STOPPED_OPEN\n") ;
	} else {
	  doorState= DS_UNDEF ;
	  fprintf( stderr, "connectOakDiginDevice: state is DS_UNDEF, door not closed, motor is off\n") ;
	}
      } else if( bits & OAK_MASK_OPENED) { // true if OAK sees the door
	doorState= DS_UNDEF ;
	fprintf( stderr, "connectOakDiginDevice: state is DS_UNDEF, door is opened, motor is undefined\n") ;
      } else if( bits & OAK_MASK_CLOSED) { // true if OAK sees the door
	doorState= DS_UNDEF ;
	fprintf( stderr, "connectOakDiginDevice: state is DS_UNDEF, door is closed, motor is undefined\n") ;
      } else {
	doorState= DS_UNDEF ;
	fprintf( stderr, "connectOakDiginDevice: state is DS_UNDEF, door is undefined, motor is undefined\n") ;
      }
      // create receive communication thread
      thread_stat = pthread_create(&oak_digin_th_id, NULL, &oak_digin_thread, NULL); // 0 success

      if (thread_stat != 0) {
	oak_thread_state= THREAD_STATE_UNDEFINED ;
	fprintf( stderr,  "connectOakDiginDevice: failure starting thread: error %d:\n", thread_stat);
      } 
    } else {
      doorState= DS_UNDEF ;
      fprintf( stderr, "connectOakDiginDevice: state is DS_UNDEF, no connection to oak device\n") ;
    }
  } else { // if (connecting)
    /* Disconnect: prepare for reconnect. */
    // Set the LED Mode to OFF
    if (oakDiginHandle >= 0) {
      thread_stat = pthread_cancel(oak_digin_th_id); // 0 = success

      if( thread_stat != 0 ) {
	fprintf( stderr,  "connectOakDiginDevice: Oak thread NOT stopped\n");
	return thread_stat ;
      }
      CHECK_OAK_CALL(setLedMode(oakDiginHandle, eLedModeOff, false));
      CHECK_OAK_CALL(closeDevice(oakDiginHandle));
    }
    if (oakDiginHandle >= 0) {
      fprintf( stderr,  "connectOakDiginDevice: closed %s.\n", oakDiginDevice);
    } else {
      fprintf( stderr,  "connectOakDiginDevice: reset for device %s.\n", oakDiginDevice);
    }
    fprintf( stderr,  "connectOakDiginDevice: Oak thread stopped\n");
  }
  return thread_stat ;
}
/*****************************************************************************
 * oak_digin_setup(..)
 *****************************************************************************/
int oak_digin_setup(int* deviceHandle, char* oakDiginDevice)
{
  int oak_stat;

  oak_stat = CHECK_OAK_CALL(openDevice(oakDiginDevice, deviceHandle));
  if (oak_stat != 0) return oak_stat;

  // get the device informations
  oak_stat = CHECK_OAK_CALL(getDeviceInfo(oakDiginHandle, &devInfo));
  if (oak_stat != 0) return oak_stat;

/*   fprintf( stderr, "oak_digin_setup: Oak device: %s\n", devInfo.deviceName); */
/*   fprintf( stderr, "oak_digin_setup: Volatile user device name: %s\n", devInfo.volatileUserDeviceName); */
/*   fprintf( stderr, "oak_digin_setup: Persistent user device name: %s\n", devInfo.persistentUserDeviceName); */
/*   fprintf( stderr, "oak_digin_setup: Serial number: %s\n", devInfo.serialNumber); */
/*   fprintf( stderr, "oak_digin_setup: VendorID: 0x%.4x :: ProductID: 0x%.4x :: Version 0x%.4x\n", devInfo.vendorID, devInfo.productID, devInfo.version); */
/*   fprintf( stderr, "oak_digin_setup: Number of channels: %d\n", devInfo.numberOfChannels); */

  if (strcmp(devInfo.deviceName, "Toradex Optical Isolated Input")) {
    return -1;
  }
  // Set the LED Mode to indicate initialize state
  oak_stat = CHECK_OAK_CALL( setLedMode(*deviceHandle, eLedModeBlinkFast, false));
  if (oak_stat != 0) return oak_stat;

  // Set the report Mode
  oak_stat = CHECK_OAK_CALL(setReportMode(*deviceHandle, eReportModeAfterSampling, false));

  if (oak_stat != 0) return oak_stat;

  // Set the sample rate
  oak_stat = CHECK_OAK_CALL( setSampleRate(*deviceHandle, 50, false));
  if (oak_stat != 0) return oak_stat;

  // Retrieve Channel infos
  /* wildi ToDo
   * ChannelInfo* channelInfos = xmalloc(devInfo.numberOfChannels *sizeof(ChannelInfo));
   * ChannelInfo* chanInfo;
   * chanInfo = &channelInfos[1];
  */
  return 0;
}
/*****************************************************************************
 * checkOakCall(..)
 *****************************************************************************/
int checkOakCall(EOakStatus status, const char *file, int line, const char* function)
{
  if (status != eOakStatusOK) {
    if (debug > 0) {
      fprintf(stderr, "checkOakCall: Error: %s in function %s (file %s, line %d)\n",
	      getStatusString(status), function, file, line);
    }
    return status;
  } else {
    return 0;
  }
}
#define STOP_MOTOR           200
#define STOP_MOTOR_UNDEFINED 201
/*****************************************************************************
 * oak_digin_thread()
 * Reads interrupt reports arriving from Oak digital input module in an
 * infinite loop, checks whether any of the inputs changed state and signals
 * the state transition
 *****************************************************************************/
// wildi ToDo this thread must go into a separate file
void *
oak_digin_thread(void * args)
{
  int ret ;
  // wildi ToDo int *values = xmalloc(devInfo.numberOfChannels * sizeof(int));
  int *values = malloc(200 * sizeof(int));
  int rd_stat;
  int bits = 0xff;
  int print_bits = 0xff;
  int bit;
  int i ;
  int lastDoorStateTest= DS_UNDEF ;
  int lastDoorState= DS_UNDEF ;
  int stop_motor= STOP_MOTOR_UNDEFINED ;
  char bits_str[32] ;
  struct timespec sl ;
  struct timespec rsl ;
  static struct timeval time_last_stat;
  static struct timeval time_enq;
  gettimeofday(&time_last_stat, NULL);
  // debug
  int print =0 ;
  oak_thread_state= THREAD_STATE_UNDEFINED ; // make it "reentrant"

  sl.tv_sec= 0. ;
  sl.tv_nsec= (long ) 200 * 1000 * 1000 ;

  if( test_oak== TEST_OAK) {
    fprintf( stderr,  "oak_digin_thread: test_oak== TEST_OK, sleep %d milliseconds\n", SLEEP_TEST_OAK_MILLISECONDS);
  } 
  fprintf( stderr,  "oak_digin_thread: thread started\n");

  while (1) {

    oak_digin_thread_heart_beat++ ;

    if( oak_digin_thread_heart_beat > HEART_BEAT_UPPER_LIMIT) {
      oak_digin_thread_heart_beat= 0 ;
    }
    rd_stat = readInterruptReport(oakDiginHandle, values);
    if ((rd_stat < 0) && (errno == EINTR)) {
      oak_thread_state= THREAD_STATE_UNDEFINED ;
      // wildi ToDo something sensible here values[1]=
      fprintf( stderr, "oak_digin_thread:  problems reading from oak device\n") ;
    }
    bits      = values[1] & 0xff ;
    print_bits= values[1] & 0xff ;
   
    print= 0 ;
    for (i = 0; i < 8; i++) {
      bit = (print_bits & 0x1) ;
      if( bit) {
	bits_str[7-i]= '1' ;
	print++ ;
      } else {
	bits_str[7-i]= '0' ;
      }
      print_bits = print_bits >> 1;
    }
    bits_str[8]= '\0' ;
    //fprintf( stderr, "oak_digin_thread: doorState=%d, bits %s\n",  doorState, bits_str) ;
    // turn off the motor first

    if((( bits & OAK_MASK_CLOSED) > 0) && (( doorState== DS_STOPPED_CLOSED) || ( doorState== DS_RUNNING_OPEN)))  {
      // let the motor open the door
      stop_motor= STOP_MOTOR_UNDEFINED ;

    } else if((( bits & OAK_MASK_OPENED) > 0) && (( doorState== DS_STOPPED_OPENED) || ( doorState== DS_RUNNING_CLOSE) || ( doorState== DS_RUNNING_CLOSE_UNDEFINED)))  {
      // let the motor close the door
      stop_motor= STOP_MOTOR_UNDEFINED ;

    } else if((( bits & OAK_MASK_CLOSED) > 0) && ( motorState== SSD650V_MS_RUNNING)) { // 2nd expr. necessary not to overload SSD650v on RS 232
      stop_motor= STOP_MOTOR ;
      fprintf( stderr, "oak_digin_thread: found ( bits & OAK_MASK_CLOSED) > 0), bits %s\n",  bits_str) ;

    } else if((( bits & OAK_MASK_OPENED) > 0) && ( motorState== SSD650V_MS_RUNNING)){
      stop_motor= STOP_MOTOR ;
      fprintf( stderr, "oak_digin_thread: found (bits & OAK_MASK_OPENED) > 0), bits %s\n",  bits_str) ;

    } else if ( (bits & OAK_MASK_END_SWITCHES) > 0) {  
      // the motor is turned off and eventually OAK devices too (no cummunication possible)
      stop_motor= STOP_MOTOR ;
    } else if ( (bits & OAK_MASK_END_SWITCHES) == 0) { // both end switches are zero 
      // the door is inbetween, (or beyond the software end switches :-(( = very close to the end switches)
      //fprintf( stderr, "oak_digin_thread:  might be a bad reading from oak device\n") ; // see above ToDo
      // wildi ToDo: not yet defined what to do
    } 

    // test
    if( test_oak== TEST_OAK) {
      if( lastDoorStateTest != doorState) {
	lastDoorStateTest= doorState ;
	gettimeofday(&time_last_stat, NULL);
	fprintf( stderr, "oak_digin_thread:  status change resetting timer\n") ;
      }
      if( motorState== SSD650V_MS_RUNNING) {
	gettimeofday(&time_enq, NULL);
	if (difftimeval(&time_enq, &time_last_stat) > (SLEEP_TEST_OAK_MILLISECONDS * 1000)) { // milliseconds
	  stop_motor= STOP_MOTOR ;
	  fprintf( stderr, "oak_digin_thread: motorState== (DS_RUNNING_OPEN|DS_RUNNING_CLOSE), stopping now, diff %d[msec]\n", (int)difftimeval(&time_enq, &time_last_stat)) ;
	  gettimeofday(&time_last_stat, NULL);
	  fprintf( stderr, "oak_digin_thread: stopping motor  after time ellapsed\n") ;
	}
      }
      if(( print > 0) &&  ( stop_motor != STOP_MOTOR)) {
	stop_motor= STOP_MOTOR ;
	fprintf( stderr, "oak_digin_thread: print stop_motor= STOP_MOTOR \n") ;
      }
    }
    // end test
    // manual mode, in case an endswitch erroneously indicates contact
    if(ignoreEndswitch==NOT_IGNORE_END_SWITCH){
    if( stop_motor== STOP_MOTOR) {
      
      fprintf( stderr, "oak_digin_thread: stopping motor now\n") ;
      while(( ret= motor_off()) != SSD650V_MS_STOPPED) { // 
	fprintf( stderr, "oak_digin_thread: can not turn motor off\n") ;
	ret= nanosleep( &sl, &rsl) ;
	if((errno== EFAULT) || ( errno== EINTR)||( errno== EINVAL ))  {
	  fprintf( stderr, "Error in nanosleep\n") ;
	  errno= 0 ;
	}
      }
      set_setpoint(0.);
      fprintf( stderr, "oak_digin_thread: motor stopped\n") ;
      stop_motor= STOP_MOTOR_UNDEFINED ;
      struct ln_date utm;
      ln_get_date_from_sys( &utm) ;
      sprintf( date_time, "%4d-%02d-%02dT%02d:%02d:%02d", utm.years, utm.months, utm.days, utm.hours, utm.minutes, (int) utm.seconds) ;

    }
    }
    // door status
    if( motorState== SSD650V_MS_STOPPED) {
      // change status here
      if( bits & OAK_MASK_CLOSED) { // true if OAK sees the door
	doorState= DS_STOPPED_CLOSED ;
	if( lastDoorState != doorState) {
	  lastDoorState= doorState ;
	  fprintf( stderr, "oak_digin_thread: state is DS_STOPPED_CLOSED\n") ;
	} 
      } else if( bits & OAK_MASK_OPENED) { // true if OAK sees the door
	doorState= DS_STOPPED_OPENED ;
	if( lastDoorState != doorState) {
	  lastDoorState= doorState ;
	  fprintf( stderr, "oak_digin_thread: state is DS_STOPPED_OPENED\n") ;
	}
      } else {
	doorState= DS_STOPPED_UNDEF ;
	lastDoorState= doorState ;
	//fprintf( stderr, "oak_digin_thread: state is DS_STOPPED_UNDEFINED\n") ;
      }
    } else if( motorState== SSD650V_MS_RUNNING) {
      // keep status here 
      if( doorState == DS_RUNNING_OPEN) {
      } else if( doorState == DS_RUNNING_CLOSE) {
      } else if( doorState == DS_RUNNING_CLOSE_UNDEFINED) {
      } else {
	// should occasianally occur if the door is stopped manually 
	doorState= DS_RUNNING_UNDEF ;
	lastDoorState= doorState ;
	//fprintf( stderr, "oak_digin_thread: motorState== SSD650V_MS_RUNNING, state is DS_RUNNING_UNDEF\n") ;
      }
    } else if( motorState== SSD650V_MS_UNDEFINED) {
      doorState= DS_UNDEF ;
      lastDoorState= doorState ;

      if( bits & OAK_MASK_OPENED) { // true if OAK sees the door
	fprintf( stderr, "oak_digin_thread: state is DS_UNDEF, door is opened, motor is undefined\n") ;
      } else if( bits & OAK_MASK_CLOSED) { // true if OAK sees the door
	fprintf( stderr, "oak_digin_thread: state is DS_UNDEF, door is closed, motor is undefined\n") ;
      } else {
	fprintf( stderr, "oak_digin_thread: state is DS_UNDEF, door is undefined, motor is undefined\n") ;
      }
    } else {
      // should never occur
      doorState= DS_UNDEF ;
      lastDoorState= doorState ;
      fprintf( stderr, "oak_digin_thread: state is DS_UNDEF, should never never occur\n") ;
    }

    CHECK_OAK_CALL(rd_stat); // wildi ToDo: what's that?
  } /* while (1) */
  
  return NULL;
}
