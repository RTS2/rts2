#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/* 
 * Astro-Physics GTO mount daemon 
 * Copyright (C) 2009, Markus Wildi, Petr Kubanek <petr@kubanek.net> and INDI
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

/*!
 * @file Driver for Astro-Physics GTO protocol based telescope. Some functions are borrowed from INDI to make the transition smoother
 * 
 * @author john,petr, Markus Wildi
 */

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>
#include <libnova/libnova.h>
#include <sys/ioctl.h>

#include "teld.h"
#include "hms.h"
#include "status.h"
#include "../utils/rts2config.h" 
#include "clicupola.h"
#include "pier-collision.h"

#include <termios.h>
// uncomment following line, if you want all apgto_fd read logging (will
// at about 10 30-bytes lines to syslog for every query).
//#define DEBUG_ALL_PORT_COMM

#define OPT_APGTO_INIT		OPT_LOCAL + 53

#define SLEW_RATE_1200 '2'
#define SLEW_RATE_0900 '1'
#define SLEW_RATE_0600 '0'

#define MOVE_RATE_120000 '0'
#define MOVE_RATE_060000 '1'
#define MOVE_RATE_006400 '2'
#define MOVE_RATE_001200 '3'
#define MOVE_RATE_000100 '4'
#define MOVE_RATE_000050 '5'
#define MOVE_RATE_000025 '6'

#define DIR_NORTH 'n'
#define DIR_EAST  'e'
#define DIR_SOUTH 's'
#define DIR_WEST  'w'

#define setAPPark()         write(apgto_fd, "#:KA#", 5)
#define setAPUnPark()       write(apgto_fd, "#:PO#", 5)// ok, no response
#define setAPLongFormat()   write(apgto_fd, "#:U#", 4) // ok, no response
#define setAPClearBuffer()  write(apgto_fd, "#", 1)    // clear buffer
#define setAPMotionStop()   write(apgto_fd, "#:Q#", 4) // ok, no response


namespace rts2teld
{
  class APGTO:public Telescope {
  private:
    const char *device_file;
    char initialization[256];
    int apgto_fd;
    
    double lastMoveRa, lastMoveDec;  
    enum { NOTMOVE, MOVE_REAL } move_state;
    time_t move_timeout;

    int f_scansexa (const char *str0, double *dp);
    void getSexComponents(double value, int *d, int *m, int *s) ;

    // low-level functions (RTS2)
    int tel_read (char *buf, int count);
    int tel_read_hash (char *buf, int count);
    int tel_write (const char *buf, int count);
    int tel_write_read (const char *wbuf, int wcount, char *rbuf, int rcount);
    int tel_write_read_hash (const char *wbuf, int wcount, char *rbuf, int rcount);
    int tel_read_hms (double *hmsptr, const char *command);

    // Astro-Physics LX200 protocol specific functions
    int getAPVersionNumber() ;
    int getAPUTCOffset() ;
    int setAPObjectAZ( double az) ;
    int setAPObjectAlt( double alt) ;
    int setAPUTCOffset( double hours) ;
    int APSyncCMR( char *matchedObject) ;
    int selectAPMoveToRate( int moveToRate) ;
    int selectAPTrackingMode( int trackMode) ;
    int tel_read_declination_axis() ;
    int tel_check_declination_axis() ;
    int setAPSiteLongitude( double Long) ;
    int setAPSiteLatitude( double Lat) ;
    int setAPLocalTime(int x, int y, int z) ;
    int setAPBackLashCompensation( int x, int y, int z) ;
    int setCalenderDate( int dd, int mm, int yy) ;
    int tel_set_slew_rate (char new_rate);
    int tel_set_move_rate (char new_rate);
    // helper
    int setBasicData();
    void ParkDisconnect() ;
    // regular LX200 protocol (RTS2)
    int tel_read_local_time ();
    int tel_read_sidereal_time ();
    int tel_read_ra ();
    int tel_read_dec ();
    int tel_read_altitude ();
    int tel_read_azimuth ();
    int tel_read_latitude ();
    int tel_read_longitude ();
    int tel_rep_write (char *command);
    // helper 
    void tel_normalize (double *ra, double *dec);
    int tel_write_ra (double ra);
    int tel_write_dec (double dec);
    int tel_start_move (char direction);
    int tel_stop_move (char direction);
    int tel_slew_to (double ra, double dec);
    void set_move_timeout (time_t plus_time);
    // further discussion with Petr required
    //int changeMasterState (int new_state);
    // Astro-Physics properties
    Rts2ValueAltAz   *APAltAz ;
    Rts2ValueInteger *APslew_rate;
    Rts2ValueInteger *APmove_rate;
    Rts2ValueDouble  *APlocal_sidereal_time;
    Rts2ValueDouble  *APlocal_time;
    Rts2ValueDouble  *APutc_offset;
    Rts2ValueDouble  *APlongitude;
    Rts2ValueDouble  *APlatitude;
    Rts2ValueString  *APfirmware ;
    Rts2ValueString  *DECaxis_HAcoordinate ; // see pier_collision.c 

  public:
    APGTO (int argc, char **argv);
    virtual ~APGTO (void);
    virtual int processOption (int in_opt);
    virtual int init ();
    virtual int initValues ();
    virtual int info ();

    virtual int setTo (double set_ra, double set_dec);
    virtual int correct (double cor_ra, double cor_dec, double real_ra, double real_dec);

    virtual int startResync ();
    virtual int isMoving ();
    virtual int stopMove ();
  
    virtual int startPark ();
    virtual int isParking ();
    virtual int endPark ();
  
    virtual int startDir (char *dir);
    virtual int stopDir (char *dir);
    virtual int commandAuthorized (Rts2Conn * conn);
    virtual void valueChanged (Rts2Value * changed_value) ;
  };

};

using namespace rts2teld;

void
APGTO::getSexComponents(double value, int *d, int *m, int *s)
{

  *d = (int) fabs(value);
  *m = (int) ((fabs(value) - *d) * 60.0);
  *s = (int) rint(((fabs(value) - *d) * 60.0 - *m) *60.0);

  if (value < 0)
    *d *= -1;
}
int
APGTO::f_scansexa ( const char *str0, double *dp) /* input string, cracked value, if return 0 */
{
  double a = 0, b = 0, c = 0;
  char str[128];
  char *neg;
  int r;

  /* copy str0 so we can play with it */
  strncpy (str, str0, sizeof(str)-1);
  str[sizeof(str)-1] = '\0';

  neg = strchr(str, '-');
  if (neg)
    *neg = ' ';
  r = sscanf (str, "%lf%*[^0-9]%lf%*[^0-9]%lf", &a, &b, &c);
  if (r < 1)
    return (-1);
  *dp = a + b/60 + c/3600;
  if (neg)
    *dp *= -1;
  return (0);
}
/*!
 * Reads some data directly from apgto_fd.
 *
 * Log all flow as LOG_DEBUG to syslog
 *
 * @exception EIO when there aren't data from apgto_fd
 *
 * @param buf 		buffer to read in data
 * @param count 	how much data will be read
 *
 * @return -1 on failure, otherwise number of read data
 */
int
APGTO::tel_read (char *buf, int count)
{
  int n_read; // compiler complains if read is used as index

  for (n_read = 0; n_read < count; n_read++) {
    int ret = read (apgto_fd, &buf[n_read], 1);
    if (ret == 0) {
      ret = -1;
    }
    if (ret < 0) {
      logStream (MESSAGE_ERROR) << "APGTO::tel_read apgto_fd read error "
				<< errno << sendLog;
      return -1;
    }
#ifdef DEBUG_ALL_PORT_COMM
    logStream (MESSAGE_DEBUG) << "APGTO::tel_read read >" << buf[n_read] << "<END" <<
      sendLog;
#endif
  }
  return n_read;
}
/*!
 * Will read from apgto_fd till it encoutered # character.
 *
 * Read ending #, but doesn't return it.
 *
 * @see tel_read() for description
 */
int
APGTO::tel_read_hash (char *buf, int count)
{
  int read;
  buf[0] = 0;

  for (read = 0; read < count; read++) {
    if (tel_read (&buf[read], 1) < 0)
      return -1;
    if (buf[read] == '#')
      break;
  }
  if (buf[read] == '#') {
    buf[read] = 0;
  } else {
    buf[read] = 0;
    logStream (MESSAGE_ERROR) << "APGTO::tel_read_hash NO hash read>" << buf << "<END" << sendLog;
  }
  return read;
}
/*!
 * Will write on telescope apgto_fd string.
 *
 * @exception EIO, .. common write exceptions
 *
 * @param buf 		buffer to write
 * @param count 	count to write
 *
 * @return -1 on failure, count otherwise
 */
int
APGTO::tel_write (const char *buf, int count)
{
  return write (apgto_fd, buf, count);
}
/*!
 * Combine write && read together.
 *
 * Flush apgto_fd to clear any gargabe.
 *
 * @exception EINVAL and other exceptions
 *
 * @param wbuf		buffer to write on apgto_fd
 * @param wcount	write count
 * @param rbuf		buffer to read from apgto_fd
 * @param rcount	maximal number of characters to read
 *
 * @return -1 and set errno on failure, rcount otherwise
 */
int
APGTO::tel_write_read (const char *wbuf, int wcount, char *rbuf, int rcount)
{
  int tmp_rcount;
  char *buf;

  if (tcflush (apgto_fd, TCIOFLUSH) < 0)
    return -1;
  if (tel_write (wbuf, wcount) < 0)
    return -1;

  tmp_rcount = tel_read (rbuf, rcount);
  if (tmp_rcount > 0) {
    buf = (char *) malloc (rcount + 1);
    memcpy (buf, rbuf, rcount);
    buf[rcount] = 0;
    logStream (MESSAGE_DEBUG) << "APGTO::tel_write_read read " <<
      tmp_rcount << " byte(s),  buffer >" << buf << "<END" << sendLog;
    free (buf);
  } else {
    logStream (MESSAGE_DEBUG) << "APGTO::tel_write_read read returns " <<
      tmp_rcount << sendLog;
  }
  return tmp_rcount;
}
/*!
 * Combine write && read_hash together.
 *
 * @see tel_write_read for definition
 */
int
APGTO::tel_write_read_hash (const char *wbuf, int wcount, char *rbuf, int rcount)
{
  int tmp_rcount;

  if (tcflush (apgto_fd, TCIOFLUSH) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::tel_write_read_hash flushing failed"<< sendLog;
    return -1;
  }
  if (tel_write (wbuf, wcount) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::tel_write_read_hash tel_write failed"<< sendLog;
    return -1;
  }
  if((tmp_rcount = tel_read_hash (rbuf, rcount)) < 0) {

    logStream (MESSAGE_ERROR) << "APGTO::tel_write_read_hash tel_read failed"<< sendLog;
    return -1;
  }
  return tmp_rcount;
}
/*!
 * Repeat APGTO write.
 *
 * Handy for setting ra and dec.
 * Meade tends to have problems with that, don't know about APGTO.
 *
 * @param command	command to write on apgto_fd
 */
int
APGTO::tel_rep_write (char *command)
{
  int count;
  char retstr;
  for (count = 0; count < 200; count++) {
    if (tel_write_read (command, strlen (command), &retstr, 1) < 0)
      return -1;
    if (retstr == '1')
      break;
    sleep (1);
    logStream (MESSAGE_DEBUG) << "APGTO::tel_rep_write - for " << count << " time" << sendLog;
  }
  if (count == 200) {
    logStream (MESSAGE_ERROR) << "APGTO::tel_rep_write unsucessful due to incorrect return." << sendLog;
    return -1;
  }
  return 0;
}
/*!
 * Reads some value from APGTO in hms format.
 *
 * Utility function for all those read_ra and other.
 *
 * @param hmsptr	where hms will be stored
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
APGTO::tel_read_hms (double *hmsptr, const char *command)
{
  char wbuf[256];
  if (tel_write_read_hash (command, strlen (command), wbuf, 11) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::tel_read_hms error tel_write_read_hash >"<< command << "<END "<< ", length "<< strlen (command)<< " failed" << sendLog;
    return -1;
  }
  *hmsptr = hmstod (wbuf);
  if (errno) {
    logStream (MESSAGE_ERROR) << "APGTO::tel_read_hms  hmstod Error :"<< errno << "END"<<sendLog;
    return -1;
  }
  return 0;
}
/*!
 * Reads from APGTO the version character of the firmware
 *
 * @param none
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
APGTO::getAPVersionNumber()
{
  int ret= -1 ;

  char version[32] ;
  if (( ret= tel_write_read_hash ( "#:V#", 4, version, 2))<1) {
    return -1 ;
  }
  APfirmware->setValueString (version);
  return 0 ;
}
int 
/*!
 * Reads from APGTO the UTC offset and corrects the output according to Astro-Physics documentation firmware revision D
 *
 *
 * @param none
 *
 * @return -1 and set errno on error, otherwise 0
 */
APGTO::getAPUTCOffset()
{
  int ret= -1 ;
  int nbytes_read=0;
  double offset ;

  char temp_string[16];

  if (( ret= tel_write_read_hash ( "#:GG#", 5, temp_string, 11))<1) { // HH:MM:SS.S# if long format
    logStream (MESSAGE_ERROR) <<"APGTO::getAPUTCOffset error " << nbytes_read << " bytes read" << sendLog;
    return -1 ;
  }
  // Negative offsets, see AP keypad manual p. 77
  if((temp_string[0]== 'A') || ((temp_string[0]== '0')&&(temp_string[1]== '0')) ||(temp_string[0]== '@')) {
    int i ;
    for( i=nbytes_read; i > 0; i--) {
      temp_string[i]= temp_string[i-1] ; 
    }
    temp_string[0] = '-' ;
    temp_string[nbytes_read + 1] = '\0' ;

    logStream (MESSAGE_DEBUG) << "APGTO::getAPUTCOffset string >" << temp_string << "<END"<< sendLog;

    if( temp_string[1]== 'A')  {
      temp_string[1]= '0' ;
	    switch (temp_string[2]) {
	    case '5':
		    
	      temp_string[2]= '1' ;
	      break ;
	    case '4':
		    
	      temp_string[2]= '2' ;
	      break ;
	    case '3':
		    
	      temp_string[2]= '3' ;
	      break ;
	    case '2':
		    
	      temp_string[2]= '4' ;
	      break ;
	    case '1':
		    
	      temp_string[2]= '5' ;
	      break ;
	    default:
	      logStream (MESSAGE_ERROR) << "APGTO::getAPUTCOffset string not handled >" << temp_string << "<END" << sendLog;
	      return -1 ;
		  break ;
	    }
    } else if( temp_string[1]== '0') {
      temp_string[1]= '0' ;
      temp_string[2]= '6' ;
    } else if( temp_string[1]== '@') {
      temp_string[1]= '0' ;
      switch (temp_string[2]) {
      case '9':
		    
	temp_string[2]= '7' ;
	break ;
      case '8':
		    
	temp_string[2]= '8' ;
	break ;
      case '7':
		    
	temp_string[2]= '9' ;
	break ;
      case '6':
		    
	temp_string[2]= '0' ;
	break ;
      case '5':
	temp_string[1]= '1' ;
	temp_string[2]= '1' ;
	break ;
      case '4':
		    
	temp_string[1]= '1' ;
	temp_string[2]= '2' ;
	break ;
      default:
	logStream (MESSAGE_DEBUG) <<"APGTO::getAPUTCOffset string not handled >" << temp_string << "<END" << sendLog;
	return -1 ;
	break;
      }    
    } else {
      logStream (MESSAGE_ERROR) <<"APGTO::getAPUTCOffset string not handled >" << temp_string << "<END" <<sendLog;
    }
  } else {
    temp_string[nbytes_read - 1] = '\0' ;
  }
  if (f_scansexa(temp_string, &offset)) {
    logStream (MESSAGE_ERROR) <<"APGTO::getAPUTCOffset string not handled  >" << temp_string << "<END" <<sendLog;
    return -1;
  }
  APutc_offset->setValueDouble( offset * 15. ) ;
  logStream (MESSAGE_DEBUG) <<"APGTO::getAPUTCOffset received string >" << temp_string << "<END offset is " <<  APutc_offset->getValueDouble() <<sendLog;
  return 0;
}
/*!
 * Writes to APGTO Az
 *
 *
 * @param az
 *
 * @return -1 on failure, count otherwise
 */
// wildi: not yet in in use
int 
APGTO::setAPObjectAZ(double az)
{
  int h, m, s;
  char temp_string[16];

  getSexComponents(az, &h, &m, &s);

  snprintf(temp_string, sizeof( temp_string ), "#:Sz %03d*%02d:%02d#", h, m, s);
  logStream (MESSAGE_DEBUG) <<"APGTO::setAPObjectAZ set object AZ string >" << temp_string << "<END" << sendLog;

  return (tel_write( temp_string, sizeof( temp_string )));
}
/*!
 * Writes to APGTO Alt
 *
 *
 * @param alt
 *
 * @return -1 on failure, count otherwise
 */
// wildi: not yet in in use
int 
APGTO::setAPObjectAlt(double alt)
{
  int d, m, s;
  char temp_string[16];
    
  getSexComponents(alt, &d, &m, &s);
  /* case with negative zero */
  if (!d && alt < 0) {
    snprintf(temp_string, sizeof( temp_string ), "#:Sa -%02d*%02d:%02d#", d, m, s) ;
  } else {
    snprintf(temp_string, sizeof( temp_string ), "#:Sa %+02d*%02d:%02d#", d, m, s) ;
  }	
  logStream (MESSAGE_DEBUG) <<"APGTO::setAPObjectAlt set object Alt string >" << temp_string << "<END" << sendLog;
  return (tel_write( temp_string, sizeof( temp_string )));
}
/*!
 * Writes to APGTO UTC offset
 *
 *
 * @param hours
 *
 * @return -1 on failure, 0 otherwise
 */
int 
APGTO::setAPUTCOffset(double hours)
{
  int ret= -1 ;
  int h, m, s ;
  char temp_string[16];
  char retstr[1] ;
  // To avoid the peculiar output format of AP controller, see p. 77 key pad manual
  if( hours < 0.) {
    hours += 24. ;
  }
  getSexComponents(hours, &h, &m, &s);
    
  snprintf(temp_string, sizeof( temp_string ), "#:SG %+03d:%02d:%02d#", h, m, s);
  logStream (MESSAGE_DEBUG) <<"APGTO::setAPUTCOffset >" << temp_string << "<END"<< sendLog ;

  if(( ret= tel_write( temp_string, sizeof( temp_string ))) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::setAPUTCOffset writing failed." << sendLog;
    return -1 ;
  }
  if(( ret= tel_read( retstr, 1)) != 1) {
    logStream (MESSAGE_ERROR) << "APGTO::setAPUTCOffset reading failed." << sendLog;
    return -1 ;
  }
  return 0;
}
/*!
 * Writes to APGTO sync #:CMR
 *
 *
 * @param *matchedObject 
 *
 * @return -1 on failure, 0 otherwise
 */
// wildi: not yet in in use
int 
APGTO::APSyncCMR(char *matchedObject)
{
  int error_type;
  int nbytes_read=0;
    
  if ( (error_type = tel_write( "#:CMR#", 6)) < 0)
    return error_type;
 
  nbytes_read= tel_read_hash (matchedObject, 33) ; //response length is 32 character plus the “#”.
  if (nbytes_read > 1) { //sloppy
    return 0;
  } else {
    return -1;
  }
}
/*!
 * Writes to APGTO the tracking mode
 *
 *
 * @param trackMode
 *
 * @return -1 on failure, 0 otherwise
 */
int 
APGTO::selectAPTrackingMode(int trackMode)
{
  int error_type;

  switch (trackMode) {
    /* Lunar */
  case 0:
    logStream (MESSAGE_DEBUG) <<"APGTO::selectAPTrackingMode setting tracking mode to lunar."<< sendLog;
    if ( (error_type = tel_write( "#:RT0#", 6)) < 0)
      return error_type;
    break;
    
    /* Solar */
  case 1:
    logStream (MESSAGE_DEBUG) <<"APGTO::selectAPTrackingMode setting tracking mode to solar."<< sendLog;
    if ( (error_type = tel_write( "#:RT1#", 6)) < 0)
      return error_type;
    break;

    /* Sidereal */
  case 2:
    logStream (MESSAGE_DEBUG) <<"APGTO::selectAPTrackingMode setting tracking mode to sidereal."<< sendLog;
    if ( (error_type = tel_write( "#:RT2#", 6)) < 0)
      return error_type;
    break;

    /* Zero */
  case 3:
    logStream (MESSAGE_DEBUG) <<"APGTO::selectAPTrackingMode setting tracking mode to zero."<< sendLog;
    if ( (error_type = tel_write( "#:RT9#", 6)) < 0)
      return error_type;
    break;
 
  default:
    return -1;
    break;
  }
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
int 
APGTO::setAPSiteLongitude(double Long)
{
  int ret= -1 ;
  int d, m, s;
  char temp_string[32];
  char retstr[1];

  getSexComponents(Long, &d, &m, &s);
  snprintf(temp_string, sizeof( temp_string ), "#:Sg %03d*%02d:%02d#", d, m, s);
  if(( ret= tel_write( temp_string, sizeof( temp_string ))) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::setAPSiteLongitude writing failed." << sendLog;
    return -1 ;
  }
  if(( ret= tel_read( retstr, 1)) != 1) {
    logStream (MESSAGE_ERROR) << "APGTO::setAPSiteLongitude reading failed." << sendLog;
    return -1 ;
  }
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
int 
APGTO::setAPSiteLatitude(double Lat)
{
  int ret= -1 ;
  int d, m, s;
  char temp_string[32];
  char retstr[1];

  getSexComponents(Lat, &d, &m, &s);
  snprintf(temp_string, sizeof( temp_string ), "#:St %+03d*%02d:%02d#", d, m, s);
  if(( ret= tel_write( temp_string, sizeof( temp_string ))) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::setAPSiteLatitude writing failed." << sendLog;
    return -1 ;
  }
  if(( ret= tel_read( retstr, 1)) != 1) {
    logStream (MESSAGE_ERROR) << "APGTO::setAPSiteLatitude reading failed." << sendLog;
    return -1 ;
  }
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
int
APGTO::setAPBackLashCompensation( int x, int y, int z)
{
  int ret= -1 ;
  char temp_string[16];
  char retstr[1];
  
  snprintf(temp_string, sizeof( temp_string ), "%s %02d:%02d:%02d#", "#:Br", x, y, z);
  
  if(( ret= tel_write( temp_string, sizeof( temp_string ))) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::setAPBackLashCompensation writing failed." << sendLog;
    return -1 ;
  }
  if(( ret= tel_read( retstr, 1)) != 1) {
    logStream (MESSAGE_ERROR) << "APGTO::setAPBackLashCompensation reading failed." << sendLog;
    return -1 ;
  }
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

int 
APGTO::setAPLocalTime(int x, int y, int z)
{
  int ret= -1 ;
  char temp_string[16];
  char retstr[1];
  
  snprintf(temp_string, sizeof( temp_string ), "%s %02d:%02d:%02d#", "#:SL" , x, y, z);
  if(( ret= tel_write( temp_string, sizeof( temp_string ))) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::setAPLocalTime writing failed." << sendLog;
    return -1 ;
  }
  if(( ret= tel_read( retstr, 1)) != 1) {
    logStream (MESSAGE_ERROR) << "APGTO::setAPLocalTime reading failed." << sendLog;
    return -1 ;
  }
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
int 
APGTO::setCalenderDate(int dd, int mm, int yy)
{
  char cmd_string[32];
  char temp_string[256];
  int ret;

  //Command: :SC MM/DD/YY#
  //Response: 32 spaces followed by “#”, followed by 32 spaces, followed by “#”
  //Sets the current date. Note that year fields equal to or larger than 97 are assumed to be 20 century, Year fields less than 97 are assumed to be 21st century.

  yy = yy % 100;
  snprintf(cmd_string, sizeof( cmd_string ), "#:SC %02d/%02d/%02d#", mm, dd, yy);

  if (( ret= tel_write_read_hash ( cmd_string, 14, temp_string, 33))<1) { // HH:MM:SS.S# if long format
    logStream (MESSAGE_ERROR) <<"APGTO::setCalenderDate inadequate answer from mount"<< sendLog;
    return -1 ;
  }
  // wildi ToDo: with out the rest fails !?!
  logStream (MESSAGE_DEBUG) <<"APGTO::setCalenderDate answer from mount >"<< temp_string<< "<END, length "<< strlen(temp_string)<< sendLog;
  if (( ret= tel_read_hash ( temp_string, 33))<1) {
    logStream (MESSAGE_ERROR) <<"APGTO::setCalenderDate inadequate answer from mount"<< sendLog;
    return -1 ;
  }
  return 0;
}
/*!
 * Reads APGTO right ascenation.
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
APGTO::tel_read_ra ()
{
  double new_ra;
  if (tel_read_hms (&new_ra, "#:GR#")) {
    logStream (MESSAGE_ERROR) <<"APGTO::tel_read_ra failed"<< sendLog;
    return -1;
  }
  setTelRa (new_ra * 15.0);
  return 0;
}
/*!
 * Reads APGTO declination.
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
APGTO::tel_read_dec ()
{
  double t_telDec;
  if (tel_read_hms (&t_telDec, "#:GD#")) {
    logStream (MESSAGE_ERROR) <<"APGTO::tel_read_dec failed"<< sendLog;
    return -1;
  }
  setTelDec (t_telDec);
  return 0;
}
/*!
 * Reads APGTO altitude.
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
APGTO::tel_read_altitude ()
{
  double new_altitude;
  if (tel_read_hms (&new_altitude, "#:GA#")) {
    logStream (MESSAGE_ERROR) <<"APGTO::tel_read_altitude failed"<< sendLog;
    return -1;
  }
  APAltAz->setAlt(new_altitude);
  return 0;
}
/*!
 * Reads APGTO azimuth.
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
APGTO::tel_read_azimuth ()
{
  double new_azimuth;
  if (tel_read_hms (&new_azimuth, "#:GZ#")) {
    logStream (MESSAGE_ERROR) <<"APGTO::tel_read_azimuth failed"<< sendLog;
    return -1;
  }
  APAltAz->setAz(new_azimuth);
  return 0;
}
/*!
 * Reads APGTO local time.
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
APGTO::tel_read_local_time ()
{
  double new_local_time;
  if (tel_read_hms (&new_local_time, "#:GL#")) {
      logStream (MESSAGE_ERROR) <<"APGTO::tel_read_local_time failed"<< sendLog;
      return -1;
  }
  APlocal_time->setValueDouble(new_local_time * 15.) ;
  return 0;
}
/*!
 * Reads APGTO local sidereal time.
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
APGTO::tel_read_sidereal_time ()
{
  double new_sidereal_time;
  if (tel_read_hms (&new_sidereal_time, "#:GS#")) {
    logStream (MESSAGE_ERROR) <<"APGTO::tel_read_sidereal_time failed"<< sendLog;
    return -1;
  }
  APlocal_sidereal_time->setValueDouble(new_sidereal_time *15.) ;
  return 0;
}
/*!
 * Reads APGTO latitude.
 *
 * @return -1 on error, otherwise 0
 *
 */
int
APGTO::tel_read_latitude ()
{
  double new_latitude;
  if (tel_read_hms (&new_latitude, "#:Gt#")) {
    logStream (MESSAGE_ERROR) <<"APGTO::tel_read_latitude failed"<< sendLog;
    return -1;
  }
  APlatitude->setValueDouble(new_latitude) ;
  return 0;
}
/*!
 * Reads APGTO longitude.
 *
 * @return -1 on error, otherwise 0
 *
 */
int
APGTO::tel_read_longitude ()
{
  double new_longitude;
  if((tel_read_hms (&new_longitude, "#:Gg#"))< 0 ) {
      logStream (MESSAGE_ERROR) << "APGTO::tel_read_longitude tel_read_hms failed" <<sendLog;
      return -1;
  }
  APlongitude->setValueDouble(new_longitude) ;
  //logStream (MESSAGE_DEBUG) << "APGTO::tel_read_longitude " << new_longitude << sendLog;
  return 0;
}
/*!
 * Reads APGTO relative position of the declination axis to the observed hour angle.
 *
 * @return -1 on error, otherwise 0
 *
 */
int
APGTO::tel_read_declination_axis ()
{
  int ret ;
  char new_declination_axis[32] ;

  if (( ret= tel_write_read_hash ( "#:pS#", 5, new_declination_axis, 5))< 0) { // result is East#, West#
    logStream (MESSAGE_DEBUG) << "APGTO::tel_read_declination_axis failed" << sendLog;
    return -1;
  }
  DECaxis_HAcoordinate->setValueString(new_declination_axis) ;
  return 0;
}
/*!
 * Reads and checks APGTO relative position of the declination axis to the observed hour angle.
 *
 * @return -1 on error, otherwise 0, or exit(1) in case of a serious failure
 *
 */
int
APGTO::tel_check_declination_axis ()
{
// make sure that the (declination - optical axis) have the correct sign
// if not you'll buy a new tube.
  if (tel_read_ra () || tel_read_dec ())
    return -1;
  
  double HA, HA_h ;
  double RA, RA_h ;
  char RA_str[32] ;
  char HA_str[32] ;
  int h, m, s ;

  double JD  = ln_get_julian_from_sys ();
  double lng = telLongitude->getValueDouble ();
  double local_sidereal_time= ln_get_mean_sidereal_time( JD) * 15. + lng;  // longitude positive to the East

  RA  = getTelRa() ;
  RA_h= RA/15. ;

  getSexComponents( RA_h, &h, &m, &s) ;
  snprintf(RA_str, 9, "%02d:%02d:%02d", h, m, s);

  HA  = fmod( local_sidereal_time- RA+ 360., 360.) ;
  HA_h= HA / 15. ;

  getSexComponents( HA_h, &h, &m, &s) ;
  snprintf(HA_str, 9, "%02d:%02d:%02d", h, m, s);
	
  logStream (MESSAGE_DEBUG) << "APGTO::initValues RA:"<< RA_str << ", HA "<< HA_str << " optical:"<< DECaxis_HAcoordinate->getValue()<< sendLog;

  if( tel_read_declination_axis()) {
    logStream (MESSAGE_ERROR) << "APGTO::tel_check_declination_axis could not retrieve sign of declination axis, severe error, exiting." << sendLog;
    exit(1); // yes, this is the end 
  }
  if (!strcmp("West", DECaxis_HAcoordinate->getValue())) {
    if(( HA > 180.) && ( HA <= 360.)) {
    } else {
      logStream (MESSAGE_ERROR) << "APGTO::initValues sign of declination and optical axis is wrong (HA="<< HA_str<<", West), severe error, exiting." << sendLog;
      exit(1); // yes, this is the end 
    }
  } else if (!strcmp("East", DECaxis_HAcoordinate->getValue())) {
    if(( HA >= 0.0) && ( HA <= 180.)) {
    } else {
      logStream (MESSAGE_ERROR) << "APGTO::initValues sign of declination and optical axis is wrong(HA="<< HA_str<<", East), severe error, exiting." << sendLog;
      exit(1); // yes, this is the end 
    }
  }
  return 0 ;
}

/*!
 * Normalize ra and dec,
 *
 * @param ra		rigth ascenation to normalize in decimal hours
 * @param dec		rigth declination to normalize in decimal degrees
 *
 * @return 0
 */
void
APGTO::tel_normalize (double *ra, double *dec)
{
  if (*ra < 0) //normalize ra
    *ra = floor (*ra / 360) * -360 + *ra;
  if (*ra > 360)
    *ra = *ra - floor (*ra / 360) * 360;
  
  if (*dec < -90) //normalize dec
    *dec = floor (*dec / 90) * -90 + *dec;
  if (*dec > 90)
    *dec = *dec - floor (*dec / 90) * 90;
}
/*!
 * Set APGTO right ascenation.
 *
 * @param ra		right ascenation to set in decimal degrees
 *
 * @return -1 and errno on error, otherwise 0
 */
int
APGTO::tel_write_ra (double ra)
{
  char command[32];
  int h, m, s;
  if (ra < 0 || ra > 360.0) {
    return -1;
  }
  ra = ra / 15;
  dtoints (ra, &h, &m, &s);
  // Astro-Physics format
  if( snprintf(command, sizeof( command), "#:Sr %02d:%02d:%02d#", h, m, s)<0)
    return -1;
  logStream (MESSAGE_DEBUG) << "---------------------APGTO::tel_write_ra >"<< command << "<END" << sendLog;
  return tel_rep_write (command); // done in method
}
/*!
 * Set APGTO declination.
 *
 * @param dec		declination to set in decimal degrees
 *
 * @return -1 and errno on error, otherwise 0
 */
int
APGTO::tel_write_dec (double dec)
{
  int ret= -1 ;
  char command[32];
  int d, m, s;
  if (dec < -90.0 || dec > 90.0) {
    return -1;
  }
  dtoints (dec, &d, &m, &s);

  // Astro-Phiscs formt 
  if (!d && dec < 0) {
    ret= snprintf( command, sizeof( command), "#:Sd -%02d*%02d:%02d#", d, m, s);
  } else {
    ret= snprintf( command, sizeof( command), "#:Sd %+03d*%02d:%02d#", d, m, s);
  }
  if( ret < 0) 
    return -1;
  logStream (MESSAGE_DEBUG) << "---------------------APGTO::tel_write_dec >"<< command << "<END" << sendLog;
  return tel_rep_write (command);
}
/*!
 * Set slew rate.
 *
 * @param new_rate	new slew speed to set.
 *
 * @return -1 on failure & set errno, 5 (>=0) otherwise
 */
int
APGTO::tel_set_slew_rate (char new_rate)
{
  char command[6];
  sprintf (command, "#:RS%c#", new_rate); // slew
  logStream (MESSAGE_DEBUG) << "APGTO::tel_set_slew_rate >" << command << "<END" << sendLog;

  return tel_write (command, 5);
}
/*!
 * Set move rate.
 *
 * @param new_rate	new move speed to set.
 *
 * @return -1 on failure & set errno, 5 (>=0) otherwise
 */
int
APGTO::tel_set_move_rate (char moveToRate)
{
  char command[6];
  switch (moveToRate) {
    /* 0.25x*/
  case MOVE_RATE_000025:
    logStream (MESSAGE_DEBUG) <<"APGTO::tel_set_move_rate setting move to rate to 0.25x." << sendLog;
    strcpy( command, "#:RG0#"); 
    /* 0.5x*/
    break;
  case MOVE_RATE_000050:
    logStream (MESSAGE_DEBUG) <<"APGTO::tel_set_move_rate setting move to rate to 0.5x." << sendLog;
    strcpy( command, "#:RG1#"); 
    break;
    /* 1x*/
  case MOVE_RATE_000100:
    logStream (MESSAGE_DEBUG) <<"APGTO::tel_set_move_rate setting move to rate to 1x." << sendLog;
    strcpy( command, "#:RG2#"); 
    break;
    /* 12x*/
  case MOVE_RATE_001200:
    logStream (MESSAGE_DEBUG) <<"APGTO::tel_set_move_rate setting move to rate to 12x." << sendLog;
    strcpy( command, "#:RC0#"); 
    break;
    /* 64x */
  case MOVE_RATE_006400:
    logStream (MESSAGE_DEBUG) <<"APGTO::tel_set_move_rate setting move to rate to 64x." << sendLog;
    strcpy( command, "#:RC1#"); 
    break;
    /* 600x */
  case MOVE_RATE_060000: 
    logStream (MESSAGE_DEBUG) <<"APGTO::tel_set_move_rate setting move to rate to 600x." << sendLog;
    strcpy( command, "#:RC2#"); 
    break;
    /* 1200x */
  case MOVE_RATE_120000:
    logStream (MESSAGE_DEBUG) <<"APGTO::tel_set_move_rate setting move to rate to 1200x." << sendLog;
    strcpy( command, "#:RC3#"); 
    break;
  default:
    return -1;
    break;
  }
  logStream (MESSAGE_DEBUG) << "APGTO::tel_set_move_rate >" << command << "<END" << sendLog;
  return tel_write (command, 5);
}

int
APGTO::tel_start_move (char direction)
{
  char command[6];
  sprintf (command, "#:M%c#", direction);
  return tel_write (command, 5) == 1 ? -1 : 0;
}

int
APGTO::tel_stop_move (char direction)
{

  char command[6];
  sprintf (command, "#:Q%c#", direction);
  return tel_write (command, 5) < 0 ? -1 : 0;
}
/*!
 * Slew (=set) APGTO to new coordinates.
 *
 * @param ra 		new right ascenation
 * @param dec 		new declination
 *
 * @return -1 on error, otherwise 0
 */
int
APGTO::tel_slew_to (double ra, double dec)
{
  int ret ;
  char retstr;
  struct ln_lnlat_posn observer;
  struct ln_equ_posn target_equ;
  struct ln_hrz_posn hrz;
  double JD;

  tel_normalize (&ra, &dec);

  target_equ.ra= ra ;
  target_equ.dec= dec ;
  observer.lng = telLongitude->getValueDouble ();
  observer.lat = telLatitude->getValueDouble ();

  JD = ln_get_julian_from_sys ();
  ln_get_hrz_from_equ (&target_equ, &observer, JD, &hrz);

  if( hrz.alt < 0.) {
    logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to target_equ ra " << target_equ.ra << " dec " <<  target_equ.dec << " is below horizon"<< sendLog;
    return -1 ;
  }

  if( !( strcmp( "West", DECaxis_HAcoordinate->getValue()))) {
    ret= pier_collision( &target_equ, &observer) ;
  } else if( !( strcmp( "East", DECaxis_HAcoordinate->getValue()))) {
    //really target_equ.dec += 180. ;
    struct ln_equ_posn t_equ;
    t_equ.ra = target_equ.ra ;
    t_equ.dec= target_equ.dec + 180. ;
    ret= pier_collision( &t_equ, &observer) ;
  }
  if(ret != NO_COLLISION) {
    if( ret== COLLIDING) {
      logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to NOT slewing ra "<< target_equ.ra << " dec " << target_equ.dec << " COLLIDING, NOT syncing cupola"  << sendLog;
    } else if( ret== WITHIN_DANGER_ZONE){
      logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to NOT slewing ra "<< target_equ.ra << " dec " << target_equ.dec << " within DANGER zone, NOT syncing cupola"  << sendLog;
    } else {
      logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to NOT slewing ra "<< target_equ.ra << " target_equ.dec " << target_equ.dec << " invalid condition, exiting"  << sendLog;
      exit(1) ;
    }
      return -1;
    }
  logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to cleared geometry  slewing ra "<< target_equ.ra << " target_equ.dec " << dec  << sendLog;

  if (( ret=tel_write_ra (target_equ.ra)) < 0) {
    logStream (MESSAGE_DEBUG) << "APGTO::tel_slew_to, not slewing, tel_write_ra return value was " << ret << sendLog ;
    return -1;
  }
  if (( ret=tel_write_dec (target_equ.dec)) < 0) {
    logStream (MESSAGE_DEBUG) << "APGTO::tel_slew_to, not slewing, tel_write_dec return value was " << ret << sendLog ;
    return -1;
  }
  logStream (MESSAGE_DEBUG) << "APGTO::tel_slew_to #:MS# on ra " << ra << ", dec " << dec << sendLog ;

  if (( ret=tel_write_read ("#:MS#", 5, &retstr, 1)) < 0) {
    logStream (MESSAGE_ERROR) <<"APGTO::tel_slew_to, not slewing, tel_write_read #:MS# failed"<< sendLog;
    return -1;
  }

  if (retstr == '0') {
    logStream (MESSAGE_DEBUG) << "APGTO::tel_slew_to syncing cupola on target ra "<< target_equ.ra << " dec " << target_equ.dec << " got '0'=>"<< retstr<<"<, syncing cupola"  << sendLog;

    if( !( strcmp( "West", DECaxis_HAcoordinate->getValue()))) {
      postEvent (new Rts2Event (EVENT_CUP_START_SYNC, (void*) &target_equ));
    } else if( !( strcmp( "East", DECaxis_HAcoordinate->getValue()))) {
      //tel_equ.dec += 180. ;
      struct ln_equ_posn t_equ;
      t_equ.ra = target_equ.ra ;
      t_equ.dec= target_equ.dec + 180. ;
      postEvent (new Rts2Event (EVENT_CUP_START_SYNC, (void*) &t_equ));
    }
    return 0;
  }
  logStream (MESSAGE_ERROR) << "APGTO::tel_slew_to NOT slewing ra "<< target_equ.ra << " dec " << target_equ.dec << " got '0'!= >"<< retstr<<"<END, NOT syncing cupola"  << sendLog;
  return -1;
}
void
APGTO::set_move_timeout (time_t plus_time)
{
	time_t now;
	time (&now);

	move_timeout = now + plus_time;
}
int
APGTO::startResync ()
{
  int ret;
  
  lastMoveRa = getTelTargetRa ();
  lastMoveDec = getTelTargetDec ();
  
  ret = tel_slew_to (lastMoveRa, lastMoveDec);
  if (ret)
    return -1;
  move_state = MOVE_REAL;
  set_move_timeout (100);
  return 0 ; 
}
int
APGTO::isMoving ()
{
  int ret;
  
  switch (move_state) {
  case MOVE_REAL:
    switch (ret) {
    case -1:
      return -1;
    case 0:
      return USEC_SEC / 10;
    case 1:
    case 2:
      move_state = NOTMOVE;
      return -2;
    }
    break;
  default:
    logStream (MESSAGE_ERROR) << "APGTO::isMoving NO case >" << sendLog ; // << move_state << "<END" << sendLog;
    break;
  }
  return -1;
}

int
APGTO::stopMove ()
{
  char dirs[] = { 'e', 'w', 'n', 's' };
  int i;
  for (i = 0; i < 4; i++) {
    if (tel_stop_move (dirs[i]) < 0)
      
      return -1;
  }
  return 0;
}
/*!
 * Set telescope to match given coordinates (sync)
 *
 * AP GTO remembers the last position only occasionally it looses it.
 *
 * @param ra		setting right ascscension
 * @param dec		setting declination
 *
 * @return -1 and set errno on error, otherwise 0
 */
int
APGTO::setTo (double ra, double dec)
{
  char readback[101];
  tel_normalize (&ra, &dec);
  if ((tel_write_ra (ra) < 0) || (tel_write_dec (dec) < 0))
    return -1;

  //AP manual:
  //Position
  //Command:  :CM#
  //Response: “Coordinates matched.     #”
  //          (there are 5 spaces between “Coordinates” and “matched”, and 8 trailing spaces before the “#”, 
  //          the total response length is 32 character plus the “#”.	  

  logStream (MESSAGE_ERROR) <<"APGTO::setTo #:CM# doing SetTo (SYNC)-----------------------------------------"<< sendLog;

  if (tel_write_read_hash ("#:CM#", 5, readback, 100) < 0) {
    logStream (MESSAGE_ERROR) <<"APGTO::setTo #:CM# failed"<< sendLog;
    return -1;
  }
  return 0 ;
}
/*!
 * Correct telescope coordinates.
 * Used for closed loop coordinates correction based on astronometry
 * of obtained images.
 * @param ra		ra correction
 * @param dec		dec correction
 * @return -1 and set errno on error, 0 otherwise.
 */
int
APGTO::correct (double cor_ra, double cor_dec, double real_ra, double real_dec)
{
  if (setTo (real_ra, real_dec)) //  means sync
    return -1;
  logStream (MESSAGE_DEBUG) <<"APGTO::correct sync on "<< real_ra<< " dec"<< real_dec<< sendLog;
  return 0;
}
/*!
 * Park telescope to neutral location.
 *
 * @return -1 and errno on error, 0 otherwise
 */
int
APGTO::startPark ()
{
  return 0 ; // wildi ToDo: testing
//   double JD= ln_get_julian_from_sys ();
//   double local_sidereal_time= ln_get_mean_sidereal_time( JD) * 15. + telLongitude->getValueDouble ();  // longitude positive to the East
  
//   return tel_slew_to (local_sidereal_time, -(90. -telLatitude->getValueDouble()));
 

  // the equatorial position is derived from the azimuth coordinates (45, 0).  
  // later it will be done using the AP GTO controller's native commands.
  struct ln_equ_posn park;
  struct ln_lnlat_posn observer;
  struct ln_hrz_posn hrz;
  double JD;

  // wildi: ToDo make an option
  // APGTO 0= N, 90=E
  hrz.az= 45. + 180. ;
  hrz.alt= 10. ;

  observer.lng = telLongitude->getValueDouble ();
  observer.lat = telLatitude->getValueDouble ();
  JD = ln_get_julian_from_sys ();
  ln_get_equ_from_hrz (&hrz, &observer, JD, &park) ;

  logStream (MESSAGE_DEBUG) << "APGTO::startPark slew alt " << hrz.alt << " az " <<  hrz.az << " EQ: ra " << park.ra << " dec " <<  park.dec << sendLog;

  return tel_slew_to ( park.ra, park.dec);
}
int
APGTO::isParking ()
{
  logStream (MESSAGE_DEBUG) << "APGTO::isParking" << sendLog;
  return -2;
}
int
APGTO::endPark ()
{
  return 0 ; // wildi ToDo: testing
  logStream (MESSAGE_DEBUG) << "APGTO::endPark" << sendLog;
  ParkDisconnect() ;
  return 0;
}
int
APGTO::startDir (char *dir)
{
  switch (*dir) {
  case DIR_EAST:
  case DIR_WEST:
  case DIR_NORTH:
  case DIR_SOUTH:
    return tel_start_move (*dir);
  }
  return -2;
}
int
APGTO::stopDir (char *dir)
{
  switch (*dir) {
  case DIR_EAST:
  case DIR_WEST:
  case DIR_NORTH:
  case DIR_SOUTH:
    return tel_stop_move (*dir);
  }
  return -2;
}
void 
APGTO::ParkDisconnect()
{
  // ToDo handle return value
  stopMove () ;
  // sleep for 200 mseconds
  usleep(200000);
  
  if ( selectAPTrackingMode(3) < 0 ) { /* tracking mode 3 = zero */
    logStream (MESSAGE_ERROR) << "APGTO::ParkDisconnect setting tracking mode ZERO." << sendLog;
    return;
  }

// The AP mount will not surely stop with #:KA alone
  if (setAPMotionStop() < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::ParkDisconnect motion stop  failed #:Q#" << sendLog;
    return;
  }
  if (setAPPark() < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::ParkDisconnect parking failed #:KA#" << sendLog;
    return;
  }
  logStream (MESSAGE_DEBUG) << "APGTO::ParkDisconnect the telescope is parked and disconnected. Turn off the telescope (a power cycle is required)." << sendLog; 
  return;
}
void 
APGTO::valueChanged (Rts2Value * changed_value)
{
  int ret= -1 ;
  int slew_rate= -1 ;
  int move_rate= -1 ;
  char command ;

  if (changed_value ==APslew_rate){
    if(( slew_rate= APslew_rate->getValueInteger())== 1200) {
      command= SLEW_RATE_1200 ;
    } else if( APslew_rate->getValueInteger()== 900) {
      command= SLEW_RATE_0900 ;
    } else if( APslew_rate->getValueInteger()== 600) {
      command= SLEW_RATE_0600 ;
    } else {
      APslew_rate->setValueInteger(-1);  
      logStream (MESSAGE_ERROR) << "APGTO::valueChanged wrong slew rate " << APslew_rate->getValue() << ", valid: 1200, 900, 600"<< sendLog;
      return ;
    }

    if(( ret= tel_set_slew_rate (command)) !=5) {
      // wildi ToDo: thinking about what to do in this case
      APslew_rate->setValueInteger(-1); 
      logStream (MESSAGE_ERROR) << "APGTO::valueChanged tel_set_slew_rate failed" << sendLog;
      // return -1 ;
      return ;
    }
  } else if (changed_value ==APmove_rate) {
    if(( move_rate= APmove_rate->getValueInteger())== 600) {
      command= MOVE_RATE_060000 ;
    } else if( APmove_rate->getValueInteger()== 64) {
      	  command= MOVE_RATE_006400 ;
    } else if( APmove_rate->getValueInteger()== 12) {
      command= MOVE_RATE_001200 ;
    } else if( APmove_rate->getValueInteger()== 1) {
      command= MOVE_RATE_000100 ;
    } else {
      APmove_rate->setValueInteger(-1);  
      logStream (MESSAGE_ERROR) << "APGTO::valueChanged wrong move rate " << APmove_rate->getValue() << ", valid: 600, 64, 12, 1"<< sendLog;
      return ;
    }

    if(( ret= tel_set_move_rate (command)) !=5) {
      // wildi ToDo: thinking about what to do in this case
      APmove_rate->setValueInteger(-1); 
      logStream (MESSAGE_ERROR) << "APGTO::valueChanged tel_set_move_rate failed" << sendLog;
      // return -1 ;
      return ;
    }
  }
  Telescope::valueChanged (changed_value);
}
int 
APGTO::commandAuthorized (Rts2Conn *conn)
{
  int ret = -1 ;

  if (conn->isCommand ("abort")) {
    if((ret = setAPMotionStop()) < 0) {
      logStream (MESSAGE_ERROR) << "stop motion #:Q# failed, SWITCH the mount OFF"<< sendLog;
      return -1;
    }
    logStream (MESSAGE_DEBUG) << "APGTO::commandAuthorized stopped any motion #:Q#" << sendLog;
    if ( selectAPTrackingMode(3) < 0 ) /* tracking mode 3 = zero */ {
      logStream (MESSAGE_ERROR) << "APGTO::commandAuthorized setting tracking mode ZERO failed." << sendLog;
      return -1;
    }
    return 0;
  } else if (conn->isCommand ("track")) {
    if ( selectAPTrackingMode(2) < 0 ) { /* tracking mode 2 = sidereal */
      logStream (MESSAGE_ERROR) << "APGTO::commandAuthorized set track mode sidereal failed." << sendLog;
      return -1;
    }
    return 0 ;
  } else if (conn->isCommand ("rot")) { // move is used for a slew to a position
    char *direction ;
    if (conn->paramNextStringNull (&direction) || !conn->paramEnd ()) { 
      logStream (MESSAGE_ERROR) << "APGTO::commandAuthorized rot failed" << sendLog;
      return -2;
    }
    if( startDir( direction)){
      logStream (MESSAGE_ERROR) <<"APGTO::commandAuthorized startDir( direction) failed"<< sendLog;
      return -1;
    }
    return 0 ;
  } else if ((conn->isCommand ("sync"))||(conn->isCommand ("sync_sg"))) {
    double sync_ra, sync_dec ;
    if(conn->isCommand ("sync")){
      if (conn->paramNextDouble (&sync_ra) || conn->paramNextDouble (&sync_dec) || !conn->paramEnd ()) { 
	logStream (MESSAGE_ERROR) << "APGTO::commandAuthorized sync paramNextDouble ra or dec failed" << sendLog;
	return -2;
      }
    } else if (conn->isCommand ("sync_sg")) {
      char *sync_ra_str;
      char *sync_dec_str;
      if (conn->paramNextStringNull (&sync_ra_str) || conn->paramNextStringNull (&sync_dec_str) || !conn->paramEnd ()) { 
	logStream (MESSAGE_ERROR) << "APGTO::commandAuthorized sync_sg paramNextString ra or dec failed" << sendLog;
	return -2;
      }
      if(( ret= f_scansexa ( sync_ra_str, &sync_ra))== -1) {
	logStream (MESSAGE_ERROR) << "APGTO::commandAuthorized sync_sg parsing ra failed" << sendLog;
	return -1;
      }
      if(( ret= f_scansexa ( sync_dec_str, &sync_dec))== -1) {
	logStream (MESSAGE_ERROR) << "APGTO::commandAuthorized sync_sg parsing dec failed" << sendLog;
	return -1;
      }
    }
    if(( ret= setTo(sync_ra, sync_dec)) !=0) {
      logStream (MESSAGE_ERROR) << "APGTO::commandAuthorized setTo failed" << sendLog;
      return -1 ;
    }
    if( tel_check_declination_axis()) {
      logStream (MESSAGE_ERROR) << "APGTO::commandAuthorized check of the declination axis failed, this message will never appear." << sendLog;
      return -1;
    }

    logStream (MESSAGE_DEBUG) << "APGTO::commandAuthorized sync on ra " << sync_ra << " dec " << sync_dec << sendLog;
    return 0 ;
  }
  return Telescope::commandAuthorized (conn);
}

int
APGTO::info ()
{
  int ret ;
  if (tel_read_ra () || tel_read_dec ())
    return -1;
  if(( ret= tel_read_sidereal_time()) != 0)
    return -1 ;
  if(( ret= tel_read_declination_axis()) != 0)
    return -1 ;
  if (tel_read_azimuth () || tel_read_altitude ())
    return -1;

  return Telescope::info ();
}
int 
APGTO::setBasicData()
{
  int ret ;
  struct ln_date utm;
  struct ln_zonedate ltm;

  if(setAPClearBuffer() < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData clearing the buffer failed" << sendLog;
    return -1;
  }
  if(setAPLongFormat() < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData setting long format failed" << sendLog;
    return -1;
  }
    
  if(setAPBackLashCompensation(0,0,0) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData setting back lash compensation failed" << sendLog;
    return -1;
  }
  ln_get_date_from_sys( &utm) ;
  ln_date_to_zonedate(&utm, &ltm, 3600); // Adds "only" offset to JD and converts back (see DST below)

  if(( ret= setAPLocalTime(ltm.hours, ltm.minutes, (int) ltm.seconds) < 0)) {
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData setting local time failed" << sendLog;
    return -1;
  }
  if (( ret= setCalenderDate(ltm.days, ltm.months, ltm.years) < 0) ) {
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData setting local date failed"<< sendLog;
    return -1;
  }
  // RTS2 counts positive to the East, AP positive to the West
  double APlng= 360. - telLongitude->getValueDouble() ;
  if (( ret= setAPSiteLongitude( APlng) < 0) ) { // AP mount: positive and only to the to west 
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData setting site coordinates failed" << sendLog;
    return -1;
  }
  if (( ret= setAPSiteLatitude( telLatitude->getValueDouble()) < 0) ) {
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData setting site coordinates failed"<< sendLog;
    return -1;
  }
  // ToDo: No DST (CEST) is given to the AP mount, so local time is off by 1 hour during DST
  // CET                           22:56:06  -1:03:54, -1.065 (valid for Obs Vermes CET)
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

  if(( ret = setAPUTCOffset( -1.065)) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData setting AP UTC offset failed" << sendLog;
    return -1;
  }
  if(( ret= tel_set_slew_rate(SLEW_RATE_0600)) < 0 ) { /* slew rate 2 = 600x, this the slowest */
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData setting slew rate failed." << sendLog;
    return -1;
  }
  APslew_rate->setValueInteger(600);

  if(( ret= tel_set_move_rate(MOVE_RATE_000100)) < 0 ) { /* move rate MOVE_RATE_000100 = 1x */
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData setting tracking mode sidereal failed." << sendLog;
    return -1;
  }
  APmove_rate->setValueInteger(1);

  if(! ( strcmp(initialization, "cold"))) {
    logStream (MESSAGE_DEBUG) << "APGTO::setBasicData performing a cold start" << sendLog;
    if(( ret= setAPUnPark()) < 0) {
      logStream (MESSAGE_ERROR) << "APGTO::setBasicData unparking failed"<< sendLog;
      return -1;
    }
  } else {
    logStream (MESSAGE_DEBUG) << "APGTO::setBasicData performing a warm start" << sendLog;
  }
  logStream (MESSAGE_DEBUG) << "APGTO::setBasicData unparking successful" << sendLog;
  if(( ret= setAPMotionStop()) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO::setBasicData stop motion #:Q# failed, check the mount"<< sendLog;
    return -1;
  }
  logStream (MESSAGE_DEBUG) << "APGTO::setBasicData stopped any motion #:Q#" << sendLog;
  
  return 0 ;
}
// further discussion with Petr required:
// int APGTO::changeMasterState (int new_state)
// {
// 	// do NOT park us during day..
// 	if (
// 		   ((new_state & SERVERD_STATUS_MASK) == SERVERD_SOFT_OFF)
// 		|| ((new_state & SERVERD_STATUS_MASK) == SERVERD_HARD_OFF)
// 		|| ((new_state & SERVERD_STANDBY_MASK) && standbyPark))
// 	{
// 		if ((getState () & TEL_MASK_MOVING) == 0)
// 		  startPark ();
// 	}

// 	if (blockOnStandby->getValueBool () == true)
// 	{
// 		if ((new_state & SERVERD_STATUS_MASK) == SERVERD_SOFT_OFF
// 		  || (new_state & SERVERD_STATUS_MASK) == SERVERD_HARD_OFF
// 		  || (new_state & SERVERD_STANDBY_MASK))
// 			blockMove->setValueBool (true);
// 		else
// 			blockMove->setValueBool (false);
// 	}
// 	// do not call Telescope::changeMasterState()
// 	return Rts2Device::changeMasterState (new_state);
// }
/*!
 * Init telescope, connect on given apgto_fd.
 *
 * @param device_name		pointer to device name
 * @param telescope_id		id of telescope, for APGTO ignored
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int
APGTO::init ()
{
  struct termios tel_termios;

  int status;

  status = Telescope::init ();
  if (status)
    return status;

  apgto_fd = open (device_file, O_RDWR);

  if (apgto_fd < 0)
    return -1;
  
  if (tcgetattr (apgto_fd, &tel_termios) < 0)
    return -1;

  if (cfsetospeed (&tel_termios, B9600) < 0 ||
      cfsetispeed (&tel_termios, B9600) < 0)
    return -1;

  tel_termios.c_iflag = IGNBRK & ~(IXON | IXOFF | IXANY);
  tel_termios.c_oflag = 0;
  tel_termios.c_cflag =	((tel_termios.c_cflag & ~(CSIZE)) | CS8) & ~(PARENB | PARODD);
  tel_termios.c_lflag = 0;
  tel_termios.c_cc[VMIN] = 0;
  tel_termios.c_cc[VTIME] = 5;

  if (tcsetattr (apgto_fd, TCSANOW, &tel_termios) < 0) {
    logStream (MESSAGE_ERROR) << "APGTO init tcsetattr" << sendLog;
    return -1;
  }
  // get current state of control signals
  ioctl (apgto_fd, TIOCMGET, &status);

  // Drop DTR
  status &= ~TIOCM_DTR;
  ioctl (apgto_fd, TIOCMSET, &status);

  logStream (MESSAGE_DEBUG) << "APGTO init RS 232 initialization complete on port " << device_file << sendLog;
  return 0;
}
/*!
 * Reads information about telescope.
 */
int
APGTO::initValues ()
{
  int ret = -1 ;
  int flip= -1 ;

  strcpy (telType, "APGTO");

  Rts2Config *config = Rts2Config::instance ();
  ret = config->loadFile ();
  if (ret)
    return -1;

  telLongitude->setValueDouble (config->getObserver ()->lng);
  telLatitude->setValueDouble (config->getObserver ()->lat);
  telAltitude->setValueDouble (config->getObservatoryAltitude ());

  if(( ret= setBasicData()) != 0)
    return -1 ;
  if (tel_read_longitude () || tel_read_latitude ())
    return -1;
  if (tel_read_azimuth () || tel_read_altitude ())
    return -1;
  if( getAPVersionNumber() != 0 )
    return -1 ; ;
  if(( ret= getAPUTCOffset()) != 0)
    return -1 ;
  if(( ret= tel_read_local_time()) != 0)
    return -1 ;
  if(( ret= tel_read_sidereal_time()) != 0)
    return -1 ;

  // Check if the sidereal time read from the telescope is correct 
  double JD  = ln_get_julian_from_sys ();
  double lng = telLongitude->getValueDouble ();
  double local_sidereal_time= fmod((ln_get_mean_sidereal_time( JD) * 15. + lng), 360.);  // longitude positive to the East
  
  logStream (MESSAGE_DEBUG) << "APGTO::initValues  local sidereal time, calculated time " 
			    << local_sidereal_time << " mount: "
			    << APlocal_sidereal_time->getValueDouble() 
			    << " difference "<< local_sidereal_time- APlocal_sidereal_time->getValueDouble()<<sendLog;
	
  if( abs(local_sidereal_time- APlocal_sidereal_time->getValueDouble()) > 1./240. ) {
    logStream (MESSAGE_ERROR) << "APGTO::initValues ap sidereal time off by "<< local_sidereal_time- APlocal_sidereal_time->getValueDouble() << "exiting" << sendLog;
    exit (1) ; // do not go beyond
  } 

  if(( ret= tel_read_declination_axis()) != 0)
    return -1 ;

  // This definition has to been confirmed by petr
  // West: HA + Pi/2 = direction where the declination axis points
  // East: HA - Pi/2
  if( !( strcmp( "West", DECaxis_HAcoordinate->getValue()))) {
      flip = 1 ;
  } else if( !( strcmp( "East", DECaxis_HAcoordinate->getValue()))) {
    flip= 0 ;
  } else {
    logStream (MESSAGE_ERROR) << "APGTO::initValues could not retrieve angle (declination axis, hour axis), exiting  " << sendLog;
    exit(1) ;
  }
  telFlip->setValueInteger (flip);

  if( tel_check_declination_axis()) {
    logStream (MESSAGE_ERROR) << "APGTO::initValues check of the declination axis failed, this message will never appear." << sendLog;
    return -1;
  }

  if(( ret= selectAPTrackingMode(2)) < 0 ) { /* tracking mode 2 = sidereal */
    logStream (MESSAGE_ERROR) << "APGTO::initValues setting tracking mode sidereal failed." << sendLog;
    return -1;
  }
  return Telescope::initValues ();
}
APGTO::~APGTO (void)
{
  close (apgto_fd);
}
int
APGTO::processOption (int in_opt)
{
  switch (in_opt) {
  case 'f':
    device_file = optarg;
    break;
  case 'b':
    strcpy( initialization, optarg) ;
    break;
  default:
    return Telescope::processOption (in_opt);
  }
  return 0;
}
APGTO::APGTO (int in_argc, char **in_argv):Telescope (in_argc,in_argv)
{
  device_file = "/dev/apmount";
	
  addOption ('f', "device_file", 1, "device file");
  addOption ('b', "init", 1, "initialization cold (after power cycle), warm");


  createValue (APAltAz,               "APALTAZ",    "AP mount Alt/Az[deg]",          true, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE, 0);
  createValue (DECaxis_HAcoordinate,  "DECXHA",     "DEC axis HA coordinate, West (==-1), East (==1)", false, RTS2_VALUE_WRITABLE);
  createValue (APslew_rate,           "APSLEWRATE", "AP slew rate (1200, 900, 600)", false, RTS2_VALUE_WRITABLE);
  createValue (APmove_rate,           "APMOVERATE", "AP move rate (600, 64, 12, 1)", false, RTS2_VALUE_WRITABLE);
  createValue (APlocal_sidereal_time, "APLST",      "AP mount local sidereal time",  true, RTS2_DT_RA);
  createValue (APlocal_time,          "APLOT",      "AP mount local time",           true, RTS2_DT_RA);
  createValue (APutc_offset,          "APUTCO",     "AP mount UTC offset",           true, RTS2_DT_RA);
  createValue (APfirmware,            "APVERSION",  "AP mount firmware revision",    true);
  createValue (APlongitude,           "APLONGITUDE","AP mount longitude",            true, RTS2_DT_DEGREES);
  createValue (APlatitude,            "APLATITUDE", "AP mount latitude",             true, RTS2_DT_DEGREES);
  
  apgto_fd = -1;
}
int
main (int argc, char **argv)
{
  APGTO device = APGTO (argc, argv);
  return device.run ();
}
