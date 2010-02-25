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
#include "../utils/rts2config.h" 


// Obs. Vermes specific 
#include "vermes.h" 
#include "dome-target-az.h"
#include "move-to-target-az_vermes.h"
#include "barcodereader_vermes.h"
#include "ssd650v_comm_vermes.h"
// wildi ev. ToDo: pthread_mutex_t mutex1
extern int is_synced ; // ==SYNCED if target_az reched
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
    Rts2Config *config ;
    Rts2ValueDouble  *traget_azimut_cupola ;
    Rts2ValueInteger *barcode_reader_state ;
    Rts2ValueDouble  *azimut_difference ;
    Rts2ValueString  *ssd650v_state ;
    Rts2ValueDouble  *ssd650v_read_setpoint ;
    Rts2ValueDouble  *ssd650v_setpoint ;
    Rts2ValueBool    *cupola_tracking ;
    Rts2ValueDouble  *ssd650v_min_setpoint ;
    Rts2ValueDouble  *ssd650v_max_setpoint ;
    Rts2ValueDouble  *ssd650v_current ;
    void parkCupola ();
  protected:
    virtual int moveStart () ;
    virtual int moveEnd () ;
    virtual long isMoving () ;
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
    virtual double getSplitWidth (double alt) ;
    virtual int info () ;
    virtual int idle ();
    virtual void valueChanged (Rts2Value * changed_value) ;
    // park copula
    virtual int standby ();
    virtual int off ();
  };
}

int Vermes::moveEnd ()
{
  return Cupola::moveEnd ();
}
long Vermes::isMoving ()
{
  if ( is_synced== SYNCED) {
    //  logStream (MESSAGE_DEBUG) << "Vermes::isMoving SYNCED"<< sendLog ;
    return -2;
  } else {
    //  logStream (MESSAGE_DEBUG) << "Vermes::isMoving NOT_SYNCED"<< sendLog ;
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
  return Cupola::moveStart ();
}

double Vermes::getSplitWidth (double alt)
{
  return -1;
}

void Vermes::parkCupola ()
{
  logStream (MESSAGE_INFO) << "Vermes::parkCupola doing nothing" << sendLog ;
}

int Vermes::standby ()
{
  logStream (MESSAGE_INFO) << "Vermes::standby doing nothing" << sendLog ;
  parkCupola ();
  return Cupola::standby ();
}

int Vermes::off ()
{
//   if(connectSSD650vDevice(SSD650V_CMD_DISCONNECT))
//     {
//       logStream (MESSAGE_ERROR) << "Vermes::off a general failure occured" << sendLog ;
//     }

  logStream (MESSAGE_DEBUG) << "Vermes::off NOT disconnecting from frequency inverter" << sendLog ;
  parkCupola ();
  return Cupola::off ();
}

void Vermes::valueChanged (Rts2Value * changed_value)
{
  int ret ;

  if (changed_value == ssd650v_setpoint) {
    movementState= MANUAL ;
    if(( ret=motor_off()) != SSD650V_MS_STOPPED ) {
      logStream (MESSAGE_ERROR) << "Vermes::valueChanged could not turn motor off " << sendLog ;
      ssd650v_state->setValueString("motor undefined") ;
    } 
    float setpoint= (float) ssd650v_setpoint->getValueDouble() ;
    float getpoint ;
    if(isnan( getpoint= set_setpoint( setpoint)))  {
      logStream (MESSAGE_ERROR) << "Vermes::valueChanged could not set setpoint "<< getpoint << sendLog ;
    } else {
      if( setpoint != 0.) {
	if(( ret=motor_on()) != SSD650V_MS_RUNNING ) {
	  ssd650v_state->setValueString("motor undefined") ;
	  logStream (MESSAGE_ERROR) << "Vermes::valueChanged could not turn motor on :" << setpoint << sendLog ;
	} 
      }
    }
    return ; // ask Petr what to do in general if something fails within ::valueChanged
  } else   if (changed_value == cupola_tracking) {
    if( cupola_tracking->getValueBool()) {
      movementState= TRACKING_ENABLED ; 
      logStream (MESSAGE_DEBUG) << "Vermes::valueChanged cupola starts tracking the telescope"<< sendLog ;
    } else {
      // motor is turned off in thread
      movementState= TRACKING_DISABLED ;
      logStream (MESSAGE_DEBUG) << "Vermes::valueChanged cupola tracking stoped"<< sendLog ;
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
  traget_azimut_cupola->setValueDouble( target_az) ;
  azimut_difference->setValueDouble(( barcodereader_az- getTargetAz())) ;
  ssd650v_current->setValueDouble(current_percentage) ;
  if( ssd650v_current->getValueDouble() > CURRENT_MAX_PERCENT) {

    logStream (MESSAGE_ERROR) << "Vermes::info current exceeding limit: "<<  ssd650v_current->getValueDouble() << sendLog ;

  }
  if( movementState == TRACKING_ENABLED) {
    cupola_tracking->setValueBool(true) ;
  } else  {
    cupola_tracking->setValueBool(false) ;
  }

  if( motorState== SSD650V_MS_RUNNING) {
    ssd650v_state->setValueString("motor running") ;
  } else if( motorState== SSD650V_MS_STOPPED) {
    ssd650v_state->setValueString("motor stopped") ;
  } else  {
    ssd650v_state->setValueString("motor undefined state") ;
  }
  ssd650v_read_setpoint->setValueDouble( readSetPoint) ;

  return Cupola::info ();
}
int Vermes::initValues ()
{
  int ret ;
  pthread_t  move_to_target_azimuth_id;

  config = Rts2Config::instance ();

  ret = config->loadFile ();
  if (ret)
    return -1;

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
    fprintf( stderr, "Vermes::initValues something went wrong with SSD650V (OFF)\n") ;
    ssd650v_state->setValueString("motor undefined") ;
  } 

  // set initial tel_eq to HA=0
  double JD= ln_get_julian_from_sys ();
  tel_equ.ra= ln_get_mean_sidereal_time( JD) * 15. + obs_location.lng;
  tel_equ.dec= 0. ;
  // thread to compare (target - current) azimuth and rotate the dome
  ret = pthread_create( &move_to_target_azimuth_id, NULL, move_to_target_azimuth, NULL) ;
  return Cupola::initValues ();
}
Vermes::Vermes (int in_argc, char **in_argv):Cupola (in_argc, in_argv) 
{
  // since this driver is Obs. Vermes specific no options are really required
  createValue (traget_azimut_cupola, "TargetAZ",        "target AZ calculated within driver", false, RTS2_DT_DEGREES  );
  createValue (azimut_difference,    "AZdiff",          "(cupola - target) AZ reading",       false, RTS2_DT_DEGREES  );
  createValue (cupola_tracking,      "Tracking",        "telescope tracking (true: enabled, false:motor off)", false, RTS2_VALUE_WRITABLE);
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

