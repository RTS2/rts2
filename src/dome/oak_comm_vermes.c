/*
 * OAK digital input communication
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
//#include <malloc.h>
//#include <sys/time.h>
//#include <math.h>
#endif
#include <errno.h>
#include <pthread.h>

// wildi ToDo: clarify with Lukas
#ifdef CHECK_MALLOC
//#include <malloc.h>
//#include <mcheck.h>
#endif

extern int doorState ; 

#include "OakHidBase.h"
#include "OakFeatureReports.h"
int checkOakCall(EOakStatus status, const char *file, int line, const char* function) ;
/// \brief wrapper function for error report that add file,
///        line and function in which an error occured
#define CHECK_OAK_CALL(status) checkOakCall(status, __FILE__, __LINE__, __FUNCTION__)

/* GNU libc dynamic memory allocation tracer. Only needed for debugging */
/* Set environment variable MALLOC_TRACE to a writeable file name. */
//#include <mcheck.h>

#include "vermes.h"
#include "door_vermes.h"
#include "oak_comm_vermes.h"
#include "util_vermes.h"
#include "move-door_vermes.h"
#include "ssd650v_comm_vermes.h"

extern int debug;

int oakDiginHandle;
DeviceInfo devInfo;      // structure holding oak digital input device informations
pthread_t oak_digin_th_id;
pthread_mutex_t oak_th_mutex;

int intr_cnt = 0;
void *oak_digin_thread(void * args);
int oak_digin_setup(int* deviceHandle, char* oakDiginDevice) ;

/******************************************************************************
 * connectOakDevice(int connecting)
 *****************************************************************************/
void
connectOakDiginDevice(int connecting)
{
    int oak_stat;
    int thread_stat;
    char oakDiginDevice[]="/dev/door_switch" ;
    if (connecting== OAKDIGIN_CONNECT) {
#ifdef CHECK_MALLOC
	print_mem_usage("connectDevice(true)");
#endif
	fprintf( stderr, "connectOakDiginDevice-------------\n") ;
	oak_stat = oak_digin_setup(&oakDiginHandle, oakDiginDevice);
    
	if (oak_stat == 0) {
	    fprintf( stderr,  "opened %s:\n", oakDiginDevice);

	    // Set the LED Mode to OFF
	    CHECK_OAK_CALL(setLedMode(oakDiginHandle, eLedModeOff, false));

	    // create receive communication thread
	    thread_stat = pthread_create(&oak_digin_th_id, NULL,
					 &oak_digin_thread, NULL);
	    if (thread_stat != 0) {
		fprintf( stderr,  "failure starting thread: error %d:\n", thread_stat);
	    } 

	    // check door state and if not closed ABORT
	    int* values = xmalloc(devInfo.numberOfChannels * sizeof(int));
	    int rd_stat;

	    rd_stat = readInterruptReport(oakDiginHandle, values);
	    if ((rd_stat < 0) && (errno == EINTR)) {
		fprintf( stderr, "connectOakDiginDevice: no connection to OAK device") ;
	    }
	    int bits = values[1] & 0xff;
	    int ret ;
	    if( bits & 0x01) {
		if(( ret= motor_off()) != SSD650V_MS_STOPPED) {
		    fprintf(stderr, "move_door: motor_on != SSD650V_MS_STOPPED\n") ;
		    // set setpoint
		    if(( ret= set_setpoint(SETPOINT_ZERO)) != 0) {
			fprintf(stderr, "move_door: set_setpoint != 0\n") ;
		    } else {

			doorState= DS_STOPPED_CLOSED ;
			fprintf( stderr, "connectOakDiginDevice: state is DS_STOPPED_CLOSED\n") ;
		    }
		} else if( bits & 0x01){
		    doorState= DS_RUNNING_OPEN ;
		    fprintf( stderr, "connectOakDiginDevice: state is DS_RUNNING_OPEN\n") ;
		} else {
		    fprintf( stderr, "connectOakDiginDevice: state is NOT    DS_STOPPED_CLOSED: %d, %d\n", bits & 0x01, bits) ;
		}
	    } else {
		fprintf( stderr,  "failure initializing %s:\n", oakDiginDevice);
	    }
	}
    } else { // if (connecting)
	/* Disconnect: prepare for reconnect. */
	
	// Set the LED Mode to OFF
	if (oakDiginHandle >= 0) {
	    CHECK_OAK_CALL(setLedMode(oakDiginHandle, eLedModeOff, false));
	    CHECK_OAK_CALL(closeDevice(oakDiginHandle));
	    pthread_cancel(oak_digin_th_id);
	}
	if (oakDiginHandle >= 0) {
	    fprintf( stderr,  "closed %s.\n", oakDiginDevice);
	} else {
	    fprintf( stderr,  "reset for device %s.\n", oakDiginDevice);
	}

#ifdef CHECK_MALLOC
	print_mem_usage("connectDevice(false)");
#endif
    }
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

  fprintf( stderr, "Oak device: %s\n", devInfo.deviceName);
  fprintf( stderr, "Volatile user device name: %s\n",
                   devInfo.volatileUserDeviceName);
  fprintf( stderr, "Persistent user device name: %s\n",
                   devInfo.persistentUserDeviceName);
  fprintf( stderr, "Serial number: %s\n", devInfo.serialNumber);
  fprintf( stderr, "VendorID: 0x%.4x :: ProductID: 0x%.4x :: Version 0x%.4x\n",
                   devInfo.vendorID, devInfo.productID, devInfo.version);
  fprintf( stderr, "Number of channels: %d\n", devInfo.numberOfChannels);

  if (strcmp(devInfo.deviceName, "Toradex Optical Isolated Input")) {
    return -1;
  }

  // Set the LED Mode to indicate initialize state
  oak_stat = CHECK_OAK_CALL(
               setLedMode(*deviceHandle, eLedModeBlinkFast, false)
             );
  if (oak_stat != 0) return oak_stat;

  // Set the report Mode
  oak_stat = CHECK_OAK_CALL(
               setReportMode(*deviceHandle, eReportModeAfterSampling, false)
             );
  if (oak_stat != 0) return oak_stat;

  // Set the sample rate
  oak_stat = CHECK_OAK_CALL(
               setSampleRate(*deviceHandle, 50, false)
             );
  if (oak_stat != 0) return oak_stat;

  // Retrieve Channel infos
  /*
  ChannelInfo* channelInfos = xmalloc(devInfo.numberOfChannels
                                      * sizeof(ChannelInfo));
  ChannelInfo* chanInfo;
  chanInfo = &channelInfos[1];
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
      fprintf(stderr, "Error: %s in function %s (file %s, line %d)\n",
                      getStatusString(status), function, file, line);
    }
    return status;

  } else {
    return 0;
  }
}

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
    int* values = xmalloc(devInfo.numberOfChannels * sizeof(int));
    int rd_stat;
    fprintf( stderr,  "oak_digin_thread\n");

    while (1) {
	rd_stat = readInterruptReport(oakDiginHandle, values);
	if ((rd_stat < 0) && (errno == EINTR)) {
	    break;
	}
	CHECK_OAK_CALL(rd_stat);
	// Set the LED Mode to ON
	CHECK_OAK_CALL(setLedMode(oakDiginHandle, eLedModeOn, false));
	
	int bits = values[1] & 0xff;
	int bit;
	int i ;
	char bits_str[32] ;
	for (i = 0; i < 8; i++) {
	    bit = (bits & 0x1) ;
	    if( bit) {
		bits_str[7-i]= '1' ;
	    } else {
		bits_str[7-i]= '0' ;
	    }
	    bits = bits >> 1;
	}
	bits_str[8]= '\0' ;
	//fprintf( stderr, "bits %s\n", bits_str) ;
	// Set the LED Mode to OFF
	CHECK_OAK_CALL(setLedMode(oakDiginHandle, eLedModeOff, false));
	
	// every 10th interrupt report as a heart beat indicator.
	intr_cnt++;
	if (intr_cnt % 10 == 0) {
	    //fprintf( stderr, "oak_digin_thread: intr_cnt %% 10 == 0\n") ;
	    //fprintf( stderr, "bits %s\n", bits_str) ;
	} else if (intr_cnt % 20 == 10) {
	    //fprintf( stderr, "oak_digin_thread: intr_cnt %% 20 == 10\n") ;
	    //fprintf( stderr, "bits %s\n", bits_str) ;
	}
    } /* while (1) */

  return NULL;
}
