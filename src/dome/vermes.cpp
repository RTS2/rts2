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

#include "cupola.h"
#include "configuration.h" 


// Obs. Vermes specific 
#include <vermes.h> 
#include <slitazimuth.h>
#include <vermes/move_to_target_az.h>
#include <vermes/barcodereader.h>
#include <vermes/ssd650v_comm.h>
// wildi ev. ToDo: pthread_mutex_t mutex1
extern int is_synced ; // ==SYNCED if target_az reached
extern int motorState ;
extern int barcodereader_state ;
extern double barcodereader_az ;
extern double barcodereader_dome_azimut_offset ; 
extern double target_az ;
extern struct ln_lnlat_posn obs_location ;
extern struct ln_equ_posn   tel_equ ;
extern double curMaxSetPoint ;
extern double curMinSetPoint ;
extern int movementState ; 
extern double current_percentage ;
extern double readSetPoint ;



using namespace rts2dome;

namespace rts2dome
{
/**
 * Obs. Vermes cupola driver.
 *
 * @author Markus Wildi <markus.wildi@one-arcsec.org>
 */
  class Vermes:public Cupola
  {
  private:
    rts2core::Configuration *config ;
    rts2core::ValueDouble  *target_azimut_cupola ;
    rts2core::ValueInteger *barcode_reader_state ;
    rts2core::ValueDouble  *azimut_difference ;
    rts2core::ValueString  *ssd650v_state ;
    rts2core::ValueDouble  *ssd650v_read_setpoint ;
    rts2core::ValueDouble  *ssd650v_setpoint ;
    rts2core::ValueBool    *synchronizeTelescope ;
    rts2core::ValueDouble  *ssd650v_min_setpoint ;
    rts2core::ValueDouble  *ssd650v_max_setpoint ;
    rts2core::ValueDouble  *ssd650v_current ;
    void parkCupola ();
  protected:
    virtual int moveStart () ;
    virtual int moveEnd () ;
    virtual long isMoving () ;
    virtual int moveStop () ;
    // there is no dome door to open 
    virtual int startOpen (){return 0;}
    virtual long isOpened (){return -2;}
    virtual int endOpen (){return 0;}
    virtual int startClose (){return 0;}
    virtual long isClosed (){return -2;}
    virtual int endClose (){return 0;}

  public:
    Vermes (int argc, char **argv) ;
    virtual int initValues () ;
    virtual double getSlitWidth (double alt) ;
    virtual int info () ;
    virtual int idle ();
    virtual void valueChanged (rts2core::Value * changed_value) ;
    // park copula
    virtual int standby ();
    virtual int off ();
  };
}

int Vermes::moveEnd ()
{
  logStream (MESSAGE_DEBUG) << "Vermes::moveEnd "<< sendLog ;
  struct ln_hrz_posn hrz;
  getTargetAltAz (&hrz);
  setCurrentAz (hrz.az);
  return Cupola::moveEnd ();
}
int Vermes::moveStop ()
{
  logStream (MESSAGE_INFO) << "Vermes::moveStop stopping cupola: "<< sendLog ;
  movementState= SYNCHRONIZATION_DISABLED ; 

  struct timespec rep_slv ;
  struct timespec rep_rsl ;
  rep_slv.tv_sec= 0 ;
  rep_slv.tv_nsec= REPEAT_RATE_NANO_SEC ;
  int ret ;
  int count= 0;

  while(( ret= motor_off()) != SSD650V_MS_STOPPED) {
    logStream (MESSAGE_WARNING) << "move_to_target_azimuth: motor_off != SSD650V_MS_STOPPED" << sendLog ;
    errno= 0;
    ret= nanosleep( &rep_slv, &rep_rsl) ;
    if((errno== EFAULT) || ( errno== EINTR)|| ( errno== EINVAL ))  {
      logStream (MESSAGE_ERROR) << "move_to_target_az: signal, or error in nanosleep: " <<ret<< sendLog ;
    }
    count++;
    if (count > 1){
      logStream (MESSAGE_ERROR) << "move_to_target_az: breaking after some trials " << sendLog ;
	break;
    }
  }
  return Cupola::moveStop ();
}
long Vermes::isMoving ()
{
  if ( is_synced== SYNCED) {
    logStream (MESSAGE_INFO) << "Vermes::isMoving cupola is SYNCED, position reached"<< sendLog ;
    return -2;
  } else if ( is_synced== SYNCED_MOTOR_ON) {
    logStream (MESSAGE_WARNING) << "Vermes::isMoving cupola is SYNCED, position reached, motor is still ON"<< sendLog ;
    return -2;
  } else {
    //logStream (MESSAGE_INFO) << "Vermes::isMoving its not SYNCED, position not yet reached"<< sendLog ;
  }
  return USEC_SEC;
}
int Vermes::moveStart ()
{
  int target_coordinate_changed= 0 ;
  static double lastRa=-9999., lastDec=-9999. ;

  tel_equ.ra= getTargetRa() ;
  tel_equ.dec= getTargetDec() ;

  if( lastRa != tel_equ.ra) {
    lastRa = tel_equ.ra ;
    target_coordinate_changed++ ;
  }
  if( lastDec != tel_equ.dec) {
    lastDec = tel_equ.dec ;
    target_coordinate_changed++ ;
  }
  if(target_coordinate_changed) {
    logStream (MESSAGE_DEBUG) << "Vermes::moveStart new RA " << tel_equ.ra  << " Dec " << tel_equ.dec << sendLog ;
  }

  movementState= SYNCHRONIZATION_ENABLED ; 
  logStream (MESSAGE_DEBUG) << "Vermes::moveStart synchronization of the telescope enabled"<< sendLog ;
  return Cupola::moveStart ();
}

double Vermes::getSlitWidth (double alt)
{
  return -1;
}

void Vermes::parkCupola ()
{
  movementState= SYNCHRONIZATION_DISABLED ; 
  logStream (MESSAGE_DEBUG) << "Vermes::parkCupola synchronization disabled"<< sendLog ;
}

int Vermes::standby ()
{
  parkCupola ();
  return Cupola::standby ();
}

int Vermes::off ()
{
  logStream (MESSAGE_INFO) << "Vermes::off NOT disconnecting from frequency inverter" << sendLog ;
  parkCupola ();
  return Cupola::off ();
}

void Vermes::valueChanged (rts2core::Value * changed_value)
{
  int ret ;
  static int lastMovementState ;
  if (changed_value == ssd650v_setpoint) {
 
    if( ssd650v_setpoint->getValueDouble() != 0.) {
      lastMovementState= movementState ;
      movementState= SYNCHRONIZATION_DISABLED ;
      if(( ret=motor_off()) != SSD650V_MS_STOPPED ) {
	logStream (MESSAGE_WARNING) << "Vermes::valueChanged motor_off != SSD650V_MS_STOPPED" << sendLog ;
	ssd650v_state->setValueString("motor undefined") ;
      } else {
	logStream (MESSAGE_INFO) << "Vermes::valueChanged switched synchronization off" << sendLog ;
      }
    } else {
      movementState= lastMovementState ;
      logStream (MESSAGE_DEBUG) << "Vermes::valueChanged restored synchronization mode" << sendLog ;
    } 
    
    float setpoint= (float) ssd650v_setpoint->getValueDouble() ;
    float getpoint ;
    if(std::isnan( getpoint= set_setpoint( setpoint)))  {
      logStream (MESSAGE_ERROR) << "Vermes::valueChanged could not set setpoint "<< getpoint << sendLog ;
    } else {
      if( setpoint == 0.) {
	if(( ret=motor_off()) != SSD650V_MS_STOPPED ) {
	  logStream (MESSAGE_WARNING) << "Vermes::valueChanged motor_off != SSD650V_MS_STOPPED" << sendLog ;
	  ssd650v_state->setValueString("motor undefined") ;
	} 
      } else {
	if(( ret=motor_on()) != SSD650V_MS_RUNNING ) {
	  ssd650v_state->setValueString("motor undefined") ;
	  logStream (MESSAGE_ERROR) << "Vermes::valueChanged could not turn motor on :" << setpoint << sendLog ;
	} 
      } 
    }
    return ; // ask Petr what to do in general if something fails within ::valueChanged
  } else   if (changed_value == synchronizeTelescope) {
    if( synchronizeTelescope->getValueBool()) {
      movementState= SYNCHRONIZATION_ENABLED ; 
      logStream (MESSAGE_DEBUG) << "Vermes::valueChanged cupola starts, synchronization of the telescope enabled"<< sendLog ;
    } else {
      // motor is turned off in thread
      movementState= SYNCHRONIZATION_DISABLED ;
      logStream (MESSAGE_DEBUG) << "Vermes::valueChanged cupola synchronization disabled"<< sendLog ;
    }
    return ;
  } else if (changed_value == ssd650v_min_setpoint) {
    curMinSetPoint= ssd650v_min_setpoint->getValueDouble() ;

  } else if (changed_value == ssd650v_max_setpoint) {

    curMaxSetPoint= ssd650v_max_setpoint->getValueDouble() ;
  }
  Cupola::valueChanged (changed_value);
}
int Vermes::idle ()
{
  return Cupola::idle ();
}
int Vermes::info ()
{
  barcode_reader_state->setValueInteger( barcodereader_state) ; 
  setCurrentAz (barcodereader_az);
  setTargetAz(target_az) ;
  target_azimut_cupola->setValueDouble( target_az) ;
  azimut_difference->setValueDouble(( barcodereader_az- getTargetAz())) ;
  ssd650v_current->setValueDouble(current_percentage) ;
  // Yes, we are open, always
  maskState (DOME_DOME_MASK, DOME_OPENED, "Dome is opened");

  if( ssd650v_current->getValueDouble() > CURRENT_MAX_PERCENT) {

    logStream (MESSAGE_WARNING) << "Vermes::info current exceeding limit: "<<  ssd650v_current->getValueDouble() << sendLog ;

  }
  if( movementState == SYNCHRONIZATION_ENABLED) {
    synchronizeTelescope->setValueBool(true) ;
  } else  {
    synchronizeTelescope->setValueBool(false) ;
  }

  if( motorState== SSD650V_MS_RUNNING) {
    ssd650v_state->setValueString("motor running") ;
  } else if( motorState== SSD650V_MS_STOPPED) {
    ssd650v_state->setValueString("motor stopped") ;
  } else  {
    ssd650v_state->setValueString("motor undefined state") ;
  }
  ssd650v_read_setpoint->setValueDouble( readSetPoint) ;
  if ( is_synced== SYNCED_MOTOR_ON) {
    logStream (MESSAGE_WARNING) << "Vermes::info is_synced== SYNCED_MOTOR_ON, keep an eye on that"<< sendLog ;
  }
  return Cupola::info ();
}
int Vermes::initValues ()
{
  int ret ;
  pthread_t  move_to_target_azimuth_id;

  config = rts2core::Configuration::instance ();

  ret = config->loadFile ();
  if (ret) {
    logStream (MESSAGE_ERROR) << "Vermes::initValues could not read configuration"<< sendLog ;
    return -1;
  }
  // make sure it forks before creating threads
  ret = doDaemonize ();
  if (ret) {
    logStream (MESSAGE_ERROR) << "Vermes::initValues could not daemonize"<< sendLog ;
    return ret;
  }

  struct ln_lnlat_posn   *obs_loc_tmp= Cupola::getObserver() ;
  obs_location.lat= obs_loc_tmp->lat;
  obs_location.lng= obs_loc_tmp->lng;

  // barcode azimuth reader
  if(!( ret= start_bcr_comm())) {
    register_pos_change_callback(position_update_callback);
  } else {
    logStream (MESSAGE_ERROR) << "Vermes::initValues could connect to barcode devices, exiting "<< sendLog ;
    exit(1) ;
  }
  // ssd650v frequency inverter
  if((ret= connectSSD650vDevice(SSD650V_CMD_CONNECT)) != SSD650V_MS_CONNECTION_OK) {
    logStream (MESSAGE_ERROR) << "Vermes::initValues a general failure on SSD650V connection occured" << sendLog ;
  }

  if(( ret=motor_off()) != SSD650V_MS_STOPPED ) {
    logStream (MESSAGE_WARNING) << "Vermes::initValues motor_off != SSD650V_MS_STOPPED" << sendLog ;
    ssd650v_state->setValueString("motor undefined") ;
  } 

  // set initial tel_eq to HA=0
  double JD= ln_get_julian_from_sys ();
  tel_equ.ra= ln_get_mean_sidereal_time( JD) * 15. + obs_location.lng;
  tel_equ.dec= 0. ;
  // thread to compare (target - current) azimuth and rotate the dome
  ret = pthread_create( &move_to_target_azimuth_id, NULL, move_to_target_azimuth, NULL) ;

  if( ret) {
    logStream (MESSAGE_ERROR) << "Vermes::initValues could not create thread, exiting"<< sendLog ;
    exit(1) ;
  }

  return Cupola::initValues ();
}
Vermes::Vermes (int in_argc, char **in_argv):Cupola (in_argc, in_argv) 
{
  // since this driver is Obs. Vermes specific no options are really required
  createValue (target_azimut_cupola, "TargetAZ",        "target AZ calculated within driver", false, RTS2_DT_DEGREES  );
  createValue (azimut_difference,    "AZdiff",          "(cupola - target) AZ reading",       false, RTS2_DT_DEGREES  );
  createValue (synchronizeTelescope, "Synchronize",     "synchronize with telescope (true: enabled, false:manual mode)", false, RTS2_VALUE_WRITABLE);
  createValue (barcode_reader_state, "BCRstate",        "barcodereader status (0: CUP_AZ valid, 1:invalid)", false);
  createValue (ssd650v_state,        "SSDstate",        "ssd650v inverter status",            false);
  createValue (ssd650v_current,      "SSDcurrent",      "ssd650v current as percentage of maximum", false);
  createValue (ssd650v_read_setpoint,"SSDread_setpoint","ssd650v read setpoint [-100.,100]",  false);
  createValue (ssd650v_setpoint,     "SSDsetpoint",     "ssd650v setpoint [-100.,100], !=0 motor on", false, RTS2_VALUE_WRITABLE);
  createValue (ssd650v_min_setpoint, "SSDmin_setpoint", "ssd650v minimum setpoint",           false, RTS2_VALUE_WRITABLE);
  createValue (ssd650v_max_setpoint, "SSDmax_setpoint", "ssd650v maximum setpoint",           false, RTS2_VALUE_WRITABLE);

  ssd650v_setpoint->setValueDouble( 0.) ;
  ssd650v_min_setpoint->setValueDouble( 40.) ;
  ssd650v_max_setpoint->setValueDouble( 80.) ;
  curMinSetPoint= ssd650v_min_setpoint->getValueDouble();
  curMaxSetPoint= ssd650v_max_setpoint->getValueDouble();

  barcode_reader_state->setValueInteger( -1) ; 
}
int main (int argc, char **argv)
{
	Vermes device (argc, argv);
	return device.run ();
}
