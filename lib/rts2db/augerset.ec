/*
 * Sets of Auger targets.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "augerset.h"
#include "rts2db/sqlerror.h"
#include "rts2db/devicedb.h"

#include "configuration.h"

using namespace rts2db;

void AugerSet::load (double _from, double _to)
{
	if (checkDbConnection ())
		throw SqlError ();

	EXEC SQL BEGIN DECLARE SECTION;
	double d_from = _from;
	double d_to = _to;

	int d_auger_t3id;
	double d_auger_date;
	double d_auger_ra;
	double d_auger_dec;

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
	double db_Xmax;           /// Slant depth of shower maximum (g/cm^2)
	double db_XmaxErr;        /// Uncertainty of Xmax (g/cm^2)
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

	int db_cut = 0;

	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE cur_augerset CURSOR FOR
	SELECT
		auger_t3id,
		EXTRACT (EPOCH FROM auger_date),
		NPix,
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
	FROM
		auger
	WHERE
		auger_date BETWEEN to_timestamp (:d_from) and to_timestamp (:d_to)
	ORDER BY
		auger_date asc;

	EXEC SQL OPEN cur_augerset;

	while (1)
	{
		EXEC SQL FETCH next FROM cur_augerset INTO
			:d_auger_t3id,
			:d_auger_date,
			:d_auger_ra,
			:d_auger_dec,

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
			:db_TTrackObs;

		if (sqlca.sqlcode)
			break;

		TargetAuger tarAuger (d_auger_t3id, d_auger_date, d_auger_ra, d_auger_dec, rts2core::Configuration::instance ()->getObserver (), rts2core::Configuration::instance ()->getObservatoryAltitude ());

		tarAuger.cut = db_cut;
		tarAuger.Eye = db_Eye;
		tarAuger.Run = db_Run;
		tarAuger.Event = db_Event;
		tarAuger.AugerId = std::string (db_AugerId.arr);
		tarAuger.GPSSec = db_GPSSec;
		tarAuger.GPSNSec = db_GPSNSec;
		tarAuger.SDId = db_SDId;

		tarAuger.NPix = db_NPix;

		tarAuger.SDPTheta = db_SDPTheta;
		tarAuger.SDPThetaErr = db_SDPThetaErr;
		tarAuger.SDPPhi = db_SDPPhi;
		tarAuger.SDPPhiErr = db_SDPPhiErr;
		tarAuger.SDPChi2 = db_SDPChi2;
		tarAuger.SDPNdf = db_SDPNdf;

		tarAuger.Rp = db_Rp;
		tarAuger.RpErr = db_RpErr;
		tarAuger.Chi0 = db_Chi0;
		tarAuger.Chi0Err = db_Chi0Err;
		tarAuger.T0 = db_T0;
		tarAuger.T0Err = db_T0Err;
		tarAuger.TimeChi2 = db_TimeChi2;
		tarAuger.TimeChi2FD = db_TimeChi2FD;
		tarAuger.TimeNdf = db_TimeNdf;

		tarAuger.Easting = db_Easting;
		tarAuger.Northing = db_Northing;
		tarAuger.Altitude = db_Altitude;
		tarAuger.NorthingErr = db_NorthingErr;
		tarAuger.EastingErr = db_EastingErr;
		tarAuger.Theta = db_Theta;
		tarAuger.ThetaErr = db_ThetaErr;
		tarAuger.Phi = db_Phi;
		tarAuger.PhiErr = db_PhiErr;

		tarAuger.dEdXmax = db_dEdXmax;
		tarAuger.dEdXmaxErr = db_dEdXmaxErr;
		tarAuger.Xmax = db_Xmax;
		tarAuger.XmaxErr = db_XmaxErr;
		tarAuger.X0 = db_X0;
		tarAuger.X0Err = db_X0Err;
		tarAuger.Lambda = db_Lambda;
		tarAuger.LambdaErr = db_LambdaErr;
		tarAuger.GHChi2 = db_GHChi2;
		tarAuger.GHNdf = db_GHNdf;
		tarAuger.LineFitChi2 = db_LineFitChi2;

		tarAuger.EmEnergy = db_EmEnergy;
		tarAuger.EmEnergyErr = db_EmEnergyErr;
		tarAuger.Energy = db_Energy;
		tarAuger.EnergyErr = db_EnergyErr;

		tarAuger.MinAngle = db_MinAngle;
		tarAuger.MaxAngle = db_MaxAngle;
		tarAuger.MeanAngle = db_MeanAngle;

		tarAuger.NTank = db_NTank;
		tarAuger.HottestTank = db_HottestTank;
		tarAuger.AxisDist = db_AxisDist;
		tarAuger.SDPDist = db_SDPDist;

		tarAuger.SDFDdT = db_SDFDdT;
		tarAuger.XmaxEyeDist = db_XmaxEyeDist;
		tarAuger.XTrackMin = db_XTrackMin;
		tarAuger.XTrackMax = db_XTrackMax;
		tarAuger.XFOVMin = db_XFOVMin;
		tarAuger.XFOVMax = db_XFOVMax;
		tarAuger.XTrackObs = db_XTrackObs;
		tarAuger.DegTrackObs = db_DegTrackObs;
		tarAuger.TTrackObs = db_TTrackObs;

		(*this)[d_auger_t3id] = tarAuger;

	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		EXEC SQL CLOSE cur_augerset;
		EXEC SQL ROLLBACK;
		throw SqlError ();
	}

	EXEC SQL CLOSE cur_augerset;
	EXEC SQL ROLLBACK;
}

void AugerSet::printHTMLTable (std::ostringstream &_os)
{
	double JD = ln_get_julian_from_sys ();\

	for (AugerSet::iterator iter = begin (); iter != end (); iter++)
	{
		iter->second.printHTMLRow (_os, JD);
	}
}

void AugerSetDate::load (int year, int month, int day, int hour, int minutes)
{
	if (checkDbConnection ())
		throw SqlError ();

	EXEC SQL BEGIN DECLARE SECTION;
	int d_value;
	int d_c;
	char *stmp_c;
	EXEC SQL END DECLARE SECTION;

	const char *group_by;
	std::ostringstream _where;

	std::ostringstream _os;
	if (year == -1)
	{
		group_by = "year";
	}
	else
	{
		_where << "EXTRACT (year FROM auger_date) = " << year;
		if (month == -1)
		{
			group_by = "month";
		}
		else
		{
			_where << " AND EXTRACT (month FROM auger_date) = " << month;
			if (day == -1)
			{
				group_by = "day";
			}
			else
			{
				_where << " AND EXTRACT (day FROM auger_date) = " << day;
				if (hour == -1)
				{
				 	group_by = "hour";
				}
				else
				{
					_where << " AND EXTRACT (hour FROM auger_date) = " << hour;
					if (minutes == -1)
					{
					 	group_by = "minute";
					}
					else
					{
						_where << " AND EXTRACT (minutes FROM auger_date) = " << minutes;
						group_by = "second";
					}
				}
			}
		}
	}

	_os << "SELECT EXTRACT (" << group_by << " FROM auger_date) as value, count (*) as c FROM auger ";

	if (_where.str ().length () > 0)
		_os << "WHERE " << _where.str ();

	_os << " GROUP BY value;";

	stmp_c = new char[_os.str ().length () + 1];
	memcpy (stmp_c, _os.str().c_str(), _os.str ().length () + 1);
	EXEC SQL PREPARE augerset_stmp FROM :stmp_c;

	delete[] stmp_c;

	EXEC SQL DECLARE augersetdate_cur CURSOR FOR augerset_stmp;

	EXEC SQL OPEN augersetdate_cur;

	while (1)
	{
		EXEC SQL FETCH next FROM augersetdate_cur INTO
			:d_value,
			:d_c;
		if (sqlca.sqlcode)
			break;
		(*this)[d_value] = d_c;
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		EXEC SQL CLOSE augersetdate_cur;
		EXEC SQL ROLLBACK;

		throw SqlError ();
	}

	EXEC SQL CLOSE augersetdate_cur;
	EXEC SQL ROLLBACK;
}
