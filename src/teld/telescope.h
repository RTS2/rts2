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
		 * Target HRZ coordinates.
		 */
		struct ln_hrz_posn tarAltAz;

		/**
		 * Target HRZ coordinates with corrections applied.
		 */
		struct ln_hrz_posn corrAltAz;

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

		/**
		 * Set ignore correction - size bellow which correction commands will
		 * be ignored.
		 */
		void setIgnoreCorrection (double new_ign)
		{
			ignoreCorrection->setValueDouble (new_ign);
		}


		/**
		 * Set ponting model. 0 is EQU, 1 is ALT-AZ
		 *
		 * @param pModel 0 for EQU, 1 for ALT-AZ.
		 */
		void setPointingModel (int pModel)
		{
			pointingModel->setValueInteger (pModel);
		}


		/**
		 * Return telescope pointing model.
		 *
		 * @return 0 if pointing model is EQU, 1 if it is ALT-AZ
		 */
		int getPointingModel ()
		{
			return pointingModel->getValueInteger ();
		}

	private:
		/**
		 * Date and time when last park command was issued.
		 */
		Rts2ValueTime *mountParkTime;

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

		/**
		 * Zero's all corrections, increment move count. Called before move.
		 */
		void incMoveNum ();

		/** 
		 * Which coordinates are used for pointing (eq, alt-az,..)
		 */
		Rts2ValueSelection *pointingModel;

	protected:
		Rts2ValueInteger * correctionsMask;

		virtual int processOption (int in_opt);

		virtual int init ();
		virtual int initValues ();
		virtual int idle ();

		/**
		 * Increment number of parks. Shall be called
		 * every time mount homing commands is issued.
		 *
		 * ParkNum is used in the modelling to find observations
		 * taken in interval with same park numbers, e.g. with sensors 
		 * homed at same location.
		 */
		void setParkTimeNow ()
		{
			mountParkTime->setNow ();
		}

		void applyModel (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, int flip, double JD);

		/**
		 * Apply corrections (at system time).
		 * Will apply corrections (precession, refraction,..) at system time.
		 * 
		 * @param tar_ra  Target RA, returns its value.
		 * @param tar_dec Target DEC, returns its value.
		 */
		void applyCorrections (double &tar_ra, double &tar_dec);

		virtual int willConnect (Rts2Address * in_addr);
		char telType[64];
		Rts2ValueAltAz *telAltAz;

		Rts2ValueInteger *telFlip;

		double defaultRotang;

		Rts2ValueDouble *rotang;

		Rts2ValueDouble *telLongitude;
		Rts2ValueDouble *telLatitude;
		Rts2ValueDouble *telAltitude;
		Rts2ValueString *telescope;

		/**
		 * Check if telescope is moving to fixed position. Called during telescope
		 * movement to detect if the target destination was reached.
		 *
		 * @return -2 when destination was reached, -1 on failure, >= 0
		 * return value is number of milliseconds for next isMovingFixed
		 * call.
		 *
		 * @see isMoving()
		 */
		virtual int isMovingFixed ()
		{
			return isMoving ();
		}

		/**
		 * Check if telescope is moving. Called during telescope
		 * movement to detect if the target destination was reached.
		 *
		 * @return -2 when destination was reached, -1 on failure, >= 0
		 * return value is number of milliseconds for next isMoving
		 * call.
		 */
		virtual int isMoving ()
		{
			return -2;
		}

		/**
		 * Check if telescope is parking. Called during telescope
		 * park to detect if parking position was reached.
		 *
		 * @return -2 when destination was reached, -1 on failure, >= 0
		 * return value is number of milliseconds for next isParking
		 * call.
		 */
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
		 * Return corrections in RA/HA.
		 *
		 * @return RA correction (in degrees).
		 */
		double getCorrRa ()
		{
			return corrRaDec->getRa ();
		}

		/**
		 * Return corrections in DEC.
		 *
		 * @return DEC correction.
		 */
		double getCorrDec ()
		{
		  	return corrRaDec->getDec ();
		}

		/**
		 * Update target and corrected ALT AZ coordinates.
		 *
		 * This call will update tarAltAz and corrAltAz coordinates, based on actuall
		 * tarRaDec and corrRaDec values.
		 */
		void calculateCorrAltAz ();

		/**
		 * Return corrections in zenit distance.
		 *
		 * @return Correction in zenit distance.
		 */
		double getCorrZd ();

		/**
		 * Return corrections in altitude.
		 *
		 * @return Correction in altitude.
		 */
		double getCorrAlt ()
		{
			return -getCorrZd ();
		}

		/**
		 * Return corrections in azimuth.
		 *
		 * @return Correction in azimuth.
		 */
		double getCorrAz ();

		/**
		 * Return distance in degrees to target position.
		 * You are responsible to call info() before this call to
		 * update telescope coordinates.
		 * 
		 * @return Sky distance in degrees to target, 0 - 180. -1 on error.
		 */
		double getTargetDistance ();

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

		/**
		 * Called at the end of telescope movement, after isMoving return
		 * -2.
		 *
		 * @return 0 on success, -1 on failure
		 */
		virtual int endMove ();

		/**
		 * Stop telescope movement. It is called in two cases. Either when new
		 * target is entered and telescope should stop movement to current target,
		 * or when some failure of telescope is detected and telescope should stop 
		 * current movement in order to prevent futher damage to the hardware.
		 *
		 * @return 0 on success, -1 on failure
		 */
		virtual int stopMove () = 0;

		/**
		 * Set telescope to match given coordinates
		 *
		 * This function is mainly used to tell the telescope, where it
		 * actually is at the beggining of observation
		 *
		 * @param ra		setting right ascennation
		 * @param dec		setting declination
		 *
		 * @return -1 on error, otherwise 0
		 */
		virtual int setTo (double set_ra, double set_dec)
		{
			return -1;
		}

		/**
		 * Called when park command is issued. Moves telescope to park position.
		 *
		 * @return 0 on success, -1 on failure
		 */
		virtual int startPark () = 0;

		/**
		 * Called when parking of the telescope is finished. Can do various
		 * important thinks - ussually switch of mount tracking, but can
		 * also switch of some power supply etc..
		 *
		 * @return 0 on success, -1 on failure
		 */
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

		int startResyncMove (Rts2Conn * conn, bool onlyCorrect);

		int setTo (Rts2Conn * conn, double set_ra, double set_dec);

		int startPark (Rts2Conn * conn);

		virtual int getFlip ();
	
		/**
		 * Apply corrections to position.
		 */
		void applyCorrections (struct ln_equ_posn *pos, double JD);

		/**
		 * Set telescope corrections mask.
		 *
		 * @param newMask   New telescope corrections mask.
		 */
		void setCorrectionMask (int newMask)
		{
			correctionsMask->setValueInteger (newMask);
		}

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
