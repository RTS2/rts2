/* 
 * Auger cosmic rays showers follow-up target.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
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

#include "rts2db/sqlerror.h"
#include "rts2db/target_auger.h"

#include "timestamp.h"
#include "infoval.h"
#include "rts2fits/image.h"

using namespace rts2db;

/*****************************************************************/
//Utility functions

/* vector cross product */
void crossProduct(struct vec * a, struct vec * b, struct vec * c)
{
	c->x = a->y * b->z - a->z * b->y;
	c->y = a->z * b->x - a->x * b->z;
	c->z = a->x * b->y - a->y * b->x;
}

/* horizontal coordinates to vector transformation */
void hrz_to_vec (struct ln_hrz_posn * dirh, struct vec * dirv)
{
	dirv->x = cos (dirh->alt) * sin (dirh->az) * (-1);
	dirv->y = cos (dirh->alt) * cos (dirh->az) * (-1);
	dirv->z = sin (dirh->alt);
}

/* vector to horizontal coordinates transformation */
void vec_to_hrz (struct vec * dirv, struct ln_hrz_posn * dirh)
{
	dirh->alt = asin (dirv->z);
	if (dirv->y > 0.)
		dirh->az = atan (dirv->x / dirv->y) + M_PI;
	if (dirv->y < 0.)
	{
		if (dirv->x < 0.)
			dirh->az = atan (dirv->x / dirv->y);
		else
			dirh->az = atan (dirv->x / dirv->y) + 2 * M_PI;
	}
	if (dirv->y == 0.)
	{
		if (dirv->x < 0.)
			dirh->az = 0.5 * M_PI;
		else
			dirh->az = 1.5 * M_PI;
	}
}

/* transformation to unit length vector */
void vec_unit (struct vec * in, struct vec * unit)
{
	double length = sqrt (in->x*in->x + in->y*in->y + in->z*in->z);
	unit->x = in->x / length;
	unit->y = in->y / length;
	unit->z = in->z / length;
}

/* rotation of vector "b" around "axis" by "angle" */
vec rotateVector(struct vec * axis, struct vec * b, double angle)
{
	struct vec ret;
	struct vec cross;
	crossProduct(axis, b, &cross);
	ret.x = b->x * cos(angle) + cross.x * sin(angle);
	ret.y = b->y * cos(angle) + cross.y * sin(angle);
	ret.z = b->z * cos(angle) + cross.z * sin(angle);
	return ret;
}

void dirToEqu (vec *dir, struct ln_lnlat_posn *observer, double JD, struct ln_equ_posn *pos)
{
	struct ln_hrz_posn hrz;
	vec_to_hrz (dir, &hrz);
	hrz.alt = ln_rad_to_deg (hrz.alt);
	hrz.az = ln_rad_to_deg (hrz.az);
	ln_get_equ_from_hrz (&hrz, observer, JD, pos);
}

/*****************************************************************/

TargetAuger::TargetAuger (int in_tar_id, struct ln_lnlat_posn * _obs, int in_augerPriorityTimeout):ConstTarget (in_tar_id, _obs)
{
	augerPriorityTimeout = in_augerPriorityTimeout;
	cor.x = NAN;
	cor.y = NAN;
	cor.z = NAN;
}

TargetAuger::TargetAuger (int _auger_t3id, double _auger_date, double _auger_ra, double _auger_dec, struct ln_lnlat_posn *_obs):ConstTarget (-1, _obs)
{
	augerPriorityTimeout = -1;

	t3id = _auger_t3id;
	auger_date = _auger_date;
	setPosition (_auger_ra, _auger_dec);
}

TargetAuger::~TargetAuger (void)
{
}

void TargetAuger::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_auger_t3id;
	double d_auger_date;
	double d_auger_ra;
	double d_auger_dec;

	EXEC SQL END DECLARE SECTION;

	struct ln_equ_posn pos;
	struct ln_hrz_posn hrz;
	double JD = ln_get_julian_from_sys ();

	EXEC SQL DECLARE cur_auger CURSOR FOR
	SELECT
		auger_t3id,
		EXTRACT (EPOCH FROM auger_date),
		ra,
		dec
	FROM
		auger
	ORDER BY
		auger_date desc;

	EXEC SQL OPEN cur_auger;
	while (1)
	{
		EXEC SQL FETCH next FROM cur_auger INTO
				:d_auger_t3id,
				:d_auger_date,
				:d_auger_ra,
				:d_auger_dec;
		if (sqlca.sqlcode)
			break;
		pos.ra = d_auger_ra;
		pos.dec = d_auger_dec;
		ln_get_hrz_from_equ (&pos, observer, JD, &hrz);
		if (hrz.alt > 10)
		{
			EXEC SQL CLOSE cur_auger;
			EXEC SQL COMMIT;
			load (d_auger_t3id);
			return;
		}
	}
	auger_date = 0;
	if (sqlca.sqlcode == ECPG_NOT_FOUND)
	{
		pos.ra = pos.dec = 0;
		EXEC SQL CLOSE cur_auger;
		EXEC SQL ROLLBACK;
		return;
	}
	logStream (MESSAGE_ERROR) << "cannot load Auger target: " << sqlca.sqlerrm.sqlerrmc << sendLog;
	EXEC SQL CLOSE cur_target;
	EXEC SQL ROLLBACK;
}

void TargetAuger::load (int auger_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_auger_t3id = auger_id;
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

	VARCHAR db_shower_params[2000]; // Shower parameters
	int db_shower_ind;

	int db_cut = 0;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT
		auger_t3id,
		EXTRACT (EPOCH FROM auger_date),
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
		TTrackObs,
		ShowerParams
	INTO
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
		:db_TTrackObs,
		:db_shower_params :db_shower_ind
	FROM
		auger
	WHERE
		auger_t3id = :d_auger_t3id;

	if (sqlca.sqlcode)
	{
		logMsgDb ("TargetAuger::load", MESSAGE_ERROR);
		EXEC SQL CLOSE cur_auger;
		EXEC SQL ROLLBACK;
		auger_date = 0;
		std::ostringstream err;
		err << "cannot load auger event for event " << d_auger_t3id;
		throw SqlError (err.str ().c_str ());
	}

	t3id = d_auger_t3id;
	auger_date = d_auger_date;
	setPosition (d_auger_ra, d_auger_dec);

	cut = db_cut;
	Eye = db_Eye;
	Run = db_Run;
	Event = db_Event;
	AugerId = db_AugerId.arr; 
	GPSSec = db_GPSSec; 
	GPSNSec = db_GPSNSec; 
	SDId = db_SDId; 
	NPix = db_NPix;
	SDPTheta = db_SDPTheta;
	SDPThetaErr = db_SDPThetaErr;
	SDPPhi = db_SDPPhi;
	SDPPhiErr = db_SDPPhiErr;
	SDPChi2 = db_SDPChi2;
	SDPNdf = db_SDPNdf; 
	Rp = db_Rp;
	RpErr = db_RpErr;
	Chi0 = db_Chi0;
	Chi0Err = db_Chi0Err;
	T0 = db_T0;
	T0Err = db_T0Err;
	TimeChi2 = db_TimeChi2;
	TimeChi2FD = db_TimeChi2FD;
	TimeNdf = db_TimeNdf;	
	Easting = db_Easting;
	Northing = db_Northing;
	Altitude = db_Altitude;
	NorthingErr = db_NorthingErr;
	EastingErr = db_EastingErr;
	Theta = db_Theta;
	ThetaErr = db_ThetaErr;
	Phi = db_Phi;
	PhiErr = db_PhiErr;
	dEdXmax = db_dEdXmax;
	dEdXmaxErr = db_dEdXmaxErr;
	Xmax = db_Xmax;
	XmaxErr = db_XmaxErr;
	X0 = db_X0;
	X0Err = db_X0Err;
	Lambda = db_Lambda;
	LambdaErr = db_LambdaErr;
	GHChi2 = db_GHChi2;
	GHNdf = db_GHNdf;
	LineFitChi2 = db_LineFitChi2;
	EmEnergy = db_EmEnergy;
	EmEnergyErr = db_EmEnergyErr;
	Energy = db_Energy;
	EnergyErr = db_EnergyErr;
	MinAngle = db_MinAngle;
	MaxAngle = db_MaxAngle;
	MeanAngle = db_MeanAngle;
	NTank = db_NTank;
	HottestTank = db_HottestTank;
	AxisDist = db_AxisDist;
	SDPDist = db_SDPDist;
	SDFDdT = db_SDFDdT;
	XmaxEyeDist = db_XmaxEyeDist;
	XTrackMin = db_XTrackMin;
	XTrackMax = db_XTrackMax;
	XFOVMin = db_XFOVMin;
	XFOVMax = db_XFOVMax;
	XTrackObs = db_XTrackObs;
	DegTrackObs = db_DegTrackObs;
	TTrackObs = db_TTrackObs;

	if (db_shower_ind == 0)
	{
		db_shower_params.arr[db_shower_params.len] = '\0';

		std::istringstream iss (db_shower_params.arr);
		while (!iss.fail())
		{
			double d1, d2;
			iss >> d1 >> d2;
			showerparams.push_back (std::pair <double, double> (d1, d2));
		}
	}

	EXEC SQL CLOSE cur_auger;
	EXEC SQL COMMIT;
	if (target_id > 0)
		Target::load ();
}

void TargetAuger::loadByOid (int _obs_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_obs_id = _obs_id;
	int db_auger_t3id;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT auger_t3id INTO :db_auger_t3id FROM auger_observation WHERE obs_id = :db_obs_id;

	if (sqlca.sqlcode)
	{
		logMsgDb ("TargetAuger::startSlew SQL error", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		throw SqlError ("cannot find auger record for a given observation ID");
	}

	load (db_auger_t3id);
}

void TargetAuger::updateShowerFields ()
{
	time_t aug_date = auger_date;
	double JD = ln_get_julian_from_timet (&aug_date);

	struct ln_equ_posn pos;
	struct ln_hrz_posn hrz;

	cor.x = Easting - 459201;
	cor.y = Northing - 6071873;
	cor.z = Altitude - 1422;

	getPosition (&pos, JD);
	ln_get_hrz_from_equ (&pos, observer, JD, &hrz);

	hrz.az = ln_deg_to_rad (hrz.az);
	hrz.alt = ln_deg_to_rad (hrz.alt);

	struct vec cortmp, dir, axis, unit;

	// shower direction vector
	hrz_to_vec (&hrz, &dir);

	// core position vector
	vec_unit (&cor, &cortmp);

	// vector perpendicular to SDP - axis of rotation
	crossProduct (&dir, &cortmp, &axis);
	vec_unit (&axis, &unit);
	axis = unit;

	// current and previous equatorial positions
	struct ln_equ_posn equ,prev_equ;

	dirToEqu (&dir, observer, JD, &equ);
	equ.ra -= pos.ra;
	equ.dec -= pos.dec;
	addShowerOffset (equ);

	vec prev_dir = dir;
	prev_equ = equ;

	double angle = 160 * M_PI / (180 * 60);

	dir = rotateVector (&axis, &dir, angle);

	while (dir.z > 0.)
	{
		dirToEqu (&dir, observer, JD, &equ);
		equ.ra -= pos.ra;
		equ.dec -= pos.dec;

		if (dir.z <= 0.5 && prev_dir.z > 0.5 && prev_dir.z < 0.520016128)
			addShowerOffset (prev_equ);
        	if (dir.z <= 0.5 && dir.z >= 0.034899497)
			addShowerOffset (equ);
        	if (prev_dir.z >= 0.034899497 && dir.z > 0.011635266 && dir.z < 0.034899497)
			addShowerOffset (equ);
		
		prev_dir = dir;
		prev_equ = equ;

		dir = rotateVector (&axis, &dir, angle);
	}
}

void TargetAuger::addShowerOffset (struct ln_equ_posn &pos)
{
	if (fabs (pos.ra) < 1 / 3600.0)
		pos.ra = 0;
	if (fabs (pos.dec) < 1 / 3600.0)
		pos.dec = 0;
	showerOffsets.push_back (pos);
}

bool TargetAuger::getScript (const char *device_name, std::string &buf)
{
	if (showerOffsets.size () == 0)
		updateShowerFields ();

	if (!strcmp (device_name, "WF"))
	{
		std::ostringstream _os;
	 	_os << "filter=B ";
		for (std::vector <struct ln_equ_posn>::iterator iter = showerOffsets.begin (); iter != showerOffsets.end (); iter++)
		{
			if (iter->ra != 0 || iter->dec != 0)
				_os << "PARA.WOFFS=(" << (iter->ra) << "," << (iter->dec) << ") ";
			_os << "E 10 ";
		}
		buf = _os.str ();
		return false;
	}
	buf = std::string ("E 5");
	return false;
}

float TargetAuger::getBonus (double JD)
{
	time_t jd_date;
	ln_get_timet_from_julian (JD, &jd_date);
	// shower too old - observe for 30 minutes..
	if (jd_date > auger_date + augerPriorityTimeout)
		return 1;
	// else return value from DB
	return ConstTarget::getBonus (JD);
}

moveType TargetAuger::afterSlewProcessed ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_obs_id;
		int d_auger_t3id = t3id;
	EXEC SQL END DECLARE SECTION;

	d_obs_id = getObsId ();
	EXEC SQL
		INSERT INTO
			auger_observation
			(
			auger_t3id,
			obs_id
			) VALUES (
			:d_auger_t3id,
			:d_obs_id
			);
	if (sqlca.sqlcode)
	{
		logMsgDb ("TargetAuger::startSlew SQL error", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		return OBS_MOVE_FAILED;
	}
	EXEC SQL COMMIT;
	return OBS_MOVE;
}

int TargetAuger::considerForObserving (double JD)
{
	load ();
	return ConstTarget::considerForObserving (JD);
}

void TargetAuger::printExtra (Rts2InfoValStream & _os, double JD)
{
	std::vector <struct ln_equ_posn> positions;
	getEquPositions (positions);

	int i = 1;

	for (std::vector <struct ln_equ_posn>::iterator iter = positions.begin (); iter != positions.end (); iter++)
	{
		std::ostringstream _osra;
		_osra << "RA " << i;

		std::ostringstream _osdec;
		_osdec << "RA " << i;

		_os
			<< std::endl
			<< InfoVal<LibnovaRaJ2000> (_osra.str ().c_str (), LibnovaRaJ2000 (iter->ra))
			<< InfoVal<LibnovaRaJ2000> (_osdec.str ().c_str (), LibnovaRaJ2000 (iter->dec));

		i++;
	}

	std::ostringstream profile;

	for (std::vector <std::pair <double, double> >::iterator iter = showerparams.begin (); iter != showerparams.end (); iter++)
	{
		if (iter != showerparams.begin ())
			profile << " ";
		profile << iter->first << " " << iter->second;
	}

	_os
		<< std::endl
		<< InfoVal<int> ("T3ID", t3id)
		<< InfoVal<Timestamp> ("DATE", Timestamp(auger_date))
		<< InfoVal<int> ("NPIXELS", NPix)
		<< InfoVal<double> ("CORE_X", cor.x)
		<< InfoVal<double> ("CORE_Y", cor.y)
		<< InfoVal<double> ("CORE_Z", cor.z)

		<< InfoVal<int> ("Eye", Eye)
		<< InfoVal<int> ("Run", Run)
		<< InfoVal<int> ("Event", Event)
		<< InfoVal<const char*> ("AugerId", AugerId.c_str ())
		<< InfoVal<int> ("GPSSec", GPSSec)
		<< InfoVal<int> ("GPSNSec", GPSNSec)
		<< InfoVal<int> ("SDId", SDId)
	
		<< InfoVal<int> ("NPix", NPix)
	
		<< InfoVal<double> ("SDPTheta", SDPTheta)
		<< InfoVal<double> ("SDPThetaErr", SDPThetaErr)
		<< InfoVal<double> ("SDPPhi", SDPPhi)
		<< InfoVal<double> ("SDPPhiErr", SDPPhiErr)
		<< InfoVal<double> ("SDPChi2", SDPChi2)
		<< InfoVal<int> ("SDPNdf", SDPNdf)
	
		<< InfoVal<double> ("Rp", Rp)
		<< InfoVal<double> ("RpErr", RpErr)
		<< InfoVal<double> ("Chi0", Chi0)
		<< InfoVal<double> ("Chi0Err", Chi0Err)
		<< InfoVal<double> ("T0", T0)
		<< InfoVal<double> ("T0Err", T0Err)
		<< InfoVal<double> ("TimeChi2", TimeChi2)
		<< InfoVal<double> ("TimeChi2FD", TimeChi2FD)
		<< InfoVal<int> ("TimeNdf", TimeNdf)
	
		<< InfoVal<double> ("Easting", Easting)
		<< InfoVal<double> ("Northing", Northing)
		<< InfoVal<double> ("Altitude", Altitude)
		<< InfoVal<double> ("NorthingErr", NorthingErr)
		<< InfoVal<double> ("EastingErr", EastingErr)
		<< InfoVal<double> ("Theta", Theta)
		<< InfoVal<double> ("ThetaErr", ThetaErr)
		<< InfoVal<double> ("Phi", Phi)
		<< InfoVal<double> ("PhiErr", PhiErr)
	
		<< InfoVal<double> ("dEdXmax", dEdXmax)
		<< InfoVal<double> ("dEdXmaxErr", dEdXmaxErr)
		<< InfoVal<double> ("Xmax", Xmax)
		<< InfoVal<double> ("XmaxErr", XmaxErr)
		<< InfoVal<double> ("X0", X0)
		<< InfoVal<double> ("X0Err", X0Err)
		<< InfoVal<double> ("Lambda", Lambda)
		<< InfoVal<double> ("LambdaErr", LambdaErr)
		<< InfoVal<double> ("GHChi2", GHChi2)
		<< InfoVal<int> ("GHNdf", GHNdf)
		<< InfoVal<double> ("LineFitChi2", LineFitChi2)
	
		<< InfoVal<double> ("EmEnergy", EmEnergy)
		<< InfoVal<double> ("EmEnergyErr", EmEnergyErr)
		<< InfoVal<double> ("Energy", Energy)
		<< InfoVal<double> ("EnergyErr", EnergyErr)
	
		<< InfoVal<double> ("MinAngle", MinAngle)
		<< InfoVal<double> ("MaxAngle", MaxAngle)
		<< InfoVal<double> ("MeanAngle", MeanAngle)
	
		<< InfoVal<int> ("NTank", NTank)
		<< InfoVal<int> ("HottestTank", HottestTank)
		<< InfoVal<double> ("AxisDist", AxisDist)
		<< InfoVal<double> ("SDPDist", SDPDist)
	
		<< InfoVal<double> ("SDFDdT", SDFDdT)
		<< InfoVal<double> ("XmaxEyeDist", XmaxEyeDist)
		<< InfoVal<double> ("XTrackMin", XTrackMin)
		<< InfoVal<double> ("XTrackMax", XTrackMax)
		<< InfoVal<double> ("XFOVMin", XFOVMin)
		<< InfoVal<double> ("XFOVMax", XFOVMax)
		<< InfoVal<double> ("XTrackObs", XTrackObs)
		<< InfoVal<double> ("DegTrackObs", DegTrackObs)
		<< InfoVal<double> ("TTrackObs", TTrackObs)

		<< InfoVal<int> ("cut", cut)
		<< InfoVal<std::string> ("Profile", profile.str ());


	ConstTarget::printExtra (_os, JD);
}

void TargetAuger::printHTMLRow (std::ostringstream &_os, double JD)
{
	struct ln_equ_posn pos;
	struct ln_hrz_posn hrz;

	getPosition (&pos, JD);
	getAltAz (&hrz, JD);

	_os
		<< "<tr><td><a href='" << t3id << "'>" << t3id << "</a></td><td>"
		<< LibnovaRaDec (pos.ra, pos.dec) << "</td><td>"
		<< LibnovaHrz (hrz.alt, hrz.az) << "</td><td>"
		<< Timestamp (auger_date) << "</td><td>"
		<< cor.x << "</td><td>"
		<< cor.y << "</td><td>"
		<< cor.z << "</td><td>"
		<< cor.x + 459201 << "</td><td>"
		<< cor.y + 6071873 << "</td><td>"
		<< cor.z + 1422 << "</td></tr>";
}

void TargetAuger::writeToImage (rts2image::Image * image, double JD)
{
	ConstTarget::writeToImage (image, JD);
	image->setValue ("AGR_T3ID", t3id, "Auger target id");

	image->setValue ("AGR_Eye", Eye, "FD eye Id");
	image->setValue ("AGR_Run", Run, "FD run number");
	image->setValue ("AGR_Event", Event, "FD event number");
	image->setValue ("AGR_AugerId", AugerId.c_str (), "Event Auger Id after SD/FD merger");
	image->setValue ("AGR_GPSSec", GPSSec, "GPS second (SD)");
	image->setValue ("AGR_GPSNSec", GPSNSec, "GPS nano second (SD)");
	image->setValue ("AGR_SDId", SDId, "SD Event Id");
	
	image->setValue ("AGR_NPix", NPix, "Num. pixels with a pulse after FdPulseFinder");
	
	image->setValue ("AGR_SDPTheta", SDPTheta, "Zenith angle of SDP normal vector (deg)");
	image->setValue ("AGR_SDPThetaErr", SDPThetaErr, "Uncertainty of SDPtheta");
	image->setValue ("AGR_SDPPhi", SDPPhi, "Azimuth angle of SDP normal vector (deg)");
	image->setValue ("AGR_SDPPhiErr", SDPPhiErr, "Uncertainty of SDPphi");
	image->setValue ("AGR_SDPChi2", SDPChi2, "Chi^2 of SDP db_it");
	image->setValue ("AGR_SDPNdf", SDPNdf, "Degrees of db_reedom of SDP db_it");
	
	image->setValue ("AGR_Rp", Rp, "Shower impact parameter Rp (m)");
	image->setValue ("AGR_RpErr", RpErr, "Uncertainty of Rp (m)");
	image->setValue ("AGR_Chi0", Chi0, "Angle of shower in the SDP (deg)");
	image->setValue ("AGR_Chi0Err", Chi0Err, "Uncertainty of Chi0 (deg)");
	image->setValue ("AGR_T0", T0, "FD time db_it T_0 (ns)");
	image->setValue ("AGR_T0Err", T0Err, "Uncertainty of T_0 (ns)");
	image->setValue ("AGR_TimeChi2", TimeChi2, "Full Chi^2 of axis db_it");
	image->setValue ("AGR_TimeChi2FD", TimeChi2FD, "Chi^2 of axis db_it (FD only)");
	image->setValue ("AGR_TimeNdf", TimeNdf, "Degrees of db_reedom of axis db_it");
	
	image->setValue ("AGR_Easting", Easting, "Core position in easting coordinate (m)");
	image->setValue ("AGR_Northing", Northing, "Core position in northing coordinate (m)");
	image->setValue ("AGR_Altitude", Altitude, "Core position altitude (m)");
	image->setValue ("AGR_NorthingErr", NorthingErr, "Uncertainty of northing coordinate (m)");
	image->setValue ("AGR_EastingErr", EastingErr, "Uncertainty of easting coordinate (m)");
	image->setValue ("AGR_Theta", Theta, "Shower zenith angle in core coords. (deg)");
	image->setValue ("AGR_ThetaErr", ThetaErr, "Uncertainty of zenith angle (deg)");
	image->setValue ("AGR_Phi", Phi, "Shower azimuth angle in core coords. (deg)");
	image->setValue ("AGR_PhiErr", PhiErr, "Uncertainty of azimuth angle (deg)");
	
	image->setValue ("AGR_dEdXmax", dEdXmax, "Energy deposit at shower max (GeV/(g/cm^2))");
	image->setValue ("AGR_dEdXmaxErr", dEdXmaxErr, "Uncertainty of Nmax (GeV/(g/cm^2))");
	image->setValue ("AGR_Xmax", Xmax, "Slant depth of shower maximum (g/cm^2)");
	image->setValue ("AGR_XmaxErr", XmaxErr, "Uncertainty of Xmax (g/cm^2)");
	image->setValue ("AGR_X0", X0, "X0 Gaisser-Hillas db_it (g/cm^2)");
	image->setValue ("AGR_X0Err", X0Err, "Uncertainty of X0 (g/cm^2)");
	image->setValue ("AGR_Lambda", Lambda, "Lambda of Gaisser-Hillas db_it (g/cm^2)");
	image->setValue ("AGR_LambdaErr", LambdaErr, "Uncertainty of Lambda (g/cm^2)");
	image->setValue ("AGR_GHChi2", GHChi2, "Chi^2 of Gaisser-Hillas db_it");
	image->setValue ("AGR_GHNdf", GHNdf, "Degrees of db_reedom of GH db_it");
	image->setValue ("AGR_LineFitChi2", LineFitChi2, "Chi^2 of linear db_it to profile");
	
	image->setValue ("AGR_EmEnergy", EmEnergy, "Calorimetric energy db_rom GH db_it (EeV)");
	image->setValue ("AGR_EmEnergyErr", EmEnergyErr, "Uncertainty of Eem (EeV)");
	image->setValue ("AGR_Energy", Energy, "Total energy db_rom GH db_it (EeV)");
	image->setValue ("AGR_EnergyErr", EnergyErr, "Uncertainty of Etot (EeV)");
	
	image->setValue ("AGR_MinAngle", MinAngle, "Minimum viewing angle (deg)");
	image->setValue ("AGR_MaxAngle", MaxAngle, "Maximum viewing angle (deg)");
	image->setValue ("AGR_MeanAngle", MeanAngle, "Mean viewing angle (deg)");
	
	image->setValue ("AGR_NTank", NTank, "Number of stations in hybrid db_it");
	image->setValue ("AGR_HottestTank", HottestTank, "Station used in hybrid-geometry reco");
	image->setValue ("AGR_AxisDist", AxisDist, "Shower axis distance to hottest station (m)");
	image->setValue ("AGR_SDPDist", SDPDist, "SDP distance to hottest station (m)");
	
	image->setValue ("AGR_SDFDdT", SDFDdT, "SD/FD time offset after the minimization (ns)");
	image->setValue ("AGR_XmaxEyeDist", XmaxEyeDist, "Distance to shower maximum (m)");
	image->setValue ("AGR_XTrackMin", XTrackMin, "First recorded slant depth of track (g/cm^2)");
	image->setValue ("AGR_XTrackMax", XTrackMax, "Last recorded slant depth of track (g/cm^2)");
	image->setValue ("AGR_XFOVMin", XFOVMin, "First slant depth inside FOV (g/cm^2)");
	image->setValue ("AGR_XFOVMax", XFOVMax, "Last slant depth inside FOV (g/cm^2)");
	image->setValue ("AGR_XTrackObs", XTrackObs, "Observed track length depth (g/cm^2)");
	image->setValue ("AGR_DegTrackObs", DegTrackObs, "Observed track length angle (deg)");
	image->setValue ("AGR_TTrackObs", TTrackObs, "Observed track length time (100 ns)");

	image->setValue ("AGR_cut", cut, "Cuts pased by shower");
}

void TargetAuger::getEquPositions (std::vector <struct ln_equ_posn> &positions)
{
	if (showerOffsets.size () == 0)
		updateShowerFields ();
	struct ln_equ_posn pos;
	getPosition (&pos, ln_get_julian_from_sys ());
	// calculate offsets
	for (std::vector <struct ln_equ_posn>::iterator iter = showerOffsets.begin (); iter != showerOffsets.end (); iter++)
	{
		struct ln_equ_posn sp;
		sp.ra = pos.ra + iter->ra;
		sp.dec = pos.dec + iter->dec;
		positions.push_back (sp);
	}
}
