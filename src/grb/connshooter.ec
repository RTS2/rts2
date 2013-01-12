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
#include "libnova_cpp.h"

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
	out_time = GPSsec + GPS_OFFSET + GPSusec / (1000 * USEC_SEC) + 14.0;
}


// is called when nbuf contains '\n'
int ConnShooter::processAuger ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	//  int db_auger_t3id;
	double db_auger_date;

	double db_ra;             /// Shower ra
	double db_dec;            /// Shower dec
  
	int    db_Eye;            /// FD eye Id
	int    db_Run;            /// FD run number
	int    db_Event;          /// FD event number
	VARCHAR db_AugerId[50];   /// rts2core::Event Auger Id after SD/FD merger
	int    db_GPSSec;         /// GPS second (SD)
	int    db_GPSNSec;        /// GPS nano second (SD)
	int    db_SDId;           /// SD rts2core::Event Id

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
	double db_Xmax;           /// Slant depth of shower maximum (g/cm^2)
	double db_XmaxErr;        /// Uncertainty of Xmax (g/cm^2)
	double db_X0;             /// X0 Gaisser-Hillas db_it (g/cm^2)
	double db_X0Err;          /// Uncertainty of X0 (g/cm^2)
	double db_Lambda;         /// Lambda of Gaisser-Hillas db_it (g/cm^2)
	double db_LambdaErr;      /// Uncertainty of Lambda (g/cm^2)
	double db_GHChi2;         /// Chi^2 of Gaisser-Hillas db_it
	int    db_GHNdf;          /// Degrees of db_reedom of GH db_it

	double db_DGHXmax1;
	double db_DGHXmax2;
	double db_DGHChi2Improv;

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
	VARCHAR db_ShowerParams[2000];

	int db_cut = 0;

	EXEC SQL END DECLARE SECTION;

	time_t now;

	std::string AugerId;
	std::string YYMMDD;
	std::string HHMMSS;
	std::string TelEvtBits;
	std::string TelDAQBits;

	double MoonCycle;
	double RecLevel;
	double RhoPT;
	double RA;
	double Dec;
	double RAErr;
	double DecErr;
	double RhoRD;
	double GalLong;
	double GalLat;
	double GalLongErr;
	double GalLatErr;
	double RhoLL;
	double ChkovFrac;
	double EyeXmaxAtt;
	double MieDatabase;
	double BadPeriod;
	double dEdX;

	char rest[2000];

	std::istringstream _is (nbuf);
	_is >> db_Eye
		>> db_Run
		>> db_Event
		>> AugerId
		>> db_GPSSec
		>> db_GPSNSec
		>> YYMMDD
		>> HHMMSS
		>> db_SDId
		>> TelEvtBits
		>> TelDAQBits
		>> MoonCycle
		>> RecLevel
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
		>> RhoPT
		>> RA
		>> Dec
		>> RAErr
		>> DecErr
		>> RhoRD
		>> GalLong
		>> GalLat
		>> GalLongErr
		>> GalLatErr
		>> RhoLL
		>> db_dEdXmax
		>> db_dEdXmaxErr
		>> db_Xmax
		>> db_XmaxErr
		>> db_X0
		>> db_X0Err
		>> db_Lambda
		>> db_LambdaErr
		>> db_GHChi2
		>> db_GHNdf
		// 3 double
		>> db_DGHXmax1
		>> db_DGHXmax2
		>> db_DGHChi2Improv
		>> db_LineFitChi2
		>> db_EmEnergy
		>> db_EmEnergyErr
		>> db_Energy
		>> db_EnergyErr
		>> db_MinAngle
		>> db_MaxAngle
		>> db_MeanAngle
		>> ChkovFrac
		>> db_NTank
		>> db_HottestTank
		>> db_AxisDist
		>> db_SDPDist
		>> db_SDFDdT
		>> db_XmaxEyeDist
		>> EyeXmaxAtt
		>> db_XTrackMin
		>> db_XTrackMax
		>> db_XFOVMin
		>> db_XFOVMax
		>> db_XTrackObs
		>> db_DegTrackObs
		>> db_TTrackObs
		>> MieDatabase
		>> BadPeriod
		>> dEdX;

	if (_is.fail ())
	{
		logStream (MESSAGE_ERROR) << "Rts2ConnShooter::processAuger failed reading stream at " << _is.tellg () << " " << _is.str () << sendLog;
		return -1;
	}

	_is.getline (rest, 2000);

	getTimeTfromGPS (db_GPSSec, db_GPSNSec, db_auger_date);
	strncpy (db_AugerId.arr, AugerId.c_str (), 50);
	db_AugerId.len = (50 > AugerId.length () ? AugerId.length () : 50);

	strncpy (db_ShowerParams.arr, rest, 2000);
	db_ShowerParams.len = (2000 > strlen (rest) ? strlen (rest) : 2000);

	time (&now);

	struct ln_hrz_posn hrz;
	hrz.alt = 90 - db_Theta;
	hrz.az = db_Phi;

	time_t aug_date = db_auger_date;

	double JD = ln_get_julian_from_timet (&aug_date);

	struct ln_equ_posn equ;

	ln_get_equ_from_hrz (&hrz, rts2core::Configuration::instance ()->getObserver (), JD, &equ);

	db_ra = equ.ra;
	db_dec = equ.dec;

	// validate shover and it's hibrid..

	cutindex = 0;

	if (compare (db_Energy, CMP_GT, master->minEnergy->getValueDouble (), "Energy")
		&& compare (db_Eye, CMP_EQ, master->EyeId1->getValueInteger (), "Eye")
		&& compare ((int) (now - db_auger_date), CMP_LT, master->maxTime->getValueInteger (), "Time from last event")
		&& compare (!((DevAugerShooter *)master)->wasSeen (db_auger_date, db_ra, db_dec), CMP_EQ, true, "was not seen recently")
		&& compare (db_Xmax, CMP_GT, db_XFOVMin, "XFOVMin")
		&& compare (db_Xmax, CMP_LT, db_XFOVMax, "XFOVMax")
		&& compare (db_XmaxErr, CMP_LT, master->maxXmaxErr->getValueDouble (), "XmaxErr")
		&& compare (db_EnergyErr / db_Energy, CMP_LT, master->maxEnergyDiv->getValueDouble (), "EnergyErr to Energy ratio")
		&& compare (db_GHChi2 / db_GHNdf, CMP_LT, master->maxGHChiDiv->getValueDouble (), "GHChi2 to GHNdf ratio")
		&& compare ((db_LineFitChi2 - db_GHChi2), CMP_GT, master->minLineFitDiff->getValueDouble (), "LineFitDiff")
		&& compare (db_AxisDist, CMP_LT, master->maxAxisDist->getValueDouble (), "AxisDist")
		&& compare (db_Rp, CMP_GT, master->minRp->getValueDouble (), "Rp")
		&& compare (db_Chi0, CMP_GT, master->minChi0->getValueDouble (), "Chi0")
		&& compare ((db_SDPChi2 / db_SDPNdf), CMP_LT, master->maxSPDDiv->getValueDouble (), "SDPChi2 to SDPNdf ratio")
		&& compare ((db_TimeChi2 / db_TimeNdf), CMP_LT, master->maxTimeDiv->getValueDouble (), "TimeChi2 to TimeNDf ratio")
		&& compare (db_Theta, CMP_LT, master->maxTheta->getValueDouble (), "Theta")
	)
		db_cut |= 1;

	cutindex = 1;

	/*       second cuts set        */

	double CoreDist = 0.001 * db_Rp / sin (db_Chi0 / 57.295779513);

	if (compare (db_Energy, CMP_GT, master->minEnergy2->getValueDouble (), "Energy")
	        && compare (db_Eye, CMP_EQ, master->EyeId2->getValueInteger (), "Eye")
		&& compare (db_NPix, CMP_GT, master->minPix2->getValueInteger (), "NPix")
		&& compare (db_AxisDist, CMP_LT, master->maxAxisDist2->getValueDouble (), "AxisDist")
		&& compare (fabs(db_SDFDdT), CMP_LT, master->maxTimeDiff2->getValueDouble (), "SDFDdT")
		&& compare (db_GHChi2 / db_GHNdf, CMP_LT, master->maxGHChiDiv2->getValueDouble (), "GHChi2 to GHNdf ratio")
		&& compare ((db_GHChi2 / db_LineFitChi2), CMP_LT, master->maxLineFitDiv2->getValueDouble (), "GHChi2 to LineFitChi2 ratio")
		&& compare (db_Xmax, CMP_GT, db_XFOVMin, "XFovMin")
		&& compare (db_Xmax, CMP_LT, db_XFOVMax, "XFovMax")
		&& compare (db_MinAngle, CMP_GT, master->minViewAngle2->getValueDouble (), "MinAngle")
		&& compare ((((db_Theta >= 35. + 10.*(log10(db_Energy)-1.)) || (log10(db_Energy) > 1.7)) &&
			((db_Theta <= 42.) || (log10(db_Energy) <= 1.7))), CMP_EQ, true, "Theta and Energy cuts")
		&& compare ((((CoreDist >= 24. + 12.*(log10(db_Energy)-1.)) || (log10(db_Energy) < 1.)) &&
          		((CoreDist > 24. + 6.*(log10(db_Energy)-1.)) || (log10(db_Energy) >= 1.))), CMP_EQ, true, "CoreDist and Energy cuts")
	)
		db_cut |= 2;
	 /*       second set of cuts - end   */

	 cutindex = 2;

	 /*       third set of cuts          */

	if (compare (db_Energy, CMP_GT, master->minEnergy3->getValueDouble (), "Energy")
		&& compare (db_Eye, CMP_EQ, master->EyeId3->getValueInteger (), "Eye")
		&& compare (db_XmaxErr, CMP_LT, master->maxXmaxErr3->getValueDouble (), "XmaxErr")
		&& compare (db_EnergyErr / db_Energy, CMP_LT, master->maxEnergyDiv3->getValueDouble (), "EnergyErr to Energy ratio")
		&& compare (db_GHChi2 / db_GHNdf, CMP_LT, master->maxGHChiDiv3->getValueDouble (), "GHChi2 to GHNdf ratio")
		&& compare ((db_GHChi2 / db_LineFitChi2), CMP_LT, master->maxLineFitDiv3->getValueDouble (), "GHChi2 to LineFitChi2 ratio")
		&& compare (db_NPix, CMP_GT, master->minPix3->getValueInteger (), "NPix")
		&& compare (db_AxisDist, CMP_LT, master->maxAxisDist3->getValueDouble (), "AxisDist")
		&& compare (db_Rp, CMP_GT, master->minRp3->getValueDouble (), "Rp")
		&& compare (db_Chi0, CMP_GT, master->minChi03->getValueDouble (), "Chi0")
		&& compare ((db_SDPChi2 / db_SDPNdf), CMP_LT, master->maxSPDDiv3->getValueDouble (), "SDPChi2 to SDPNdf ratio")
		&& compare ((db_TimeChi2 / db_TimeNdf), CMP_LT, master->maxTimeDiv3->getValueDouble (), "TimeChi2 to TimeNdf ratio")
	)
		db_cut |= 4;
 	/*       third set of cuts - end    */


	cutindex = 3;

	/*       fourth set of cuts         */
	if (compare (db_NPix, CMP_GT, master->minNPix4->getValueInteger (), "NPix")
		&& compare (db_Eye, CMP_EQ, master->EyeId4->getValueInteger (), "Eye")
		&& compare (db_DGHChi2Improv, CMP_LT, master->maxDGHChi2Improv4->getValueDouble (), "DGHChi2Improv")
	)
		db_cut |= 8;
	/*       fourth set of cuts - end   */

	if (db_cut == 0)
	{
		logStream (MESSAGE_INFO) << "Rts2ConnShooter::processAuger ignore (date " << LibnovaDateDouble (db_auger_date)
			<< " Energy " << db_Energy
			<< " Eye " << db_Eye
			<< " minEnergy " << master->minEnergy->getValueDouble ()
			<< " Xmax " << db_Xmax
			<< " XFOVMin " << db_XFOVMin
			<< " XFOVMax " << db_XFOVMax
			<< " XmaxErr " << db_XmaxErr
			<< " maxXmaxErr " << master->maxXmaxErr->getValueDouble ()
			<< " EnergyErr " << db_EnergyErr
			<< " maxEnergyDiv " << master->maxEnergyDiv->getValueDouble ()
			<< " GHChi2 " << db_GHChi2
			<< " GHNdf " << db_GHNdf
			<< " DGHXmax1 " << db_DGHXmax1
			<< " DGHXmax2 " << db_DGHXmax2
			<< " DGHChi2Improv " << db_DGHChi2Improv
			<< " maxGHChiDiv " << master->maxGHChiDiv->getValueDouble ()
			<< " (LineFitChi2 " << db_LineFitChi2 << " - GHChi2 " << db_GHChi2 << ")"
			<< " minLineFitDiff " << master->minLineFitDiff->getValueDouble ()
			<< " AxisDist " << db_AxisDist
			<< " maxAxisDist " << master->maxAxisDist->getValueDouble ()
			<< " Rp " << db_Rp
			<< " minRp " << master->minRp->getValueDouble ()
			<< " Chi0 " << db_Chi0
			<< " minChi0 " << master->minChi0->getValueDouble ()
			<< " SDPChi2 " << db_SDPChi2
			<< " SDPNdf " << db_SDPNdf
			<< " maxSPDDiv " << master->maxSPDDiv->getValueDouble ()
			<< " TimeChi2 " << db_TimeChi2
			<< " TimeNdf " << db_TimeNdf
			<< " maxTimeDiv " << master->maxTimeDiv->getValueDouble ()
			<< " Theta " << db_Theta
			<< " maxTheta " << master->maxTheta->getValueDouble ()
			<< " ra " << db_ra
			<< " dec " << db_dec
			<< " params " << rest
			<< ")" << sendLog;
		for (int j = 0, i = 1; i < (1 << NUM_CUTS); i = i << 1, j++)
		{
			logStream (MESSAGE_INFO) << "cut set " << (j + 1) << " failed: " << failedCutsString (j) << sendLog;
			failedCuts[j].clear ();
		}
		((DevAugerShooter*) master)->rejectedShower (db_auger_date, db_ra, db_dec);
		return -1;
	}

	EXEC SQL INSERT INTO auger
	(
		auger_t3id,
		auger_date,
		ra,
		dec,
		cut,
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
		SDPNdf, 
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
		DGHXmax1,
		DGHXmax2,
		DGHChi2Improv,
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
		TTrackObs,
		ShowerParams
	)	
	VALUES
	(
		nextval('auger_t3id'),
		to_timestamp (:db_auger_date),
		:db_ra,
		:db_dec,
		:db_cut,
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
		:db_Xmax,
		:db_XmaxErr,
		:db_X0,
		:db_X0Err,
		:db_Lambda,
		:db_LambdaErr,
		:db_GHChi2,
		:db_GHNdf,
		:db_DGHXmax1,
		:db_DGHXmax2,
		:db_DGHChi2Improv,
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
		:db_TTrackObs,
		:db_ShowerParams
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
	last_auger_date = db_auger_date;
	last_auger_ra = db_ra;
	last_auger_dec = db_dec;
	return 0;
}

std::string ConnShooter::failedCutsString (int i)
{
  	std::ostringstream os;
	for (std::list < std::string >::iterator iter = failedCuts[i].begin (); iter != failedCuts[i].end (); iter++)
		os << *iter;
	return os.str ();
}

ConnShooter::ConnShooter (int _port, DevAugerShooter * _master):rts2core::ConnNoSend (_master)
{
	nbuf_pos = 0;
	port = _port;

	master = _master;

	time (&last_packet.tv_sec);
	last_packet.tv_sec -= 600;
	last_packet.tv_usec = 0;

	last_target_time = -1;

	setConnTimeout (-1);
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

	ret = bind (sock, (struct sockaddr *) &server, sizeof (server));
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
	logStream (MESSAGE_DEBUG) << "Rts2ConnShooter::connectionError " << last_data_size << sendLog;
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
		ret = read (sock, nbuf + nbuf_pos, sizeof (nbuf) - nbuf_pos);
		if (ret == 0 && isConnState (CONN_CONNECTING))
		{
			setConnState (CONN_CONNECTED);
		}
		else if (ret == 0)
		{
			logStream (MESSAGE_WARNING) << "read 0 bytes from augershooter connection - that should not happen " << (sizeof (nbuf) - nbuf_pos) << sendLog;
			return 0;
		}
		else if (ret < 0)
		{
			connectionError (ret);
			return -1;
		}
		int nbuf_end = nbuf_pos + ret;
		nbuf[nbuf_end] = '\0';
		// find '\n', mark it, copy buffer, mark next one,..
		char *it = nbuf;
		last_auger_date = NAN;
		while (*it)
		{
			if (*it == '\n')
			{
				*it = '\0';
				logStream (MESSAGE_DEBUG) << "Rts2ConnShooter::processing data: " << nbuf << sendLog;
				if (nbuf[0] != '#')
				{
					processAuger ();
					// enable others to catch-up (FW connections will forward packet to their sockets)
					getMaster ()->postEvent (new rts2core::Event (RTS2_EVENT_AUGER_SHOWER, nbuf));
				}

				// move unprocessed to begging
				it++;
				nbuf_end -= it - nbuf;
				memmove (nbuf, it, nbuf_end + 1);
				it = nbuf;
			}
			else
			{
				it++;
			}
		}

		nbuf_pos = it - nbuf;

		if (!isnan (last_auger_date))
		{
			((DevAugerShooter*) master)->newShower (last_auger_date, last_auger_ra, last_auger_dec);
		}

		successfullRead ();
		gettimeofday (&last_packet, NULL);
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

void ConnShooter::processBuffer (const char *_buf)
{
	strncpy (nbuf, _buf, AUGER_BUF_SIZE);
	processAuger ();
}
