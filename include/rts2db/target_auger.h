/* 
 * Auger cosmic rays showers follow-up target.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TARGET_AUGER__
#define __RTS2_TARGET_AUGER__

#include "rts2db/target.h"

namespace rts2db
{

struct vec
{
	double x;
	double y;
	double z;
};	

class TargetAuger:public rts2db::ConstTarget
{
	public:
		TargetAuger () {}
		TargetAuger (int in_tar_id, struct ln_lnlat_posn *_obs, int in_augerPriorityTimeout);
		/**
		 * Crate target if targets fields are know. You should not call load ont target
		 * created with this contructor.
		 */
		TargetAuger (int _auger_t3id, double _auger_date, double _auger_ra, double _auger_dec, struct ln_lnlat_posn *_obs);
		virtual ~ TargetAuger (void);

		virtual void load ();
		virtual void getPosition (struct ln_equ_posn *pos, double JD);

		/**
		 * Load target from given auger_id.
		 *
		 * @param auger_id  Auger id.
		 */
		void load (int auger_id);

		/**
		 * Load by observation ID.
		 *
		 * @param obs_id   Observation ID.
		 */
		void loadByOid (int obs_id);

		virtual bool getScript (const char *device_name, std::string & buf);
		virtual float getBonus (double JD);
		virtual moveType afterSlewProcessed ();
		virtual int considerForObserving (double JD);
		virtual int changePriority (int pri_change, time_t * time_ch)
		{
			// do not drop priority
			return 0;
		}
		virtual int isContinues ()
		{
			return 1;
		}

		virtual void printExtra (Rts2InfoValStream & _os, double JD);

		virtual void printHTMLRow (std::ostringstream &_os, double JD);

		virtual void writeToImage (rts2image::Image * image, double JD);

		/**
		 * Return fiels for observations.
		 */
		void getEquPositions (std::vector <struct ln_equ_posn> &positions);

		double getShowerJD () { time_t a = auger_date; return ln_get_julian_from_timet (&a); }

		int t3id;
		double auger_date;

		int    Eye;            /// FD eye Id
		int    Run;            /// FD run number
		int    Event;          /// FD event number
		std::string AugerId;   /// Event Auger Id after SD/FD merger
		int    GPSSec;         /// GPS second (SD)
		int    GPSNSec;        /// GPS nano second (SD)
		int    SDId;           /// SD Event Id
	
		int    NPix;           /// Num. pixels with a pulse after FdPulseFinder
	
		double SDPTheta;       /// Zenith angle of SDP normal vector (deg)
		double SDPThetaErr;    /// Uncertainty of SDPtheta
		double SDPPhi;         /// Azimuth angle of SDP normal vector (deg)
		double SDPPhiErr;      /// Uncertainty of SDPphi
		double SDPChi2;        /// Chi^2 of SDP db_it
		int    SDPNdf;         /// Degrees of db_reedom of SDP db_it
	
		double Rp;             /// Shower impact parameter Rp (m)
		double RpErr;          /// Uncertainty of Rp (m)
		double Chi0;           /// Angle of shower in the SDP (deg)
		double Chi0Err;        /// Uncertainty of Chi0 (deg)
		double T0;             /// FD time db_it T_0 (ns)
		double T0Err;          /// Uncertainty of T_0 (ns)
		double TimeChi2;       /// Full Chi^2 of axis db_it
		double TimeChi2FD;     /// Chi^2 of axis db_it (FD only)
		int    TimeNdf;        /// Degrees of db_reedom of axis db_it
	
		double Easting;        /// Core position in easting coordinate (m)
		double Northing;       /// Core position in northing coordinate (m)
		double Altitude;       /// Core position altitude (m)
		double NorthingErr;    /// Uncertainty of northing coordinate (m)
		double EastingErr;     /// Uncertainty of easting coordinate (m)
		double Theta;          /// Shower zenith angle in core coords. (deg)
		double ThetaErr;       /// Uncertainty of zenith angle (deg)
		double Phi;            /// Shower azimuth angle in core coords. (deg)
		double PhiErr;         /// Uncertainty of azimuth angle (deg)
	
		double dEdXmax;        /// Energy deposit at shower max (GeV/(g/cm^2))
		double dEdXmaxErr;     /// Uncertainty of Nmax (GeV/(g/cm^2))
		double Xmax;           /// Slant depth of shower maximum (g/cm^2)
		double XmaxErr;        /// Uncertainty of Xmax (g/cm^2)
		double X0;             /// X0 Gaisser-Hillas db_it (g/cm^2)
		double X0Err;          /// Uncertainty of X0 (g/cm^2)
		double Lambda;         /// Lambda of Gaisser-Hillas db_it (g/cm^2)
		double LambdaErr;      /// Uncertainty of Lambda (g/cm^2)
		double GHChi2;         /// Chi^2 of Gaisser-Hillas db_it
		int    GHNdf;          /// Degrees of db_reedom of GH db_it
		double LineFitChi2;    /// Chi^2 of linear db_it to profile
	
		double EmEnergy;       /// Calorimetric energy db_rom GH db_it (EeV)
		double EmEnergyErr;    /// Uncertainty of Eem (EeV)
		double Energy;         /// Total energy db_rom GH db_it (EeV)
		double EnergyErr;      /// Uncertainty of Etot (EeV)
	
		double MinAngle;       /// Minimum viewing angle (deg)
		double MaxAngle;       /// Maximum viewing angle (deg)
		double MeanAngle;      /// Mean viewing angle (deg)
	
		int    NTank;          /// Number of stations in hybrid db_it
		int    HottestTank;    /// Station used in hybrid-geometry reco
		double AxisDist;       /// Shower axis distance to hottest station (m)
		double SDPDist;        /// SDP distance to hottest station (m)
	
		double SDFDdT;         /// SD/FD time offset after the minimization (ns)
		double XmaxEyeDist;    /// Distance to shower maximum (m)
		double XTrackMin;      /// First recorded slant depth of track (g/cm^2)
		double XTrackMax;      /// Last recorded slant depth of track (g/cm^2)
		double XFOVMin;        /// First slant depth inside FOV (g/cm^2)
		double XFOVMax;        /// Last slant depth inside FOV (g/cm^2)
		double XTrackObs;      /// Observed track length depth (g/cm^2)
		double DegTrackObs;    /// Observed track length angle (deg)
		double TTrackObs;      /// Observed track length time (100 ns)

		std::vector <std::pair <double, double> > showerparams; // shower parameters - sland depth, florescence

		int cut;               /// Cuts pased by shower

	private:
		int augerPriorityTimeout;
		// core coordinates
		struct vec cor;

		// valid positions
		std::vector <struct ln_equ_posn> showerOffsets;

		void updateShowerFields ();
		void addShowerOffset (struct ln_equ_posn &pos);
};

}
#endif							 /* !__RTS2_TARGET_AUGER__ */
