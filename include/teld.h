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
#include "pluto/norad.h"

#include "device.h"
#include "objectcheck.h"

// pointing models
#define POINTING_RADEC          0
#define POINTING_ALTAZ          1
#define POINTING_ALTALT         2


#define SIDEREAL_HOURS          23.9344696

// Limit on number of steps for trajectory check
#define TRAJECTORY_CHECK_LIMIT  2000

namespace rts2telmodel
{
	class TelModel;
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
 *   - <b>ORIRA, ORIDEC</b> contains original, J2000 coordinates. Those are usually entered by observing program (rts2-executor).
 *   - <b>OFFSRA, OFFSDEC</b> contains offsets applied to ORI coordinates.
 *   - <b>OBJRA,OBJDEC</b> contains offseted coordinates. OBJRA = ORIRA + OFFSRA, OBJDEC = ORIDEC + OFFSDEC.
 *   - <b>TARRA,TARDEC</b> contains precessed etc. coordinates. Those coordinates do not contain modelling, which is stored in MORA and MODEC
 *   - <b>CORR_RA, CORR_DEC</b> contains offsets from on-line astrometry which are fed totelescope. Telescope coordinates are then calculated as TARRA-CORR_RA, TAR_DEC - CORR_DEC
 *   - <b>TELRA,TELDEC</b> contains coordinates read from telescope driver. In ideal word, they should eaual to TARRA - CORR_RA - MORA, TARDEC - CORR_DEC - MODEC. But they might differ. The two major sources of differences are: telescope do not finish movement as expected and small deviations due to rounding errors in mount or driver.
 *   - <b>MORA,MODEC</b> contains offsets comming from ponting model. They are shown only if this information is available from the mount (OpenTpl) or when they are caculated by RTS2 (Paramount).
 *
 * Following auxiliary values are used to track telescope offsets:
 *
 *   - <b>woffsRA, woffsDEC</b> contains offsets which weren't yet applied. They can be set either directly or when you change OFFS value. When move command is executed, OFFSRA += woffsRA, OFFSDEC += woffsDEC and woffsRA and woffsDEC are set to 0.
 *   - <b>wcorrRA, wcorrDEC</b> contains corrections which weren't yet applied. They can be set either directy or by correct command. When move command is executed, CORRRA += wcorrRA, CORRDEC += wcorrDEC and wcorrRA = 0, wcorrDEC = 0.
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
		Telescope (int argc, char **argv, bool diffTrack = false, bool hasTracking = false, int hasUnTelCoordinates = 0, bool hasAltAzDiff = false);
		virtual ~ Telescope (void);

		virtual void postEvent (rts2core::Event * event);

		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

		virtual rts2core::DevClient *createOtherType (rts2core::Connection * conn, int other_device_type);

		double getLatitude () { return telLatitude->getValueDouble (); }
		double getLongitude () { return telLongitude->getValueDouble (); }
		double getAltitude () { return telAltitude->getValueDouble (); }
		float getPressure () { return telPressure->getValueFloat (); }

		void setTelLongLat (double longitude, double latitude);
		void setTelAltitude (float altitude);

		double getHourAngle () { return hourAngle->getValueDouble (); }

		double getAltitudePressure (double alt, double sea_press);

		// callback functions from telescope connection
		virtual int info ();
		int infoJD (double JD);
		int infoLST (double telLST);
		virtual int infoJDLST (double JD, double telLST);

		virtual int scriptEnds ();

		int setTo (rts2core::Connection * conn, double set_ra, double set_dec);
		int setToPark (rts2core::Connection * conn);

		int startPark (rts2core::Connection * conn);

		virtual int getFlip ();

		/**
		 * Apply corrections to position.
		 */
		void applyCorrections (struct ln_equ_posn *pos, double JD, bool writeValues);

		/**
		 * Set telescope correctios.
		 *
		 * @param _aberation     If aberation should be calculated.
		 * @param _nutation      If nutation should be calculated.
		 * @param _precession    If precession should be calculated.
		 * @param _refraction    If refraction should be calculated.
		 */
		void setCorrections (bool _aberation, bool _nutation, bool _precession, bool _refraction)
		{
			calAberation->setValueBool (_aberation);
			calNutation->setValueBool (_nutation);
			calPrecession->setValueBool (_precession);
			calRefraction->setValueBool (_refraction);
		}

		/**
		 * If aberation should be calculated in RTS2.
		 */
		bool calculateAberation () { return calAberation->getValueBool (); }

		/**
		 * If nutation should be calculated in RTS2.
		 */
		bool calculateNutation () { return calNutation->getValueBool (); }

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

		virtual int commandAuthorized (rts2core::Connection * conn);

		virtual void setFullBopState (rts2_status_t new_state);

		/**
		 * Check if given hrz position is above horizon.
		 *
		 * @param hrz   horizontal position to check
		 *
		 * @return -1 if hard horizon was not specified, 0 if not above hard horizon, 1 if above hard horizon
		 */
		int isGood (struct ln_hrz_posn *hrz)
		{
			if (hardHorizon)
				return hardHorizon->is_good (hrz);
			else
				return -1;
		}

		void setModel (rts2telmodel::TelModel *_model) { model = _model; calModel->setValueBool (model != NULL); }
		
	protected:
		/**
		 * Creates values for guiding movements.
		 */
		void createRaPAN ();
		void createDecPAN ();

		rts2core::ValueSelection *raPAN;
		rts2core::ValueSelection *decPAN;

		rts2core::ValueDouble *raPANSpeed;
		rts2core::ValueDouble *decPANSpeed;

		void applyOffsets (struct ln_equ_posn *pos)
		{
			pos->ra = oriRaDec->getRa () + offsRaDec->getRa ();
			pos->dec = oriRaDec->getDec () + offsRaDec->getDec ();
		}

		void applyAltAzOffsets (struct ln_hrz_posn *hrz)
		{
			hrz->az = ln_range_degrees (hrz->az + offsAltAz->getAz ());
			hrz->alt += offsAltAz->getAlt ();
		}

		void reverseAltAzOffsets (double &alt, double &az)
		{
			alt -= offsAltAz->getAlt ();
			az = ln_range_degrees (az - offsAltAz->getAz ());
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
			normalizeRaDec (equ->ra, equ->dec);
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
		 * Set telescope untouched (i.e. physical) RA and DEC.
		 */
		void setTelUnRaDec (double new_ra, double new_dec) 
		{
			telUnRaDec->setRa (new_ra);
			telUnRaDec->setDec (new_dec);
		}

		void setTelUnAltAz (double alt, double az)
		{
			telUnAltAz->setAlt (alt);
			telUnAltAz->setAz (az);
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
		void zeroCorrRaDec () {corrRaDec->setValueRaDec (0, 0); corrRaDec->resetValueChanged (); wcorrRaDec->setValueRaDec (0, 0); wcorrRaDec->resetValueChanged ();};

		/**
		 * Apply model for RA/DEC position pos, for specified flip and JD.
		 * All resulting coordinates also includes corrRaDec corection. 
		 * If writeValue is set to true, changes tel_target (telTargetRA) variable, including model computation and corrRaDec.
		 * Also sets MO_RTS2 (modelRaDec) variable, mirroring (only) computed model difference.
		 * Can be used to compute non-cyclic model, with flip=0 and pos in raw mount coordinates.
		 *
		 * @param pos ln_equ_posn RA/DEC position (typically TAR, i.e. precessed coordinates)
		 * @param model_change ln_equ_posn difference against original pos position, includes coputed model's difference together with correction corrRaDec.
		 */
		void applyModel (struct ln_equ_posn *m_pos, struct ln_equ_posn *tt_pos, struct ln_equ_posn *model_change, double JD);

		void applyModelAltAz (struct ln_hrz_posn *hrz, struct ln_hrz_posn *err);

		/**
		 * Apply precomputed model by computeModel (), set everything equivalently what applyModel () does.
		 * Sets MO_RTS2 (modelRaDec) and tel_target (telTargetRA) variables, also includes applyCorrRaDec if applyCorr parameter set to true.
		 */
		void applyModelPrecomputed (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, bool applyCorr);

		/**
		 * Compute model for RA/DEC position pos and JD.
		 * Can be used to compute non-cyclic model, with flip=0 and pos in raw mount coordinates.
		 *
		 * @param pos ln_equ_posn RA/DEC position (typically TAR, i.e. precessed coordinates), will be corrected by computed model.
		 * @param model_change ln_equ_posn coputed model's difference.
		 */
		void computeModel (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, double JD);

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
		virtual void applyCorrections (double &tar_ra, double &tar_dec, bool writeValues);

		virtual int willConnect (rts2core::NetworkAddress * in_addr);
		rts2core::ValueAltAz *telAltAz;

		rts2core::ValueInteger *telFlip;
		rts2core::ValueInteger *peekFlip;

		int flip_move_start;
		int flip_longest_path;

		double defaultRotang;

		rts2core::ValueDouble *rotang;

		rts2core::ValueDouble *telLongitude;
		rts2core::ValueDouble *telLatitude;
		rts2core::ValueDouble *telAltitude;
		rts2core::ValueFloat *telPressure;


		/**
		 * Precalculated latitude values..
		 */
		double cos_lat;
		double sin_lat;
		double tan_lat;

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
		void getOrigin (struct ln_equ_posn *_ori)
		{
			_ori->ra = oriRaDec->getRa ();
			_ori->dec = oriRaDec->getDec ();
		}
	
		void setOrigin (double ra, double dec)
		{
			oriRaDec->setValueRaDec (ra, dec);
		}

		void setObject (double ra, double dec)
		{
			objRaDec->setValueRaDec (ra, dec);
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
			modelRaDec->setValueRaDec (0, 0);
		}

                void setTelTarget (double ra, double dec)
                {
                        telTargetRaDec->setValueRaDec (ra, dec);
                }

		/**
		 * Sets new movement target, used exclusively to change target flip.
		 */
		void setTarTelRaDec (struct ln_equ_posn *pos);
		void setTarTelAltAz (struct ln_hrz_posn *hrz);

		/**
		 * Set WCS reference values telescope is reporting.
		 *
		 * @param ra  WCS RA (CRVAL1)
		 * @param dec WCS DEC (CRVAL2)
		 */
		void setCRVAL (double ra, double dec)
		{
			if (wcs_crval1)
			{
				wcs_crval1->setValueDouble (ra);
				sendValueAll (wcs_crval1);
			}

			if (wcs_crval2)
			{
				wcs_crval2->setValueDouble (dec);
				sendValueAll (wcs_crval2);
			}
		}

		/**
		 * Sets target to nan. If startResync is called, it forced
		 * it to recompute target positions.
		 */
		void resetTelTarget ()
		{
			tarRaDec->setValueRaDec (NAN, NAN);
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

		double getTargetRa () { return tarRaDec->getRa (); }
		double getTargetDec () { return tarRaDec->getDec (); }

		/**
		 * Calculate target position for given JD.
		 * This function shall be used for adaptive tracking.
		 *
		 * If tracking is set to object, future position is calculated from
		 * current RA DEC position. Otherwise, future position is calculated
		 * from target position, where target position can change in time (for 
		 * MPEC and LTE targets).
		 *
		 * @param JD	         Julian date for which position will be calculated
		 * @param out_tar	 target position, excluding precission, ..
		 * @param ac	         current (input) and target (output) HA axis value
		 * @param dc	         current (input) and target (output) DEC axis value
		 * @param writeValues    if true, values inside RTS2 will be updated to reflect new target position
		 * @param haMargin       margin (in degrees) for which HA axis must be allowed to move in sidereal tracking
		 * @param forceShortest  if true, shortest path wlll be taken - desired flipping will be ignored
		 */
		int calculateTarget (double JD, struct ln_equ_posn *out_tar, int32_t &ac, int32_t &dc, bool writeValues, double haMargin, bool forceShortest);

		/**
		 * Calculate speed vector from arc of given duration.
		 *
		 * This method calculates position in time JD and JD + sec_step / 86400.0.
		 *
		 * @param JD             start Julian date
		 * @param sec_step       lenght of arc in seconds
		 * @param ac             current (input) and target (output) first axis (HA, AZ) count value
		 * @param dc             current (intput) and target (output) second axis (DEC, ALT) count value
		 * @param ac_speed       first axis speed in counts per second
		 * @param dc_speed       second axis speed in counts per second
		 */
		int calculateTracking (double JD, double sec_step, int32_t &ac, int32_t &dc, int32_t &ac_speed, int32_t &dc_speed);

		/**
		 * Transform sky coordinates to axis coordinates. Implemented in classes
                 * commanding directly the telescope axes in counts.
                 *
                 * @param JD		date for which transformation will be valid
                 * @param pos		target position (sky position, excluding precession, refraction, and corections ...)
		 * @param ac		current (input) and target (output) HA axis counts value
		 * @param dc		current (input) and target (output) DEC axis counts value
		 * @param writeValues   when true, RTS2 values will be updated to reflect new target values
                 * @param haMargin      ha value (in degrees), for which mount must be allowed to move
		 * @param forceShortest if true, shortest path will be taken - desired flipping will be ignored
		 */
		virtual int sky2counts (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc, bool writeValue, double haMargin, bool forceShortest);

		void addDiffRaDec (struct ln_equ_posn *tar, double secdiff);
		void addDiffAltAz (struct ln_hrz_posn *hrz, double secdiff);

		/**
		 * Returns time when move start was commanded (in ctime).
		 */
		double getTargetStarted ()
		{
			return targetStarted->getValueDouble ();
		}

		double getOffsetRa () { return offsRaDec->getRa (); }

		double getOffsetDec () { return offsRaDec->getDec (); }

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

		double getStoredTargetDistance () { return targetDistance->getValueDouble (); }

		/**
		 * Returns ALT AZ coordinates of target.
		 *
		 * @param hrz ALT AZ coordinates of target.
		 * @param jd  Julian date for which position will be calculated.
		 */
		void getTargetAltAz (struct ln_hrz_posn *hrz, double jd);

		double getTargetHa ();
		double getTargetHa (double jd);

		double getLstDeg (double JD);

		virtual bool isBellowResolution (double ra_off, double dec_off) { return (ra_off == 0 && dec_off == 0); }

		void needStop () { maskState (TEL_MASK_NEED_STOP, TEL_NEED_STOP); }

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual void valueChanged (rts2core::Value * changed_value);

		virtual int deleteConnection (rts2core::Connection * in_conn)
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
		 * Offset telescope coordinates. Called when some offset values (either OFFS or CORR) changed
		 * and telescope needs to be reposiented.
		 */
		virtual void startOffseting (rts2core::Value *changed_value);

		/**
		 * Preview (peek) movement to given RA DEC coordinates. Peek
		 * variables names are prefixed with peek_, and depends on telescope
		 * model (GEM, Alt-Az, Fork etc..). Usually target axis counts values
		 * are present, with optional data on axis orientation and time
		 * telescope can remain tracking at sidereal rate.
		 *
		 * @param ra    peek RA coordinate
		 * @param dec   peek DEC coordinate
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int peek (double ra, double dec);

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
		 * Called when movements fails.
		 */
		virtual void failedMove ();

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
		 * Called when telescope is above horizon. Usefull to retrieve
		 * tick values, which are checked as good.
		 */
		virtual void telescopeAboveHorizon () {}

		/** 
		 * Called when telescope is suddently pointed below horizon. Returns < 0 on error, 0 on sucess, 1 when abort was not called (temproary allowed violation of bellow horizon..)
		 */
		virtual int abortMoveTracking ();

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
		 * Set telescope to park position.
		 *
		 * @return -1 on error, otherwise 0
		 */
		virtual int setToPark () { return -1; }

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
		 * Set telescope tracking.
		 *
		 * @param track		0 - no tracking, 1 - on object, 2 - sidereal
		 * @param addTrackingTimer     if true and tracking, add tracking timer; cannot be set when called from tracking function!
		 * @param send		 if true, set rts2value and send in to all connections
		 * @return 0 on success, -1 on error
		 */
		virtual int setTracking (int track, bool addTrackingTimer = false, bool send = true);

		void startTracking (bool check = false);

		/**
		 * Returns if tracking is requested.
		 *
		 * @return 0 - no tracking, 1 - tracking to object, 2 - sidereal tracking
		 */
		int trackingRequested () { return tracking->getValueInteger (); }

		bool isTracking () { return (getState () & TEL_MASK_TRACK) == TEL_TRACKING; }

		/**
		 * Stops tracking. Calls stopMove and set tracking state to NOTRACK.
		 */
		void stopTracking (const char *msg = "tracking stopped");

		/**
		 * Called to run tracking. It is up to driver implementation
		 * to send updated position to telescope driver.
		 *
		 * If tracking timer shall not be called agin, call setTracking (false).
		 * Run every trackingInterval seconds.
		 *
		 * @see setTracking
		 * @see trackingInterval
		 */
		virtual void runTracking ();

		/**
		 * Writes tracking information to the log.
		 */
		void logTracking ();

		/**
		 * Update tracking frequency. Should be run after new tracking vector
		 * is send to the mount motion controller.
		 */
		void updateTrackingFrequency ();

		/**
		 * Calculate TLE RA DEC for given time.
		 */
		void calculateTLE (double JD, double &ra, double &dec, double &dist_to_satellite);

		/**
		 * Set differential tracking values. All inputs is in degrees / hour.
		 *
		 * @param dra  differential tracking in RA
		 * @param ddec differential tracking in DEC
		 */
		virtual void setDiffTrack (double dra, double ddec);

		virtual void setDiffTrackAltAz (double daz, double dalt);

		/**
		 * Hard horizon. Use it to check if telescope coordinates are within limits.
		 */
		ObjectCheck *hardHorizon;

		/**
		 * Telescope parking position.
		 */
		rts2core::ValueAltAz *parkPos;
		
		/**
		 * Desired flip when parking.
		 */
		rts2core::ValueInteger *parkFlip;

		/**
		 * Add option for parking position.
		 *
		 * @warning parkPos is created only if this option was passed to the programe. If you
		 * need parkPos to exists (and fill some default value), call createParkPos first.
		 */
		void addParkPosOption ();

		/**
		 * Create parkPos variable.
		 */
		void createParkPos (double alt, double az, int flip);

		bool useParkFlipping;

		/**
		 * Local sidereal time.
		 */
		rts2core::ValueDouble *lst;

		/**
		 * Telescope idea of julian date.
		 */
		rts2core::ValueDouble *jdVal;

		rts2core::ValueFloat *trackingInterval;

		/**
		 * Returns differential tracking values. Telescope must support
		 * differential tracking for those to not core dump. Those methods
		 * should be called only from child subclass which pass true for
		 * diffTrack in Telescope contructor.
		 */
		double getDiffTrackRa () { return diffTrackRaDec->getRa (); }
		double getDiffTrackDec () { return diffTrackRaDec->getDec (); }

		double getDiffTrackAz () { return diffTrackAltAz->getAz (); }
		double getDiffTrackAlt () { return diffTrackAltAz->getAlt (); }

		void setBlockMove () { blockMove->setValueBool (true); sendValueAll (blockMove); }
		void unBlockMove () { blockMove->setValueBool (false); sendValueAll (blockMove); }

		void getHrzFromEqu (struct ln_equ_posn *pos, double JD, struct ln_hrz_posn *hrz);
		void getEquFromHrz (struct ln_hrz_posn *hrz, double JD, struct ln_equ_posn *pos);

		/**
		 * Apply aberation correction.
		 */
		void applyAberation (struct ln_equ_posn *pos, double JD, bool writeValue);

		/**
		 * Apply nutation correction.
		 */
		void applyNutation (struct ln_equ_posn *pos, double JD, bool writeValue);

		/**
		 * Apply precision correction.
		 */
		void applyPrecession (struct ln_equ_posn *pos, double JD, bool writeValue);

		/**
		 * Apply refraction correction.
		 */
		void applyRefraction (struct ln_equ_posn *pos, double JD, bool writeValue);

		virtual void afterMovementStart ();

	private:
		rts2core::Connection * move_connection;
		int moveInfoCount;
		int moveInfoMax;

		rts2core::ValueSelection *tracking;
		rts2core::ValueDoubleStat *trackingFrequency;
		rts2core::ValueInteger *trackingFSize;
		rts2core::ValueFloat *trackingWarning;
		double lastTrackingRun;

		/**
		 * Last error.
		 */
		rts2core::ValueDouble *posErr;

		/**
		 * If correction is bellow that value, it is ignored.
		 */
		rts2core::ValueDouble *ignoreCorrection;

		rts2core::ValueDouble *defIgnoreCorrection;

		/**
		 * If correction is bellow that value, it is considered as small correction.
		 */
		rts2core::ValueDouble *smallCorrection;

		/**
		 * Limit for corrections.
		 */
		rts2core::ValueDouble *correctionLimit; 

		/**
		 * If move is above this limit, correction is rejected.
		 */
		rts2core::ValueDouble *modelLimit;

		/**
		 * If correction is above that limit, cancel exposures and
		 * move immediatelly. This is to signal we are out of all cameras
		 * FOV.
		 */
		rts2core::ValueDouble *telFov;

		/**
		 * Object we are observing original positions (in J2000).
		 */
		rts2core::ValueRaDec *oriRaDec;

		/**
		 * User offsets, used to create dithering pattern.
		 */
		rts2core::ValueRaDec *offsRaDec;

		/**
		 * User alt-az offsets. Supported primary on alt-az mounts.
		 */
		rts2core::ValueAltAz *offsAltAz;

		/**
		 * Offsets which should be applied from last movement.
		 */
		rts2core::ValueRaDec *woffsRaDec;

		rts2core::ValueRaDec *diffTrackRaDec;

		/**
		 * Start time of differential tracking.
		 */
		rts2core::ValueDouble *diffTrackStart;

		/**
		 * Differential tracking on/off.
		 */
		rts2core::ValueBool *diffTrackOn;

		/**
		 * Alt/az diferential tracking.
		 */
		rts2core::ValueAltAz *diffTrackAltAz;
		rts2core::ValueDouble *diffTrackAltAzStart;
		rts2core::ValueBool *diffTrackAltAzOn;

		/**
		 * Coordinates of the object, after offsets are applied (in J2000).
		 * OBJ[RA|DEC] = ORI[|RA|DEC] + OFFS[|RA|DEC]
		 */
		rts2core::ValueRaDec *objRaDec;

		/**
		 * Real sky coordinates of target, with computed corrections (precession, aberation, refraction). Still without corrRaDec (astrometry feedback) and pointing model. Dec in 180..-180 range for GEMs, forks etc mounts.
		 * TAR[RA|DEC] = OBJ[RA|DEC] + precession, etc.
		 */
		rts2core::ValueRaDec *tarRaDec;

		rts2core::ValueRaDec *tarTelRaDec;
		rts2core::ValueAltAz *tarTelAltAz;

		/**
		 * Corrections from astrometry/user.
		 */
		rts2core::ValueRaDec *corrRaDec;

		/**
		 * RA DEC correction which waits to be applied.
		 */
		rts2core::ValueRaDec *wcorrRaDec;

		rts2core::ValueRaDec *total_offsets;

		rts2core::ValueRaDec *precessed;
		rts2core::ValueRaDec *nutated;
		rts2core::ValueRaDec *aberated;

		rts2core::ValueDouble *refraction;

		/**
		 * Modelling changes.
		 */
		rts2core::ValueRaDec *modelRaDec;

		/**
		 * Telescope target coordinates, corrected (precessed, aberated, refracted) with modelling offset.
		 */
		rts2core::ValueRaDec *telTargetRaDec;

		/**
		 * Sky speed vector. Speeds in RA and DEC for in degrees/hour.
		 */
		rts2core::ValueRaDec *skyVect;

		/**
		 * If this value is true, any software move of the telescope is blocked.
		 */
		rts2core::ValueBool *blockMove;

		rts2core::ValueBool *blockOnStandby;

		// object + telescope position

		rts2core::ValueBool *calPrecession;
		rts2core::ValueBool *calNutation;
		rts2core::ValueBool *calAberation;
		rts2core::ValueBool *calRefraction;
		rts2core::ValueBool *calModel;

		rts2core::StringArray *cupolas;

		rts2core::StringArray *rotators;

		/**
		 * Target HRZ coordinates.
		 */
		struct ln_hrz_posn tarAltAz;

		/**
		 * Target HRZ coordinates with corrections applied.
		 */
		struct ln_hrz_posn corrAltAz;

		rts2core::ValueDouble *wcs_crval1;
		rts2core::ValueDouble *wcs_crval2;

		/**
		 * Telescope RA and DEC. In perfect world read from sensors, transformed to sky coordinates (i.e. within standard limits)
		 * target + model + corrRaDec = requested position -> telRaDec
		 */
		rts2core::ValueRaDec *telRaDec;

		/**
		 * Telescope untouched physical axis angles, read from sensors, without flip-transformation.
		 * Alt-Az for altaz, hadec for GEM
		 */
		rts2core::ValueRaDec *telUnRaDec;
		rts2core::ValueAltAz *telUnAltAz;

		/**
		 * Current airmass.
		 */
		rts2core::ValueDouble *airmass;

		/**
		 * Hour angle.
		 */
		rts2core::ValueDouble *hourAngle;

		/**
		 * Distance to target in degrees.
		 */
		rts2core::ValueDouble *targetDistance;

		/**
		 * Time when movement was started.
		 */
		rts2core::ValueTime *targetStarted;

		/**
		 * Estimate time when current movement will be finished.
		 */
		rts2core::ValueTime *targetReached;

		/**
		 *
		 * @param correction   correction type bitmask - 0 for no corerction, 1 for offsets, 2 for correction
		 */
		int startResyncMove (rts2core::Connection * conn, int correction);

		/**
		 * Date and time when last park command was issued.
		 */
		rts2core::ValueTime *mountParkTime;

		rts2core::ValueInteger *moveNum;
		rts2core::ValueInteger *corrImgId;
		rts2core::ValueInteger *failedMoveNum;
		rts2core::ValueTime *lastFailedMove;

		rts2core::ValueInteger *wCorrImgId;

		/**
		 * Tracking / idle refresh interval
		 */
		rts2core::ValueDouble *refreshIdle;

		/**
		 * Slewing refresh interval
		 */
		rts2core::ValueDouble *refreshSlew;

		void checkMoves ();

		struct timeval dir_timeouts[4];

		char *tPointModelFile;
		char *rts2ModelFile;
		rts2telmodel::TelModel *model;

		rts2core::ValueSelection *standbyPark;
		const char *horizonFile;

		/**
		 * Zero's all corrections, increment move count. Called before move.
		 */
		void incMoveNum ();

		/** 
		 * Which coordinates are used for pointing (eq, alt-az,..)
		 */
		rts2core::ValueSelection *pointingModel;

		struct ln_ell_orbit mpec_orbit;

		/**
		 * Minor Planets Ephemerids one-line element. If set, target position and differential
		 * tracking are calculated from this string.
		 */
		rts2core::ValueString *mpec;

		rts2core::ValueDouble *mpec_refresh;
		rts2core::ValueDouble *mpec_angle;

		/**
		 * Satellite (from Two Line Element, passed as string with lines separated by :)
		 * tracking.
		 */

		rts2core::ValueString *tle_l1;
		rts2core::ValueString *tle_l2;
		rts2core::ValueInteger *tle_ephem;
		rts2core::ValueDouble *tle_distance;
		rts2core::ValueBool *tle_freeze;
		rts2core::ValueDouble *tle_rho_sin_phi;
		rts2core::ValueDouble *tle_rho_cos_phi;

		rts2core::ValueDouble *tle_refresh;

		rts2core::ValueDouble *trackingLogInterval;

		tle_t tle;

		// Value for RA DEC differential tracking
		rts2core::ValueRaDec *diffRaDec;

		void recalculateMpecDIffs ();
		void recalculateTLEDiffs ();

		char wcs_multi;

		rts2core::ValueFloat *decUpperLimit;

		void resetMpecTLE ();

		double nextCupSync;
		double lastTrackLog;
};

};
#endif							 /* !__RTS2_TELD_CPP__ */
