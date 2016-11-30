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
	crossProduct (axis, b, &cross);
	ret.x = b->x * cos (angle) + cross.x * sin (angle);
	ret.y = b->y * cos (angle) + cross.y * sin (angle);
	ret.z = b->z * cos (angle) + cross.z * sin (angle);
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

float Ddist (float a, float b, float d, float e)
{
	int dist;
	dist = sqrt((a-d)*(a-d)+(b-e)*(b-e));
	return dist;
}

/* calculate slant depth */  
float slantd_elev (float est, float nor, float X, float azimut, float sdphi, float theta, int month)
{
	float param[12][5][4] = {{{6.5,-185.45512,1217.75792,1051253.676},{13.1,-77.961719233,1149.3974354,879044.2975},{35.1,1.0659697329,1536.7105029,610800.76274},{100.0,0.000317101,670.97827193,743424.16762},{10000.0,0.01128292,1.0,1000000000.0}},{{6,-193.482906649,1227.277526,1057565.8919},{12.3,-88.047606534,1152.3562565,897335.90273},{37.6,0.67671779025,1457.9361143,625248.19669},{100,0.000318027297,664.79773683,743878.54879},{10000.0,0.01128292,1.0,1000000000.0}},{{7,-171.23355185,1207.1490848, 1026045.9111},{12.9,-73.878750552,1154.8360718,862344.55423},{35.7,0.94871056328,1508.6973496,612947.06993},{100, 0.00044621670109,658.64089387,743345.02082},{10000.0,0.01128292,1.0,1000000000.0}},{{8,-143.09810516,1181.8291825,980060.24638},{12.1,-76.979091502,1161.8632157,856027.7804},{36.2,0.83594885933,1440.4714686,620377.25203},{100,0.00033614110605,656.86460489,743498.68639},{10000.0,0.01128292,1.0,1000000000.0}},{{4.5,-273.88905394,1309.5120835,1129207.4375},{11.8,-98.373681864,1157.6082795,903757.61519},{36.3,0.82578384579,1447.8069611,617971.89079},{100,0.00035413747604,646.83919414,743288.08998},
		{10000.0,0.01128292,1.0,1000000000.0}},{{8.9,-148.08345247,1189.0575867,972111.82261},{13.5,-26.154607106,1212.2994332,723093.94615},{36.4,0.79538251139,1420.2496317,619111.17794},{100,0.00036902177863,639.35345217,743038.7238},
		{10000.0,0.01128292,1.0,1000000000.0}},{{9,-146.11959987,1187.8837556,971965.53755},{13.9,-24.213336126,1216.6305834,720173.65642},{36.1,0.8451498785,1436.5439047,617971.32975},{100,0.00035837316229,645.89014934,743116.71676},{10000.0,0.01128292,1.0,1000000000.0}},{{7,-177.43352676,1217.6174485,1002673.5826},{12.4,-51.167425841,1158.3369382,793680.93946},{35.8,0.86038576465,1393.5697049,623636.4628},{100,0.00033471821682,658.76353049,743422.13408},{10000.0,0.01128292,1.0,1000000000.0}},{{6.8,-162.00591523,1205.1568777,983444.97916},{11.8,-65.658426267,1154.9309051,826467.09708},{36.6,0.74755311473,1377.3533786,628338.66854},{100,0.00032665957565,660.22102783,743767.87925},{10000.0,0.01128292,1.0,1000000000.0}},{{5.9,-211.44374611,1247.6302703,1052538.3549},{11.7,-76.981259878,1153.125055,853838.85144},{36.5,0.76787883237,1389.7534388,627217.41029},{100,0.00032265825392,661.7309533,743872.51473},{10000.0,0.01128292,1.0,1000000000.0}},{{5,-272.848922,1303.7845877,1138387.7904},{11.9,-93.151868349,1155.0540363,894957.20606},{35.5,0.93890348893,1444.3630867,619397.04191},{100,0.0003225458181,665.9739223,743525.48653},{10000.0,0.01128292,1.0,1000000000.0}},{{6.4,-224.57497779,1256.7556773,1083848.7232},{12.9,-61.11725221,1152.7165161,829418.27646},{36.5,0.8079227276,1461.2679827,618371.09904},{100,0.00034574221706,650.81800915,743446.46709},{10000.0,0.01128292,1.0,1000000000.0}}};

	float elev = 0;
	float dist = 0;
	int i = 0;
	float depth = 0;
	float haltitude = 0;
	float altitude = 0.0;
	float alpha = 0.0;
	float az_alpha = 0.0;
	float delta = 0.0;
	float gamma = 0.0;
	float disttopoint = 0.0;
	const float east = 459218.67;
	const float north = 6071875.9;
	const float elevat = 1418.575; //vyska
	dist = Ddist(est,nor,east,north);
	alpha = atan((est-east)/(nor-north));
	az_alpha = M_PI + alpha;
	azimut = azimut + M_PI; //pro fram od jihu po smeru hodinovych rucicek, transformace na klasicky azimut
	if (alpha<0)
		delta = fabs(azimut-(alpha+2*M_PI));
	else
		delta = fabs(azimut-alpha);
	sdphi = sdphi;
	sdphi = (2*M_PI-sdphi)+M_PI/2;
	theta = theta;
	gamma = fabs(sdphi-az_alpha);
	delta = M_PI-(delta+gamma);
	disttopoint =dist * sin(gamma)/sin(delta);
	depth = X*cos(theta);
	altitude = -param[month][0][3]*log((depth-param[month][0][1])/param[month][0][2]);
	haltitude = altitude/100000;
	while (haltitude >= 0)
	{
		haltitude = haltitude-param[month-1][i][0];
		i = i+1;
	}
	if (i==2)
	{
		altitude = -param[month][1][3]*log((depth-param[month][1][1])/param[month][1][2]);
	}
	if (i==3)
	{
		altitude = -param[month][2][3]*log((depth-param[month][2][1])/param[month][2][2]);
	}
	altitude = altitude/100-elevat;
	elev = atan(altitude/disttopoint);
	return elev;
}

/*****************************************************************/

TargetAuger::TargetAuger (int in_tar_id, struct ln_lnlat_posn * _obs, double _altitude, int in_augerPriorityTimeout):ConstTarget (in_tar_id, _obs, _altitude)
{
	augerPriorityTimeout = in_augerPriorityTimeout;
	cor.x = NAN;
	cor.y = NAN;
	cor.z = NAN;
}

TargetAuger::TargetAuger (int _auger_t3id, double _auger_date, double _auger_ra, double _auger_dec, struct ln_lnlat_posn *_obs, double _altitude):ConstTarget (-1, _obs, _altitude)
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

void TargetAuger::getPosition (struct ln_equ_posn *pos, double JD)
{
	struct ln_hrz_posn hrz;

	hrz.alt = 90 - Theta;
	hrz.az = 270 - Phi;

	ln_get_equ_from_hrz (&hrz, observer, JD, pos);
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

	hrz.alt = 90 - Theta;
	hrz.az = 270 - Phi;

	ln_get_equ_from_hrz (&hrz, observer, JD, &pos);

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

	double angle = 350 * M_PI / (180 * 60);

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

	if (!strcmp (device_name, "WF6"))
	{
		std::ostringstream _os;
	 	_os << "filter=B ";
		for (std::vector <struct ln_equ_posn>::iterator iter = showerOffsets.begin (); iter != showerOffsets.end (); iter++)
		{
			if (iter->ra != 0 || iter->dec != 0)
				_os << "PARA.WOFFS=(" << (iter->ra) << "," << (iter->dec) << ") ";
			_os << "E 30 ";
		}
		buf = _os.str ();
		return false;
	}
	buf = std::string ("filter=B E 30");
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
		_osdec << "DEC " << i;

		std::ostringstream _osalt;
		_osalt << "ALT " << i;

		std::ostringstream _osaz;
		_osaz << "AZ " << i;

		struct ln_hrz_posn hrz;
		ln_get_hrz_from_equ (&(*iter), observer, JD, &hrz);

		_os
			<< std::endl
			<< InfoVal<LibnovaRaJ2000> (_osra.str ().c_str (), LibnovaRaJ2000 (iter->ra))
			<< InfoVal<LibnovaRaJ2000> (_osdec.str ().c_str (), LibnovaRaJ2000 (iter->dec))
			<< InfoVal<LibnovaDeg90> (_osalt.str ().c_str (), LibnovaDeg90 (hrz.alt))
			<< InfoVal<LibnovaDeg360> (_osaz.str ().c_str (), LibnovaDeg360 (hrz.az));

		i++;
	}

	std::ostringstream profile;
	std::ostringstream slants;

	for (std::vector <std::pair <double, double> >::iterator iter = showerparams.begin (); iter != showerparams.end (); iter++)
	{
		if (iter != showerparams.begin ())
		{
			profile << " ";
			slants << " ";
		}
		profile << iter->first << " " << iter->second;
		slants << slantd_elev (Easting, Northing, iter->first, 0, SDPPhi, Theta, 0);
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
		<< InfoVal<std::string> ("Profile", profile.str ())
		<< InfoVal<std::string> ("Slant depths", slants.str ());


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
