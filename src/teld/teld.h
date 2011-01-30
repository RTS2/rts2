/* 
 * Telescope control daemon.
 * Copyright (C) 2003-2009 Petr Kubanek <petr@kubanek.net>
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
#include "../utils/device.h"
#include "../utils/objectcheck.h"

// pointing models
#define POINTING_RADEC       0
#define POINTING_ALTAZ       1
#define POINTING_ALTALT      2

namespace rts2telmodel
{
	class Model;
};

/**
 * Telescope interface and relating things (modelling, pointing, ...)
 */
namespace rts2teld
{

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
 * Those values are important for telescope:
 *
 * <ul>
 *   <li><b>ORIRA, ORIDEC</b> contains original, J2000 coordinates. Those are
 *          usually entered by observing program (rts2-executor).</li>
 *   <li><b>OFFSRA, OFFSDEC</b> contains offsets applied to ORI coordinates.</li>
 *   <li><b>OBJRA,OBJDEC</b> contains offseted coordinates. OBJRA = ORIRA + OFFSRA, OBJDEC = ORIDEC + OFFSDEC.</li>
 *   <li><b>TARRA,TARDEC</b> contains precessed etc. coordinates. Those coordinates do not contain modelling, which is stored in MORA and MODEC</li>
 *   <li><b>CORR_RA, CORR_DEC</b> contains offsets from on-line astrometry which are fed totelescope. Telescope coordinates are then calculated as TARRA-CORR_RA, TAR_DEC - CORR_DEC</li>
 *   <li><b>TELRA,TELDEC</b> contains coordinates read from telescope driver. In ideal word, they should eaual to TARRA - CORR_RA - MORA, TARDEC - CORR_DEC - MODEC. But they might differ. The two major sources of differences are: telescope do not finish movement as expected and small deviations due to rounding errors in mount or driver.<li>
 *   <li><b>MORA,MODEC</b> contains offsets comming from ponting model. They are shown only if this information is available from the mount (OpenTpl) or when they are caculated by RTS2 (Paramount).</li>
 * </ul>
 *
 * Following auxiliary values are used to track telescope offsets:
 *
 * <ul>
 *   <li><b>woffsRA, woffsDEC</b> contains offsets which weren't yet applied. They can be set either directly or when you change OFFS value. When move command is executed, OFFSRA += woffsRA, OFFSDEC += woffsDEC and woffsRA and woffsDEC are set to 0.</li>
 *   <li><b>wcorrRA, wcorrDEC</b> contains corrections which weren't yet
 *   applied. They can be set either directy or by correct command. When move
 *   command is executed, CORRRA += wcorrRA, CORRDEC += wcorrDEC and wcorrRA = 0, wcorrDEC = 0.</li>
 * </ul>
 *
 * Please see startResync() documentation for functions available to
 * retrieve various coordinates. startResync is the routine which is called
 * each time coordinates written as target should be matched to physical
 * telescope coordinates. Using various methods in the class, driver can 
 * get various coordinates which will be put to telescope.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Telescope:public rts2core::Device
{
	public:
		Telescope (int argc, char **argv, bool diffTrack = false);
		virtual ~ Telescope (void);

		virtual void postEvent (Rts2Event * event);

		virtual int changeMasterState (int new_state);

		virtual rts2core::Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);

		double getLatitude () { return telLatitude->getValueDouble (); }

		// callback functions from telescope connection
		virtual int info ();

		virtual int scriptEnds ();

		int setTo (Rts2Conn * conn, double set_ra, double set_dec);

		int startPark (Rts2Conn * conn);

		virtual int getFlip ();

		/**
		 * Apply corrections to position.
		 */
		void applyCorrections (struct ln_equ_posn *pos, double JD);

		/**
		 * Set telescope correctios.
		 *
		 * @param _aberation     If aberation should be calculated.
		 * @param _precession    If precession should be calculated.
		 * @param _refraction    If refraction should be calculated.
		 */
		void setCorrections (bool _aberation, bool _precession, bool _refraction)
		{
			calAberation->setValueBool (_aberation);
			calPrecession->setValueBool (_precession);
			calRefraction->setValueBool (_refraction);
		}

		/**
		 * If aberation should be calculated in RTS2.
		 */
		bool calculateAberation () { return calAberation->getValueBool (); }

		/**
		 * If precession should be calculated in RTS2.
		 */
		bool calculatePrecession () { return calPrecession->getValueBool (); }

		/**
		 * If refraction should be calculated in RTS2.
		 */
		bool calculateRefraction () { return calRefraction->getValueBool (); }

		/**
		 * Switch model off model will not be used to transform coordinates.
		 */
		void modelOff () { calModel->setValueBool (false); }

		void modelOn () { calModel->setValueBool (true); }

		bool isModelOn () { return (calModel->getValueBool ()); }

		virtual int commandAuthorized (Rts2Conn * conn);

		virtual void setFullBopState (int new_state);

	protected:
		void applyOffsets (struct ln_equ_posn *pos)
		{
			pos->ra = oriRaDec->getRa () + offsRaDec->getRa ();
			pos->dec = oriRaDec->getDec () + offsRaDec->getDec ();
		}

		/**
		 * Called before corrections are processed. If returns 0, then corrections
		 * will skip the standard correcting mechanism.
		 *
		 * @param ra  RA correction
		 * @param dec DEC correction
		 */
		virtual int applyCorrectionsFixed (double ra, double dec) { return -1; }
	
		/**
		 * Returns telescope target RA.
		 */
		double getTelTargetRa () { return telTargetRaDec->getRa (); }

		/**
		 * Returns telescope target DEC.
		 */
		double getTelTargetDec () { return telTargetRaDec->getDec (); }

		/**
		 * Return telescope target coordinates.
		 */
		void getTelTargetRaDec (struct ln_equ_posn *equ)
		{
			equ->ra = getTelTargetRa ();
			equ->dec = getTelTargetDec ();
		}
		  
		/**
		 * Set telescope RA.
		 *
		 * @param ra Telescope right ascenation in degrees.
		 */
		void setTelRa (double new_ra) { telRaDec->setRa (new_ra); }

		/**
		 * Set telescope DEC.
		 *
		 * @param new_dec Telescope declination in degrees.
		 */
		void setTelDec (double new_dec) { telRaDec->setDec (new_dec); }

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
		double getTelRa () { return telRaDec->getRa (); }

		/**
		 * Returns current telescope DEC.
		 *
		 * @return Current telescope DEC.
		 */
		double getTelDec () { return telRaDec->getDec (); }

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
		void setIgnoreCorrection (double new_ign) { ignoreCorrection->setValueDouble (new_ign); }


		/**
		 * Set ponting model. 0 is EQU, 1 is ALT-AZ
		 *
		 * @param pModel 0 for EQU, 1 for ALT-AZ.
		 */
		void setPointingModel (int pModel) { pointingModel->setValueInteger (pModel); }


		/**
		 * Return telescope pointing model.
		 *
		 * @return 0 if pointing model is EQU, 1 if it is ALT-AZ
		 */
		int getPointingModel () { return pointingModel->getValueInteger (); }

		/**
		 * Creates values for guiding movements.
		 */
		void createRaGuide ();
		void createDecGuide ();

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
		void setParkTimeNow () { mountParkTime->setNow (); }

		/**
		 * Apply corrRaDec. Return -1if correction is above correctionLimit and was not applied.
		 */
		int applyCorrRaDec (struct ln_equ_posn *pos, bool invertRa = false, bool invertDec = false);

		void applyModel (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, int flip, double JD);

		/**
		 * Apply corrections (at system time).
		 * Will apply corrections (precession, refraction,..) at system time.
		 * Override this method for any custom corrections. You most
		 * probably would like to call parent method in overrided
		 * method to get coordinates to start with.
		 *
		 * void MyClass::applyCorrections (double &tar_ra, double &tar_dec)
		 * {
		 *      Telescope::applyCorrections (tar_ra, tar_dec);
		 *      tar_ra += myoffs.ra);
		 *      tar_dec += myoffs.dec;
		 * }
		 *
		 * @param tar_ra  target RA, returns its value.
		 * @param tar_dec target DEC, returns its value.
		 */
		virtual void applyCorrections (double &tar_ra, double &tar_dec);

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
		virtual int isMovingFixed () { return isMoving (); }

		/**
		 * Check if telescope is moving. Called during telescope
		 * movement to detect if the target destination was reached.
		 *
		 * @return -2 when destination was reached, -1 on failure, >= 0
		 * return value is number of milliseconds for next isMoving
		 * call.
		 */
		virtual int isMoving () = 0;

		/**
		 * Check if telescope is parking. Called during telescope
		 * park to detect if parking position was reached.
		 *
		 * @return -2 when destination was reached, -1 on failure, >= 0
		 * return value is number of milliseconds for next isParking
		 * call.
		 */
		virtual int isParking () { return -2; }

		/**
		 * Returns local sidereal time in hours (0-24 range).
		 * Multiply return by 15 to get degrees.
		 *
		 * @return Local sidereal time in hours (0-24 range).
		 */
		double getLocSidTime () { return getLocSidTime (ln_get_julian_from_sys ()); }

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
		 * Returns true if origin was changed from the last movement.
		 *
		 * @see targetChangeFromLastResync
		 */
		bool originChangedFromLastResync () { return oriRaDec->wasChanged (); }

		/**
		 * Returns original, J2000 coordinates, used as observational
		 * target.
		 */
		void getOrigin (struct ln_equ_posn _ori)
		{
			_ori.ra = oriRaDec->getRa ();
			_ori.dec = oriRaDec->getDec ();
		}

		/**
		 * Sets new movement target.
		 *
		 * @param ra New object right ascenation.
		 * @param dec New object declination.
		 */
		void setTarget (double ra, double dec)
		{
			tarRaDec->setValueRaDec (ra, dec);
			telTargetRaDec->setValueRaDec (ra, dec);
			modelRaDec->setValueRaDec (0, 0);
		}

		/**
		 * Sets ALT-AZ target. The program should call moveAltAz() to
		 * start alt-az movement.
		 *
		 * @see moveAltAz()
		 */
		void setTargetAltAz (double alt, double az) { telAltAz->setValueAltAz (alt, az); }

		/**
		 * Return target position. This is equal to ORI[RA|DEC] +
		 * OFFS[RA|DEC] + any transformations required for mount
		 * operation.
		 *
		 * @param out_tar Target position
		 */
		void getTarget (struct ln_equ_posn *out_tar)
		{
			out_tar->ra = tarRaDec->getRa ();
			out_tar->dec = tarRaDec->getDec ();
		}

		void getTargetAltAz (struct ln_hrz_posn *hrz)
		{
			hrz->alt = telAltAz->getAlt ();
			hrz->az = telAltAz->getAz ();
		}

		/**
		 * Returns true if target was changed from the last
		 * sucessfull move command. Target position is position which includes
		 * telescope transformations (precession, modelling) and offsets specified by
		 * OFFS.
		 *
		 * If telescope has different strategy for setting offsets,
		 * e.g. offsets can be send by separate command, please use
		 * originChangedFromLastResync() and apply offsets retrieved by
		 * getUnappliedRaOffsetRa() and getUnappliedDecOffsetDec().
		 */
		bool targetChangeFromLastResync () { return tarRaDec->wasChanged (); }

		/**
		 * Return corrections in RA/HA.
		 *
		 * @return RA correction (in degrees).
		 */
		double getCorrRa () { return corrRaDec->getRa (); }

		/**
		 * Return corrections in DEC.
		 *
		 * @return DEC correction.
		 */
		double getCorrDec () { return corrRaDec->getDec (); }

		/**
		 * Return offset from last applied correction.
		 *
		 * @return RA offset - corrections which arrives from last applied correction.
		 */
		double getWaitCorrRa () { return wcorrRaDec->getRa (); }

		/**
		 * Return offset from last applied correction.
		 *
		 * @return DEC offset - corrections which arrives from last applied correction.
		 */
		double getWaitCorrDec () { return wcorrRaDec->getDec (); }

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
		double getCorrAlt () { return -getCorrZd (); }

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
		 * @param jd  Julian date for which position will be calculated.
		 */
		void getTelTargetAltAz (struct ln_hrz_posn *hrz, double jd);

		double getTargetHa ();
		double getTargetHa (double jd);

		double getLstDeg (double JD);

		virtual bool isBellowResolution (double ra_off, double dec_off) { return (ra_off == 0 && dec_off == 0); }

		void needStop () { maskState (TEL_MASK_NEED_STOP, TEL_NEED_STOP); }

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		virtual void valueChanged (Rts2Value * changed_value);

		virtual int deleteConnection (Rts2Conn * in_conn)
		{
			if (in_conn == move_connection)
				move_connection = NULL;
			return rts2core::Device::deleteConnection (in_conn);
		}

		// reload model
		virtual void signaledHUP ();

		/**
		 * Send telescope to requested coordinates. This function does not
		 * have any parameters, as they are various ways how to obtain
		 * telescope coordinates.
		 *
		 * If you want to get raw, J2000 target coordinates, without any offsets, check with
		 *
		 *
		 * @return 0 on success, -1 on error.
		 *
		 * @see originChangedFromLastResync
		 * @see getOrigin()
		 */
		virtual int startResync () = 0;

		/**
		 * Move telescope to target ALTAZ coordinates.
		 */
		virtual int moveAltAz ();

		/**
		 * Issue cupola synchronization event.
		 * Should use getTarget to obtain telescope target coordinates. Please note that
		 * if you modify telescope coordinates in startResync, and do not update telTarget (
		 * with setTarget call), you are responsible to overwrite this method and modify
		 * coordinates accordingly.
		 */
		virtual void startCupolaSync ();

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
		virtual int setTo (double set_ra, double set_dec) { return -1; }

		/**
		 * Called when park command is issued. Moves telescope to park position. Target
		 * positions are set to NAN after startPark returns 0. If driver needs to retain
		 * set target position (e.g. it is using moveAltAz to move to predefined AltAz pozition),
		 * startPark must reuturn 1.
		 *
		 * @return 0 on success, 1 on success if target value reset is not needed, -1 on failure
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
		virtual int saveModel () { return -1; }

		/**
		 * Load model from telescope.
		 */
		virtual int loadModel () { return -1; }

		virtual int stopWorm () { return -1; }

		virtual int startWorm () { return -1; }
		virtual int resetMount () { return 0; }

		/**
		 * Get current telescope altitude and azimuth. This
		 * function updates telAltAz value. If you want to get target
		 * altitude and azimuth, please use getTargetAltAz().
		 */
		virtual void getTelAltAz (struct ln_hrz_posn *hrz);

		/**
		 * Return expected time in seconds it will take to reach
		 * destination from current position.  This abstract method
		 * returns getTargetDistance / 2.0, estimate mount slew speed
		 * to 2 degrees per second. You can provide own estimate by
		 * overloading this method.
		 */
		virtual double estimateTargetTime ();

		double getTargetReached () { return targetReached->getValueDouble (); }

		/**
		 * Set differential tracking values. All inputs is in degrees / hour.
		 *
		 * @param dra  differential tracking in RA
		 * @param ddec differential tracking in DEC
		 */
		virtual void setDiffTrack (double dra, double ddec) {}

		/**
		 * Local sidereal time.
		 */
		Rts2ValueDouble *lst;

		Rts2ValueSelection *raGuide;
		Rts2ValueSelection *decGuide;

		void setBlockMove () { blockMove->setValueBool (true); sendValueAll (blockMove); }
		void unBlockMove () { blockMove->setValueBool (false); sendValueAll (blockMove); }
	private:
		Rts2Conn * move_connection;
		int moveInfoCount;
		int moveInfoMax;

		/**
		 * Last error.
		 */
		Rts2ValueDouble *posErr;

		/**
		 * If correction is bellow that value, it is ignored.
		 */
		Rts2ValueDouble *ignoreCorrection;

		Rts2ValueDouble *defIgnoreCorrection;

		/**
		 * If correction is bellow that value, it is considered as small correction.
		 */
		Rts2ValueDouble *smallCorrection;

		/**
		 * Limit for corrections.
		 */
		Rts2ValueDouble *correctionLimit; 

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
		 * Object we are observing original positions.
		 */
		Rts2ValueRaDec *oriRaDec;

		/**
		 * User offsets, used to create dithering pattern.
		 */
		Rts2ValueRaDec *offsRaDec;

		/**
		 * Offsets which should be applied from last movement.
		 */
		Rts2ValueRaDec *woffsRaDec;

		Rts2ValueRaDec *diffTrackRaDec;

		/**
		 * Real coordinates of the object, after offsets are applied.
		 * OBJ[RA|DEC] = ORI[|RA|DEC] + OFFS[|RA|DEC]
		 */
		Rts2ValueRaDec *objRaDec;

		/**
		 * Target we are pointing to. Coordinates feeded to telescope.
		 * TAR[RA|DEC] = OBJ[RA|DEC] + modelling, precession, etc.
		 */
		Rts2ValueRaDec *tarRaDec;

		/**
		 * Corrections from astrometry/user.
		 */
		Rts2ValueRaDec *corrRaDec;

		/**
		 * RA DEC correction which waits to be applied.
		 */
		Rts2ValueRaDec *wcorrRaDec;

		/**
		 * Modelling changes.
		 */
		Rts2ValueRaDec *modelRaDec;

		/**
		 * Corrected, modelled coordinates feeded to telescope.
		 */
		Rts2ValueRaDec *telTargetRaDec;

		/**
		 * If this value is true, any software move of the telescope is blocked.
		 */
		Rts2ValueBool *blockMove;

		Rts2ValueBool *blockOnStandby;

		// object + telescope position

		Rts2ValueBool *calAberation;
		Rts2ValueBool *calPrecession;
		Rts2ValueBool *calRefraction;
		Rts2ValueBool *calModel;

		rts2core::StringArray *cupolas;

		/**
		 * Target HRZ coordinates.
		 */
		struct ln_hrz_posn tarAltAz;

		/**
		 * Target HRZ coordinates with corrections applied.
		 */
		struct ln_hrz_posn corrAltAz;

		/**
		 * Telescope RA and DEC. In perfect world readed from sensors.
		 * target + model + corrRaDec = requested position -> telRaDec
		 */
		Rts2ValueRaDec *telRaDec;

		/**
		 * Current airmass.
		 */
		Rts2ValueDouble *airmass;

		/**
		 * Hour angle.
		 */
		Rts2ValueDouble *hourAngle;

		/**
		 * Distance to target in degrees.
		 */
		Rts2ValueDouble *targetDistance;

		/**
		 * Time when movement was started.
		 */
		Rts2ValueTime *targetStarted;

		/**
		 * Estimate time when current movement will be finished.
		 */
		Rts2ValueTime *targetReached;

		int startMove (Rts2Conn * conn, double tar_ra, double tar_dec, bool onlyCorrect);

		int startResyncMove (Rts2Conn * conn, bool onlyCorrect);

		/**
		 * Date and time when last park command was issued.
		 */
		Rts2ValueTime *mountParkTime;

		Rts2ValueInteger *moveNum;
		Rts2ValueInteger *corrImgId;

		Rts2ValueInteger *wCorrImgId;

		void checkMoves ();

		struct timeval dir_timeouts[4];

		char *modelFile;
		rts2telmodel::Model *model;

		Rts2ValueSelection *standbyPark;
		const char *horizonFile;

		ObjectCheck *hardHorizon;

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

		struct ln_ell_orbit mpec_orbit;

		/**
		 * Minor Planets Ephemerids one-line element. If set, target position and differential
		 * tracking are calculated from this string.
		 */
		Rts2ValueString *mpec;

		Rts2ValueDouble *mpec_refresh;
		Rts2ValueDouble *mpec_angle;

		// Value for RA DEC differential tracking
		Rts2ValueRaDec *diffRaDec;

		void recalculateMpecDIffs ();
};

};
#endif							 /* !__RTS2_TELD_CPP__ */
