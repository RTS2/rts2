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

int is_synced            = NOT_SYNCED ;   // ==SYNCED if target_az reached
int cupola_tracking_state= TRACKING_DISABLED ; 
int motor_on_off_state   = MOTOR_RUNNING ;
int barcodereader_state ;
double barcodereader_az ;
double barcodereader_dome_azimut_offset= -253.6 ; // wildi ToDo: make an option
double target_az ;

struct ln_lnlat_posn obs_location ;
struct ln_equ_posn   tel_equ ;



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
    Rts2ValueInteger *barcode_reader_state ;
    Rts2ValueDouble  *azimut_difference ;
    Rts2ValueString  *ssd650v_state ;
    Rts2ValueBool    *ssd650v_on_off ;
    Rts2ValueDouble  *ssd650v_setpoint ;
    Rts2ValueBool    *cupola_tracking ;

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
  logStream (MESSAGE_DEBUG) << "Vermes::moveEnd did nothing "<< sendLog ;
  return Cupola::moveEnd ();
}
long Vermes::isMoving ()
{
  if ( is_synced== SYNCED) {
    logStream (MESSAGE_DEBUG) << "Vermes::isMoving SYNCED"<< sendLog ;
    return -2;
  } else {
    logStream (MESSAGE_DEBUG) << "Vermes::isMoving NOT_SYNCED"<< sendLog ;
  }
  return USEC_SEC;
}
int Vermes::moveStart ()
{
  int target_coordinate_changed= 0 ;
  static double lastRa=-9999., lastDec=-9999. ;

  tel_equ.ra= getTargetRa() ;
  tel_equ.dec= getTargetDec() ;
  // thread compare_az take care of the calculations

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
  //  logStream (MESSAGE_DEBUG) << "Vermes::getSplitWidth returning -1" << sendLog ;
  return -1;
}

void Vermes::parkCupola ()
{
  logStream (MESSAGE_DEBUG) << "Vermes::parkCupola doing nothing" << sendLog ;
}

int Vermes::standby ()
{
  logStream (MESSAGE_DEBUG) << "Vermes::standby doing nothing" << sendLog ;
  parkCupola ();
  return Cupola::standby ();
}

int Vermes::off ()
{
//   if(connectDevice(SSD650V_DISCONNECT))
//     {
//       logStream (MESSAGE_ERROR) << "Vermes::off a general failure occured" << sendLog ;
//     }

  logStream (MESSAGE_DEBUG) << "Vermes::off NOT disconnecting from frequency inverter" << sendLog ;
  parkCupola ();
  return Cupola::off ();
}

void Vermes::valueChanged (Rts2Value * changed_value)
{
  int res ;
  if (changed_value == ssd650v_on_off) {
    if( ssd650v_on_off->getValueBool()) {
      logStream (MESSAGE_DEBUG) << "Vermes::valueChanged starting azimuth motor, setpoint: "<< ssd650v_setpoint->getValueDouble() << sendLog ;
      if(( res=motor_on()) != SSD650V_RUNNING ) {
	logStream (MESSAGE_ERROR) << "Vermes::valueChanged something went wrong with  azimuth motor, error: "<< ssd650v_setpoint->getValueDouble() << sendLog ;
	motor_on_off_state= MOTOR_UNDEFINED ;
      } else {
	ssd650v_state->setValueString("motor running") ;
	motor_on_off_state= MOTOR_RUNNING ;
      }
    } else {
      logStream (MESSAGE_DEBUG) << "Vermes::valueChanged stopping azimuth motor, setpoint: "<< ssd650v_setpoint->getValueDouble() << sendLog ;
      if(( res=motor_off()) !=  SSD650V_STOPPED) {
	logStream (MESSAGE_ERROR) << "Vermes::valueChanged something went wrong with  azimuth motor, error: "<< ssd650v_setpoint->getValueDouble() << sendLog ;
	motor_on_off_state= MOTOR_UNDEFINED ;
      } else {
	ssd650v_state->setValueString("motor stopped") ;
	motor_on_off_state= MOTOR_NOT_RUNNING ;
      }
    }
    return ;
  } else if (changed_value == ssd650v_setpoint) {

    float setpoint= (float) ssd650v_setpoint->getValueDouble() ;
    if( set_setpoint( setpoint))  {
      logStream (MESSAGE_ERROR) << "Vermes::valueChanged could not set setpoint "<< setpoint << sendLog ;
    }
    return ; // ask Petr what to do in general if something fails within ::valueChanged
  } else   if (changed_value == cupola_tracking) {
    if( cupola_tracking->getValueBool()) {
      cupola_tracking_state= TRACKING_ENABLED ;
      logStream (MESSAGE_DEBUG) << "Vermes::valueChanged cupola starts tracking the telescope"<< sendLog ;
    } else {
      cupola_tracking_state= TRACKING_DISABLED ;
      logStream (MESSAGE_DEBUG) << "Vermes::valueChanged cupola tracking stoped"<< sendLog ;
    }
    return ;
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
  azimut_difference->setValueDouble(( barcodereader_az- getTargetAz())) ;

  if( cupola_tracking_state == TRACKING_ENABLED) {
    cupola_tracking->setValueBool(true) ;
  } else if( motor_on_off_state== MOTOR_NOT_RUNNING) {
    cupola_tracking->setValueBool(false) ;
  }

  if( motor_on_off_state== MOTOR_RUNNING) {
    ssd650v_on_off->setValueBool(true) ;
    ssd650v_state->setValueString("motor running") ;
  } else if( motor_on_off_state== MOTOR_NOT_RUNNING) {
    ssd650v_on_off->setValueBool(false) ;
    ssd650v_state->setValueString("motor stopped") ;
  } else  {
    ssd650v_state->setValueString("motor undefined state") ;
  }
  ssd650v_setpoint->setValueDouble( (double) get_setpoint()) ;

  return Cupola::info ();
}
int Vermes::initValues ()
{
  int ret ;
  pthread_t  thread_0;

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
  if(connectDevice(SSD650V_CONNECT)) {
    logStream (MESSAGE_ERROR) << "Vermes::initValues a general failure on SSD650V connection occured" << sendLog ;
  }
  if(( ret=motor_off()) != SSD650V_STOPPED ) {
    fprintf( stderr, "Vermes::initValues something went wrong with SSD650V (OFF)\n") ;
    motor_on_off_state= MOTOR_UNDEFINED ;
  } else {
    motor_on_off_state= MOTOR_NOT_RUNNING ;
  }

  // set initial tel_eq to HA=0
  double JD= ln_get_julian_from_sys ();
  tel_equ.ra= ln_get_mean_sidereal_time( JD) * 15. + obs_location.lng;
  tel_equ.dec= 0. ;
  // thread to compare (target - current) azimuth and rotate the dome
  int *value ;
  ret = pthread_create( &thread_0, NULL, move_to_target_azimuth, value) ;
  return Cupola::initValues ();
}
Vermes::Vermes (int in_argc, char **in_argv):Cupola (in_argc, in_argv) 
{
  // since this driver is Obs. Vermes specific no options are really required
  createValue (azimut_difference,   "AZdiff",          "target - actual azimuth reading",    false, RTS2_DT_DEGREES  );
  createValue (barcode_reader_state,"BCRstate",        "state of the barcodereader value CUP_AZ (0=valid, 1=invalid)", false);
  createValue (ssd650v_state,       "SSDstate",        "status of the ssd650v inverter ",    false);
  createValue (ssd650v_on_off,      "SSDmotor_switch", "(true=on, false=off running)",       false, RTS2_VALUE_WRITABLE);
  createValue (ssd650v_setpoint,    "SSDsetpoint",     "ssd650v setpoint",                   false, RTS2_VALUE_WRITABLE);
  createValue (cupola_tracking,     "Tracking",        "true=tracking, false=not tracking ", false, RTS2_VALUE_WRITABLE);

  barcode_reader_state->setValueInteger( -1) ; 
}
int main (int argc, char **argv)
{
	Vermes device (argc, argv);
	return device.run ();
}

