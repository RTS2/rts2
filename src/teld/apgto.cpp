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

#include <termios.h>
// uncomment following line, if you want all tel_desc read logging (will
// at about 10 30-bytes lines to syslog for every query).
//#define DEBUG_ALL_PORT_COMM
#define DEBUG
/* TTY Error Codes */
enum TTY_ERROR { TTY_OK=0, TTY_READ_ERROR=-1, TTY_WRITE_ERROR=-2, TTY_SELECT_ERROR=-3, TTY_TIME_OUT=-4, TTY_PORT_FAILURE=-5, TTY_PARAM_ERROR=-6, TTY_ERRNO = -7};

#define OPT_APGTO_INIT		OPT_LOCAL + 53

#define RATE_SLEW 'S'
#define RATE_FIND 'M'
#define RATE_CENTER 'C'
#define RATE_GUIDE  'G'
#define DIR_NORTH 'n'
#define DIR_EAST  'e'
#define DIR_SOUTH 's'
#define DIR_WEST  'w'

// must go away
#define LX200_TIMEOUT 2
//#define getAPDeclinationAxis( x)           getCommandString( x, "#:pS#")
//#define getAPVersionNumber( x)             getCommandString( x, "#:V#")
#define setAPPark()                        write(tel_desc, "#:KA", 4)
#define setAPUnPark()                      write(tel_desc, "#:PO", 4)
#define setAPLongFormat()                  write(tel_desc, "#:U", 3)
#define setAPClearBuffer()                 write(tel_desc, "#", 1) /* AP key pad manual startup sequence */
#define setAPMotionStop()                  write(tel_desc, "#:Q", 3)
#define setAPBackLashCompensation(x,y,z)   setCommandXYZ( x,y,z, "#:Br")
#define setLocalTime(x,y,z)                setCommandXYZ( x,y,z, "#:SL")

namespace rts2teld
{
  class APGTO:public Telescope
  {
  private:
    const char *device_file;
    char initialisation[256];
    int tel_desc;
    
    double lastMoveRa, lastMoveDec;  
    time_t move_timeout;

    // low-level functions which must go away
    int tty_read(int fd, char *buf, int nbytes, int timeout, int *nbytes_read) ;
    int tty_read_section(int fd, char *buf, char stop_char, int timeout, int *nbytes_read) ;
    int tty_timeout(int fd, int timeout) ;
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
    int check_apgto_connection() ;
    int getAPUTCOffset( double *value) ;
    int setAPObjectAZ( double az) ;
    int setAPObjectAlt( double alt) ;
    int setAPUTCOffset( double hours) ;
    int APSyncCM( char *matchedObject) ;
    int APSyncCMR( char *matchedObject) ;
    int selectAPMoveToRate( int moveToRate) ;
    int selectAPSlewRate( int slewRate) ;
    int selectAPTrackingMode( int trackMode) ;
    int swapAPButtons( int currentSwap) ; // not used in this driver
    //  see tel_write_ra  int setAPObjectRA( double ra) ;
    //  see tel_write_dec int setAPObjectDEC( double dec) ;
    int setAPSiteLongitude( double Long) ;
    int setAPSiteLatitude( double Lat) ;
    int setCommandXYZ( int x, int y, int z, const char *cmd) ;
    int setCalenderDate( int dd, int mm, int yy) ;
    // regular LX200 protocol (RTS2)
    int tel_read_ra ();
    int tel_read_dec ();
    int tel_read_latitude ();
    int tel_read_longtitude ();
    int tel_rep_write (char *command);
    void tel_normalize (double *ra, double *dec);
  
    int tel_write_ra (double ra);
    int tel_write_dec (double dec);

    int tel_set_rate (char new_rate);
    int telescope_start_move (char direction);
    int telescope_stop_move (char direction);
  
    int tel_slew_to (double ra, double dec);

    int tel_check_coords (double ra, double dec);

    void set_move_timeout (time_t plus_time);
    int setBasicData();

    void ParkDisconnect(int* established) ;

    // Astro-Physics properties
    Rts2ValueAltAz *APAltAz ;
    Rts2ValueDouble *APlst;
    Rts2ValueDouble *APlot;
    Rts2ValueDouble *APutco;
    Rts2ValueDouble *APLongitude;
    Rts2ValueDouble *APLatitude;

  public:
    APGTO (int argc, char **argv);
    virtual ~ APGTO (void);
    virtual int processOption (int in_opt);
    virtual int init ();
    virtual int initValues ();
    virtual int info ();

    virtual int setTo (double set_ra, double set_dec);
    virtual int correct (double cor_ra, double cor_dec, double real_ra, double real_dec);

    virtual int startResync ();
    virtual int stopMove ();
  
    virtual int startPark ();
    virtual int isParking ();
    virtual int endPark ();
  
    virtual int startDir (char *dir);
    virtual int stopDir (char *dir);
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
int 
APGTO::tty_timeout(int fd, int timeout)
{
  struct timeval tv;
  fd_set readout;
  int retval;

  FD_ZERO(&readout);
  FD_SET(fd, &readout);

  /* wait for 'timeout' seconds */
  tv.tv_sec = timeout;
  tv.tv_usec = 0;

  /* Wait till we have a change in the fd status */
  retval = select (fd+1, &readout, NULL, NULL, &tv);

  /* Return 0 on successful fd change */
  if (retval > 0)
    return TTY_OK;
  /* Return -1 due to an error */
  else if (retval == -1)
    return TTY_SELECT_ERROR;
  /* Return -2 if time expires before anything interesting happens */
  else
    return TTY_TIME_OUT;
}
int 
APGTO::tty_read(int fd, char *buf, int nbytes, int timeout, int *nbytes_read)
{
  int bytesRead = 0;
  int err = 0;
  *nbytes_read =0;

  if (nbytes <=0)
    return TTY_PARAM_ERROR;

  while (nbytes > 0)
    {
      if ( (err = tty_timeout(fd, timeout)) )
	return err;

      bytesRead = read(fd, buf, ((unsigned) nbytes));

      if (bytesRead < 0 )
	return TTY_READ_ERROR;

      buf += bytesRead;
      *nbytes_read += bytesRead;
      nbytes -= bytesRead;
    }
  return TTY_OK;
}
int 
APGTO::tty_read_section(int fd, char *buf, char stop_char, int timeout, int *nbytes_read)
{
  int bytesRead = 0;
  int err = TTY_OK;
  *nbytes_read = 0;

  for (;;)
    {
      if ( (err = tty_timeout(fd, timeout)) )
	return err;

      bytesRead = read(fd, buf, 1);

      if (bytesRead < 0 )
	return TTY_READ_ERROR;

      if (bytesRead)
	(*nbytes_read)++;

      if (*buf == stop_char)
	return TTY_OK;

      buf += bytesRead;
    }

  return TTY_TIME_OUT;
}
int
APGTO::check_apgto_connection()
{
  int i=0;
/*  char ack[1] = { (char) 0x06 }; does not work for AP moung */
  char temp_string[64];
  int error_type;
  int nbytes_read=0;

  logStream (MESSAGE_DEBUG) <<"Testing telescope's connection using #:GG#" << sendLog;

  if (tel_desc <= 0)
  {
      logStream (MESSAGE_DEBUG) <<"check_lx200ap_connection: not a valid file descriptor received"<< sendLog;
      return -1;
  }
  for (i=0; i < 2; i++)
  {
      if ( (error_type = tel_write( "#:GG#", 5)) != TTY_OK)
      {
	  logStream (MESSAGE_DEBUG) <<"check_lx200ap_connection: unsuccessful write to telescope,"<< sendLog;

	  return error_type;
      }
      error_type = tty_read_section(tel_desc, temp_string, '#', LX200_TIMEOUT, &nbytes_read) ;
      if (nbytes_read > 1)
      {
	  temp_string[ nbytes_read -1] = '\0';
	  logStream (MESSAGE_DEBUG) <<"check_lx200ap_connection: received bytes " << nbytes_read << "string: "<<  temp_string << sendLog;
	  return 0;
      }
    usleep(50000);
  }
  
  logStream (MESSAGE_DEBUG) <<"check_lx200ap_connection: wrote, but nothing received"<< sendLog;
  return -1;
}
int 
APGTO::getAPUTCOffset(double *value)
{
    int error_type;
    int nbytes_read=0;

    char temp_string[16];

    if ( (error_type = tel_write( "#:GG#", 5)) != TTY_OK)
	return error_type;

    if(( error_type = tty_read_section(tel_desc, temp_string, '#', LX200_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
      logStream (MESSAGE_DEBUG) <<"getAPUTCOffset: saying good bye, error" << error_type << " bytes read " << sendLog;

	return error_type ;
    }
/* Negative offsets, see AP keypad manual p. 77 */
    if((temp_string[0]== 'A') || ((temp_string[0]== '0')&&(temp_string[1]== '0')) ||(temp_string[0]== '@'))
    {
	int i ;
	for( i=nbytes_read; i > 0; i--)
	{
	    temp_string[i]= temp_string[i-1] ; 
	}
	temp_string[0] = '-' ;
	temp_string[nbytes_read + 1] = '\0' ;

	if( temp_string[1]== 'A') 
	{
	    temp_string[1]= '0' ;
	    switch (temp_string[2])
	    {
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
		  logStream (MESSAGE_DEBUG) << "getAPUTCOffset: string not handled: " << temp_string << sendLog;
		    return -1 ;
		    break ;
	    }
	}    
	else if( temp_string[1]== '0')
	{
	    temp_string[1]= '0' ;
	    temp_string[2]= '6' ;
	    logStream (MESSAGE_DEBUG) <<"getAPUTCOffset: done here: "<< temp_string << sendLog;
	}
	else if( temp_string[1]== '@')
	{
	    temp_string[1]= '0' ;
	    switch (temp_string[2])
	    {
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
		  logStream (MESSAGE_DEBUG) <<"getAPUTCOffset: string not handled " << temp_string << sendLog;
		    return -1 ;
		    break;
	    }    
	}
	else
	{
	  logStream (MESSAGE_DEBUG) <<"getAPUTCOffset: string not handled " << temp_string <<sendLog;
	}
    }
    else
    {
	temp_string[nbytes_read - 1] = '\0' ;
    }
    logStream (MESSAGE_DEBUG) <<"getAPUTCOffset: received string " << temp_string <<sendLog;
    if (f_scansexa(temp_string, value))
    {
	fprintf(stderr, "getAPUTCOffset: unable to process [%s]\n", temp_string);
	return -1;
    }
    return 0;
}
int 
APGTO::setAPObjectAZ(double az)
{
    int h, m, s;
    char temp_string[16];

    getSexComponents(az, &h, &m, &s);

    snprintf(temp_string, sizeof( temp_string ), "#:Sz %03d*%02d:%02d#", h, m, s);
    logStream (MESSAGE_DEBUG) <<"setAPObjectAZ: Set Object AZ String " << temp_string << sendLog;

    return (tel_write( temp_string, sizeof( temp_string )));
}
/* wildi Valid set Values are positive, add error condition */
int 
APGTO::setAPObjectAlt(double alt)
{
    int d, m, s;
    char temp_string[16];
    
    getSexComponents(alt, &d, &m, &s);

    /* case with negative zero */
    if (!d && alt < 0)
    {
	snprintf(temp_string, sizeof( temp_string ), "#:Sa -%02d*%02d:%02d#", d, m, s) ;
    }
    else
    {
	snprintf(temp_string, sizeof( temp_string ), "#:Sa %+02d*%02d:%02d#", d, m, s) ;
    }	

    logStream (MESSAGE_DEBUG) <<"setAPObjectAlt: Set Object Alt String %s" << temp_string << sendLog;
    return (tel_write( temp_string, sizeof( temp_string )));
}
int 
APGTO::setAPUTCOffset(double hours)
{
    int h, m, s ;

    char temp_string[16];
/* To avoid the peculiar output format of AP controller, see p. 77 key pad manual */
    if( hours < 0.)
    {
	hours += 24. ;
    }

    getSexComponents(hours, &h, &m, &s);
    
    snprintf(temp_string, sizeof( temp_string ), "#:SG %+03d:%02d:%02d#", h, m, s);
    logStream (MESSAGE_DEBUG) <<"setAPUTCOffset: " << temp_string << sendLog ;

    return (tel_write( temp_string, sizeof( temp_string )));
}
int 
APGTO::APSyncCM(char *matchedObject)
{
    int error_type;
    int nbytes_read=0;

    logStream (MESSAGE_DEBUG) <<"APSyncCM"<< sendLog;
    if ( (error_type = tel_write( "#:CM#", 5)) != TTY_OK)
	return error_type ;
  
    if(( error_type = tty_read_section(tel_desc, matchedObject, '#', LX200_TIMEOUT, &nbytes_read)) != TTY_OK)
	return error_type ;
   
    matchedObject[nbytes_read-1] = '\0';
  
    return 0;
}
int 
APGTO::APSyncCMR(char *matchedObject)
{
    int error_type;
    int nbytes_read=0;
    
    logStream (MESSAGE_DEBUG) <<"APSyncCMR"<< sendLog;
    if ( (error_type = tel_write( "#:CMR#", 6)) != TTY_OK)
	return error_type;
 
    /* read_ret = portRead(matchedObject, -1, LX200_TIMEOUT); */
    if(( error_type = tty_read_section(tel_desc, matchedObject, '#', LX200_TIMEOUT, &nbytes_read))  != TTY_OK)
	return error_type ;

    matchedObject[nbytes_read-1] = '\0';
    return 0;
}
int 
APGTO::selectAPMoveToRate(int moveToRate)
{
    int error_type;

    switch (moveToRate)
    {
	    /* 0.25x*/
 	case 0:
	    if ( (error_type = tel_write( "#:RG0#", 6)) != TTY_OK)
		return error_type;
	    break;
	    /* 0.5x*/
	case 1:
	    if ( (error_type = tel_write( "#:RG1#", 6)) != TTY_OK)
		return error_type;
	    break;
	    /* 1x*/
	case 2:
	    if ( (error_type = tel_write( "#:RG2#", 6)) != TTY_OK)
		return error_type;
	    break;
	    /* 12x*/
	case 3:
	    if ( (error_type = tel_write( "#:RC0#", 6)) != TTY_OK)
		return error_type;
	    break;
	    /* 64x */
	case 4:
	    if ( (error_type = tel_write( "#:RC1#", 6)) != TTY_OK)
		return error_type;
	    break;
	    /* 600x */
	case 5: 
	    if ( (error_type = tel_write( "#:RC2#", 6)) != TTY_OK)
		return error_type;
	    break;
	/* 1200x */
	case 6:
	    if ( (error_type = tel_write( "#:RC3#", 6)) != TTY_OK)
		return error_type;
	    break;
    
	default:
	    return -1;
	    break;
   }
   return 0;
}
int 
APGTO::selectAPSlewRate(int slewRate)
{
    int error_type;
    switch (slewRate)
    {
	/* 1200x */
	case 0:
	    logStream (MESSAGE_DEBUG) <<"selectAPSlewRate: Setting slew rate to 1200x."<< sendLog;
	    if ( (error_type = tel_write( "#:RS2#", 6)) != TTY_OK)
		return error_type;
	    break;
    
	    /* 900x */
	case 1:
	    logStream (MESSAGE_DEBUG) <<"selectAPSlewRate: Setting slew rate to 900x."<< sendLog;
	    if ( (error_type = tel_write( "#:RS1#", 6)) != TTY_OK)
		return error_type;
	    break;

	    /* 600x */
	case 2:
	    logStream (MESSAGE_DEBUG) <<"selectAPSlewRate: Setting slew rate to 600x."<< sendLog;
	    if ( (error_type = tel_write( "#:RS0#", 6)) != TTY_OK)
		return error_type;
	    break;

	default:
	    return -1;
	    break;
    }
    return 0;
}
int 
APGTO::selectAPTrackingMode(int trackMode)
{
    int error_type;

    switch (trackMode)
    {
	/* Lunar */
	case 0:
	    logStream (MESSAGE_DEBUG) <<"selectAPTrackingMode: Setting tracking mode to lunar."<< sendLog;
	    if ( (error_type = tel_write( "#:RT0#", 6)) != TTY_OK)
		return error_type;
	    break;
    
	    /* Solar */
	case 1:
	    logStream (MESSAGE_DEBUG) <<"selectAPTrackingMode: Setting tracking mode to solar."<< sendLog;
	    if ( (error_type = tel_write( "#:RT1#", 6)) != TTY_OK)
		return error_type;
	    break;

	    /* Sidereal */
	case 2:
            logStream (MESSAGE_DEBUG) <<"selectAPTrackingMode: Setting tracking mode to sidereal."<< sendLog;
	    if ( (error_type = tel_write( "#:RT2#", 6)) != TTY_OK)
		return error_type;
	    break;

	    /* Zero */
	case 3:
            logStream (MESSAGE_DEBUG) <<"selectAPTrackingMode: Setting tracking mode to zero."<< sendLog;
	    if ( (error_type = tel_write( "#:RT9#", 6)) != TTY_OK)
		return error_type;
	    break;
 
	default:
	    return -1;
	    break;
    }
    return 0;
}
int 
APGTO::swapAPButtons(int currentSwap)
{
    int error_type;

    switch (currentSwap)
    {
	case 0:
            logStream (MESSAGE_DEBUG) <<"#:NS#"<< sendLog;
	    if ( (error_type = tel_write( "#:NS#", 5)) != TTY_OK)
		return error_type;
	    break;
	    
	case 1:
            logStream (MESSAGE_DEBUG) <<"#:EW#"<< sendLog;
	    if ( (error_type = tel_write( "#:EW#", 5)) != TTY_OK)
		return error_type;
	    break;

	default:
	    return -1;
	    break;
    }
    return 0;
}
// int 
// APGTO::setAPObjectRA(double ra)
// {
// /*ToDo AP accepts "#:Sr %02d:%02d:%02d.%1d#"*/
//  int h, m, s;
//  char temp_string[16];

//  getSexComponents(ra, &h, &m, &s);

//  snprintf(temp_string, sizeof( temp_string ), "#:Sr %02d:%02d:%02d#", h, m, s);

//  logStream (MESSAGE_DEBUG) <<"setAPObjectRA: Set Object RA String " << temp_string << " ra " << ra << sendLog;

//  return (tel_write( temp_string, sizeof( temp_string )));
// }

// int 
// APGTO::setAPObjectDEC(double dec)
// {
//   int d, m, s;
//   char temp_string[16];

//   getSexComponents(dec, &d, &m, &s);
//   /* case with negative zero */
//   if (!d && dec < 0)
//   {
//       snprintf(temp_string, sizeof( temp_string ), "#:Sd -%02d*%02d:%02d#", d, m, s);
//   }
//   else
//   {
//       snprintf(temp_string, sizeof( temp_string ),   "#:Sd %+03d*%02d:%02d#", d, m, s);
//   }
//   logStream (MESSAGE_DEBUG) <<"setAPObjectDEC: Set Object DEC String " << temp_string << sendLog;

//   return (tel_write( temp_string, sizeof( temp_string )));
// }
int 
APGTO::setAPSiteLongitude(double Long)
{
   int d, m, s;
   char temp_string[32];

   getSexComponents(Long, &d, &m, &s);
   snprintf(temp_string, sizeof( temp_string ), "#:Sg %03d*%02d:%02d#", d, m, s);
   return (tel_write( temp_string, sizeof( temp_string )));
}

int 
APGTO::setAPSiteLatitude(double Lat)
{
   int d, m, s;
   char temp_string[32];

   getSexComponents(Lat, &d, &m, &s);
   snprintf(temp_string, sizeof( temp_string ), "#:St %+03d*%02d:%02d#", d, m, s);
   return (tel_write( temp_string, sizeof( temp_string )));
}
int 
APGTO::setCommandXYZ(int x, int y, int z, const char *cmd)
{
  char temp_string[16];

  snprintf(temp_string, sizeof( temp_string ), "%s %02d:%02d:%02d#", cmd, x, y, z);

  return (tel_write( temp_string, sizeof( temp_string )));
}
int 
APGTO::setCalenderDate(int dd, int mm, int yy)
{
  char temp_string[32];
  char dumpPlanetaryUpdateString[64];
  char bool_return[2];
  int error_type;
  int nbytes_read=0;
  yy = yy % 100;

  snprintf(temp_string, sizeof( temp_string ), "#:SC %02d/%02d/%02d#", mm, dd, yy);

  if((error_type = tel_write( temp_string, sizeof( temp_string ))) != TTY_OK)
    return error_type;

  error_type = tty_read(tel_desc, bool_return, 1, LX200_TIMEOUT, &nbytes_read);

  if (nbytes_read < 1)
    return error_type;

  bool_return[1] = '\0';

  if (bool_return[0] == '0')
    return -1;

  /* Read dumped data */
  error_type = tty_read_section(tel_desc, dumpPlanetaryUpdateString, '#', LX200_TIMEOUT, &nbytes_read);
  error_type = tty_read_section(tel_desc, dumpPlanetaryUpdateString, '#', 5, &nbytes_read);

  return 0;
}
/*!
 * Reads some data directly from tel_desc.
 *
 * Log all flow as LOG_DEBUG to syslog
 *
 * @exception EIO when there aren't data from tel_desc
 *
 * @param buf 		buffer to read in data
 * @param count 	how much data will be read
 *
 * @return -1 on failure, otherwise number of read data
 */
int
APGTO::tel_read (char *buf, int count)
{
  int n_read; // compiler compleas if read is used as index

	for (n_read = 0; n_read < count; n_read++)
	{
		int ret = read (tel_desc, &buf[n_read], 1);
		if (ret == 0)
		{
			ret = -1;
		}
		if (ret < 0)
		{
			logStream (MESSAGE_DEBUG) << "" << "" << sendLog;
			logStream (MESSAGE_DEBUG) << "APGTO tel_read: tel_desc read error "
				<< errno << sendLog;
			return -1;
		}
		#ifdef DEBUG_ALL_PORT_COMM
		logStream (MESSAGE_DEBUG) << "APGTO tel_read: read " << buf[n_read] <<
			sendLog;
		#endif
	}
	return n_read;
}
/*!
 * Will read from tel_desc till it encoutered # character.
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

	for (read = 0; read < count; read++)
	{
		if (tel_read (&buf[read], 1) < 0)
			return -1;
		if (buf[read] == '#')
			break;
	}
	if (buf[read] == '#')
		buf[read] = 0;
	logStream (MESSAGE_DEBUG) << "APGTO tel_read_hash: Hash-read: " << buf <<
		sendLog;
	return read;
}
/*!
 * Will write on telescope tel_desc string.
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
	logStream (MESSAGE_DEBUG) << "APGTO tel_write :will write: " << buf <<
		sendLog;
	return write (tel_desc, buf, count);
}
/*!
 * Combine write && read together.
 *
 * Flush tel_desc to clear any gargabe.
 *
 * @exception EINVAL and other exceptions
 *
 * @param wbuf		buffer to write on tel_desc
 * @param wcount	write count
 * @param rbuf		buffer to read from tel_desc
 * @param rcount	maximal number of characters to read
 *
 * @return -1 and set errno on failure, rcount otherwise
 */
int
APGTO::tel_write_read (const char *wbuf, int wcount, char *rbuf, int rcount)
{
	int tmp_rcount;
	char *buf;

	if (tcflush (tel_desc, TCIOFLUSH) < 0)
		return -1;
	if (tel_write (wbuf, wcount) < 0)
		return -1;

	tmp_rcount = tel_read (rbuf, rcount);
	if (tmp_rcount > 0)
	{
		buf = (char *) malloc (rcount + 1);
		memcpy (buf, rbuf, rcount);
		buf[rcount] = 0;
		logStream (MESSAGE_DEBUG) << "APGTO tel_write_read: read " <<
			tmp_rcount << " " << buf << sendLog;
		free (buf);
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "APGTO tel_write_read: read returns " <<
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

	if (tcflush (tel_desc, TCIOFLUSH) < 0)
		return -1;
	if (tel_write (wbuf, wcount) < 0)
		return -1;

	tmp_rcount = tel_read_hash (rbuf, rcount);

	return tmp_rcount;
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
	char wbuf[11];
	if (tel_write_read_hash (command, strlen (command), wbuf, 10) < 6)
		return -1;
	*hmsptr = hmstod (wbuf);
	if (errno)
		return -1;
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
	if (tel_read_hms (&new_ra, "#:GR#"))
		return -1;
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
	if (tel_read_hms (&t_telDec, "#:GD#"))
		return -1;
	setTelDec (t_telDec);
	return 0;
}
/*!
 * Reads APGTO latitude.
 *
 * @return -1 on error, otherwise 0
 *
 * MY EDIT APGTO latitude
 *
 * Hardcode latitude and return 0
 */
int
APGTO::tel_read_latitude ()
{
	return 0;
}
/*!
 * Reads APGTO longtitude.
 *
 * @return -1 on error, otherwise 0
 *
 * MY EDIT APGTO longtitude
 *
 * Hardcode longtitude and return 0
 */
int
APGTO::tel_read_longtitude ()
{
	return 0;
}
/*!
 * Repeat APGTO write.
 *
 * Handy for setting ra and dec.
 * Meade tends to have problems with that, don't know about APGTO.
 *
 * @param command	command to write on tel_desc
 */
int
APGTO::tel_rep_write (char *command)
{
	int count;
	char retstr;
	for (count = 0; count < 200; count++)
	{
		if (tel_write_read (command, strlen (command), &retstr, 1) < 0)
			return -1;
		if (retstr == '1')
			break;
		sleep (1);
		logStream (MESSAGE_DEBUG) << "APGTO tel_rep_write - for " << count <<
			" time" << sendLog;
	}
	if (count == 200)
	{
		logStream (MESSAGE_ERROR) <<
			"APGTO tel_rep_write unsucessful due to incorrect return." << sendLog;
		return -1;
	}
	return 0;
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
	if (*ra < 0)
								 //normalize ra
		*ra = floor (*ra / 360) * -360 + *ra;
	if (*ra > 360)
		*ra = *ra - floor (*ra / 360) * 360;

	if (*dec < -90)
								 //normalize dec
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
  if (ra < 0 || ra > 360.0)
    {
      return -1;
    }
  ra = ra / 15;
  dtoints (ra, &h, &m, &s);
  // Astro-Physics format
  if( snprintf(command, sizeof( command), "#:Sr %02d:%02d:%02d#", h, m, s)<0)
    return -1;
  return tel_rep_write (command);
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
  if (dec < -90.0 || dec > 90.0)
    {
      return -1;
    }
  dtoints (dec, &d, &m, &s);

  // Astro-Phiscs formt 
  if (!d && dec < 0)
    {
      ret= snprintf( command, sizeof( command), "#:Sd -%02d*%02d:%02d#", d, m, s);
    }
  else
    {
      ret= snprintf( command, sizeof( command), "#:Sd %+03d*%02d:%02d#", d, m, s);
    }
  if( ret < 0) 
    return -1;

  return tel_rep_write (command);
}
APGTO::APGTO (int in_argc, char **in_argv):Telescope (in_argc,in_argv)
{
	device_file = "/dev/apmount";

	addOption ('f', "device_file", 1, "device file");
	//	addOption (OPT_APGTO_INIT, "init", 1, "mount initialisation, after power cycle: cold, else warm");

	// get values from config file
	//	status = opentplConn->getValueDouble ("LOCAL.LATITUDE", telLatitude, &status);
 
	// object
	createValue (APAltAz, "APAltAz", "AP mount Alt/Az[deg]", true, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE, 0);
	APAltAz->setValueAltAz ( 30.01123, 40.0223);

	// Tracking mode sidereal and zero
	// ev. Sync :CM# (:CMR#)
	// ev. slew rate

	createValue (APlst,  "APLST",  "AP mount local sidereal time", true, RTS2_DT_RA);
	createValue (APlot,  "APLOT",  "AP mount local time", true, RTS2_DT_RA);
	createValue (APutco, "APUTCO", "AP mount UTC offset", true, RTS2_DT_RA);

	createValue (APLatitude,  "APLATITUDE",  "AP mount latitude", true, RTS2_DT_DEGREES);
	createValue (APLongitude, "APLONGITUDE", "AP mount longitude", true, RTS2_DT_DEGREES);


	// ev. firmware
	// realtive position of the Dec axis

	tel_desc = -1;
}
APGTO::~APGTO (void)
{
	close (tel_desc);
}
int
APGTO::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		case OPT_APGTO_INIT:
			strcpy( initialisation, optarg) ;
			break;
		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}
/*!
 * Init telescope, connect on given tel_desc.
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

	tel_desc = open (device_file, O_RDWR);

	if (tel_desc < 0)
		return -1;

	if (tcgetattr (tel_desc, &tel_termios) < 0)
		return -1;

	if (cfsetospeed (&tel_termios, B9600) < 0 ||
		cfsetispeed (&tel_termios, B9600) < 0)
		return -1;

	tel_termios.c_iflag = IGNBRK & ~(IXON | IXOFF | IXANY);
	tel_termios.c_oflag = 0;
	tel_termios.c_cflag =
		((tel_termios.c_cflag & ~(CSIZE)) | CS8) & ~(PARENB | PARODD);
	tel_termios.c_lflag = 0;
	tel_termios.c_cc[VMIN] = 0;
	tel_termios.c_cc[VTIME] = 5;

	if (tcsetattr (tel_desc, TCSANOW, &tel_termios) < 0)
	{
		logStream (MESSAGE_ERROR) << "APGTO init tcsetattr" << sendLog;
		return -1;
	}
	// get current state of control signals
	ioctl (tel_desc, TIOCMGET, &status);

	// Drop DTR
	status &= ~TIOCM_DTR;
	ioctl (tel_desc, TIOCMSET, &status);

	logStream (MESSAGE_DEBUG) << "APGTO init RS 232 initialization complete on port " << device_file << sendLog;
	logStream (MESSAGE_DEBUG) << "" << sendLog;

	int ret = -1 ;
	if(( ret= setBasicData()) != 0)
	  {
	    // wildi Todo: set errno! 
	    return -1 ;
	  }
	return 0;
}
/*!
 * Reads information about telescope.
 */
int
APGTO::initValues ()
{
	if (tel_read_longtitude () || tel_read_latitude ())
		return -1;

	strcpy (telType, "APGTO");

	// wildi ToDo:
	telAltitude->setValueDouble (600);

	// wildi ToDo:
	telFlip->setValueInteger (0);

	return Telescope::initValues ();
}
int
APGTO::info ()
{
	if (tel_read_ra () || tel_read_dec ())
		return -1;

	return Telescope::info ();
}
/*!
 * Set slew rate. For completness?
 *
 * This functions are there IMHO mainly for historical reasons. They
 * don't have any use, since if you start move and then die, telescope
 * will move forewer till it doesn't hurt itself. So it's quite
 * dangerous to use them for automatic observation. Better use quide
 * commands from attached CCD, since it defines timeout, which rules
 * CCD.
 *
 * @param new_rate	new rate to set. Uses RATE_<SPEED> constant.
 *
 * @return -1 on failure & set errno, 5 (>=0) otherwise
 */
int
APGTO::tel_set_rate (char new_rate)
{
  // wildi, there are further tracking modes (lunar, solar) not used here
  // IDLog("selectAPTrackingMode: Setting tracking mode to sidereal.\n");
  // if ( (error_type = tel_write( "#:RT2#", &nbytes_write)) != TTY_OK)
  // 	return error_type;
  // break;


  char command[6];
  sprintf (command, "#:R%c#", new_rate);
  return tel_write (command, 5);
}
int
APGTO::telescope_start_move (char direction)
{

  // wildi: ToDo
	char command[6];
	tel_set_rate (RATE_FIND);
	sprintf (command, "#:M%c#", direction);
	return tel_write (command, 5) == 1 ? -1 : 0;
}

int
APGTO::telescope_stop_move (char direction)
{
  // wildi: ToDo
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
	char retstr;

	tel_normalize (&ra, &dec);

	if (tel_write_ra (ra) < 0 || tel_write_dec (dec) < 0)
		return -1;
	// wildi ToDo
	if (tel_write_read ("#:MS#", 5, &retstr, 1) < 0)
		return -1;

	if (retstr == '0')
		return 0;

	return -1;
}
/*!
 * Check, if telescope match given coordinates.
 *
 * @param ra		target right ascenation
 * @param dec		target declination
 *
 * @return -1 on error, 0 if not matched, 1 if matched, 2 if timeouted
 */
int
APGTO::tel_check_coords (double ra, double dec)
{
	// ADDED BY JF
	double JD;
	double sep;
	time_t now;

	struct ln_equ_posn object, target;
	struct ln_lnlat_posn observer;
	struct ln_hrz_posn hrz;

	time (&now);
	if (now > move_timeout)
		return 2;
	if ((tel_read_ra () < 0) || (tel_read_dec () < 0))
		return -1;
	// ADDED BY JF
	// CALCULATE & PRINT ALT/AZ & HOUR ANGLE TO LOG
	object.ra = getTelRa ();
	object.dec = getTelDec ();

	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();

	JD = ln_get_julian_from_sys ();
	ln_get_hrz_from_equ (&object, &observer, JD, &hrz);

	logStream (MESSAGE_DEBUG) << "APGTO tel_check_coords TELESCOPE ALT " << hrz.alt << " AZ " << hrz.az << sendLog;

	target.ra = ra;
	target.dec = dec;

	sep = ln_get_angular_separation (&object, &target);

	logStream (MESSAGE_ERROR) << "tel_check_coords separation >" <<sep << "<" << sendLog;
	if (sep > 0.1)
		return 0;
	return 1;
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
 	{
 		return -1;
 	}

 	set_move_timeout (100);
 	return 0;
}
int
APGTO::stopMove ()
{
  // wildi: ToDo
	char dirs[] = { 'e', 'w', 'n', 's' };
	int i;
	for (i = 0; i < 4; i++)
	{
		if (telescope_stop_move (dirs[i]) < 0)
			return -1;
	}
	return 0;
}
/*!
 * Set telescope to match given coordinates
 *
 * This function is mainly used to tell the telescope, where it
 * actually is at the beggining of observation (remember, that APGTO
 * doesn't have absolute position sensors)
 *
 * @param ra		setting right ascennation
 * @param dec		setting declination
 *
 * @return -1 and set errno on error, otherwise 0
 */
// wildi ToDo this is SYNC!!
int
APGTO::setTo (double ra, double dec)
{
	char readback[101];
	int ret;

	tel_normalize (&ra, &dec);
	if ((tel_write_ra (ra) < 0) || (tel_write_dec (dec) < 0))
		return -1;
	// wildi: ToDo
	if (tel_write_read_hash ("#:CM#", 5, readback, 100) < 0)
		return -1;
	// since we are carring operation critical for next movements of telescope,
	// we are obliged to check its correctness
	set_move_timeout (10);
	ret = tel_check_coords (ra, dec);
	return ret == 1;
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
APGTO::correct (double cor_ra, double cor_dec, double real_ra,
double real_dec)
{
	if (setTo (real_ra, real_dec))
		return -1;
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
  // wildi: ToDo : homing in on Az=south east 0 Alt=0, not EQ
	return tel_slew_to (0, 0);
}
int
APGTO::isParking ()
{
	return -2;
}
int
APGTO::endPark ()
{
	return 0;
}
int
APGTO::startDir (char *dir)
{
	switch (*dir)
	{
		case DIR_EAST:
		case DIR_WEST:
		case DIR_NORTH:
		case DIR_SOUTH:
		  // wildi: ToDo
			tel_set_rate (RATE_FIND);
			return telescope_start_move (*dir);
	}
	return -2;
}


int
APGTO::stopDir (char *dir)
{
	switch (*dir)
	{
		case DIR_EAST:
		case DIR_WEST:
		case DIR_NORTH:
		case DIR_SOUTH:
			return telescope_stop_move (*dir);
	}
	return -2;
}
int APGTO::setBasicData()
{
    int err ;
    struct ln_date utm;
    struct ln_zonedate ltm;
    logStream (MESSAGE_DEBUG) << "----APGTO::setBasicData in" << sendLog;

    if(setAPClearBuffer() < 0)
    {
      logStream (MESSAGE_ERROR) << "error clearing the buffer" << sendLog;
      return -1;
    }
    if(setAPLongFormat() < 0)
    {
      logStream (MESSAGE_ERROR) << "setting long format failed" << sendLog;
      return -1;
    }
    
    if(setAPBackLashCompensation(0,0,0) < 0)
    {
      logStream (MESSAGE_ERROR) << "setting back lash compensation" << sendLog;
      return -1;
    }
    ln_get_date_from_sys( &utm) ;
    ln_date_to_zonedate(&utm, &ltm, 3600); // Adds "only" offset to JD and converts back (see DST below)

    if((  err = setLocalTime(ltm.hours, ltm.minutes, (int) ltm.seconds) < 0))
    {
      logStream (MESSAGE_ERROR) << "setting local time" << sendLog;
      return -1;
    }
    if ( ( err = setCalenderDate(ltm.days, ltm.months, ltm.years) < 0) )
    {
      logStream (MESSAGE_ERROR) << "setting local date"<< sendLog;
      return -1;
    }
    // wildi ToDo: logStream (MESSAGE_DEBUG) << "" << sendLog;
    fprintf(stderr, "UT time is: %04d/%02d/%02d T %02d:%02d:%02d\n", utm.years, utm.months, utm.days, utm.hours, utm.minutes, (int)utm.seconds);
    fprintf(stderr, "Local time is: %04d/%02d/%02d T %02d:%02d:%02d\n", ltm.years, ltm.months, ltm.days, ltm.hours, ltm.minutes, (int)ltm.seconds);
    // wildi: ToDo 2x
    if ( ( err = setAPSiteLongitude(352.514008) < 0) ) // AP mount: positive and only to the to west
    {
      logStream (MESSAGE_ERROR) << "setting site coordinates" << sendLog;
      return -1;
    }
    if ( ( err = setAPSiteLatitude(47.332189) < 0) )
    {
      logStream (MESSAGE_ERROR) << "setting site coordinates"<< sendLog;
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

    if((err = setAPUTCOffset( -1.065)) < 0)
    {
      logStream (MESSAGE_ERROR) << "setting AP UTC offset" << sendLog;
      return -1;
    }
    if((err = setAPUnPark()) < 0)
    {
      logStream (MESSAGE_ERROR) << "unparking failed"<< sendLog;
      return -1;
    }
    logStream (MESSAGE_DEBUG) << "unparking successful" << sendLog;
    if((err = setAPMotionStop()) < 0)
    {
      logStream (MESSAGE_ERROR) << "stop motion (:Q#) failed, check the mount"<< sendLog;
      return -1;
    }
    logStream (MESSAGE_DEBUG) << "Stopped any motion (:Q#)" << sendLog;
    // retrieve basic data and write it to the log




    logStream (MESSAGE_DEBUG) << ">>>>APGTO::setBasicData out" << sendLog;

    return 0 ;
}
void APGTO::ParkDisconnect(int *established)
{
  // ToDo handle return value
  // wildi ToDo: abortSlew(tel_desc);
  // sleep for 200 mseconds
  usleep(200000);
  
  if ( selectAPTrackingMode(3) < 0 ) /* Tracking Mode 3 = zero */
    {
      logStream (MESSAGE_ERROR) << "FAILED: Setting tracking mode ZERO." << sendLog;
      return;
    }
// The AP mount will not surely stop with #:KA alone
  if (setAPMotionStop() < 0)
    {
      logStream (MESSAGE_ERROR) << "motion stop  failed (#:Q)" << sendLog;
      return;
    }
  if (setAPPark() < 0)
    {
      logStream (MESSAGE_ERROR) << "parking failed (#:KA)" << sendLog;
      return;
    }
  logStream (MESSAGE_DEBUG) << "the telescope is parked and disconnected. Turn off the telescope (a power cycle is required)." << sendLog; 
  return;
}
int
main (int argc, char **argv)
{
	APGTO device = APGTO (argc, argv);
	return device.run ();
}
