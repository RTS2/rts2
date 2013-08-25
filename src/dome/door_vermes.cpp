/* 
 * dome door driver for Obs. Vermes.
 * Copyright (C) 2010 Markus Wildi
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <signal.h>
#include <pthread.h>

#include <connection.h>
#include <dome.h>
#include <vermes.h> 
#include <door_vermes.h> 
#include <vermes/move_door.h>
#include <vermes/ssd650v_comm.h>
#include <vermes/oak_comm.h>

extern int oak_thread_state ;
extern int doorState;
extern int doorEvent ;
extern useconds_t sleep_max ;
extern int oak_digin_thread_heart_beat ;
extern char *lastMotorStop_str ;
extern int motorState ;
extern int ignoreEndswitch;


int last_oak_digin_thread_heart_beat ;
extern pthread_t  move_door_id;

#define DOOR_MOVING_TIME_SIMULATION 30  //[sec]

namespace rts2dome {
/*
 * Class to control of Obs. Vermes cupola door. 
 *
 * @author Markus Wildi <markus.wildi@one-arcsec.org>
 */
  class DoorVermes: public Dome 
  {
  private:
    rts2core::ValueString  *doorStateMessage;
    rts2core::ValueBool *block_door;
    rts2core::ValueBool *open_door;
    rts2core::ValueBool *stop_door;
    rts2core::ValueBool *close_door;
    rts2core::ValueBool *close_door_undefined;
    rts2core::ValueBool *simulate_door;
    rts2core::ValueBool *manual;
    rts2core::ValueDouble *ssd650v_setpoint ;
    time_t nextDeadCheck; // wildi ToDo: clarify what happens!
    rts2core::ValueString *lastMotorStop ;  
    rts2core::ValueDouble  *ssd650v_read_setpoint ;
    rts2core::ValueDouble  *ssd650v_current ;

    /**
     * Update status messages.
     *
     * @return 0 on success.
     */
    void updateDoorStatus ();
    void updateDoorStatusMessage ();

  protected:
    virtual int processOption (int _opt);
    virtual int init ();
    virtual int info ();

    virtual int idle ();
    
    virtual int startOpen ();
    virtual long isOpened ();
    virtual int endOpen ();
    
    virtual int startClose ();
    virtual long isClosed ();
    virtual int endClose ();
  public:
    DoorVermes (int argc, char **argv);
    virtual ~DoorVermes ();
    virtual void valueChanged (rts2core::Value * changed_value) ;
  };
}

using namespace rts2dome;

// only those door_vermes internal conditions are listed below
// which can be detected by the sensors (ssd650v, oak digital inputs).
// DOME_DOME_MASK is set by dome::methods and it relies on it.

void
DoorVermes::updateDoorStatus ()
{
  switch (doorState){
  case DS_UNDEF:
    maskState (DOME_DOME_MASK, DOME_UNKNOW, "dome state is undefined") ;
    break;
  case DS_STOPPED_CLOSED:
    //maskState (DOME_DOME_MASK, DOME_CLOSED, "dome state is stopped, closed") ;
    break;
  case DS_STOPPED_OPENED:
    //NO: maskState (DOME_DOME_MASK, DOME_OPENED, "dome state is stopped, open") ;
    break;
  case DS_STOPPED_UNDEF:
    maskState (DOME_DOME_MASK, DOME_UNKNOW, "dome state is stopped, undefined") ;
    break;
  case DS_EMERGENCY_ENDSWITCH:
    maskState (DOME_DOME_MASK, DOME_UNKNOW, "emergency end switch opened or closed is ON") ;
    break;
  }
}
void
DoorVermes::updateDoorStatusMessage ()
{
  switch (doorState){
  case DS_UNDEF:
    doorStateMessage->setValueString("undefined") ;
    break;
  case DS_STOPPED_CLOSED:
    doorStateMessage->setValueString("stopped, closed") ;
    break;
  case DS_STOPPED_OPENED:
    doorStateMessage->setValueString("stopped, open") ;
    break;
  case DS_STOPPED_UNDEF:
    doorStateMessage->setValueString("stopped, undefined") ;
    break;
  case DS_RUNNING_OPEN:
    doorStateMessage->setValueString("opening") ;
    break;
  case DS_RUNNING_CLOSE:
    doorStateMessage->setValueString("closing") ;
    break;
  case DS_RUNNING_UNDEF:
  case DS_RUNNING_CLOSE_UNDEFINED:
    doorStateMessage->setValueString("closing, undefined") ;
    break;
  case DS_EMERGENCY_ENDSWITCH:
    doorStateMessage->setValueString("emergency end switch ON") ;
    break;
  default:
    char msg[256] ;
    sprintf( msg, "door state is undefined, state %3d", doorState) ;
    doorStateMessage->setValueString( msg) ;
  }
}
void 
DoorVermes::valueChanged (rts2core::Value * changed_value)
{
  if( simulate_door->getValueBool()) { 
    // do not set status here
  } else {
    last_oak_digin_thread_heart_beat= oak_digin_thread_heart_beat ;
    struct timespec sl ;
    struct timespec rsl ;
    sl.tv_sec= 0. ;
    sl.tv_nsec= WAITING_FOR_HEART_BEAT_NANO_SEC; 
    nanosleep( &sl, &rsl) ;

    if( oak_digin_thread_heart_beat != last_oak_digin_thread_heart_beat) {
      //logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged heart beat present" << sendLog ;
      oak_thread_state= THREAD_STATE_RUNNING ;
    } else {
      logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged no heart beat from Oak thread" << sendLog ;
      oak_thread_state= THREAD_STATE_UNDEFINED ;
      // do not exit here
    }
  }
  // stop motor
  if(changed_value == stop_door) {
    if( stop_door->getValueBool()) {
      logStream (MESSAGE_INFO) << "DoorVermes::valueChanged stopping door" << sendLog ;
      // it is already stop_door->setValueBool(true) ; 
      open_door->setValueBool(false) ;
      close_door->setValueBool(false) ;
      close_door_undefined->setValueBool(false) ;
      stop_door->setValueBool(false) ; // in order that it can be repeated faster
      updateDoorStatus () ;

      if( simulate_door->getValueBool()){
	doorState=DS_UNDEF ;
      } else {
        // sigHandler stops motor
	pthread_kill( move_door_id, SIGUSR2);
      }
    } else {
      logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged use TRUE to stop motor" << sendLog ;
    }
    return ;
    // open door
  } else if (changed_value == open_door) {
    if( open_door->getValueBool()) {

      logStream (MESSAGE_INFO) << "DoorVermes::valueChanged opening door" << sendLog ;
      stop_door->setValueBool(false) ;
      // it is already open_door->setValueBool(true) ;
      close_door->setValueBool(false) ;
      close_door_undefined->setValueBool(false) ;

      if( oak_thread_state== THREAD_STATE_RUNNING) {
	if( simulate_door->getValueBool()){
	  doorState=DS_STOPPED_OPENED ;
	  updateDoorStatus () ;

	} else { // the real thing
	  doorEvent= EVNT_DOOR_CMD_OPEN ;
	  logStream (MESSAGE_INFO) << "DoorVermes::valueChanged doorEvent= EVNT_DOOR_CMD_OPEN: opening" << sendLog ;
	  int ret ;
	  int j ;
	  struct timespec sl ;
	  struct timespec rsl ;
	  sl.tv_sec= 0. ;
	  sl.tv_nsec= REPEAT_RATE_NANO_SEC; 

	  for( j= 0 ; j < 6 ; j++) {
	    ret= nanosleep( &sl, &rsl) ;
	    if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
	      fprintf( stderr, "Error in nanosleep\n") ;
	    }
	    if( motorState== SSD650V_MS_RUNNING) {
	      logStream (MESSAGE_INFO) << "DoorVermes::valueChanged doorEvent= EVNT_DOOR_CMD_OPEN: motor running" << sendLog ;
	      break ;
	    }
	  }
	  if( j== 6) {
	    logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged doorEvent= EVNT_DOOR_CMD_OPEN: failed" << sendLog ;
	  }
	}
      } else {
	block_door->setValueBool(true) ;
	logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged oak_digin_thread died" << sendLog ;
      }
    } else {
      logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged use CLOSE_DOOR to close the door" << sendLog ;
    }
    return ;
    // close door
  } else if (changed_value == close_door) {
    if( close_door->getValueBool()) {

      logStream (MESSAGE_INFO) << "DoorVermes::valueChanged closing door" << sendLog ;
      stop_door->setValueBool(false) ; 
      open_door->setValueBool(false) ;
      // it is already close_door->setValueBool(true) ;
      close_door_undefined->setValueBool(false) ;

      if( oak_thread_state== THREAD_STATE_RUNNING) {
	if( simulate_door->getValueBool()){
	  doorState=DS_STOPPED_CLOSED ;
	  updateDoorStatus () ;

	} else { // the real thing
	  doorEvent= EVNT_DOOR_CMD_CLOSE ;
	  logStream (MESSAGE_INFO) << "DoorVermes::valueChanged doorEvent= EVNT_DOOR_CMD_CLOSE: closing" << sendLog ;
	  int ret ;
	  int j ;
	  struct timespec sl ;
	  struct timespec rsl ;
	  sl.tv_sec= 0. ;
	  sl.tv_nsec= REPEAT_RATE_NANO_SEC; 
	
	  for( j= 0 ; j < 6 ; j++) {
	    ret= nanosleep( &sl, &rsl) ;
	    if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
	      fprintf( stderr, "Error in nanosleep\n") ;
	    }
	    if( motorState== SSD650V_MS_RUNNING) {
	      logStream (MESSAGE_INFO) << "DoorVermes::valueChanged doorEvent= EVNT_DOOR_CMD_CLOSE: motor running" << sendLog ;
	      break ;
	    }
	  }
	  if( j== 6) {
	    logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged doorEvent= EVNT_DOOR_CMD_CLOSE: failed" << sendLog ;
	  }
	}
      } else {
	block_door->setValueBool(true) ;
	logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged oak_digin_thread died" << sendLog ;
      }
    } else {
      logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged use OPEN_DOOR to open the door" << sendLog ;
    }
    return ;
    // close door if state is undefined
  } else if (changed_value == close_door_undefined) {
    if( close_door_undefined->getValueBool()) {

      logStream (MESSAGE_WARNING) << "DoorVermes::valueChanged door state undefined, closing door slowly" << sendLog ;
      stop_door->setValueBool(false) ; 
      open_door->setValueBool(false) ;
      close_door->setValueBool(false) ;
      // it is already close_door_undefined->setValueBool(true) ;

      if( doorState== DS_STOPPED_CLOSED) {
	  logStream (MESSAGE_INFO) << "DoorVermes::valueChanged do not close due to being already in state DS_STOPPED_CLOSED" << sendLog ;
	  return ;
      }

      if( oak_thread_state== THREAD_STATE_RUNNING) {
	if( simulate_door->getValueBool()){
	  doorState=DS_STOPPED_CLOSED ;
	  updateDoorStatus () ;

	} else { // the real thing
	  doorEvent= EVNT_DOOR_CMD_CLOSE_IF_UNDEFINED_STATE ;
	  logStream (MESSAGE_INFO) << "DoorVermes::valueChanged doorEvent= EVNT_DOOR_CMD_CLOSE_IF_UNDEFINED_STATE: closing undefined" << sendLog ;
	  int ret ;
	  int j ;
	  struct timespec sl ;
	  struct timespec rsl ;
	  sl.tv_sec= 0. ;
	  sl.tv_nsec= REPEAT_RATE_NANO_SEC; 

	  for( j= 0 ; j < 6 ; j++) {
	    ret= nanosleep( &sl, &rsl) ;
	    if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
	      fprintf( stderr, "Error in nanosleep\n") ;
	    }
	    if( motorState== SSD650V_MS_RUNNING) {
	      logStream (MESSAGE_INFO) << "DoorVermes::valueChanged doorEvent= EVNT_DOOR_CMD_CLOSE_IF_UNDEFINED_STATE: motor running" << sendLog ;
	      break ;
	    }
	  }
	  if( j== 6) {
	    logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged doorEvent= EVNT_DOOR_CMD_CLOSE_IF_UNDEFINED_STATE: failed" << sendLog ;
	  }
	}
      } else {
	logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged oak_digin_thread died" << sendLog ;
      }
    } else {
      logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged use CLOSE_UDFD=true to close the door" << sendLog ;
    }
    return ;
    // toggle simulation, real mode
  } else if (changed_value == simulate_door) {

    if( simulate_door->getValueBool()) {

      // do not switch to simulation mode while the door is not stopped, closed
      if( doorState!= DS_STOPPED_CLOSED) {
	simulate_door->setValueBool (false);
	logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged do not switch to simulation mode while doorState!= DS_STOPPED_CLOSED" << sendLog ;
	return ; 
      }
      logStream (MESSAGE_DEBUG) << "DoorVermes::valueChanged simulate door movements, stopping thread, entering simulation mode" << sendLog ;
      // strictly spoken: the motor has been already stopped, repeated here to make it sure
      // turning off the motor has to be done here since move_door opens,closes it during a sleep
      int ret ;
      struct timespec sl ;
      struct timespec rsl ;
      sl.tv_sec= 0. ;
      sl.tv_nsec= REPEAT_RATE_NANO_SEC; 
	
      while(( ret= motor_off()) != SSD650V_MS_STOPPED) { // 
	fprintf( stderr, "DoorVermes::valueChanged: can not turn motor off\n") ;
	ret= nanosleep( &sl, &rsl) ;
	if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
	  fprintf( stderr, "Error in nanosleep\n") ;
	}
      }
      if(connectSSD650vDevice(SSD650V_CMD_DISCONNECT) != SSD650V_MS_CONNECTION_OK) {
	logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged a general failure on SSD650V connection occured" << sendLog ;
      }

      if( connectOakDiginDevice(OAKDIGIN_CMD_DISCONNECT)) {
	logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged Oak thread did not stop, exiting" << sendLog ;
	exit(1) ;
      }
      // we are now in simulation mode
      oak_thread_state= THREAD_STATE_RUNNING ;
      doorState=DS_STOPPED_CLOSED ;
      updateDoorStatus () ;

    } else {
      logStream (MESSAGE_DEBUG) << "DoorVermes::valueChanged stopping simulation, starting threads, entering real mode" << sendLog ;

      if( connectOakDiginDevice(OAKDIGIN_CMD_CONNECT)) {
	logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged oak thread did not start, exiting" << sendLog ;
	exit(1) ;
      }
      doorState=DS_UNDEF ;
      updateDoorStatus () ;

      last_oak_digin_thread_heart_beat= oak_digin_thread_heart_beat ;
      struct timespec sl ;
      struct timespec rsl ;
      sl.tv_sec= 0. ;
      sl.tv_nsec= WAITING_FOR_HEART_BEAT_NANO_SEC; 
      nanosleep( &sl, &rsl) ;

      if( oak_digin_thread_heart_beat != last_oak_digin_thread_heart_beat) {
	//logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged heart beat present" << sendLog ;
	oak_thread_state= THREAD_STATE_RUNNING ;
      } else {

	logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged no heart beat from Oak thread, exiting" << sendLog ;
	exit(1) ;
      }
      logStream (MESSAGE_INFO) << "DoorVermes::valueChanged Connect to SSD650v" << sendLog;

      if(connectSSD650vDevice(SSD650V_CMD_CONNECT) != SSD650V_MS_CONNECTION_OK) {
	logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged a general failure on SSD650V connection occured" << sendLog ;
      }
    }
    return ;
  } else if (changed_value == manual) {
    if( manual->getValueBool()){
	block_door->setValueBool(true) ;
	ignoreEndswitch=IGNORE_END_SWITCH;
	logStream (MESSAGE_WARNING) << "DoorVermes::valueChanged to MANUAL mode, steer door using SSDsetpoint" << sendLog ;
    } else {
	block_door->setValueBool(false) ;
	ignoreEndswitch= NOT_IGNORE_END_SWITCH;
	logStream (MESSAGE_INFO) << "DoorVermes::valueChanged to automatic mode, ignoring SSDsetpoint" << sendLog ;
    }
    return ;
  } else if (changed_value == ssd650v_setpoint) {
    if( manual->getValueBool()) {
      logStream (MESSAGE_INFO) << "DoorVermes::valueChanged SETPOINT" << send ;
      int ret  ;
      if(( ret= move_manual(ssd650v_setpoint->getValueDouble())) != 0) {
	logStream (MESSAGE_WARNING) << "DoorVermes::valueChanged with move_manual something went wrong" << sendLog ;
      }
    } else {
	logStream (MESSAGE_INFO) << "DoorVermes::valueChanged ignoring changed value for ssd650v_setpoint, beeing in automatic mode, switch to MANUAL mode first" << sendLog ;
    } 
    
    return ;
  }
  Dome::valueChanged (changed_value);
}

int
DoorVermes::init ()
{
  int ret;
  ret = Dome::init ();
  if (ret)
    return ret;
  // start in automatic mode
  ignoreEndswitch= NOT_IGNORE_END_SWITCH;

  // make sure it forks before creating threads                                                                                    
  ret = doDaemonize ();
  if (ret) {
      logStream (MESSAGE_ERROR) << "Doorvermes::initValues could not daemonize"<< sendLog ;
      return ret;
  }

  if( simulate_door->getValueBool()){
    oak_thread_state= THREAD_STATE_RUNNING ;
  } else {
    // Toradex Oak digitial inputs, connect and start thread which can only stop motor
    if( connectOakDiginDevice(OAKDIGIN_CMD_CONNECT)) {
      logStream (MESSAGE_ERROR) << "DoorVermes::initValues Oak thread did not start, exiting" << sendLog ;
      exit(1) ;
    }
  }
  if( simulate_door->getValueBool()){
    doorState=DS_STOPPED_CLOSED ;
    updateDoorStatus () ;

  } else {
    // ssd650v frequency inverter, connect and start thread which only can start motor
    logStream (MESSAGE_DEBUG) << "DoorVermes::initValues to SSD650v" << sendLog;

    if(connectSSD650vDevice(SSD650V_CMD_CONNECT) != SSD650V_MS_CONNECTION_OK) {
      logStream (MESSAGE_ERROR) << "DoorVermes::initValues a general failure on SSD650V connection occured" << sendLog ;
    }
  }
  if(( doorState== DS_STOPPED_CLOSED) || (doorState== DS_STOPPED_OPENED)) {
  } else {
    logStream (MESSAGE_ERROR) << "DoorVermes::initValues Attention door is not closed nor open" << sendLog ;
  }

  ret = info ();
  if (ret)
    return ret;
  nextDeadCheck = 0;
  setIdleInfoInterval (20);
  return 0;  
}

int
DoorVermes::info ()
{

  if( simulate_door->getValueBool()) { 
    oak_thread_state= THREAD_STATE_RUNNING ;
  } else {
    last_oak_digin_thread_heart_beat= oak_digin_thread_heart_beat ;
    struct timespec sl ;
    struct timespec rsl ;
    sl.tv_sec= 0. ;
    sl.tv_nsec= WAITING_FOR_HEART_BEAT_NANO_SEC; 
    nanosleep( &sl, &rsl) ;

    if( oak_digin_thread_heart_beat != last_oak_digin_thread_heart_beat) {
      //logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged heart beat present" << sendLog ;
	oak_thread_state= THREAD_STATE_RUNNING ;
    } else {
      oak_thread_state= THREAD_STATE_UNDEFINED ;
      logStream (MESSAGE_ERROR) << "DoorVermes::valueChanged no heart beat from Oak thread" << sendLog ;
      // do not exit here 
    }
  }
  //updateDoorStatus() ;
  updateDoorStatusMessage() ;
  lastMotorStop-> setValueString ( lastMotorStop_str) ; 
  double readSetPoint= get_setpoint() ;
  double current_percentage = get_current_percentage() ;
  ssd650v_read_setpoint->setValueDouble( readSetPoint) ;
  ssd650v_current->setValueDouble(current_percentage) ;

  return Dome::info ();
}

/**
 * Open dome. Called either for open command, or when system
 * transitioned to on state.
 *
 * @return 0 on success, -1 on error.
 */
int
DoorVermes::startOpen ()
{
  updateDoorStatusMessage() ;

  if (!isGoodWeather ()){
    //logStream (MESSAGE_ERROR) << "DoorVermes::startOpen isGoodWeather==false" << sendLog;

    return -1;
  }
  if( block_door->getValueBool()) {
    logStream (MESSAGE_ERROR) << "DoorVermes::startOpen blocked door opening (see BLOCK_DOOR)" << sendLog ;
    return -1;
  }  
  stop_door->setValueBool(false) ;
  open_door->setValueBool(false) ;
  close_door->setValueBool(false) ;
  close_door_undefined->setValueBool(false) ;
  
  if( doorState== DS_RUNNING_OPEN) {
    logStream (MESSAGE_WARNING) << "DoorVermes::startOpen doorState== DS_RUNNING_OPEN, ignoring startOpen"<< sendLog ;
    return 0;

  } else if( doorState== DS_STOPPED_CLOSED) {
    logStream (MESSAGE_INFO) << "DoorVermes::startOpen opening door" << sendLog ;

    if( oak_thread_state== THREAD_STATE_RUNNING) {
      if( simulate_door->getValueBool()){
	doorState=DS_RUNNING_OPEN ;
	updateDoorStatus() ;
      } else { // the real thing

	int k = 0 ;
	do {
	  doorEvent= EVNT_DOOR_CMD_OPEN ;
	  logStream (MESSAGE_INFO) << "DoorVermes::startOpen doorEvent= EVNT_DOOR_CMD_OPEN, doorState="<<doorState<<" should be "<<DS_STOPPED_CLOSED << sendLog ;
	  int ret ;
	  int j ;
	  struct timespec sl ;
	  struct timespec rsl ;
	  sl.tv_sec= 0. ;
	  sl.tv_nsec= REPEAT_RATE_NANO_SEC; 

	  for( j= 0 ; j < 6 ; j++) {
	    ret= nanosleep( &sl, &rsl) ;
	    if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
	      fprintf( stderr, "Error in nanosleep\n") ;
	    }
	    if( motorState== SSD650V_MS_RUNNING) {
	      logStream (MESSAGE_INFO) << "DoorVermes::startOpen doorEvent= EVNT_DOOR_CMD_OPEN: motor running" << sendLog ;
	      break ;
	    }
	  }
	  if( j== 6) {
	    logStream (MESSAGE_ERROR) << "DoorVermes::startOpen doorEvent= EVNT_DOOR_CMD_OPEN: failed" << sendLog ;
	  }
	} while(( motorState != SSD650V_MS_RUNNING) && ( k < 3)) ;

	if( k== 3) {
	  logStream (MESSAGE_INFO) << "DoorVermes::startOpen doorEvent= EVNT_DOOR_CMD_OPEN: returning failiure (-1)" << sendLog ;
	  return -1 ;
	} else {  
	  open_door->setValueBool(true) ;
	  //wdoorState=DS_RUNNING_OPEN ;
	  //wupdateDoorStatus() ;
	  logStream (MESSAGE_INFO) << "DoorVermes::startOpen doorEvent= EVNT_DOOR_CMD_OPEN: returning succes" << sendLog ;
	  return 0 ;
	}
      }
    } else {
      block_door->setValueBool(true) ;
      logStream (MESSAGE_ERROR) << "DoorVermes::startOpen oak_digin_thread died" << sendLog ;
      return -1 ;
    }
    return 0;

  } else if ( doorState== DS_STOPPED_OPENED) {
    logStream (MESSAGE_ERROR) << "DoorVermes::startOpen can not open door doorState== DS_STOPPED_OPENED" << sendLog ;
    return -1 ;
  } else if ( doorState== DS_UNDEF){
    logStream (MESSAGE_ERROR) << "DoorVermes::startOpen can not open door doorState== DS_UNDEF" << sendLog ;
    return -1 ;
  } else {
    logStream (MESSAGE_ERROR) << "DoorVermes::startOpen can not open door doorStateis completely undefined" << sendLog ;
    return -1 ;
  }
  return -1;
}
/**
 * Check if dome is opened.
 *
 * @return -2 if dome is opened, -1 if there was an error, >=0 is timeout in miliseconds till
 * next isOpened call.
 */
long
DoorVermes::isOpened ()
{
  updateDoorStatusMessage() ;

  if( block_door->getValueBool()) {
    logStream (MESSAGE_ERROR) << "DoorVermes::isOpened blocked door opening (see BLOCK_DOOR)" << sendLog ;
    return -1;
  }  

  stop_door->setValueBool(false) ;
  open_door->setValueBool(false) ;
  close_door->setValueBool(false) ;
  close_door_undefined->setValueBool(false) ;

  if ( doorState!= DS_STOPPED_OPENED) {
    if( simulate_door->getValueBool()) {
	doorState=DS_STOPPED_OPENED ;
	updateDoorStatus() ;

	logStream (MESSAGE_DEBUG) << "DoorVermes::isOpened simulated doorState=DS_STOPPED_OPENED"<< sendLog ;
	return -2;
    }
    //logStream (MESSAGE_INFO) << "DoorVermes::isOpened is called"<< sendLog ;
    open_door->setValueBool(true) ;
    return USEC_SEC;
  }
  doorState=DS_STOPPED_OPENED; 
  //updateDoorStatus() ;
  logStream (MESSAGE_INFO) << "DoorVermes::isOpened returning -2: it is OPEN"<< sendLog ;
  return -2;
}
/**
 * Called when dome is fully opened. Can be used to turn off compressors used
 * to open dome etc..
 *
 * @return -1 on error, 0 on success.
 */
int
DoorVermes::endOpen ()
{
  //updateDoorStatusMessage() ;

  stop_door->setValueBool(false) ;
  open_door->setValueBool(false) ;
  close_door->setValueBool(false) ;
  close_door_undefined->setValueBool(false) ;
  
  if( doorState== DS_STOPPED_CLOSED) {
    logStream (MESSAGE_ERROR) << "DoorVermes::endOpen door is not open doorState== DS_STOPPED_CLOSED" << sendLog ;
    return -1 ;
  } else if ( doorState== DS_STOPPED_OPENED) {
    if( simulate_door->getValueBool()){
      logStream (MESSAGE_DEBUG) << "DoorVermes::endOpen simulated door is open"<< sendLog ;
    } else {
      logStream (MESSAGE_INFO) << "DoorVermes::endOpen door is open"<< sendLog ;
    }
    return 0;

  } else if ( doorState== DS_UNDEF){
    logStream (MESSAGE_ERROR) << "DoorVermes::endOpen door is not open doorState== DS_UNDEF" << sendLog ;
    return -1 ;
  } else {
    logStream (MESSAGE_ERROR) << "DoorVermes::endOpen door is not open doorStateis completely undefined" << sendLog ;
    return -1 ;
  }
  logStream (MESSAGE_ERROR) << "DoorVermes::endOpen returng -1 ERROR" << sendLog ;
  return -1;
}
/**
 * Called when dome needs to be closed. Should start dome
 * closing sequence.
 *
 * @return -1 on error, 0 on success.
 */
int
DoorVermes::startClose ()
{
  logStream (MESSAGE_INFO) << "DoorVermes::startClose, doorState: "<< doorState<< sendLog ;
  updateDoorStatusMessage() ;
  
  if( block_door->getValueBool()) {
    logStream (MESSAGE_ERROR) << "DoorVermes::startClosed blocked door closing (see BLOCK_DOOR)" << sendLog ;
    return -1;
  }  

  stop_door->setValueBool(false) ;
  open_door->setValueBool(false) ;
  close_door->setValueBool(false) ;
  close_door_undefined->setValueBool(false) ;

  if(( doorState== DS_RUNNING_CLOSE) || (doorState== DS_RUNNING_UNDEF) || (doorState== DS_RUNNING_CLOSE_UNDEFINED)) {
    //logStream (MESSAGE_DEBUG) << "DoorVermes::startClosed doorState== (DS_RUNNING_CLOSE|DS_RUNNING_UNDEF|DS_RUNNING_CLOSE_UNDEFINED), ignoring startClose"<< sendLog ;
    return 0;

  } else if( doorState== DS_STOPPED_CLOSED) {
    logStream (MESSAGE_WARNING) << "DoorVermes::startClose door is closed doorState== DS_STOPPED_CLOSED, ignoring startClose" << sendLog ;
    return 0 ;

  } else if ( doorState== DS_STOPPED_OPENED) {
    logStream (MESSAGE_INFO) << "DoorVermes::startClose closing door"<< sendLog ;
    if( oak_thread_state== THREAD_STATE_RUNNING) {
      if( simulate_door->getValueBool()){
	doorState=DS_RUNNING_CLOSE ;
	updateDoorStatus() ;

      } else { // the real thing
	int k = 0 ;
	do {
	  doorEvent= EVNT_DOOR_CMD_CLOSE ;
	  logStream (MESSAGE_INFO) << "DoorVermes::startClose doorEvent= EVNT_DOOR_CMD_CLOSE: closing" << sendLog ;

	  int ret ;
	  int j ;
	  struct timespec sl ;
	  struct timespec rsl ;
	  sl.tv_sec= 0. ;
	  sl.tv_nsec= REPEAT_RATE_NANO_SEC; 

	  for( j= 0 ; j < 6 ; j++) {
	    ret= nanosleep( &sl, &rsl) ;
	    if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
	      fprintf( stderr, "Error in nanosleep\n") ;
	    }
	    if( motorState== SSD650V_MS_RUNNING) {
	      logStream (MESSAGE_INFO) << "DoorVermes::startClose doorEvent= EVNT_DOOR_CMD_CLOSE: motor running" << sendLog ;
	      break ;
	    }
	  }
	  if( j== 6) {
	    logStream (MESSAGE_ERROR) << "DoorVermes::startClose doorEvent= EVNT_DOOR_CMD_CLOSE: failed" << sendLog ;
	  }
	  k++ ;
	} while(( motorState != SSD650V_MS_RUNNING) && ( k < 3)) ;

	if( k== 3) {
	  return -1 ;
	} else {  
	  close_door->setValueBool(true) ;
	  return 0 ;
	}
      }
    } else {
      block_door->setValueBool(true) ;
      logStream (MESSAGE_ERROR) << "DoorVermes::startClose oak_digin_thread died" << sendLog ;
      return -1 ;
    }
    // wildi ToDo eventually:
    // 2 minutes timeout..
    //setWeatherTimeout (120, "closing dome");
    return 0;

  } else {

    logStream (MESSAGE_INFO) << "DoorVermes::startClose closing door, doorState" << doorState<< sendLog ;
    if( oak_thread_state== THREAD_STATE_RUNNING) {
      if( simulate_door->getValueBool()){
      
      } else {

	if( doorState== DS_RUNNING_OPEN) {
          // sigHandler stops motor
	  pthread_kill( move_door_id, SIGUSR2);
	  logStream (MESSAGE_INFO) << "DoorVermes::startClose was doorState== DS_RUNNING_OPEN: stopped motor" << sendLog ;
	}
	doorEvent= EVNT_DOOR_CMD_CLOSE_IF_UNDEFINED_STATE ;
	logStream (MESSAGE_INFO) << "DoorVermes::startClose doorEvent= EVNT_DOOR_CMD_CLOSE_IF_UNDEFINED_STATE: closing" << sendLog ;
      }
    } else {
      logStream (MESSAGE_ERROR) << "DoorVermes::startClose oak_digin_thread died" << sendLog ;
      close_door_undefined->setValueBool(true) ;
      return -1 ;
    }
    return 0 ;
  }
  return -1;
}
/**
 * Called to check if dome is closed. It is called also outside
 * of the closing sequence, to check if dome is closed when bad
 * weather arrives. When implemented correctly, it should check
 * state of dome end switches, and return proper values.
 *
 * @return -2 if dome is closed, -1 if there was an error, >=0 is timeout in miliseconds till
 * next isClosed call (when dome is closing).
 */
long
DoorVermes::isClosed ()
{   
  updateDoorStatusMessage() ;

  //logStream (MESSAGE_DEBUG) << "DoorVermes::isClosed"<< sendLog ;

  if( block_door->getValueBool()) {
      //logStream (MESSAGE_DEBUG) << "DoorVermes::isClosed blocked door closing (see BLOCK_DOOR), returning -2 (is closed)" << sendLog ;
    return -2;
  }  

  //return -2 ;
  stop_door->setValueBool(false) ;
  open_door->setValueBool(false) ;
  close_door->setValueBool(false) ;
  close_door_undefined->setValueBool(false) ;
  if( doorState== DS_RUNNING_CLOSE) {

      //logStream (MESSAGE_DEBUG) << "DoorVermes::isClosed doorState== DS_RUNNING_CLOSE "<< sendLog ;
      return USEC_SEC;
  } else if ( doorState!= DS_STOPPED_CLOSED) {
    if( simulate_door->getValueBool()) {
	doorState=DS_STOPPED_CLOSED ;
	updateDoorStatus() ;

	logStream (MESSAGE_DEBUG) << "DoorVermes::isClosed simulated doorState=DS_STOPPED_CLOSED"<< sendLog ;
	return -2 ;
    }
    close_door->setValueBool(true) ;
    return USEC_SEC;
  } 
  return -2 ;
}

/**
 * Called after dome is closed. Can turn of various devices
 * used to close dome,..
 *
 * @return -1 on error, 0 on sucess.
 */
int
DoorVermes::endClose ()
{
  updateDoorStatusMessage() ;

  stop_door->setValueBool(false) ;
  open_door->setValueBool(false) ;
  close_door->setValueBool(false) ;
  close_door_undefined->setValueBool(false) ;
  
  if( doorState== DS_STOPPED_CLOSED) {
    if( simulate_door->getValueBool()){
      logStream (MESSAGE_DEBUG) << "DoorVermes::endClose simulated door is closed" << sendLog ;
    } else {
      logStream (MESSAGE_INFO) << "DoorVermes::endClose door is closed" << sendLog ;
    }
    return 0 ;
    
  } else if ( doorState== DS_STOPPED_OPENED) {
    logStream (MESSAGE_ERROR) << "DoorVermes::endClose door is open doorState== DS_STOPPED_OPENED"<< sendLog ;
    return -1;
  } else if ( doorState== DS_UNDEF){
    logStream (MESSAGE_ERROR) << "DoorVermes::endClose door is not closed doorState== DS_UNDEF" << sendLog ;
    return -1 ;
  } else {
    logStream (MESSAGE_ERROR) << "DoorVermes::endClose door is not closed doorStateis completely undefined" << sendLog ;
    return -1 ;
  }
  return -1;
}
int
DoorVermes::processOption (int _opt)
{
  switch (_opt) {
  default:
    return Dome::processOption (_opt);
  }
  return 0;
}

// wildi ToDo: check the threads
int
DoorVermes::idle ()
{
  //logStream (MESSAGE_ERROR) << "DoorVermes::idle" << sendLog ;
  if (isGoodWeather ()) {
    if ((getState () & DOME_DOME_MASK) == DOME_OPENED || (getState () & DOME_DOME_MASK) == DOME_OPENING) {
      info() ;
    }
  } else{
    //logStream (MESSAGE_ERROR) << "DoorVermes::idle isGoodWeather==false" << sendLog;
  }
  return Dome::idle ();
}

DoorVermes::DoorVermes (int argc, char **argv): Dome (argc, argv)
{
  createValue( lastMotorStop,        "LASTSTOP", "date and time, when motor was last stopped by Oak sensors", false) ;
  createValue (doorStateMessage,     "DOORSTATE", "door state as clear text", false);
  createValue (block_door,           "BLOCK_DOOR", "true inhibits rts2-centrald initiated door movements", false, RTS2_VALUE_WRITABLE);
  block_door->setValueBool (false); 
  createValue (stop_door,            "STOP_DOOR",  "true stops door",  false, RTS2_VALUE_WRITABLE);
  stop_door->setValueBool  (false);
  createValue (open_door,            "OPEN_DOOR",  "true opens door",  false, RTS2_VALUE_WRITABLE);
  open_door->setValueBool  (false);
  createValue (close_door,           "CLOSE_DOOR", "true closes door", false, RTS2_VALUE_WRITABLE);
  close_door->setValueBool (false);
  createValue (close_door_undefined, "CLOSE_UDFD", "true closes door", false, RTS2_VALUE_WRITABLE);
  close_door_undefined->setValueBool (false);
  createValue (simulate_door, "SIMULATION", "true simulation door movements", false, RTS2_VALUE_WRITABLE);
  simulate_door->setValueBool (false);

  createValue (manual, "Manual", "true manual door operation ignoring endswitch", false, RTS2_VALUE_WRITABLE);
  manual->setValueBool (false);

  createValue (ssd650v_setpoint,     "SSDsetpoint",     "ssd650v setpoint [-100.,100], !=0 motor on", false, RTS2_VALUE_WRITABLE);
  ssd650v_setpoint->setValueDouble( 0.) ;

  createValue (ssd650v_current,      "SSDcurrent",      "ssd650v current as percentage of maximum", false);
  createValue (ssd650v_read_setpoint,"SSDread_setpoint","ssd650v read setpoint [-100.,100]",  false);
}

DoorVermes::~DoorVermes ()
{
  // wildi ToDo disconnect/switch off devices
}

int
main (int argc, char **argv)
{
    DoorVermes device = DoorVermes (argc, argv);
    return device.run ();
}
