/* 
 * Receive and reacts to Auger showers.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#include "connshooter.h"
#include "../utils/libnova_cpp.h"

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

#include <libnova/libnova.h>

EXEC SQL include sqlca;

#define GPS_OFFSET  315964800

using namespace rts2grbd;

void ConnShooter::getTimeTfromGPS (long GPSsec, long GPSusec, double &out_time)
{
  // we need to handle somehow better leap seconds, but that can wait
  out_time = GPSsec + GPS_OFFSET + GPSusec / USEC_SEC + 14.0;
}


// is called when nbuf contains '\n'
int ConnShooter::processAuger ()
{
  EXEC SQL BEGIN DECLARE SECTION;
    //  int db_auger_t3id;
    double db_auger_date;
  
    int    db_Eye;            /// FD eye Id
    int    db_Run;            /// FD run number
    int    db_Event;          /// FD event number
    VARCHAR db_AugerId[50];   /// Event Auger Id after SD/FD merger
    int    db_GPSSec;         /// GPS second (SD)
    int    db_GPSNSec;        /// GPS nano second (SD)
    int    db_SDId;           /// SD Event Id

    int    db_NPix;           /// Num. pixels with a pulse after FdPulseFinder

    double db_SDPTheta;       /// Zenith angle of SDP normal vector (deg)
    double db_SDPThetaErr;    /// Uncertainty of SDPtheta
    double db_SDPPhi;         /// Azimuth angle of SDP normal vector (deg)
    double db_SDPPhiErr;      /// Uncertainty of SDPphi
    double db_SDPChi2;        /// Chi^2 of SDP db_it
    int    db_SDPNdf;         /// Degrees of db_reedom of SDP db_it

    double db_Rp;             /// Shower impact parameter Rp (m)
    double db_RpErr;          /// Uncertainty of Rp (m)
    double db_Chi0;           /// Angle of shower in the SDP (deg)
    double db_Chi0Err;        /// Uncertainty of Chi0 (deg)
    double db_T0;             /// FD time db_it T_0 (ns)
    double db_T0Err;          /// Uncertainty of T_0 (ns)
    double db_TimeChi2;       /// Full Chi^2 of axis db_it
    double db_TimeChi2FD;     /// Chi^2 of axis db_it (FD only)
    int    db_TimeNdf;        /// Degrees of db_reedom of axis db_it

    double db_Easting;        /// Core position in easting coordinate (m)
    double db_Northing;       /// Core position in northing coordinate (m)
    double db_Altitude;       /// Core position altitude (m)
    double db_NorthingErr;    /// Uncertainty of northing coordinate (m)
    double db_EastingErr;     /// Uncertainty of easting coordinate (m)
    double db_Theta;          /// Shower zenith angle in core coords. (deg)
    double db_ThetaErr;       /// Uncertainty of zenith angle (deg)
    double db_Phi;            /// Shower azimuth angle in core coords. (deg)
    double db_PhiErr;         /// Uncertainty of azimuth angle (deg)

    double db_dEdXmax;        /// Energy deposit at shower max (GeV/(g/cm^2))
    double db_dEdXmaxErr;     /// Uncertainty of Nmax (GeV/(g/cm^2))
    double db_X_max;           /// Slant depth of shower maximum (g/cm^2)
    double db_X_maxErr;        /// Uncertainty of Xmax (g/cm^2)
    double db_X0;             /// X0 Gaisser-Hillas db_it (g/cm^2)
    double db_X0Err;          /// Uncertainty of X0 (g/cm^2)
    double db_Lambda;         /// Lambda of Gaisser-Hillas db_it (g/cm^2)
    double db_LambdaErr;      /// Uncertainty of Lambda (g/cm^2)
    double db_GHChi2;         /// Chi^2 of Gaisser-Hillas db_it
    int    db_GHNdf;          /// Degrees of db_reedom of GH db_it
    double db_LineFitChi2;    /// Chi^2 of linear db_it to profile

    double db_EmEnergy;       /// Calorimetric energy db_rom GH db_it (EeV)
    double db_EmEnergyErr;    /// Uncertainty of Eem (EeV)
    double db_Energy;         /// Total energy db_rom GH db_it (EeV)
    double db_EnergyErr;      /// Uncertainty of Etot (EeV)

    double db_MinAngle;       /// Minimum viewing angle (deg)
    double db_MaxAngle;       /// Maximum viewing angle (deg)
    double db_MeanAngle;      /// Mean viewing angle (deg)

    int    db_NTank;          /// Number of stations in hybrid db_it
    int    db_HottestTank;    /// Station used in hybrid-geometry reco
    double db_AxisDist;       /// Shower axis distance to hottest station (m)
    double db_SDPDist;        /// SDP distance to hottest station (m)

    double db_SDFDdT;         /// SD/FD time offset after the minimization (ns)
    double db_XmaxEyeDist;    /// Distance to shower maximum (m)
    double db_XTrackMin;      /// First recorded slant depth of track (g/cm^2)
    double db_XTrackMax;      /// Last recorded slant depth of track (g/cm^2)
    double db_XFOVMin;        /// First slant depth inside FOV (g/cm^2)
    double db_XFOVMax;        /// Last slant depth inside FOV (g/cm^2)
    double db_XTrackObs;      /// Observed track length depth (g/cm^2)
    double db_DegTrackObs;    /// Observed track length angle (deg)
    double db_TTrackObs;      /// Observed track length time (100 ns)

  EXEC SQL END DECLARE SECTION;

  time_t now;

  std::string AugerId;

  std::istringstream _is (nbuf);
  _is >> db_Eye
    >> db_Run
    >> db_Event
    >> AugerId
    >> db_GPSSec
    >> db_GPSNSec
    >> db_SDId
    >> db_NPix
    >> db_SDPTheta
    >> db_SDPThetaErr
    >> db_SDPPhi
    >> db_SDPPhiErr
    >> db_SDPChi2
    >> db_SDPNdf
    >> db_Rp
    >> db_RpErr
    >> db_Chi0
    >> db_Chi0Err
    >> db_T0
    >> db_T0Err
    >> db_TimeChi2
    >> db_TimeChi2FD
    >> db_TimeNdf
    >> db_Easting
    >> db_Northing
    >> db_Altitude
    >> db_NorthingErr
    >> db_EastingErr
    >> db_Theta
    >> db_ThetaErr
    >> db_Phi
    >> db_PhiErr
    >> db_dEdXmax
    >> db_dEdXmaxErr
    >> db_X_max
    >> db_X_maxErr
    >> db_X0
    >> db_X0Err
    >> db_Lambda
    >> db_LambdaErr
    >> db_GHChi2
    >> db_GHNdf
    >> db_LineFitChi2
    >> db_EmEnergy
    >> db_EmEnergyErr
    >> db_Energy
    >> db_EnergyErr
    >> db_MinAngle
    >> db_MaxAngle
    >> db_MeanAngle
    >> db_NTank
    >> db_HottestTank
    >> db_AxisDist
    >> db_SDPDist
    >> db_SDFDdT
    >> db_XmaxEyeDist
    >> db_XTrackMin
    >> db_XTrackMax
    >> db_XFOVMin
    >> db_XFOVMax
    >> db_XTrackObs
    >> db_DegTrackObs
    >> db_TTrackObs;

  if (_is.fail ())
  {
    logStream (MESSAGE_ERROR) << "Rts2ConnShooter::processAuger failed reading stream" << sendLog;
    return -1;
  }

  getTimeTfromGPS (db_GPSSec, db_GPSNSec, db_auger_date);
  strncpy (db_AugerId.arr, AugerId.c_str (), 50);
  db_AugerId.len = (50 > AugerId.length () ? AugerId.length () : 50);

  time (&now);

  // validate shover and it's hibrid..

/*  if ((!(gap_comp && gap_isT5 && gap_energy > minEnergy))
    || now - db_auger_date > maxTime
    || master->wasSeen (db_auger_date, db_auger_ra, db_auger_dec))
  {
    logStream (MESSAGE_INFO) << "Rts2ConnShooter::processAuger ignore (gap_comp "
      << gap_comp
      << " date " << LibnovaDateDouble (db_auger_date)
      << " gap_isT5 " << gap_isT5
      << " gap_energy " << gap_energy
      << " minEnergy " << minEnergy
      << " ra " << db_auger_ra
      << " dec " << db_auger_dec
      << ")" << sendLog;
    return -1;
  } */

  EXEC SQL INSERT INTO
      auger
      (
      	auger_t3id,
	auger_date,
        eye,
	run,
	event,
	augerid, 
	GPSSec, 
	GPSNSec, 
	SDId, 
	NPix,
	SDPTheta,
	SDPThetaErr,
	SDPPhi,
	SDPPhiErr,
	SDPChi2,
	SDPNd, 
	Rp,
	RpErr,
	Chi0,
	Chi0Err,
	T0,
	T0Err,
	TimeChi2,
	TimeChi2FD,
	TimeNdf,	
	Easting,
	Northing,
	Altitude,
	NorthingErr,
	EastingErr,
	Theta,
	ThetaErr,
	Phi,
	PhiErr,
	dEdXmax,
	dEdXmaxErr,
	X_max,
	X_maxErr,
	X0,
	X0Err,
	Lambda,
	LambdaErr,
	GHChi2,
	GHNdf,
	LineFitChi2,
	EmEnergy,
	EmEnergyErr,
	Energy,
	EnergyErr,
	MinAngle,
	MaxAngle,
	MeanAngle,
	NTank,
	HottestTank,
	AxisDist,
	SDPDist,
	SDFDdT,
	XmaxEyeDist,
	XTrackMin,
	XTrackMax,
	XFOVMin,
	XFOVMax,
	XTrackObs,
	DegTrackObs,
	TTrackObs
     )	
    VALUES
      (
        nextval('auger_t3id'),
        (TIMESTAMP 'epoch' + :db_auger_date * INTERVAL '1 seconds'),
	:db_Eye,
	:db_Run,
	:db_Event,
	:db_AugerId,
	:db_GPSSec,
	:db_GPSNSec,
	:db_SDId,
	:db_NPix,
	:db_SDPTheta,
	:db_SDPThetaErr,
	:db_SDPPhi,
	:db_SDPPhiErr,
	:db_SDPChi2,
	:db_SDPNdf,
	:db_Rp,
	:db_RpErr,
	:db_Chi0,
	:db_Chi0Err,
	:db_T0,
	:db_T0Err,
	:db_TimeChi2,
	:db_TimeChi2FD,
	:db_TimeNdf,
	:db_Easting,
	:db_Northing,
	:db_Altitude,
	:db_NorthingErr,
	:db_EastingErr,
	:db_Theta,
	:db_ThetaErr,
	:db_Phi,
	:db_PhiErr,
	:db_dEdXmax,
	:db_dEdXmaxErr,
	:db_X_max,
	:db_X_maxErr,
	:db_X0,
	:db_X0Err,
	:db_Lambda,
	:db_LambdaErr,
	:db_GHChi2,
	:db_GHNdf,
	:db_LineFitChi2,
	:db_EmEnergy,
	:db_EmEnergyErr,
	:db_Energy,
	:db_EnergyErr,
	:db_MinAngle,
	:db_MaxAngle,
	:db_MeanAngle,
	:db_NTank,
	:db_HottestTank,
	:db_AxisDist,
	:db_SDPDist,
	:db_SDFDdT,
	:db_XmaxEyeDist,
	:db_XTrackMin,
	:db_XTrackMax,
	:db_XFOVMin,
	:db_XFOVMax,
	:db_XTrackObs,
	:db_DegTrackObs,
	:db_TTrackObs
      );
  if (sqlca.sqlcode)
  {
    logStream (MESSAGE_ERROR)
      << "Rts2ConnShooter::processAuger cannot add new value to db: "
      << sqlca.sqlerrm.sqlerrmc << " (" << sqlca.sqlcode << ")";
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
//  return master->newShower (db_auger_date, db_auger_ra, db_auger_dec);
}


ConnShooter::ConnShooter (int in_port, DevAugerShooter * in_master, double in_minEnergy, int in_maxTime):Rts2ConnNoSend (in_master)
{
  master = in_master;
  port = in_port;

  time (&last_packet.tv_sec);
  last_packet.tv_sec -= 600;
  last_packet.tv_usec = 0;

  last_target_time = -1;

  setConnTimeout (-1);

  minEnergy = in_minEnergy;
  maxTime = in_maxTime;
}


ConnShooter::~ConnShooter (void)
{
}


int ConnShooter::idle ()
{
  int ret;
  int err;
  socklen_t len = sizeof (err);

  time_t now;
  time (&now);

  switch (getConnState ())
  {
    case CONN_CONNECTING:
      ret = getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &len);
      if (ret)
      {
        logStream (MESSAGE_ERROR) << "Rts2ConnShooter::idle getsockopt " << strerror (errno) << sendLog;
        connectionError (-1);
      }
      else if (err)
      {
        logStream (MESSAGE_ERROR) << "Rts2ConnShooter::idle getsockopt " << strerror (err) << sendLog;
        connectionError (-1);
      }
      else
      {
        setConnState (CONN_CONNECTED);
      }
      break;
    case CONN_CONNECTED:
      // mayby handle connection error?
      break;
    default:
      break;
  }
  // we don't like to get called upper code with timeouting stuff..
  return 0;
}


int ConnShooter::init_listen ()
{
  int ret;

  connectionError (-1);

  sock = socket (PF_INET, SOCK_DGRAM, 0);
  if (sock == -1)
  {
    logStream (MESSAGE_ERROR) << "Rts2ConnShooter::init_listen socket " << strerror (errno) << sendLog;
    return -1;
  }
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons (port);
  server.sin_addr.s_addr = htonl (INADDR_ANY);

  ret = fcntl (sock, F_SETFL, O_NONBLOCK);
  if (ret)
  {
    logStream (MESSAGE_ERROR) << "Rts2ConnShooter::init_listen fcntl: " << strerror (errno) << sendLog;
    return -1;
  }

  ret =
    bind (sock, (struct sockaddr *) &server, sizeof (server));
  if (ret)
  {
    logStream (MESSAGE_ERROR) << "Rts2ConnShooter::init_listen bind: " << strerror (errno) << sendLog;
    return -1;
  }
  setConnState (CONN_CONNECTED);
  return 0;
}


int ConnShooter::init ()
{
  return init_listen ();
}


void ConnShooter::connectionError (int last_data_size)
{
  logStream (MESSAGE_DEBUG) << "Rts2ConnShooter::connectionError" << sendLog;
  if (sock > 0)
  {
    close (sock);
    sock = -1;
  }
  if (!isConnState (CONN_BROKEN))
  {
    sock = -1;
    setConnState (CONN_BROKEN);
  }
}


int ConnShooter::receive (fd_set * set)
{
  int ret = 0;
  if (sock >= 0 && FD_ISSET (sock, set))
  {
    ret = read (sock, nbuf, sizeof (nbuf));
    if (ret == 0 && isConnState (CONN_CONNECTING))
    {
      setConnState (CONN_CONNECTED);
    }
    else if (ret <= 0)
    {
      connectionError (ret);
      return -1;
    }
    nbuf[ret] = '\0';
    processAuger ();
    logStream (MESSAGE_DEBUG) << "Rts2ConnShooter::receive data: " << nbuf << sendLog;
    successfullRead ();
    gettimeofday (&last_packet, NULL);
    // enable others to catch-up (FW connections will forward packet to their sockets)
    getMaster ()->postEvent (new Rts2Event (RTS2_EVENT_AUGER_SHOWER, nbuf));
  }
  return ret;
}


int ConnShooter::lastPacket ()
{
  return last_packet.tv_sec;
}


double ConnShooter::lastTargetTime ()
{
  return last_target_time;
}
