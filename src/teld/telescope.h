/* 
 * Telescope control daemon.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TELD_CPP__
#define __RTS2_TELD_CPP__

#include <libnova/libnova.h>
#include <sys/time.h>
#include <time.h>

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

// types of corrections
#define COR_ABERATION        0x01
#define COR_PRECESSION       0x02
#define COR_REFRACTION       0x04
// if we will use model corrections..
#define COR_MODEL            0x08

class Rts2TelModel;

/**
 * Basic class for telescope drivers.
 *
 * This class provides telescope driver with basic functionalities, such as
 * handles to start movement, do all target calculation and end movement.
 *
 * Coordinates entering driver are in J2000. When it is required to do so,
 * driver handles all transformations from J2000 to current, including
 * refraction. Then this number is feeded to TPoint model and used to point
 * telescope.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevTelescope:public Rts2Device
{
	private:
		Rts2Conn * move_connection;
		int moveInfoCount;
		int moveInfoMax;

		// object + telescope position

		/**
		 * Last error.
		 */
		Rts2ValueDouble *posErr;

		/**
		 * If correction is bellow that value, it is ignored.
		 */
		Rts2ValueDouble *ignoreCorrection;

		/**
		 * If correction is bellow that value, it is considered as small correction.
		 */
		Rts2ValueDouble *smallCorrection;

		/**
		 * If move is above this limit, correction is rejected.
		 */
		Rts2ValueDouble *modelLimit;

		/**
		 * If correction is above that limit, cancel exposures and
		 * move immediatelly. This is to signal we are out of all cameras
		 * FOV.
		 */
		Rts2ValueDouble *telFov;

		/**
		 * Object we are observing.
		 */
		Rts2ValueRaDec *objRaDec;

		/**
		 * User offsets, used to create dithering pattern.
		 */
		Rts2ValueRaDec *offsetRaDec;

		/**
		 * Target we are pointing to
		 * object + user offsets = target
		 */
		Rts2ValueRaDec *tarRaDec;

		/**
		 * Corrections from astrometry/user.
		 */
		Rts2ValueRaDec *corrRaDec;

		/**
		 * RA DEC correction which waits to be applied.
		 */
		Rts2ValueRaDec *waitingCorrRaDec;

		/**
		 * Telescope RA and DEC. In perfect world readed from sensors.
		 * target + model + corrRaDec = requested position -> telRaDec
		 */
		Rts2ValueRaDec *telRaDec;

	protected:
		/**
		 * Set telescope RA.
		 *
		 * @param ra Telescope right ascenation in degrees.
		 */
		void setTelRa (double new_ra)
		{
			telRaDec->setRa (new_ra);
		}

		/**
		 * Set telescope DEC.
		 *
		 * @param new_dec Telescope declination in degrees.
		 */
		void setTelDec (double new_dec)
		{
			telRaDec->setDec (new_dec);
		}

		/**
		 * Set telescope RA and DEC.
		 */
		void setTelRaDec (double new_ra, double new_dec)
		{
			setTelRa (new_ra);
			setTelDec (new_dec);
		}

		/**
		 * Returns current telescope RA.
		 *
		 * @return Current telescope RA.
		 */
		double getTelRa ()
		{
			return telRaDec->getRa ();
		}

		/**
		 * Returns current telescope DEC.
		 *
		 * @return Current telescope DEC.
		 */
		double getTelDec ()
		{
			return telRaDec->getDec ();
		}

		/**
		 * Returns telescope RA and DEC.
		 *
		 * @param tel ln_equ_posn which will be filled with telescope RA and DEC.
		 */
		void getTelRaDec (struct ln_equ_posn *tel)
		{
			tel->ra = getTelRa ();
			tel->dec = getTelDec ();
		}

	private:
		Rts2ValueInteger *moveNum;
		Rts2ValueInteger *corrImgId;

		Rts2ValueInteger *wCorrImgId;

		void checkMoves ();
		void checkGuiding ();

		struct timeval dir_timeouts[4];

		char *modelFile;
		Rts2TelModel *model;

		bool standbyPark;

		/**
		 * Apply aberation correction.
		 */
		void applyAberation (struct ln_equ_posn *pos, double JD);

		/**
		 * Apply precision correction.
		 */
		void applyPrecession (struct ln_equ_posn *pos, double JD);

		/**
		 * Apply refraction correction.
		 */
		void applyRefraction (struct ln_equ_posn *pos, double JD);

	protected:
		Rts2ValueInteger * correctionsMask;

		void applyModel (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, int flip, double JD);
		void applyCorrections (struct ln_equ_posn *pos, double JD);
		// apply corrections (at system time)
		void applyCorrections (double &tar_ra, double &tar_dec);

		virtual int willConnect (Rts2Address * in_addr);
		char *device_file;
		char telType[64];
		Rts2ValueDouble *telAlt;
		Rts2ValueDouble *telAz;
		Rts2ValueInteger *telFlip;

		double defaultRotang;

		Rts2ValueDouble *rotang;

								 // which coordinates are used for pointing (eq, alt-az,..)
		Rts2ValueSelection *pointingModel;

		Rts2ValueDouble *telLongitude;
		Rts2ValueDouble *telLatitude;
		Rts2ValueDouble *telAltitude;
		Rts2ValueString *telescope;
		// in multiply of sidereal speed..eg 1 == 15 arcsec/sec
		virtual int isMovingFixed ()
		{
			return isMoving ();
		}
		virtual int isMoving ()
		{
			return -2;
		}
		virtual int isSearching ()
		{
			return -2;
		}
		virtual int isParking ()
		{
			return -2;
		}

		/**
		 * Returns local sidereal time in hours (0-24 range).
		 * Multiply return by 15 to get degrees.
		 *
		 * @return Local sidereal time in hours (0-24 range).
		 */
		double getLocSidTime ()
		{
			return getLocSidTime (ln_get_julian_from_sys ());
		}

		/**
		 * Returns local sidereal time in hours (0-24 range).
		 * Multiply return by 15 to get degrees.
		 *
		 * @param JD Julian date for which sideral time will be returned.
		 *
		 * @return Local sidereal time in hours (0-24 range).
		 */
		double getLocSidTime (double JD);

		virtual int processOption (int in_opt);

		virtual void cancelPriorityOperations ()
		{
			if ((getState () & TEL_MASK_SEARCHING) == TEL_SEARCH)
			{
				stopSearch ();
			}
			else
			{
				stopMove ();
			}
			clearStatesPriority ();
			Rts2Device::cancelPriorityOperations ();
		}

		/**
		 * Sets new movement target.
		 *
		 * @param ra New object right ascenation.
		 * @param dec New object declination.
		 */
		void setTarget (double ra, double dec)
		{
			objRaDec->setValueRaDec (ra, dec);
		}

		/**
		 * Return target position.
		 *
		 * @param out_tar Target position
		 */
		void getTarget (struct ln_equ_posn *out_tar)
		{
			out_tar->ra = tarRaDec->getRa ();
			out_tar->dec = tarRaDec->getDec ();
		}

		/**
		 * Returns ALT AZ coordinates of target.
		 *
		 * @param hrz ALT AZ coordinates of target.
		 */
		void getTargetAltAz (struct ln_hrz_posn *hrz);
		void getTargetAltAz (struct ln_hrz_posn *hrz, double jd);

		double getLstDeg (double JD);

		virtual bool isBellowResolution (double ra_off, double dec_off)
		{
			return (ra_off == 0 && dec_off == 0);
		}

		void needStop ()
		{
			maskState (TEL_MASK_NEED_STOP, TEL_NEED_STOP);
		}

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		virtual void valueChanged (Rts2Value * changed_value);

		virtual int deleteConnection (Rts2Conn * in_conn)
		{
			if (in_conn == move_connection)
				move_connection = NULL;
			return Rts2Device::deleteConnection (in_conn);
		}

		/**
		 * Send telescope to requested coordinates. Coordinates
		 * can be obtained using getTarget () call.
		 *
		 * @return 0 on success, -1 on error.
		 */
		virtual int startMove () = 0;
		virtual int endMove ();
		virtual int stopMove () = 0;
		virtual int startSearch ()
		{
			return -1;
		}
		virtual int stopSearch ();
		virtual int endSearch ();

		virtual int setTo (double set_ra, double set_dec)
		{
			return -1;
		}

		virtual int startPark () = 0;
		virtual int endPark () = 0;

		/**
		 * Save model from telescope to file.
		 */
		virtual int saveModel ()
		{
			return -1;
		}

		/**
		 * Load model from telescope.
		 */
		virtual int loadModel ()
		{
			return -1;
		}
		virtual int stopWorm ()
		{
			return -1;
		}
		virtual int startWorm ()
		{
			return -1;
		}
		virtual int resetMount ()
		{
			return 0;
		}

	public:
		Rts2DevTelescope (int argc, char **argv);
		virtual ~ Rts2DevTelescope (void);
		virtual int init ();
		virtual int idle ();

		virtual void postEvent (Rts2Event * event);

		virtual int changeMasterState (int new_state);

		virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
			int other_device_type);

		double getLatitude ()
		{
			return telLatitude->getValueDouble ();
		}

		virtual int startGuide (char dir, double dir_dist);
		virtual int stopGuide (char dir);
		virtual int stopGuideAll ();

		virtual int getAltAz ();

		// callback functions from telescope connection
		virtual int info ();

		virtual int scriptEnds ();

		int startMove (Rts2Conn * conn, double tar_ra, double tar_dec,
			bool onlyCorrect);
		int startSearch (Rts2Conn * conn, double radius, double in_searchSpeed);

		int startResyncMove (Rts2Conn * conn, bool onlyCorrect);

		int setTo (Rts2Conn * conn, double set_ra, double set_dec);

		int startPark (Rts2Conn * conn);

		virtual int getFlip ();

		/**
		 * Swicth model on.
		 */
		void modelOff ()
		{
			correctionsMask->setValueInteger (
				correctionsMask->getValueInteger () & ~COR_MODEL
				);
		}

		void modelOn ()
		{
			correctionsMask->setValueInteger (
				correctionsMask->getValueInteger () | COR_MODEL
				);
		}

		bool isModelOn ()
		{
			return (correctionsMask->getValueInteger () & COR_MODEL);
		}

		// reload model
		virtual void signaledHUP ();

		virtual int commandAuthorized (Rts2Conn * conn);

		virtual void setFullBopState (int new_state);
};
#endif							 /* !__RTS2_TELD_CPP__ */
