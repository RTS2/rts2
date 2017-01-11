/* 
 * Astro-Physics GTO mount driver 
 * Copyright (C) 2009-2017, Markus Wildi, Petr Kubanek and INDI
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

#include <string>
#include "libnova_cpp.h"
#include "configuration.h"

#include "rts2lx200/tellx200.h"
#include "rts2lx200/hms.h"
#include "clicupola.h"
#include "rts2lx200/pier-collision.h"

#define OPT_APGTO_ASSUME_PARKED  OPT_LOCAL + 55
#define OPT_APGTO_KEEP_HORIZON   OPT_LOCAL + 58
#define OPT_APGTO_TIMEOUT_SLEW_START     OPT_LOCAL + 60

#define setAPUnPark()       serConn->writePort ("#:PO#", 5)// ok, no response
#define setAPLongFormat()   serConn->writePort ("#:U#", 4) // ok, no response

#define DIFFERENCE_MAX_WHILE_NOT_TRACKING 1.  // [deg]
#define PARK_POSITION_RA -270.
#define PARK_POSITION_DEC 5.
#define TIMEOUT_SLEW_START 3600. // [second]
#define PARK_POSITION_ANGLE "West"
#define PARK_POSITION_DIFFERENCE_MAX 2.


namespace rts2teld
{
/**
 * Driver for Astro-Physics GTO protocol based mount. 
 * 
 * @author Markus Wildi
 * @author Petr Kubanek
 */
class APGTO:public TelLX200 {
	public:
		APGTO (int argc, char **argv);
		virtual ~APGTO (void);

		virtual void postEvent (rts2core::Event *event);

		virtual int processOption (int in_opt);
		virtual int initHardware ();
		virtual int initValues ();
		virtual int info ();
		virtual int idle ();

		virtual int setTo (double set_ra, double set_dec);

		virtual int startResync ();
		virtual int isMoving ();
		virtual int endMove ();
		virtual int stopMove ();
  
		virtual int startPark ();
		virtual int isParking ();
		virtual int endPark ();
  
		virtual int abortAnyMotion () ;
	protected:
		virtual int setValue (rts2core::Value * oldValue, rts2core::Value *newValue);
		virtual int willConnect (rts2core::NetworkAddress * in_addr);
		virtual void notMoveCupola ();

private:
  double on_set_HA ;
  double on_zero_HA ;
  time_t slew_start_time;
  // Astro-Physics LX200 protocol specific functions
  int getAPVersionNumber ();
  int getAPUTCOffset ();
  //ToDo:
  //int setAPUTCOffset (int hours);
  int setAPUTCOffset(double hours) ;
  // commands not listed anymore on:
  // http://astro-physics.com/tech_support/mounts/command_lang.htm
  int syncAPCMR (char *matchedObject);
  int getAPRelativeAngle ();
  // and ToDo needs further investigation, RT9 does not exist
  int setAPTrackingMode ();
  // ToDo method "disappeared"
  // see setValue()
  // int setAPMoveToRate (int moveToRate);

  int setAPLongitude ();
  int setAPLatitude ();
  int setAPLocalTime (int x, int y, int z);
  int setAPBackLashCompensation(int x, int y, int z);
  int setAPCalenderDate (int dd, int mm, int yy);
  // helper
  double localSiderealTime ();
  int checkSiderealTime (double limit) ;
  int checkLongitudeLatitude (double limit) ;
  int dateTime ();
  int setBasicData ();
  int tel_slew_to (double ra, double dec);
  // ToDo: no ToDo??
  //int tel_slew_to_altaz (double alt, double az);
  int checkAPCoords (double ra, double dec);

		
  rts2core::ValueSelection *trackingMode;
  rts2core::ValueSelection *APslew_rate;
  rts2core::ValueSelection *APcenter_rate;
  rts2core::ValueSelection *APguide_rate;

  rts2core::ValueDouble *APutc_offset;
  rts2core::ValueString *APfirmware ;
  rts2core::ValueString *DECaxis_HAcoordinate ; // see pier_collision.c 
  rts2core::ValueBool *assume_parked;
  rts2core::ValueBool *collision_detection;

  rts2core::ValueBool    *block_sync_apgto;
  rts2core::ValueBool    *block_move_apgto;
  //ToDo should goaway inf favor of base class
  rts2core::ValueBool    *slew_state; // (display: move_state)
  rts2core::ValueBool    *tracking ;
  rts2core::ValueBool    *transition_while_tracking ; 

  rts2core::ValueDouble  *APlocalSiderealTime;
  rts2core::ValueDouble  *APhourAngle;
  rts2core::ValueDouble  *APTime;
  // RTS2 counts positive to the East, AP positive to the West
  rts2core::ValueDouble  *APlongitude;
  rts2core::ValueDouble  *APlatitude;

  void set_move_timeout (time_t plus_time);
  time_t move_timeout;
  double lastMoveRa, lastMoveDec;  

  virtual void startCupolaSync ();
  int checkAPRelativeAngle() ;
  virtual int commandAuthorized (rts2core::Connection * conn);

  int f_scansexa (const char *str0, double *dp);
  virtual int startDir (char *dir);
  virtual int stopDir (char *dir);

  rts2core::Configuration *config ;
  int getAPLongitude ();
  int getAPLatitude ();
  int getAPlocalSiderealTime ();

  const char *device_file;

};
}

using namespace rts2teld;

int APGTO::f_scansexa (const char *str0, double *dp) /* input string, cracked value, if return 0 */
{
  double a = 0, b = 0, c = 0;
  char str[128];
  char *neg;
  int r;

  /* copy str0 so we can play with it */
  strncpy (str, str0, sizeof(str)-1);
  str[sizeof(str)-1] = '\0';

  neg = strchr(str, '-');
  if(neg)
    *neg = ' ';
  r = sscanf (str, "%lf%*[^0-9]%lf%*[^0-9]%lf", &a, &b, &c);
  if(r < 1)
    return (-1);
  *dp = a + b/60. + c/3600.;
  if(neg)
    *dp *= -1;
  return (0);
}
/**
 * Reads from APGTO the version character of the firmware
 *
 * @param none
 *
 * @return -1 and set errno on error, otherwise 0
 */
int APGTO::getAPVersionNumber()
{
	int ret= -1 ;

	char version[32];
	APfirmware->setValueString ("none");
	if((ret = serConn->writeRead ("#:V#", 4, version, 32, '#')) < 1 )
		return -1 ;
	version[ret - 1] = '\0';
	APfirmware->setValueString (version); //version);
	return 0 ;
}

/**
 * Reads from APGTO the UTC offset and corrects the output according to Astro-Physics documentation firmware revision D
 * @param none
 *
 * @return -1 and set errno on error, otherwise 0
 */
int APGTO::getAPUTCOffset()
{
  int nbytes_read = 0;
  double offset ;
  char temp_string[16];

  if((nbytes_read = serConn->writeRead ("#:GG#", 5, temp_string, 11, '#')) < 1 )
    return -1 ;
  // Negative offsets, see AP keypad manual p. 77
  if((temp_string[0]== 'A') || ((temp_string[0]== '0') && (temp_string[1]== '0')) || (temp_string[0]== '@'))
    {
      int i;
      for (i = nbytes_read; i > 0; i--)
	{
	  temp_string[i]= temp_string[i-1];
	}
      temp_string[0] = '-';
      temp_string[nbytes_read + 1] = '\0';

      if(temp_string[1] == 'A')
	{
	  temp_string[1]= '0';
	  switch (temp_string[2])
	    {
	    case '5':
	      temp_string[2]= '1';
	      break;
	    case '4':
	      temp_string[2]= '2';
	      break;
	    case '3':
	      temp_string[2]= '3';
	      break;
	    case '2':
	      temp_string[2]= '4';
	      break;
	    case '1':
	      temp_string[2]= '5';
	      break;
	    default:
	      logStream (MESSAGE_DEBUG) << "APGTO::getAPUTCOffset string not handled >" << temp_string << "<END" << sendLog;
	      return -1;
	    }
	}
      else if(temp_string[1]== '0')
	{
	  temp_string[1]= '0';
	  temp_string[2]= '6';
	}
      else if(temp_string[1]== '@')
	{
	  temp_string[1]= '0' ;
	  switch (temp_string[2])
	    {
	    case '9':
	      temp_string[2]= '7';
	      break;
	    case '8':
	      temp_string[2]= '8';
	      break;
	    case '7':
	      temp_string[2]= '9';
	      break;
	    case '6':
	      temp_string[2]= '0';
	      break;
	    case '5':
	      temp_string[1]= '1';
	      temp_string[2]= '1';
	      break;
	    case '4':
	      temp_string[1]= '1';
	      temp_string[2]= '2';
	      break;
	    default:
	      logStream (MESSAGE_DEBUG) <<"APGTO::getAPUTCOffset string not handled >" << temp_string << "<END" << sendLog;
	      return -1;
	    }    
	}
      else
	logStream (MESSAGE_ERROR) <<"APGTO::getAPUTCOffset string not handled >" << temp_string << "<END" << sendLog;
    }
  else
    temp_string[nbytes_read - 1] = '\0';
  
  offset = hmstod (temp_string);
  if(isnan (offset))
    {
      logStream (MESSAGE_ERROR) <<"APGTO::getAPUTCOffset string not handled  >" << temp_string << "<END" <<sendLog;
      return -1;
    }
  APutc_offset->setValueDouble(offset * 15.) ;
  return 0;
}

/*!
 * Writes to APGTO UTC offset
 *
 *
 * @param hours
 *
 * @return -1 on failure, 0 otherwise
 */
int APGTO::setAPUTCOffset(double hours) 
{
  int ret= -1;
  int h, m, s ;
  char temp_string[16];
  char retstr[1] ;
  // To avoid the peculiar output format of AP controller, see p. 77 key pad manual
  if(hours < 0.)
    hours += 24.;

  dtoints (hours, &h, &m, &s);
  snprintf(temp_string, sizeof(temp_string), "#:SG %+03d:%02d:%02d#", h, m, s);

  if((ret = serConn->writeRead (temp_string, sizeof (temp_string), retstr, 1)) < 0){
    logStream (MESSAGE_ERROR) << "APGTO::setAPUTCOffset writing failed." << sendLog;
    return -1;
  }
  return 0;
}
/*!
 * Writes to APGTO the tracking mode
 *
 * @return -1 on failure, 0 otherwise
 */
int APGTO::setAPTrackingMode ()
{
  int ret;
  char *v = (char *) trackingMode->getData ();

  ret = serConn->writePort (v, strlen (v));
  if(ret != 0)
    return -1;
  if(!strcmp(v, ":RT9#")) {  //ToDo find a better solution
    notMoveCupola() ;
    on_zero_HA= fmod(localSiderealTime()-getTelRa()+360.,360.);
    tracking->setValueBool(false);
  } else 
    tracking->setValueBool(true);
  return 0;
}
/*!
 * Writes to APGTO the observatory longitude
 *
 *
 * @param Long
 *
 * @return -1 on failure, 0 otherwise
 */
int APGTO::setAPLongitude ()
{
  int ret = -1;
  int d, m, s;
  char temp_string[32];
  char retstr[1];

  double APlng=fmod(360.- telLongitude->getValueDouble(),360.0); 
	
  dtoints (APlng, &d, &m, &s);
  snprintf (temp_string, sizeof(temp_string), "#:Sg %03d*%02d:%02d#", d, m, s);
  logStream (MESSAGE_DEBUG) << "APGTO::setAPLongitude :"<<temp_string << sendLog;
  
  if((ret= serConn->writeRead(temp_string, sizeof(temp_string),retstr,1)) < 0)
    return -1;
  
  return 0;
}

/*!
 * Writes to APGTO the observatory latitude
 *
 *
 * @param Lat
 *
 * @return -1 on failure, 0 otherwise
 */
int APGTO::setAPLatitude()
{
  int ret = -1;
  int d, m, s;
  char temp_string[32];
  char retstr[1];
  double APLat= telLatitude->getValueDouble();
  dtoints (APLat, &d, &m, &s);
  snprintf (temp_string, sizeof(temp_string), "#:St %+02d*%02d:%02d#", d, m, s);
  if((ret = serConn->writeRead (temp_string, sizeof(temp_string), retstr, 1)) < 0)
    return -1 ;
  return 0;
}

/*!
 * Writes to APGTO the gear back lash compensation
 *
 *
 * @param x see Astro-Physics documentation
 * @param y see Astro-Physics documentation
 * @param z see Astro-Physics documentation
 *
 * @return -1 on failure, 0 otherwise
 */
int APGTO::setAPBackLashCompensation(int x, int y, int z)
{
  int ret = -1;
  char temp_string[16];
  char retstr[1];
  
  snprintf(temp_string,sizeof(temp_string),"%s %02d:%02d:%02d#","#:Br",x,y,z);
  
  if((ret = serConn->writeRead(temp_string,sizeof(temp_string),retstr,1)) < 0)
    return -1 ;
  return 0;
}
/*!
 * Writes to APGTO the local timezone time
 *
 *
 * @param x hour
 * @param y minute
 * @param z seconds
 *
 * @return -1 on failure, 0 otherwise
 */
int APGTO::setAPLocalTime(int x, int y, int z)
{
  int ret = -1;
  char temp_string[16];
  char retstr[1];
  
  snprintf (temp_string, sizeof(temp_string), "%s %02d:%02d:%02d#", "#:SL" , x, y, z);
  if((ret = serConn->writeRead (temp_string, sizeof(temp_string), retstr, 1)) < 0)
    return -1;
  return 0;
}
/*!
 * Writes to APGTO the local timezone date
 *
 *
 * @param dd day
 * @param mm month
 * @param yy year
 *
 * @return -1 on failure, 0 otherwise
 */
int APGTO::setAPCalenderDate(int dd, int mm, int yy)
{
  char cmd_string[32];
  char temp_string[256];
  
  //Command: :SC MM/DD/YY#
  //Response: 32 spaces followed by “#”, followed by 32 spaces, followed by “#”
  //Sets the current date. Note that year fields equal to or larger than 97 are assumed to be 20 century, 
  //Year fields less than 97 are assumed to be 21st century.

  yy = yy % 100;
  snprintf(cmd_string, sizeof(cmd_string), "#:SC %02d/%02d/%02d#", mm, dd, yy);
  
  if(serConn->writeRead (cmd_string, 14, temp_string, 33, '#') < 1)
    return -1;
  if(serConn->readPort (temp_string, 33, '#') < 1)
    return -1 ;
  return 0;
}

/*!
 * Reads APGTO relative position of the declination axis to the observed hour angle.
 *
 * @return -1 on error, otherwise 0
 *
 */
int APGTO::getAPRelativeAngle ()
{
  int ret ;
  char new_declination_axis[32] ;
  
  ret = serConn->writeRead ("#:pS#", 5, new_declination_axis, 5, '#');
  if(ret < 0)
    return -1;
  new_declination_axis[ret-1] = '\0';
  
  DECaxis_HAcoordinate->setValueString(new_declination_axis) ;
  return 0;
}
/*!
 * Reads and checks APGTO relative position of the declination axis to the observed hour angle.
 *
 * @return -1 on error, otherwise 0, or exit(1) in case of a serious failure
 *
 */
int APGTO::checkAPRelativeAngle ()
{
  int stop= 0 ;
  // make sure that the (declination - optical axis) have the correct sign
  // if not you'll buy a new tube.
  if(tel_read_ra () || tel_read_dec ()) {
    stop= 1 ;
  }  
  double HA, HA_h ;
  double RA, RA_h ;
  char RA_str[32] ;
  char HA_str[32] ;
  int h, m, s ;

  RA  = getTelRa() ;
  RA_h= RA/15. ;

  dtoints(RA_h, &h, &m, &s) ;
  snprintf(RA_str, 9, "%02d:%02d:%02d", h, m, s);

  HA  = fmod(localSiderealTime()- RA+ 360., 360.) ;
  HA_h= HA / 15. ;

  dtoints(HA_h, &h, &m, &s) ;
  snprintf(HA_str, 9, "%02d:%02d:%02d", h, m, s);
	
  if(getAPRelativeAngle()) {
    block_sync_apgto->setValueBool(true) ;
    block_move_apgto->setValueBool(true) ;
    stop = 1 ;
    logStream (MESSAGE_ERROR) << "APGTO::checkAPRelativeAngle could not retrieve sign of declination axis, severe error, blocking." << sendLog;
    return -1;
  }
  if(!strcmp("West", DECaxis_HAcoordinate->getValue())) {
    if((HA > 180.) && (HA <= 360.)) {
    } else {
      if(transition_while_tracking->getValueBool()) {
	// the mount is allowed to transit the meridian while tracking
      } else {
	block_sync_apgto->setValueBool(true) ;
	block_move_apgto->setValueBool(true) ;
	stop = 1 ;
	logStream (MESSAGE_ERROR) << "APGTO::checkAPRelativeAngle sign of declination and optical axis is wrong (HA=" << HA_str<<", West), severe error, blocking any sync and moves" << sendLog;
	// can not exit here because if HA>0, West the telecope must be manually "rot e" to the East
      }
    }
  } else if(!strcmp("East", DECaxis_HAcoordinate->getValue())) {
    if((HA >= 0.0) && (HA <= 180.)) {
    } else {
      block_sync_apgto->setValueBool(true) ;
      block_move_apgto->setValueBool(true) ;
      stop = 1 ;
      logStream (MESSAGE_ERROR) << "APGTO::checkAPRelativeAngle sign of declination and optical axis is wrong (HA=" << HA_str<<", East), severe error, sync manually, blocking any sync and moves" << sendLog;
    }
  }
  if(stop != 0) {
    if((abortAnyMotion () !=0)) {
      logStream (MESSAGE_WARNING) << "APGTO::checkAPRelativeAngle failed to stop any tracking and motion" << sendLog;
    }
    return -1 ;
  } else {
    return 0 ;
  }
}

/*!
 * Slew (=set) APGTO to new coordinates.
 *
 * @param ra 		new right ascenation
 * @param dec 		new declination
 *
 * @return -1 on error, otherwise 0
 */
int APGTO::tel_slew_to (double ra, double dec)
{
  int ret ;
  char retstr;
  char target_DECaxis_HAcoordinate[32] ;
  struct ln_lnlat_posn observer;
  struct ln_equ_posn target_equ;
  struct ln_hrz_posn hrz;
 
  if(block_move_apgto->getValueBool()) {
    logStream (MESSAGE_INFO) << "APGTO::tel_slew_to move is blocked, see BLOCK_MOVE_APGTO, doing nothing" << sendLog;
    return -1 ;
  } 
  if(slew_state->getValueBool()) {
    logStream (MESSAGE_INFO) << "APGTO::tel_slew_to mount is slewing, ignore slew command to RA " << ra << "Dec "<< dec << sendLog;
    return -1 ;
  }
  if(!(collision_detection->getValueBool())) {
    logStream (MESSAGE_INFO) << "APGTO::tel_slew_to do not slew due to collision detection is disabled" << sendLog;
    return -1 ;
  }
  target_equ.ra = fmod(ra + 360., 360.) ;
  target_equ.dec= fmod(dec, 90.);
  observer.lng = telLongitude->getValueDouble ();
  observer.lat = telLatitude->getValueDouble ();

  ln_get_hrz_from_equ (&target_equ, &observer, ln_get_julian_from_sys (), &hrz);

  if(hrz.alt < 0.) {
    logStream (MESSAGE_WARNING) << "APGTO::tel_slew_to target_equ ra " << target_equ.ra << " dec " <<  target_equ.dec << " is below horizon" << sendLog;
    return -1 ;
  }
  if(checkAPRelativeAngle()) {
    logStream (MESSAGE_WARNING) << "APGTO::tel_slew_to check of the declination axis failed." << sendLog;
    return -1;
  }

  double target_HA= fmod(localSiderealTime()- target_equ.ra+ 360., 360.) ;
  if((target_HA > 180.) && (target_HA <= 360.)) {
    strcpy(target_DECaxis_HAcoordinate, "West");
  } else {
    strcpy(target_DECaxis_HAcoordinate, "East");
  }

  if(!(strcmp("West", target_DECaxis_HAcoordinate))) {
    ret= pier_collision(&target_equ, &observer) ;
  } else if(!(strcmp("East", target_DECaxis_HAcoordinate))) {
    //really target_equ.dec += 180. ;
    struct ln_equ_posn t_equ;
    t_equ.ra = target_equ.ra ;
    t_equ.dec= target_equ.dec + 180. ;
    ret= pier_collision(&t_equ, &observer) ;
  }
  if(ret != NO_COLLISION) {
    if(ret== COLLIDING) {
      logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to NOT slewing ra " << target_equ.ra << " dec " << target_equ.dec << " COLLIDING, NOT syncing cupola"  << sendLog;
    } else if(ret== WITHIN_DANGER_ZONE){
      logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to NOT slewing ra " << target_equ.ra << " dec " << target_equ.dec << " within DANGER zone, NOT syncing cupola"  << sendLog;
    } else {
      logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to NOT slewing ra " << target_equ.ra << " target_equ.dec " << target_equ.dec << " invalid condition"  << sendLog;
    }
    return -1;
  }
  logStream (MESSAGE_INFO) << "APGTO::tel_slew_to cleared geometry  slewing ra " << target_equ.ra << " target_equ.dec " << dec  << " target_HA " << target_HA << sendLog;

  if((ret=tel_write_ra (target_equ.ra)) < 0) {
    logStream (MESSAGE_DEBUG) << "APGTO::tel_slew_to, not slewing, tel_write_ra return value was " << ret << sendLog ;
    return -1;
  }
  if((ret=tel_write_dec (target_equ.dec)) < 0) {
    logStream (MESSAGE_DEBUG) << "APGTO::tel_slew_to, not slewing, tel_write_dec return value was " << ret << sendLog ;
    return -1;
  }
  // (re-)enable tracking
  // in case the mount detected a collision, tracking and any motion is stopped
  // but:
  // if target position is ok, mount can track again
  // renable tracking before slew
  if(! tracking->getValueBool()) {

    trackingMode->setValueInteger (1); //TRACK_MODE_SIDEREAL
    if(setAPTrackingMode() < 0) { 
      logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to set track mode sidereal failed." << sendLog;
      if((abortAnyMotion () !=0)) {
	logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to abortAnyMotion failed" << sendLog;
      }
      return -1;
    } 
    //startCupolaSync ();
    logStream (MESSAGE_DEBUG) << "APGTO::tel_slew_to set track mode sidereal (re-)enabled, cupola synced." << sendLog;
  }
  // slew now
  if(serConn->writeRead ("#:MS#", 5, &retstr, 1) < 0){
    logStream (MESSAGE_ERROR) <<"APGTO::tel_slew_to, not slewing, tel_write_read #:MS# failed" << sendLog;
    return -1;
  }
  if(retstr == '0') {
    if((ret= getAPRelativeAngle()) != 0)
      return -1 ;
    // slew was successful, set and reset the state 
    on_set_HA= target_HA ; // used for defining state transition while tracking
    transition_while_tracking->setValueBool(false) ;
    time(&slew_start_time);
    slew_state->setValueBool(true) ;
    return 0;
  }
  logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to NOT slewing ra " << target_equ.ra << " dec " << target_equ.dec << " got '0'!=" << retstr<<"<END, NOT syncing cupola"  << sendLog;
  return -1;
}
/*!
 * Check, if mount match given coordinates.
 *
 * @param ra		target right ascenation
 * @param dec		target declination
 *
 * @return -1 on error, 0 if not matched, 1 if matched, 2 if timeouted
 */
int APGTO::checkAPCoords (double ra, double dec)
{
  // ADDED BY JF
  double sep;
  struct ln_equ_posn object, target;

  if((tel_read_ra () < 0) || (tel_read_dec () < 0)) {
    logStream (MESSAGE_WARNING) << "APGTO::checkAPCoords read ra, dec failed" << sendLog;
    return -1;
  }
  object.ra = fmod(getTelRa () + 360., 360.);
  object.dec= fmod(getTelDec (), 90.);

  target.ra = fmod(ra+ 360., 360.);
  target.dec= fmod(dec , 90.) ;
  
  sep = ln_get_angular_separation (&object, &target);

  if(sep > 0.1)
    return 0;
  
  return 1;
}
void APGTO::set_move_timeout (time_t plus_time)
{
	time_t now;
	time (&now);
	move_timeout = now + plus_time;
}

int APGTO::startResync ()
{
  struct ln_equ_posn tar;
  // used in ::isMoving() 
  lastMoveRa = fmod(getTelTargetRa () + 360., 360.);
  lastMoveDec = fmod(getTelTargetDec (), 90.);
  // from dummy.cpp
  getTarget (&tar);
  setTarget (tar.ra, tar.dec);
  setTelTarget (tar.ra, tar.dec);
  
  if(tel_slew_to (getTelTargetRa(), getTelTargetDec())) {
    if((abortAnyMotion () !=0)) 
      logStream (MESSAGE_ERROR) << "APGTO::startResync abortAnyMotion failed" << sendLog;
    
    return -1;
  }
  set_move_timeout (10); //ToDo: see base class, used?? propbably not
  return 0;
}

void APGTO::startCupolaSync ()
{
  if(block_move_apgto->getValueBool()) {
    logStream (MESSAGE_INFO) << "APGTO::startCupolaSync DO NOT sync cupola, while move is blocked" << sendLog;
    return;
  }
  struct ln_equ_posn target_equ;
  getTarget(&target_equ);
  double dec_offset_east=0.;
  
  target_equ.ra = fmod(target_equ.ra+360.,360.) ;
  target_equ.dec= fmod(target_equ.dec,90.) ;
    
  if(!(strcmp("East", DECaxis_HAcoordinate->getValue()))){
    dec_offset_east=180.;  
  }
  rts2core::CommandCupolaSyncTel cmd (this, target_equ.ra, target_equ.dec+dec_offset_east);
  queueCommandForType (DEVICE_TYPE_CUPOLA, cmd);
  logStream (MESSAGE_INFO) << "APGTO::startCupolaSync sync cupola" << sendLog;
}

void APGTO::notMoveCupola ()
{
  postEvent (new rts2core::Event (EVENT_CUP_NOT_MOVE));
}

int APGTO::isMoving ()
{
  struct ln_equ_posn target_equ;
  getTarget (&target_equ);

  switch (checkAPCoords (lastMoveRa, lastMoveDec)) {
  case -1:
    return -1;
  case 1: //isMoving return 1 if(sep < 0.1) 
    return -2;
  default: //isMoving return 0 if(sep > 0.1)  
    return USEC_SEC / 10;
  }
}

int APGTO::endMove ()
{
  slew_state->setValueBool(false) ;
  logStream (MESSAGE_DEBUG) << "APGTO::endMove: end of slew" << sendLog;
  return Telescope::endMove ();
}

int APGTO::stopMove ()
{
  int error_type;
    
  if((error_type = serConn->writePort ("#:Q#", 4)) < 0)
    return error_type;
  
  notMoveCupola ();
  sleep (1);
  maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING | BOP_EXPOSURE, TEL_NOT_CORRECTING | TEL_OBSERVING, "move stopped");
  slew_state->setValueBool(false) ;
  logStream (MESSAGE_DEBUG) << "APGTO::stopMove: end of slew" << sendLog;
  
  return 0;
}

/*!
 * Set mount to match given coordinates (sync)
 *
 * AP GTO remembers the last position only occasionally it looses it.
 *
 * @param ra		setting right ascscension
 * @param dec		setting declination
 *
 * @return -1 and set errno on error, otherwise 0
 */
int APGTO::setTo (double ra, double dec)
{
  char readback[101];

  if(block_sync_apgto->getValueBool()) {
    logStream (MESSAGE_WARNING) << "APGTO::setTo sync is blocked, see BLOCK_SYNC_APGTO, doing nothing" << sendLog;
    return -1 ;
  }
  //ToDo: I would like to use this code, but unsure
  //if((getState () & TEL_MASK_MOVING) != TEL_OBSERVING)
  //{
  if(slew_state->getValueBool()) {
    logStream (MESSAGE_DEBUG) << "APGTO::setTo mount is slewing, ignore sync command to RA " << ra << "Dec "<< dec << sendLog;
    return -1 ;
  }
  if(!(collision_detection->getValueBool()))
    {
      logStream (MESSAGE_INFO) << "APGTO::setTo collision detection is disabled, enable it" << sendLog;
      return -1;
    }
  ra = fmod(ra + 360., 360.);
  dec = fmod(dec, 90.);
  if((tel_write_ra (ra) < 0) || (tel_write_dec (dec) < 0))
    return -1;
  //AP manual:
  //Position
  //Command:  :CM#
  //Response: “Coordinates matched.     #”
  //          (there are 5 spaces between “Coordinates” and “matched”, and 8 trailing spaces before the “#”, 
  //          the total response length is 32 character plus the “#”.	  

  if(serConn->writeRead ("#:CM#", 5, readback, 100, '#') < 0){
    logStream (MESSAGE_WARNING) <<"APGTO::setTo #:CM# failed" << sendLog;
    return -1;
  }
  return 0 ;
}
/*
 * Writes to APGTO sync #:CMR
 *
 *
 * @param *matchedObject 
 *
 * @return -1 on failure, 0 otherwise
 */
// wildi: not yet in in use
// 2017-01-09, seems gone in modern controllers
int APGTO::syncAPCMR(char *matchedObject)
{
  int error_type;
  
  if((error_type = serConn->writeRead ("#:CMR#", 6, matchedObject, 33, '#')) < 0)
    return error_type;
  return 0;
}
/**
 * Park mount to neutral location.
 *
 * @return -1 and errno on error, 0 otherwise
 */
int APGTO::startPark ()
{
  //wstartparksetTargetAltAz (0, 355);
  //return moveAltAz () ? -1 : 1;

  notMoveCupola() ;
  double park_ra= fmod(localSiderealTime()+ PARK_POSITION_RA, 360.);
  logStream (MESSAGE_DEBUG) <<"APGTO::startPark "<< park_ra<<  sendLog;
  setTarget(park_ra, PARK_POSITION_DEC);
  return startResync ();
}

int APGTO::isParking ()
{
  return isMoving ();
}

int APGTO::endPark ()
{
  // The AP mount will not surely stop with #:KA alone
  if(stopMove () < 0)
    {
      logStream (MESSAGE_ERROR) << "APGTO::endPark motion stop failed #:Q#" << sendLog;
      return -1;
    }
  trackingMode->setValueInteger(0); 
  if(setAPTrackingMode())
    return -1;

  // wildi ToDo: not yet decided
  // issuing #:KA# means a power cycle of the mount (not always)
  // From experience I know that the Astro-Physics controller saves the last position
  // without #:KA#, but not always!
  // so no #:KA# is sent
  // serConn->writePort (":KA#", 4);

  logStream (MESSAGE_INFO) << "APGTO::endPark telescope parked successfully" << sendLog;
  return 0;
}
int APGTO::startDir (char *dir)
{
  int ret= -1;
  if((ret= tel_start_slew_move (*dir))==0) 
    slew_state->setValueBool(true) ;
  
  return ret;
}

int APGTO::stopDir (char *dir)
{
  int ret = -1 ;
  if((ret= tel_stop_slew_move (*dir))==0)
    slew_state->setValueBool(false) ;
  
  return ret;
}

int APGTO::abortAnyMotion ()
{
  int ret = 0 ;
  int failed= 0 ;

  if((ret = stopMove()) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::abortAnyMotion stop motion #:Q# failed, SWITCH the mount OFF" << sendLog;
    failed = 1 ;
  }
  trackingMode->setValueInteger (0);
  if((ret= setAPTrackingMode()) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::abortAnyMotion setting tracking mode ZERO failed." << sendLog;
    failed = 1 ;
  }
  if(failed == 1) {
    logStream (MESSAGE_ERROR) << "APGTO::abortAnyMotion failed: check motion and tracking now" << sendLog;
    return -1 ;
  }
  notMoveCupola() ;
  logStream (MESSAGE_DEBUG) << "APGTO::abortAnyMotion successfully stoped motion #:Q# and tracking" << sendLog;
  return 0 ;
 }

int APGTO::setValue (rts2core::Value * oldValue, rts2core::Value *newValue)
{  
	if(oldValue == trackingMode)
	{
		trackingMode->setValueInteger (newValue->getValueInteger ());
		return setAPTrackingMode () ? -2 : 0;
	}
	if(oldValue == APslew_rate || oldValue == APcenter_rate || oldValue == APguide_rate)
	{
		char cmd[] = ":RGx#";
		if(oldValue == APslew_rate){
		  cmd[2] = 'S';
		}else if(oldValue == APcenter_rate){
		  cmd[2] = 'C';
		}
		cmd[3] = '0' + newValue->getValueInteger ();
		logStream (MESSAGE_ERROR) << "APGTO::setValue, sending cmd: " << cmd << sendLog; 
		// 5 was 9 
		return serConn->writePort (cmd, 5) ? -2 : 0;
	}
	if(oldValue == raPAN || oldValue == decPAN)
	{
		const char *cmd[2][3] = {{":Qe#:Qw#", ":Mw#", ":Me#"}, {":Qs#:Qn#", ":Ms#", ":Mn#"}};
		const char *c;
		if(oldValue == raPAN)
			c = cmd[0][newValue->getValueInteger ()];
		else
			c = cmd[1][newValue->getValueInteger ()];
		return serConn->writePort (c, strlen (c)) ? -2 : 0;
	}
	return TelLX200::setValue (oldValue, newValue);
}

int APGTO::willConnect (rts2core::NetworkAddress * in_addr)
{
  return TelLX200::willConnect (in_addr);
}
int APGTO::commandAuthorized (rts2core::Connection *conn)
{
  int ret = -1 ;

  if(conn->isCommand ("abort")) {
    if((abortAnyMotion () !=0)) {
      logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized failed to abort any tracking and motion" << sendLog;
      return -1 ;
    }
    logStream (MESSAGE_INFO) << "APGTO::commandAuthorized abort any tracking and motion" << sendLog;
    return 0;
  } else if(conn->isCommand ("track")) {
    trackingMode->setValueInteger (1);//TRACK_MODE_SIDEREAL
    if(setAPTrackingMode() < 0) { 
      logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized set track mode sidereal failed." << sendLog;
      return -1;
    }
    return 0 ;
  } else if(conn->isCommand ("rot")) { // move is used for a slew to a specific position
    char *direction ;
    if(conn->paramNextStringNull (&direction) || !conn->paramEnd ()) { 
      logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized rot failed" << sendLog;
      return -2;
    }
    if(startDir(direction)){
      logStream (MESSAGE_WARNING) <<"APGTO::commandAuthorized startDir(direction) failed" << sendLog;
      return -1;
    }
    return 0 ;
    //2017-01-07, wildi, dropped unusable move_* cmds.
  } else if((conn->isCommand("sync"))||(conn->isCommand("sync_sg"))||(conn->isCommand("sync_ha"))||(conn->isCommand("sync_ha_sg"))) {
    logStream (MESSAGE_INFO) << "APGTO::commandAuthorized start sync " << sendLog;
    
    double sync_ra, sync_dec ;
    double sync_ha ;
    if(conn->isCommand ("sync")){
      if(conn->paramNextDouble (&sync_ra) || conn->paramNextDouble (&sync_dec) || !conn->paramEnd ()) { 
	logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized sync paramNextDouble ra or dec failed" << sendLog;
	return -2;
      }
    } else if(conn->isCommand ("sync_ha")){
      if(conn->paramNextDouble (&sync_ha) || conn->paramNextDouble (&sync_dec) || !conn->paramEnd ()) { 
	logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized sync_ha paramNextDouble ra or dec failed" << sendLog;
	return -2;
      }
      sync_ra= localSiderealTime() - sync_ha; // RA is a right, HA left system

    } else if(conn->isCommand ("sync_sg")) {
      char *sync_ra_str;
      char *sync_dec_str;
      if(conn->paramNextStringNull (&sync_ra_str) || conn->paramNextStringNull (&sync_dec_str) || !conn->paramEnd ()) { 
	logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized sync_sg paramNextString ra or dec failed" << sendLog;
	return -2;
      }
      if((ret= f_scansexa (sync_ra_str, &sync_ra))== -1) {
	logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized sync_sg parsing ra failed" << sendLog;
	return -1;
      }
      sync_ra *= 15. ;
      if((ret= f_scansexa (sync_dec_str, &sync_dec))== -1) {
	logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized sync_sg parsing dec failed" << sendLog;
	return -1;
      }
    } else if(conn->isCommand ("sync_ha_sg")) {

      char *sync_ha_str;
      char *sync_dec_str;
      logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized sync_ha_sg" << sendLog;
      if(conn->paramNextStringNull (&sync_ha_str) || conn->paramNextStringNull (&sync_dec_str) || !conn->paramEnd ()) { 
	logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized sync_ha_sg paramNextString ra or dec failed" << sendLog;
	return -2;
      }
      if((ret= f_scansexa (sync_ha_str, &sync_ha))== -1) {
	logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized sync_ha_sg parsing ra failed" << sendLog;
	return -1;
      }
      sync_ha *= 15. ;
      if((ret= f_scansexa (sync_dec_str, &sync_dec))== -1) {
	logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized sync_ha_sg parsing dec failed" << sendLog;
	return -1;
      }
      logStream (MESSAGE_INFO) << "APGTO::commandAuthorized start sync_ha_sg " << sendLog;
      sync_ra= localSiderealTime() - sync_ha; // RA is a right, HA left system
    } 
    // do not sync while slewing
    // this can occur if a script tries to sync after e.g. an astrometric calibration
    //
    if(slew_state->getValueBool()) {
      logStream (MESSAGE_INFO) << "APGTO::commandAuthorized sync, do not sync while slewing" << sendLog;
      return -1 ;
    }
    if((ret= setTo(sync_ra, sync_dec)) !=0) {
      logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized setTo (sync) failed" << sendLog;
      return -1 ;
    }
    
    if(checkAPRelativeAngle()) {
      logStream (MESSAGE_WARNING) << "APGTO::commandAuthorized check of the declination axis failed." << sendLog;
      return -1;
    }
    logStream (MESSAGE_INFO) << "APGTO::commandAuthorized sync on ra " << sync_ra << " dec " << sync_dec << sendLog;
    return 0 ;
  }
  return Telescope::commandAuthorized (conn);
}

#define ERROR_IN_INFO 1
int APGTO::info ()
{
  int ret ;
  int error= -1 ;
  time_t now;
  
  // ToDo check which parts can be carried out
  // ToDo see what is done by lx200
  // do not check while slewing
  if((slew_start_time - time(&now) + TIMEOUT_SLEW_START) < 0.) {
    if(tracking->getValueBool()){
      if((abortAnyMotion () !=0)) {
	logStream (MESSAGE_WARNING) << "APGTO::info abortAnyMotion failed" << sendLog;
	return -1;
      } else {
	logStream (MESSAGE_INFO) << "APGTO::info stopped tracking due to TIMEOUT_SLEW_START" << sendLog;
      }
    }
  }  
  if(tel_read_ra () || tel_read_dec ()) {
    error = ERROR_IN_INFO ;
    logStream (MESSAGE_WARNING) << "APGTO::info could not retrieve ra, dec  " << sendLog;
  }
  if((ret= tel_read_local_time()) != 0) {
    error = ERROR_IN_INFO ;
    logStream (MESSAGE_WARNING) << "APGTO::info could not retrieve localtime  " << sendLog;
  }
  
  if((ret= getAPlocalSiderealTime()) != 0) {
    error = ERROR_IN_INFO ;
    logStream (MESSAGE_WARNING) << "APGTO::info could not retrieve sidereal time  " << sendLog;
  }
  if((ret= getAPRelativeAngle()) != 0) {
    error = ERROR_IN_INFO ;
    logStream (MESSAGE_WARNING) << "APGTO::info could not retrieve position of declination axis  " << sendLog;
  }
  
  if(!(strcmp("West", DECaxis_HAcoordinate->getValue()))) {
    telFlip->setValueInteger (1);
  } else if(!(strcmp("East", DECaxis_HAcoordinate->getValue()))) {
    telFlip->setValueInteger (0);
  } else {
    error = ERROR_IN_INFO ;
    logStream (MESSAGE_WARNING) << "APGTO::info could not retrieve relative angle (declination axis, hour axis)" << sendLog;
  }

  if(tel_read_azimuth () || tel_read_altitude ()) {
    error = ERROR_IN_INFO ;
    logStream (MESSAGE_WARNING) << "APGTO::info could not retrieve horizontal coordinates" << sendLog;
  }

  if(error== ERROR_IN_INFO) {
    return -1 ;
  }
  // check meridian transition while tracking
  struct ln_equ_posn object;
  struct ln_lnlat_posn observer;
  object.ra = fmod(getTelRa () + 360., 360.);
  object.dec= fmod(getTelDec (), 90.);
  observer.lng = telLongitude->getValueDouble ();
  observer.lat = telLatitude->getValueDouble ();

  double HA= fmod(localSiderealTime()- object.ra+ 360., 360.) ;
  if((HA < on_set_HA)&& (tracking->getValueBool())&& (slew_state->getValueBool()==false)){
    transition_while_tracking->setValueBool(true) ;
    logStream (MESSAGE_WARNING) << "APGTO::info transition while tracking occured" << sendLog;
  } else {
    // Do not reset here!
    // Do that if a new successful slew occured 
  }

  // while the telescope is tracking Astro-Physics controller does not
  // carry out any checks, meaning that it turns for ever
  // 
  // 0 <= HA <=180.: check if the mount approaches horizon
  // 180 < HA < 360: check if the mount crosses the meridian
  // Astro-Physics controller has the ability to delay the meridian flip.
  // Here I assume that the flip takes place on HA=0 on slew.
  // If the telescope does not collide, it may track as long as:
  // West:   HA < 15.
  // East:  Alt > 10.

  //check only while not slewing
  //ToDo:  was if((getState () & TEL_MOVING) || (getState () & TEL_PARKING))
  // check that if it works, otherwise use slew_state->getValueBool()==false
  //if(! (((getState () & TEL_MASK_MOVING) == TEL_MOVING) || ((getState () & TEL_MASK_MOVING) == TEL_PARKING)))
  // 2014-04-14, it did not work
  if(slew_state->getValueBool()==false) {
    int stop= 0 ;
    if(!(strcmp("West", DECaxis_HAcoordinate->getValue()))) {
      ret = pier_collision(&object, &observer) ;
      if(ret != NO_COLLISION) {
	stop= 1 ;
	if((tracking->getValueBool()) || ((slew_state->getValueBool()))) {
	  logStream (MESSAGE_WARNING) << "APGTO::info collision detected (West)" << sendLog ;
	}
      } else {
	if((HA >= 15.) && (HA < 180.)) { // 15. degrees are a matter of taste
	  if(tracking->getValueBool()) {
	    stop= 1 ;
	    logStream (MESSAGE_WARNING) << "APGTO::info t_equ ra " << getTelRa() << " dec " <<  getTelDec() << " is crossing the (meridian + 15 deg), stopping any motion" << sendLog;
	  }
	}
      }
    } else if(!(strcmp("East", DECaxis_HAcoordinate->getValue()))) {
      //really target_equ.dec += 180. ;
      struct ln_equ_posn t_equ;
      t_equ.ra = object.ra;
      t_equ.dec= object.dec + 180.;
      ret = pier_collision (&t_equ, &observer);
      if(ret != NO_COLLISION) {
	stop= 1 ;
	if((tracking->getValueBool()) || ((slew_state->getValueBool()))) {
	  logStream (MESSAGE_WARNING) << "APGTO::info collision detected (East)" << sendLog ;
	}
      } else {
	if(telAltAz->getAlt() < 10.) {
	  if(tracking->getValueBool()) {
	    stop= 1 ;
	    notMoveCupola() ;
	    logStream (MESSAGE_WARNING) << "APGTO::info t_equ ra " << getTelRa() << " dec " <<  getTelDec() << " is below 10 deg, stopping any motion" << sendLog;
	  }
	}
      }
    } 
    if(stop !=0) {
      // if a collision is detected it is necessary that the mount can be moved with rot e, w, n, s commands
      // set collision_detection to true if that occurs
      if(collision_detection->getValueBool()) {
	if((abortAnyMotion () !=0)) {
	  logStream (MESSAGE_WARNING) << "APGTO::info collision detected, setting tracking mode ZERO failed" << sendLog;
	  return -1;
	}
      } else {
	trackingMode->setValueInteger (0);
	if(setAPTrackingMode() < 0) {
	  logStream (MESSAGE_WARNING) << "APGTO::info setting tracking mode ZERO failed" << sendLog;
	  return -1;
	}
	logStream (MESSAGE_WARNING) << "APGTO::info stop tracking but not motion" << sendLog;
      }
    }
  }
  // There is a bug in the revision D Astro-Physics controller
  // find out, when the local sidereal time gets wrong, difference is 237 sec
  // Check if the sidereal time read from the mount is correct 
  double diff_loc_time = localSiderealTime() - APlocalSiderealTime->getValueDouble ();
  if(diff_loc_time >= 180.)
    diff_loc_time -=360. ;
  
  else if((diff_loc_time) <= -180.)
    diff_loc_time += 360. ;
  
  // TAG TIME UPDATE
  if(fabs(diff_loc_time) > (1./8.)) {
    // 30 time seconds
    logStream (MESSAGE_WARNING) << "APGTO::info  local sidereal time, calculated time " 
				<< localSiderealTime() << " mount: "
				<< APlocalSiderealTime->getValueDouble() 
				<< " difference " << diff_loc_time <<sendLog;

    logStream (MESSAGE_DEBUG) << "APGTO::info ra " << getTelRa() << " dec " << getTelDec() << " alt " <<   telAltAz->getAlt() << " az " << telAltAz->getAz()  <<sendLog;

    if(dateTime()<0)
      return -1;
    
    // read the coordinates times again
    if(tel_read_ra () || tel_read_dec ())
      return -1;
    if(tel_read_azimuth () || tel_read_altitude ())
      return -1;
    if((ret= tel_read_local_time()) != 0)
      return -1 ;
    if((ret= getAPlocalSiderealTime()) != 0) 
      return -1 ;
      
    diff_loc_time = localSiderealTime()-APlocalSiderealTime->getValueDouble() ;
      
    logStream (MESSAGE_DEBUG) << "APGTO::info ra " << getTelRa() << " dec " << getTelDec() << " alt " <<   telAltAz->getAlt() << " az " << telAltAz->getAz()  <<sendLog;
      
    logStream (MESSAGE_DEBUG) << "APGTO::info  local sidereal time, calculated time " 
			      << localSiderealTime() << " mount: "
			      << APlocalSiderealTime->getValueDouble() 
			      << " difference " << diff_loc_time <<sendLog;
  }     
  // The mount unexpectedly starts to track, stop that
  if(! tracking->getValueBool()) {

    HA= fmod(localSiderealTime()-getTelRa()+ 360.,360.) ;
    double diff=HA-on_zero_HA ;

    if(fabs(diff)>DIFFERENCE_MAX_WHILE_NOT_TRACKING) {
      logStream (MESSAGE_INFO) << "APGTO::info HA changed while mount is not tracking." << sendLog;
      notMoveCupola() ;
      trackingMode->setValueInteger (0);
      if(setAPTrackingMode() < 0)
      {
	logStream (MESSAGE_WARNING) << "APGTO::info setting tracking mode ZERO failed." << sendLog;
	return -1;
      }
    }
  }
  if((ret= getAPlocalSiderealTime()) != 0)
    return -1;
  
  APhourAngle->setValueDouble (APlocalSiderealTime->getValueDouble ()-getTelRa());
  return TelLX200::info ();
}
int
APGTO::idle ()
{
  // make sure that the checks in ::info are done
  info() ;
  return TelLX200::idle() ;
}

double APGTO::localSiderealTime()
{
  double JD  = ln_get_julian_from_sys ();  
  double lng = telLongitude->getValueDouble ();
  return fmod((ln_get_apparent_sidereal_time(JD) * 15. + lng + 360.), 360.);  // longitude positive to the East
}

int APGTO::checkSiderealTime(double limit) 
{
  int ret ;
  // Check if the sidereal time read from the mount is correct 
  double local_sidereal_time= localSiderealTime() ;
  if((ret= getAPlocalSiderealTime()) != 0) {
    logStream (MESSAGE_WARNING) << "APGTO::checkSiderealTime could not retrieve sidereal time  " << sendLog;
    return -1 ;
  }
  logStream (MESSAGE_DEBUG) << "APGTO::checkSiderealTime local sidereal time, calculated time " 
			    << local_sidereal_time << " mount: "
			    << APlocalSiderealTime->getValueDouble() 
			    << ", difference: " << local_sidereal_time- APlocalSiderealTime->getValueDouble()<<sendLog;
	
  if(fabs(local_sidereal_time- APlocalSiderealTime->getValueDouble()) <= limit)
    return 0 ;

  // usually 30 time seconds
  logStream (MESSAGE_WARNING) << "APGTO::checkSiderealTime AP sidereal time off by: " << local_sidereal_time- APlocalSiderealTime->getValueDouble() << sendLog;
  logStream (MESSAGE_WARNING) << "APGTO::checkSiderealTime  local sidereal time, calculated time: " 
			      << local_sidereal_time << ", mount: "
			      << APlocalSiderealTime->getValueDouble() 
			      << ", difference: " << local_sidereal_time- APlocalSiderealTime->getValueDouble()<<sendLog;
  return -1 ; 
}
/*!
 * Reads APGTO longitude.
 *
 * @return -1 on error, otherwise 0
 *
 */
int APGTO::getAPLongitude ()
{
  double new_longitude;
  if((tel_read_hms (&new_longitude, "#:Gg#"))< 0) {
    logStream (MESSAGE_WARNING) << "APGTO::tel_read_longitude failed" <<sendLog;
    return -1;
  }
  APlongitude->setValueDouble(new_longitude) ;
  logStream (MESSAGE_DEBUG) << "APGTO::tel_read_longitude " << new_longitude << "" << strlen("#:Gg#") <<sendLog;
  return 0;
}
/*!
 * Reads APGTO latitude.
 *
 * @return -1 on error, otherwise 0
 */
int APGTO::getAPLatitude ()
{
  double new_latitude;
  if(tel_read_hms (&new_latitude, "#:Gt#")) {
    logStream (MESSAGE_WARNING) <<"APGTO::tel_read_latitude failed" << sendLog;
    return -1;
  }
  APlatitude->setValueDouble(new_latitude) ;
  return 0;
}
int APGTO::checkLongitudeLatitude (double limit) {

  if((getAPLongitude() != 0) || (getAPLatitude() != 0)) {
      logStream (MESSAGE_WARNING) << "APGTO::checkLongitudeLatitude could not retrieve longitude or latitude  " << sendLog;
      return -1 ;
  }
  // AP mount counts positive to the west, RTS2 to east
  double lng= 360. -APlongitude->getValueDouble();
  if(fabs(config->getObserver ()->lng - lng)> limit){

    logStream (MESSAGE_INFO) << "APGTO::checkLongitude long off by c" << config->getObserver ()->lng << " t:"<< APlongitude->getValueDouble() << sendLog;
    return -1 ;
  }
  if(fabs(config->getObserver ()->lat- APlatitude->getValueDouble())> limit){
    logStream (MESSAGE_INFO) << "APGTO::checkLongitude lat off by " << config->getObserver ()->lng- telLatitude->getValueDouble() << sendLog;

      return -1 ;
  }
  return 0 ;
}
int APGTO::getAPlocalSiderealTime ()
{
  double new_sidereal_time;
  if(tel_read_hms (&new_sidereal_time, "#:GS#"))
    return -1;
  APlocalSiderealTime->setValueDouble (new_sidereal_time *15.);
  return 0;
}

int APGTO::dateTime()
{
  //struct ln_date utm;
  //struct ln_zonedate ltm;
  //ln_get_date_from_sys(&utm) ; // That's UTC, good
  //ln_date_to_zonedate (&utm, &ltm, -1 * timezone + 3600 * daylight);
  //2017-01-06T18:23:49.661 CET            utc time h: 17 m: 23 s: 49.661122
  //2017-01-06T18:23:49.661 CET  setting local time h: 19 m: 23 s: 49.661152 timezone (val):-3600 daylight: 1 timezone name: CET, CEST
  // 
  time_t urt=  time (NULL);
  tm utm;
  gmtime_r(&urt,&utm);
  time_t lrt=  time (NULL);
  tm ltm;
  localtime_r(&lrt,&ltm);
  char l_date_time[256];  
  sprintf(l_date_time, "%4d-%02d-%02dT%02d:%02d:%02d", ltm.tm_year+1900, ltm.tm_mon+1, ltm.tm_mday, ltm.tm_hour, ltm.tm_min, (int) ltm.tm_sec) ;
  logStream (MESSAGE_INFO) << "APGTO::dateTime  local" << l_date_time <<sendLog;

  char u_date_time[256];
  sprintf(u_date_time, "%4d-%02d-%02dT%02d:%02d:%02d", utm.tm_year+1900, utm.tm_mon+1, utm.tm_mday, utm.tm_hour, utm.tm_min, (int) utm.tm_sec) ;
  logStream (MESSAGE_INFO) << "APGTO::dateTime    utc" << u_date_time <<sendLog;
  
  // this is local time including dst
  if(setAPLocalTime(ltm.tm_hour, ltm.tm_min, (int) ltm.tm_sec) < 0) {
    logStream (MESSAGE_WARNING) << "APGTO::setBasicData setting local time failed" << sendLog;
    return -1;
  }
  if(setAPCalenderDate(ltm.tm_mday, ltm.tm_mon+1, ltm.tm_year+1900) < 0) {
    logStream (MESSAGE_WARNING) << "APGTO::setBasicData setting local date failed" << sendLog;
    return -1;
  }
  return 0;
}
int APGTO::setBasicData()
{
  // 600 slew speed, 0.25x guide, 12x centering
  // ToDo: see if 0.25 is a good value for guiding
  int ret = serConn->writePort (":RS0#:RG0#:RC0#", 15);
  if(ret < 0)
    return -1;
	
  //if the sidereal time read from the mount is correct then consider it as a warm start 
  if(checkSiderealTime(1./60.) == 0) {
    logStream (MESSAGE_INFO) << "APGTO::setBasicData performing warm start due to correct sidereal time" << sendLog;
    return 0 ;
  }

  logStream (MESSAGE_INFO) << "APGTO::setBasicData performing cold start due to incorrect sidereal time" << sendLog;

  if(setAPLongFormat() < 0) {
    logStream (MESSAGE_WARNING) << "APGTO::setBasicData setting long format failed" << sendLog;
    return -1;
  }
  if(setAPBackLashCompensation(0,0,0) < 0){
    logStream (MESSAGE_WARNING) << "APGTO::setBasicData setting back lash compensation failed" << sendLog;
    return -1;
  }
  if( dateTime() <0)
    return -1 ;  
  // AP mount counts positive and only to the west, RTS2 to east
  if(setAPLongitude() < 0) {  
    logStream (MESSAGE_WARNING) << "APGTO::setBasicData setting site coordinates failed" << sendLog;
    return -1;
  }
  if(setAPLatitude() < 0) {
    logStream (MESSAGE_WARNING) << "APGTO::setBasicData setting site coordinates failed" << sendLog;
    return -1;
  }
  // TAG TIME UTC OFFSET
  // CET APUTC offset:     22:56:06  -1:03:54, -1.065 (valid for Obs Vermes CET)
  // This value has been determied in the following way
  // longitude from google earth
  // set it in the AP controller
  // vary the value until the local sidereal time, obtained from the AP controller,
  // matches the externally calculated local sidereal time.
  // Understand what happens with AP controller sidereal time, azimut coordinates

  // Astro-Physics says:
  //You will be correct to enter your longitude as a positive value with the
  //command:
  //:Sg 352*30#
  //The correct command for the GMT offset (assuming European time zone 1
  //with daylight savings time in effect) would then be:
  //:SG -02#
  //You can easily test your initialization of the mount by polling the
  //mount for the sidereal time and comparing it to the sidereal time
  //calculated by a planetarium program for your location.   The two values
  ///should be within several (<30) seconds of each other.  I have verified
  //this with a control box containing a "D" chip compared to TheSky6.   You
  //should not need to do the verification every time you use the mount, but
  //it is a good idea after first writing your initialization sequence to
  //test it once. 
  //Mag. 7 skies!
  //Howard Hedlund
  //Astro-Physics, Inc.

  if(getAPVersionNumber() != 0)
    return -1;

  if(APfirmware->getValueString().compare("D") ==0){
    double off= -1.065;
    //time.h
    //The value of tm_isdst shall be positive if Daylight Savings Time is in effect,
    // 0 if Daylight Savings Time is not in effect,
    // and negative if the information is not available.
    // 2017-01-11: daylight=1, which is wrong
    time_t lrt=  time (NULL);
    tm ltm;
    localtime_r(&lrt,&ltm);
    //tm_isdst is greater than zero if DST is in effect,
    //         zero if DST is not in effect
    //         less than zero if the information is not available.
    // restart apgto, if DST changes
    if( ltm.tm_isdst < 0){
      logStream (MESSAGE_WARNING) << "APGTO::setBasicData DST information not available, ignoring"<< sendLog;
    } else if(ltm.tm_isdst > 1){ 
      logStream (MESSAGE_DEBUG) << "APGTO::setBasicData DST is in effect"<< sendLog;
      off= -2.065;
    }
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData This is a version D chip, seting UTC offset to:"<< off << sendLog;
    if((ret = setAPUTCOffset(off)) < 0) {
      logStream (MESSAGE_WARNING) << "APGTO::setBasicData setting AP UTC offset failed" << sendLog;
      return -1;
    }
  }
  if(setAPUnPark() < 0)	{
    logStream (MESSAGE_WARNING) << "APGTO::setBasicData unparking failed" << sendLog;
    return -1;
  }
  logStream (MESSAGE_DEBUG) << "APGTO::setBasicData unparking (cold start) successful" << sendLog;
  
  return 0 ;
}

/*!
 * Init mount
 *
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int APGTO::initHardware ()
{
  on_set_HA= 0.;
  time(&slew_start_time) ;
  // Controller Revision D is incompatible with an ordinary LX200 controller
  // see "TAG TIME UTC OFFSET" and "TAG TIME UPDATE"
  // status = TelLX200::initHardware ();
  serConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 5, 5);
  if(connDebug == true)
    serConn->setDebug (true);
  if(serConn->init ())
    return -1;
  serConn->flushPortIO ();

  logStream (MESSAGE_DEBUG) << "APGTO::init RS 232 initialization complete" << sendLog;
  return 0;
}

int APGTO::initValues ()
{
  int ret = -1;
  int flip= -1 ;

  config = rts2core::Configuration::instance ();
  if(config->loadFile ())
    return -1;
  // read from configuration, will not be overwritten (e.g. by getAPLongitude, latitude)
  setTelLongLat (config->getObserver ()->lng, config->getObserver ()->lat);
  setTelAltitude (config->getObservatoryAltitude ());

  if((ret= setBasicData()) != 0)
    return -1 ;
  sleep(10); // ToDo schroetig aber noetig
  if(getAPVersionNumber() != 0)
    return -1;
  if(getAPUTCOffset() != 0)
    return -1 ;
  if((ret= tel_read_local_time()) != 0)
    return -1 ;
  // Check if longitude and latitude is correct
  if(checkLongitudeLatitude (1./120.) != 0) {
      logStream (MESSAGE_WARNING) << "initValues longitude, latitude difference larger than 1./120, exiting" << sendLog;
      exit (1) ; // do not go beyond, at least for the moment
  }
  // Check if the sidereal time read from the mount is correct 
  if(checkSiderealTime (1./120.) != 0) {
    logStream (MESSAGE_WARNING) << "initValues sidereal time difference larger than 1./120, exiting" << sendLog;
    exit (1) ; // do not go beyond, at least for the moment
  }

  //Do not track at the beginning
  trackingMode->setValueInteger(0); 
  if((ret= setAPTrackingMode()) < 0) { 
    logStream (MESSAGE_WARNING) << "APGTO::initValues setting tracking mode:"  << trackingMode->getData() << " failed." << sendLog;
    return -1;
  }
  if((ret= getAPRelativeAngle()) != 0)
    return -1 ;

  // West: HA + Pi/2 = direction where the declination axis points
  // East: HA - Pi/2
  if(!(strcmp("West", DECaxis_HAcoordinate->getValue()))) {
    flip = 1 ;
  } else if(!(strcmp("East", DECaxis_HAcoordinate->getValue()))) {
    flip= 0 ;
  } else {
    logStream (MESSAGE_ERROR) << "APGTO::initValues could not retrieve relative angle (declination axis, hour axis), exiting" << sendLog;
    exit(1) ;
  }
  telFlip->setValueInteger (flip);

  if(tel_read_ra () || tel_read_dec ())
    return -1;

  // check if the assumption "mount correctly parked" holds true
  // if not block any sync and slew operations.
  //
  // This is only a formal test based on the idea that the
  // mount has a park position and was parked by RTS2. If the
  // mount is not at this position assume that position is
  // undefined or incorrect. A visual check is required.
  if(assume_parked->getValueBool()) {

    struct ln_equ_posn object;
    struct ln_equ_posn park_position;
    object.ra = getTelRa();
    object.dec= fmod(getTelDec(), 90.) ;

    park_position.ra = fmod(PARK_POSITION_RA + localSiderealTime() + 360., 360.);
    park_position.dec= PARK_POSITION_DEC  ;

    double diff_ra = fabs(fmod((park_position.ra - object.ra) + 360., 360.));
    
    if(diff_ra >= 180.) 
      diff_ra -=360. ;
    else if(diff_ra <= -180.) 
      diff_ra += 360. ;

    double diff_dec= fabs((park_position.dec- object.dec));
    if((diff_ra < PARK_POSITION_DIFFERENCE_MAX) && (diff_dec < PARK_POSITION_DIFFERENCE_MAX)) {
      if(!(strcmp(PARK_POSITION_ANGLE, DECaxis_HAcoordinate->getValue()))) {
      } else {
	logStream (MESSAGE_WARNING) << "APGTO::initValues block any sync and slew opertion due to wrong initial position, RA:"<< object.ra << " dec: "<< object.dec << " diff ra: "<< diff_ra << "diff dec: " << diff_dec << sendLog;
	block_sync_apgto->setValueBool(true) ;
	block_move_apgto->setValueBool(true) ;
	logStream (MESSAGE_WARNING) << "APGTO::initValues blocked any sync and slew opertion" << sendLog;
      }
    }
  }  

  logStream (MESSAGE_DEBUG) << "APGTO::initValues entering TelLX200::initValues ():"   << sendLog;
  return TelLX200::initValues ();
}

APGTO::~APGTO (void)
{
}

void APGTO::postEvent (rts2core::Event *event)
{
  Telescope::postEvent (event);
}

int APGTO::processOption (int in_opt)
{
  switch (in_opt){
  case 'f':
    device_file = optarg;
    break;

  case OPT_APGTO_ASSUME_PARKED:
    assume_parked->setValueBool(true);
    break;
  default:
    return TelLX200::processOption (in_opt);
  }
  return 0;
}

APGTO::APGTO (int in_argc, char **in_argv):TelLX200 (in_argc,in_argv)
{	
	createValue (DECaxis_HAcoordinate, "DECXHA", "DEC axis HA coordinate, West/East",true);

	createValue (trackingMode, "TRACK", "tracking mode", true, RTS2_VALUE_WRITABLE);
	// http://astro-physics.com/tech_support/mounts/command_lang.htm
	// no RT9 but in
	// http://www.baader-planetarium.de/astro_physics/downloads/keypad_v412.pdf
	// RT9, p.64 :
	// Zero tracking rate (stops all tracking)
	trackingMode->addSelVal ("NONE", (rts2core::Rts2SelData *) ":RT9#");
	trackingMode->addSelVal ("SIDEREAL", (rts2core::Rts2SelData *) ":RT2#");
	trackingMode->addSelVal ("LUNAR", (rts2core::Rts2SelData *) ":RT0#");
	trackingMode->addSelVal ("SOLAR", (rts2core::Rts2SelData *) ":RT1#");

	createValue (APslew_rate, "slew_rate", "AP slew rate (1200, 900, 600)", false, RTS2_VALUE_WRITABLE);
	APslew_rate->addSelVal ("600");
	APslew_rate->addSelVal ("900");
	APslew_rate->addSelVal ("1200");
       
	createValue (APcenter_rate, "center_rate", "AP center rate (1200,600, 64, 12, 1)", false, RTS2_VALUE_WRITABLE);
	APcenter_rate->addSelVal ("12");
	APcenter_rate->addSelVal ("64");
	APcenter_rate->addSelVal ("600");
	APcenter_rate->addSelVal ("1200");

	createValue (APguide_rate, "guide_rate", "AP guide rate (0.25, 0.5, 1x sidereal", false, RTS2_VALUE_WRITABLE);
	APguide_rate->addSelVal ("0.25");
	APguide_rate->addSelVal ("0.5");
	APguide_rate->addSelVal ("1.0");

	createRaPAN ();
	createDecPAN ();
	
	createValue (collision_detection, "COLLILSION_DETECTION", "true: mount stop if it collides", true, RTS2_VALUE_WRITABLE);
	collision_detection->setValueBool (true) ;

	addOption (OPT_APGTO_ASSUME_PARKED, "parked", 0, "assume a regularly parked mount");

        addOption (OPT_APGTO_TIMEOUT_SLEW_START, "tracking timeout",  1, "if no new slew occurs within tracking timeout, tracking is stopped");
	// 
        createValue (block_sync_apgto,         "BLOCK_SYNC_APGTO", "true inhibits any sync", false, RTS2_VALUE_WRITABLE);
        createValue (block_move_apgto,         "BLOCK_MOVE_APGTO", "true inhibits any slew", false, RTS2_VALUE_WRITABLE);
        createValue (slew_state,               "SLEW",        "true: mount is slewing",      false);
        createValue (tracking,                 "TRACKING",    "true: mount is tracking",     false);
        createValue (transition_while_tracking,"TRANSITION",  "transition while tracking",   false);
	// AP mount counts positive to the west, RTS2 to east
	// in order to sperate these values:
        createValue (APhourAngle,              "APHOURANGLE", "AP mount hour angle",         true,  RTS2_DT_RA);
        createValue (APlocalSiderealTime,      "APLOCSIDTME", "AP mount local sidereal time",true,  RTS2_DT_RA);
        createValue (APlongitude,              "APLONGITUDE", "AP mount longitude",          true,  RTS2_DT_DEGREES);
        createValue (APlatitude,               "APLATITUDE",  "AP mount latitude",           true,  RTS2_DT_DEGREES);
	createValue (APutc_offset, "APUTCOFFSET", "AP mount UTC offset", true,  RTS2_DT_RA);
	createValue (APfirmware, "APVERSION", "AP mount firmware revision", true);
	createValue (assume_parked, "ASSUME_PARKED", "true check initial mount position",    false);
	assume_parked->setValueBool (false);
        createValue (APTime,                   "APTIME",      "AP mount time see LOCATIME!", true,  RTS2_DT_RA);
}

int main (int argc, char **argv)
{
  	APGTO device = APGTO (argc, argv);
  	return device.run ();
}

