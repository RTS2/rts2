/* Thread to move the cupola Obs. Vermes to the target AZ */
/* Copyright (C) 2010, Markus Wildi */

/* The transformations are based on the paper Matrix Method for  */
/* Coodinates Transformation written by Toshimi Taki  */
/* (http://www.asahi-net.or.jp/~zs3t-tk).  */

/* Documentation: */
/* https://azug.minpet.unibas.ch/wikiobsvermes/index.php/Robotic_ObsVermes#Telescope */

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
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#endif

#include <libnova/libnova.h>
#include <vermes.h>
#include <slitazimuth.h>
#include <vermes/move_to_target_az.h>
#include <vermes/ssd650v_comm.h>

int is_synced            = NOT_SYNCED ;   // ==SYNCED if target_az reached
int barcodereader_state ;
double barcodereader_az ;
double barcodereader_dome_azimut_offset= -253.6 ; // wildi ToDo: make an option
double target_az ;
double manual_target_az=UNDEFINED ;
double curMaxSetPoint= 80. ;
double curMinSetPoint= 40. ;
double readSetPoint ;
int movementState= SYNCHRONIZATION_DISABLED ; 
double current_percentage ;

extern int motorState ;

struct ln_lnlat_posn obs_location ;
struct ln_equ_posn   tel_equ ;

void getSexComponents(double value, int *d, int *m, int *s) ;
void move_to_az();

const struct tk_geometry tk_obsvermes = {
  -0.0684, // xd [m]
  -0.1934, // zd [m]
   0.338,  // rdec [m]
   1.265   // rdome [m]
} ;
const struct geometry obsvermes = {
  0.0,     //  x_m [m]
  0.0684,  //  y_m [m]
 -0.1934,  //  z_m [m]
  0.,      //  p [m]
  0.338,   //  q [m]
  0.,      //  r [m]
  1.265,   //  r_D [m]
} ;
//It is not the fastest dome, one revolution in 5 minutes
#define AngularSpeed 2. * M_PI/ 98. 
#define POLLMICROS 1. * 1000. * 1000. 
#define DIFFMAX 60 /* Difference where curMaxSetPoint is reached */
#define DIFFMIN  5 /* Difference where curMinSetPoint is reached*/
void getSexComponents(double value, int *d, int *m, int *s)
{
  *d = (int) fabs(value);
  *m = (int) ((fabs(value) - *d) * 60.0);
  *s = (int) rint(((fabs(value) - *d) * 60.0 - *m) *60.0);
  
  if (value < 0)
    *d *= -1;
}
void *move_to_target_azimuth(__attribute__ ((unused)) void *value)
{
  double lastRa=-9999., lastDec=-9999. ;
  bool target_coordinate_changed= false ;
  //bool first=true; - unused
  while( 1==1) {

    current_percentage= get_current_percentage() ;
    if ( movementState == SYNCHRONIZATION_ENABLED) {
      target_coordinate_changed= false ;
      if( lastRa != tel_equ.ra)	{
	lastRa = tel_equ.ra ;
	target_coordinate_changed=true ;
      }
      if( lastDec != tel_equ.dec) {
	lastDec = tel_equ.dec ;
	target_coordinate_changed=true ;
      }    
      if(target_coordinate_changed) {
	fprintf( stderr, "move_to_target_azimuth: target_coordinate_changed, now ra:%8.5f, dec:%8.5f\n", tel_equ.ra, tel_equ.dec) ;
	double HA, HA_h ;
	double RA, RA_h ;
	char RA_str[32] ;
	char HA_str[32] ;
	int h, m, s ;
      
	double JD  = ln_get_julian_from_sys ();
	double lng = obs_location.lng;
	double local_sidereal_time= ln_get_mean_sidereal_time( JD) * 15. + lng;  // longitude positive to the East
      
	RA  = tel_equ.ra ;
	RA_h= RA/15. ;

	getSexComponents( RA_h, &h, &m, &s) ;
	snprintf(RA_str, 9, "%02d:%02d:%02d", h, m, s);

	HA  = fmod( local_sidereal_time- RA+ 360., 360.) ;
	HA_h= HA / 15. ;
      
	getSexComponents( HA_h, &h, &m, &s) ;
	snprintf(HA_str, 9, "%02d:%02d:%02d", h, m, s);
	//fprintf( stderr, "move_to_target_azimuth: HA: %s\n", HA_str) ;
      }
      //fprintf( stderr, "move_to_target_azimuth: sync enabled: yes\n") ;
      target_az= dome_target_az( tel_equ, obs_location, obsvermes) ;
      //fprintf( stderr, "move_to_target_azimuth: target_az: %8.5f\n",target_az) ;
      if (isnan(target_az)) {
	fprintf( stderr, "move_to_target_azimuth: target_az is NAN\n") ;
      } else {
	move_to_az();
      }
    } else if(manual_target_az != UNDEFINED){
      target_az=manual_target_az;
      //fprintf( stderr, "move_to_target_azimuth: manual target_az: %8.5f\n",target_az) ;
      move_to_az();
    }
    readSetPoint= (double) get_setpoint() ;
    usleep(POLLMICROS) ;
  }
  return NULL ;
}

void move_to_az(){
  int ret ;
  double abs_az_diff = -1 ;
  double curAzimutDifference ;
  static double curSetPoint=0. ;
  static double lastSetPointSign=0., curSetPointSign=0. ;
  double tmpSetPoint= 0. ;
  const double limit= ( .25 * AngularSpeed * (POLLMICROS/(1000. * 1000.)));

  curAzimutDifference=  barcodereader_az- target_az;
  // fmod is here just in case if there is something out of bounds
  curAzimutDifference= fmod( curAzimutDifference, 360.) ;
  // Shortest path
  if(( curAzimutDifference) >= 180.) {
    curAzimutDifference -=360. ;
  } else if(( curAzimutDifference) <= -180.) {
    curAzimutDifference += 360. ;
  }
  // curMaxSetPoint, curMinSetPoint are always > 0, curSetPoint is [-100.,100.] 
  tmpSetPoint= (( curMaxSetPoint- curMinSetPoint) / (DIFFMAX- DIFFMIN) * fabs( curAzimutDifference)  + curMinSetPoint ) ;
  if( tmpSetPoint > curMaxSetPoint) {
    tmpSetPoint= curMaxSetPoint;
  } else if( tmpSetPoint < curMinSetPoint) {
    tmpSetPoint= curMinSetPoint ;
  }
  if(fabs(curAzimutDifference) < 1.2) { // 0.9 deg resolution https://azug.minpet.unibas.ch/wikiobsvermes/index.php/Azimut_feedback
    curSetPointSign= 0 ;
  } else if( curAzimutDifference != 0) {
    curSetPointSign= -curAzimutDifference/fabs(curAzimutDifference) ;
  } else {
    curSetPointSign= 0 ;
  }
  // limit the maximum current
  current_percentage= get_current_percentage() ;
  if( isnan(current_percentage)) {
    current_percentage= CURRENT_MAX_PERCENT ;
  }
  if( current_percentage >= 100.) { // hard limit
    struct timespec rep_slv ;
    struct timespec rep_rsl ;
    rep_slv.tv_sec= 0 ;
    rep_slv.tv_nsec= REPEAT_RATE_NANO_SEC ;

    while(( ret= motor_off()) != SSD650V_MS_STOPPED) {
      fprintf(stderr, "move_to_target_azimuth: motor_off != SSD650V_MS_STOPPED\n") ;
      errno= 0;
      ret= nanosleep( &rep_slv, &rep_rsl) ;
      if((errno== EFAULT) || ( errno== EINTR)|| ( errno== EINVAL ))  {
	fprintf( stderr, "off_zero: signal, or error in nanosleep %d\n", ret) ;
      }
    }
  } else if( current_percentage >= CURRENT_MAX_PERCENT) {
    tmpSetPoint *= CURRENT_MAX_PERCENT/ (current_percentage); 
  } 
  curSetPoint= curSetPointSign * tmpSetPoint ;
  //fprintf(stderr,"move_to_az: curSetPoint: %7.2f, az diff: %7.2f\n",curSetPoint, curAzimutDifference);
  if(( motorState== SSD650V_MS_RUNNING)&&( abs_az_diff= fabs( curAzimutDifference/180. * M_PI)) < limit) {
    // MotorOFF
    struct timespec rep_slv ;
    struct timespec rep_rsl ;
    rep_slv.tv_sec= 0 ;
    rep_slv.tv_nsec= REPEAT_RATE_NANO_SEC ;

    while(( ret= motor_off()) != SSD650V_MS_STOPPED) {
      fprintf(stderr, "move_to_target_azimuth: motor_off != SSD650V_MS_STOPPED\n") ;
      errno= 0;
      ret= nanosleep( &rep_slv, &rep_rsl) ;
      if((errno== EFAULT) || ( errno== EINTR)|| ( errno== EINVAL ))  {
	fprintf( stderr, "off_zero: signal, or error in nanosleep %d\n", ret) ;
      }
    }

    curSetPoint= 0. ;
    set_setpoint( curSetPoint) ;
    lastSetPointSign= 0. ;
    if( motorState== SSD650V_MS_STOPPED) {
      is_synced= SYNCED ;
    } else {
      is_synced= SYNCED_MOTOR_ON ;
    }
  } else if (( abs_az_diff= fabs( curAzimutDifference/180. * M_PI)) >= limit) {
    if( curSetPointSign !=  lastSetPointSign) {
      // MotorOFF
      struct timespec rep_slv ;
      struct timespec rep_rsl ;
      rep_slv.tv_sec= 0 ;
      rep_slv.tv_nsec= REPEAT_RATE_NANO_SEC ;

      while(( ret= motor_off()) != SSD650V_MS_STOPPED) {
	fprintf(stderr, "move_to_target_azimuth: motor_off != SSD650V_MS_STOPPED\n") ;
	errno= 0;
	ret= nanosleep( &rep_slv, &rep_rsl) ;
	if((errno== EFAULT) || ( errno== EINTR)|| ( errno== EINVAL ))  {
	  fprintf( stderr, "off_zero: signal, or error in nanosleep %d\n", ret) ;
	}
      }
    }
    //  MotorON
    set_setpoint( curSetPoint) ;
    if( motorState != SSD650V_MS_RUNNING) {
      if(( ret=motor_on()) != SSD650V_MS_RUNNING ) {
	fprintf( stderr, "move_to_target_azimuth: something went wrong with  azimuth motor (ON)\n") ;
      } 
    }
    lastSetPointSign=  curSetPointSign ;
  }
  if(( motorState== SSD650V_MS_RUNNING) || (( abs_az_diff= fabs( curAzimutDifference/180. * M_PI)) >= limit)) {
    is_synced= NOT_SYNCED ;
  }
}
