/* 
 * Target classes.
 * Copyright (C) 2003-2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS_TARGETDB__
#define __RTS_TARGETDB__

#include "imgdisplay.h"
#include <errno.h>
#include <libnova/libnova.h>
#include <ostream>
#include <stdio.h>
#include <time.h>

#include "status.h"

#include "infoval.h"
#include "objectcheck.h"
#include "device.h"
#include "rts2target.h"
#include "counted_ptr.h"

#include "targetset.h"
#include "labels.h"

#include "scriptcommands.h"

#define MAX_READOUT_TIME    120
#define PHOT_TIMEOUT         10
#define EXPOSURE_TIMEOUT     50

#define TARGET_DARK           1
#define TARGET_FLAT           2
#define TARGET_FOCUSING       3
// orimary model
#define TARGET_MODEL          4
// primary terestial - based on HAM/FRAM number
#define TARGET_TERRESTIAL     5
// master calibration target
#define TARGET_CALIBRATION    6
// master plan target
#define TARGET_PLAN           7

#define TARGET_SWIFT_FOV     10
#define TARGET_INTEGRAL_FOV  11
#define TARGET_SHOWER        12

#define CONSTRAINTS_NONE     0x0000
#define CONSTRAINTS_SYSTEM   0x0001
#define CONSTRAINTS_GROUP    0x0002
#define CONSTRAINTS_TARGET   0x0004

namespace rts2image
{
class Image;
}

namespace rts2db {

class TargetSet;
class Constraint;
class Constraints;
class Observation;

class ConstraintsList;
class ConstraintDoubleInterval;

typedef counted_ptr <Constraint> ConstraintPtr;

typedef std::vector < std::pair < time_t, time_t > > interval_arr_t;

/**
 * Execption raised when target name cannot be resolved.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class UnresolvedTarget:public rts2core::Error
{
	public:
		UnresolvedTarget (const char *n)
		{
			name = std::string (n);
			setMsg (std::string ("cannot resolve target name/ID ") + name);
		}
		virtual ~UnresolvedTarget () throw () {}

		const char* getTargetName () { return name.c_str (); }
	
	private:
		std::string name;
};

/**
 * Exception raised when default device script cannot be found in configuration file.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class DeviceMissingExcetion:public rts2core::Error
{
	public:
		DeviceMissingExcetion (const char *deviceName):rts2core::Error ()
		{
			setMsg (std::string ("default script of device ") + deviceName + " cannot be found in RTS2 configuration file.");
		}
};

class CameraMissingExcetion:public rts2core::Error
{
	public:
		CameraMissingExcetion (const char *cameraName):rts2core::Error ()
		{
			setMsg (std::string ("camera ") + cameraName + " cannot be found in cameras table.");
		}
};

/**
 * Class for one observation target.
 *
 * It is created when it's possible to observer such target, and when such
 * observation have some scientific value.
 *
 * It's put on the observation que inside planc. It's then executed.
 *
 * During execution it can do centering (acquiring images to better know scope position),
 * it can compute importance of future observations.
 *
 * After execution planc select new target for execution, run image processing
 * on first observation target in the que.
 *
 * After all images are processed, new priority can be computed, some results
 * (e.g. light curves) can be put to the database.
 *
 * Target is connected to observation entry in the DB.
 *
 * All files loaded in constraints are added to inotify watch descriptor, and
 * their watch ID is appended to array of watch ID. Please see
 * revalideConstraints(int) method for futher discussion.
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
class Target:public Rts2Target
{
	public:
		/**
		 * Construct new target. As Target is abstract class, used mainly in 
		 * childrens.
		 *
		 * @param  in_tar_id     target number
		 * @param  in_obs        observer position (longitude, latitude)
		 * @param  in_altitude   observer altitude (in meters)
		 */
		Target (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);

		// create new target. Save will save it to the database..
		Target ();
		virtual ~ Target (void);

		void logMsgDb (const char *message, messageType_t msgType);
		void getTargetSubject (std::string & subj);

		/**
		 * Load target from the database.
		 *
		 * @throw rts2core::Error and descendants on error
		 */
		virtual void load ();
		// load target data from give target id
		void loadTarget (int in_tar_id);

		virtual int save (bool overwrite);
		virtual int saveWithID (bool overwrite, int tar_id);

		/**
		 * Delete target and all associated entries from database.
		 */
		void deleteTarget ();

		/** 
		 * Return script for target action.
		 *
		 * @brief Returns script for device action. Scripts are used to perform actions
		 * on device during target execution.
		 * 
		 * @param device_name script device
		 * @param buf buffer script
		 * 
		 * @throw CameraMissingExcetion when script was not specified and camera cannot be found in rts2.ini
		 */
		virtual bool getScript (const char *device_name, std::string & buf);
		void setScript (const char *device_name, const char *buf);

		/**
		 * Get target project investigator name.
		 */
		std::string getPIName ();

		/**
		 * Set PI id.
		 */
		void setPIName (const char *name);

		/**
		 * Get target program name.
		 */
		std::string getProgramName ();

		/**
		 * Set program ID.
		 */
		void setProgramName (const char *program);

		/**
		 * Set target constraints. Overwrite present target constraints with constraints from the given
		 * Constraints structure.
		 *
		 * @param cons   set constraints to this structure
		 *
		 * @throw  rts2core::Error if output file cannot be created
		 */
		void setConstraints (Constraints &cons);

		/**
		 * Append constraints to target. If consiraint value is
		 * outside of what is expected, constraint is removed.
		 */
		void appendConstraints (Constraints &cons);

		struct ln_lnlat_posn *getObserver () { return observer; }

		int getTelescopeMode () { return tar_telescope_mode; }

		// return target semi-diameter, in degrees
		double getSDiam () { return getSDiam (ln_get_julian_from_sys ()); }

		virtual double getSDiam (double JD)
		{
			// 30 arcsec..
			return 30.0 / 3600.0;
		}

		/**
		 * Return target horizontal coordinates for actual system time.
		 *
		 * @param hrz Returned coordinates.
		 *
		 * @return 0 if coordinates can be calculated.
		 */
		void getAltAz (struct ln_hrz_posn *hrz) { getAltAz (hrz, ln_get_julian_from_sys ()); }

		/**
		 * Return target horizontal coordinates for given date on given position.
		 *
		 * @param hrz returned coordinates
		 * @param JD  Julian data for which target coordinates will be calculated
		 */
		void getAltAz (struct ln_hrz_posn *hrz, double JD) { getAltAz (hrz, JD, observer); };


		/**
		 * Return target horizontal coordinates for given date on given position.
		 *
		 * @param hrz returned coordinates
		 * @param JD  Julian data for which target coordinates will be calculated
		 * @param obs observer position
		 */
		virtual void getAltAz (struct ln_hrz_posn *hrz, double JD, struct ln_lnlat_posn *obs);

		/**
		 * Returns target minimal and maximal altitude during
		 * given time period. This method may return negative values
		 * for both minimal and maximal altitude - those indicate that
		 * target remain bellow horizont for the whole period.
		 *
		 * @param _start JD of start period.
		 * @param _end   JD of end period.
		 * @param _min   Minimal altitude during period.
		 * @param _max   Maximal altitude during period.
		 */
		void getMinMaxAlt (double _start, double _end, double &_min, double &_max);

		/**
		 * Return target minimal observational altitude.
		 *
		 * @return Minimal target observational altitude in degrees.
		 */
		virtual double getMinObsAlt () { return minObsAlt; }

		/**
		 * Returns galactic coordinates of the target at actual date.
		 *
		 * @param gal Returned galactic coordinates of the target.
		 */
		void getGalLng (struct ln_gal_posn *gal) { getGalLng (gal, ln_get_julian_from_sys ()); }

		/**
		 * Returns galactic coordinates of the target at give Julian day.
		 *
		 * @param gal Returned galactic coordinates of the target.
		 * @param JD Julian date for which galactic coordinates will be calculated.
		 */
		virtual void getGalLng (struct ln_gal_posn *gal, double JD);

		/**
		 * Returns angular distance to galactic center.
		 *
		 * @return Angular distance to galactic center.
		 */
		double getGalCenterDist () { return getGalCenterDist (ln_get_julian_from_sys ()); }

		/**
		 * Returns angular distance to galactic center.
		 *
		 * @param JD Julian Date for which galactic coordinates will be returned.
		 *
		 * @return Angular distance to galactic center.
		 */
		double getGalCenterDist (double JD);

		double getAirmass () { return getAirmass (ln_get_julian_from_sys ()); }
		virtual double getAirmass (double JD);

		double getZenitDistance () { return getZenitDistance (ln_get_julian_from_sys ()); }

		double getZenitDistance (double JD);

		/**
		 * Returns target hour angle in arcdeg at actual time.
		 */
		double getHourAngle () { return getHourAngle (ln_get_julian_from_sys ()); }

		/**
		 * Returns target hour angle in arcdeg at given Julian day.
		 *
		 * @param JD    Julian day for which target hour angle will be calculated.
		 *
		 * @return Hour angle of the target at given Julian day.
		 */
		double getHourAngle (double JD) { return getHourAngle (JD, observer); }

		/**
		 * Returns target hour angle in arcdeg at given Julian day.
		 *
		 * @param JD    Julian day for which target hour angle will be calculated.
		 * @param obs   Observer position on the Earth surface
		 *
		 * @return Hour angle of the target at given Julian day.
		 */
		double getHourAngle (double JD, struct ln_lnlat_posn *obs);

		/**
		 * Returns angular distance of the target to a given position.
		 *
		 * @param _pos Position for which distance of the target will be calculated.
		 *
		 * @return Angular distance to given position at system time in arcdeg.
		 */
		double getDistance (struct ln_equ_posn *in_pos) { return getDistance (in_pos, ln_get_julian_from_sys ()); }

		/**
		 * Returns angular distance of the target at a given Julian date to a given position.
		 *
		 * @param _pos Position for which distance will be calculated.
		 * @param JD Julian date for which target position will be calculated.
		 *
		 * @return Angular distance to given position in arcdeg.
		 */
		double getDistance (struct ln_equ_posn *in_pos, double JD);

		double getRaDistance (struct ln_equ_posn *in_pos) { return getRaDistance (in_pos, ln_get_julian_from_sys ()); }

		double getRaDistance (struct ln_equ_posn *in_pos, double JD);

		/**
		 * Returns degree distance of the target to Sun center.
		 */
		double getSolarDistance () { return getSolarDistance (ln_get_julian_from_sys ()); }

		double getSolarDistance (double JD);

		double getSolarRaDistance () { return getSolarRaDistance (ln_get_julian_from_sys ()); }

		double getSolarRaDistance (double JD);

		double getLunarDistance () { return getLunarDistance (ln_get_julian_from_sys ()); }

		double getLunarDistance (double JD);

		double getLunarRaDistance () { return getLunarRaDistance (ln_get_julian_from_sys ()); }

		double getLunarRaDistance (double JD);

		double getMeridianDistance () { return fabs (getHourAngle ()); }

		double getMeridianDistance (double JD) { return fabs (getHourAngle (JD)); }

		// time in seconds to object set/rise/meridian pass (if it's visible, otherwise -1 (for
		// circumpolar & not visible objects)
		int secToObjectSet () { return secToObjectSet (ln_get_julian_from_sys ()); }

		int secToObjectSet (double JD);

		int secToObjectRise () { return secToObjectRise (ln_get_julian_from_sys ()); }

		int secToObjectRise (double JD);

		int secToObjectMeridianPass () { return secToObjectMeridianPass (ln_get_julian_from_sys ()); }

		int secToObjectMeridianPass (double JD);

		int getObsId ();

		int getPlanId ();

		/**
		 * Sets target information string.
		 *
		 * @param _tar_info New target info parameter.
		 */
		void setTargetInfo (std::string _tar_info) { tar_info = _tar_info; }

		const char *getTargetInfo () { return tar_info.c_str (); }

		const char *getTargetComment () { return target_comment; }
		void setTargetComment (const char *in_target_comment)
		{
			delete target_comment;
			target_comment = new char[strlen (in_target_comment) + 1];
			strcpy (target_comment, in_target_comment);
		}

		float getTargetPriority () { return tar_priority; }

		void setTargetPriority (float new_priority) { tar_priority = new_priority; }

		float getTargetBonus () { return tar_bonus; }

		const time_t *getTargetBonusTime () { return &tar_bonus_time; }

		virtual void setTargetBonus (float new_bonus, time_t * new_time = NULL)
		{
			tar_bonus = new_bonus;
			if (new_time)
				setTargetBonusTime (new_time);
		}

		void setTargetBonusTime (time_t * new_time) { tar_bonus_time = *new_time; }

		int getRST (struct ln_rst_time *rst) { return getRST (rst, ln_get_julian_from_sys (), LN_STAR_STANDART_HORIZON); }

		/**
		 * Return next rise/set time for given horizon.
		 */
		virtual int getRST (struct ln_rst_time *rst, double jd, double horizon) = 0;

		// return if object is visible at observatory location during specified night
		bool isVisibleDuringNight (double jd, double horizon);

		bool isVisibleDuringCurrentNight () { return isVisibleDuringNight (ln_get_julian_from_sys (), getMinObsAlt ()); }

		// returns 1 when we are almost the same target, so
		// interruption of this target is not necessary
		// otherwise (when interruption is necessary) returns 0
		virtual int compareWithTarget (Target * in_target, double in_sep_limit);

                /**
		 * Get coordinates of the new move.
		 *
		 * @param position           target RA/DEC
		 * @param p1                 1st extra parameter; holds MPEC one line or TLE1 for MPEC or TLE
		 * @param p2                 2nd extra parameter; holds TLE2 for TLE
		 * @param update_position    true if new position shall be calculated
		 * @param plan_id            plan ID of the move
		 */
		virtual moveType startSlew (struct ln_equ_posn *position, std::string &p1, std::string &p2, bool update_position, int plan_id = -1);

		/**
		 * Start new observation of the target.
		 */
		int newObsSlew (struct ln_equ_posn *position, std::string &p1, std::string &p2, int plan_id);

		/**
		 * Update slew position - record in Observation class.
		 */
		int updateSlew (struct ln_equ_posn *position, std::string &p1, std::string &p2, int plan_id);

		virtual moveType afterSlewProcessed ();
		virtual int startObservation ();

		// similar to startSlew - return 0 if observation ends, 1 if
		// it doesn't ends (ussually in case when in_next_id == target_id),
		// -1 on errror
		int endObservation (int in_next_id, rts2core::Block * master);

		virtual int endObservation (int in_next_id);

		// returns 1 if target is continuus - in case next target is same | next
		// targer doesn't exists, we keep exposing and we will not move mount between
		// exposures. Good for darks observation, partial good for GRB (when we solve
		// problem with moving mount in exposures - position updates)
		// returns 2 when target need change - usefull for plan target and other targets,
		// which set obs_target_id
		virtual int isContinues ();

		int observationStarted ();

		// called when we can move to next observation - good to generate next target in mosaic observation etc..
		virtual int beforeMove ();
		int postprocess ();

		/**
		 * Returns true if the target is above horizon.
		 *
		 * @param JD Julian day with time to test.
		 *
		 * @return True if the target is above minimal observation altitude.
		 *
		 * @see getMinObsAlt
		 */
		bool isAboveHorizon (double JD);

		/**
		 * Return true if the target is above horizon.
		 *
		 * @param hrz Horizonal position of the target.
		 *
		 * @return True if the target is above minimal observation altitude.
		 *
		 * @see getMinObsAlt
		 */
		bool isAboveHorizon (struct ln_hrz_posn *hrz);

		/**
		 * Returns true if the target is above horizon and do not violate any constrains.
		 */
		bool isGood (double JD);

		/**
		 *   Return -1 if target is not suitable for observing,
		 *   otherwise return 0.
		 */
		virtual int considerForObserving (double JD);
		virtual int dropBonus ();
		float getBonus () { return getBonus (ln_get_julian_from_sys ()); }
		virtual float getBonus (double JD);
		virtual int changePriority (int pri_change, time_t * time_ch);
		int changePriority (int pri_change, double validJD);

		virtual int setNextObservable (time_t * time_ch);
		int setNextObservable (double validJD);

		int getNumObs (time_t * start_time, time_t * end_time);

		/**
		 * Return total number of observations.
		 */
		int getTotalNumberOfObservations ();

		double getLastObsTime ();// return time in seconds to last observation of same target

		/**
		 * Return total number of target images.
		 */
		int getTotalNumberOfImages (); 

		/**
		 * Returns total open shutter time in seconds.
		 */
		double getTotalOpenTime ();

		int getCalledNum () { return startCalledNum; }

		double getAirmassScale () { return airmassScale; }

		/**
		 * Returns time of first observation, or nan if no observation.
		 */
		double getFirstObs ();

		/**
		 * Returns time of last observation, or nan if no observation.
		 */
		double getLastObs ();

		/**
		 * Prints extra informations about target, which can be interested to user.
		 *
		 * @param _os stream to print that
		 */
		virtual void printExtra (Rts2InfoValStream & _os, double JD);

		/**
		 * Print target altitude in 1 hour interval.
		 *
		 * @param _os       Stream where informations will be printed.
		 * @param jd_start  Print heigh from this Julian Date.
		 * @param h_start   Print heigh from this hour.
		 * @param h_end     Print heigh to this hour.
		 * @param h_step    Step in hours. Default to 1 hour step.
		 * @param header    If this routine should print header
		 * @param format_output    If this routine should output formatted text
		 */
		void printAltTable (std::ostream & _os, double jd_start, double h_start, double h_end, double h_step = 1.0, bool header = true, bool format_output = true);

		/**
		 * Prints target altitude informations.
		 *
		 * @param _os   Stream which will receive target informations.
		 * @param JD    Julian date around which the information will be based.
		 */
		void printAltTable (std::ostream & _os, double JD);

		/**
		 * Print short version of target informations.
		 *
		 * @param _os Stream which will receive target informations.
		 */
		void printShortInfo (std::ostream & _os) { printShortInfo (_os, ln_get_julian_from_sys ()); }

		/**
		 * Prints short target information to given date.
		 *
		 * @param _os  Stream which will receive target informations.
		 * @param JD   Julian date for which which informations should be calculated.
		 */
		virtual void printShortInfo (std::ostream & _os, double JD);

		/**
		 * Prints one-line info with bonus inforamtions
		 */
		void printShortBonusInfo (std::ostream & _os) { printShortBonusInfo (_os, ln_get_julian_from_sys ()); }

		virtual void printShortBonusInfo (std::ostream & _os, double JD);

		void printDS9Reg (std::ostream & _os) { printDS9Reg (_os, ln_get_julian_from_sys ()); }

		virtual void printDS9Reg (std::ostream & _os, double JD);

		/**
		 * Prints position info for given JD.
		 *
		 * @param _os stream to print that
		 * @param JD date for which to print info
		 */
		virtual void sendPositionInfo (Rts2InfoValStream & _os, double JD, int extended);
		void sendInfo (Rts2InfoValStream & _os) { sendInfo (_os, ln_get_julian_from_sys (), 0); }
		void sendInfo (Rts2InfoValStream & _os, double JD, int extended);

		/**
		 * Print constraints.
		 *
		 * @param _os  stream where constrainst will be printed
		 * @param JD   date for constrints check
		 */
		virtual void sendConstraints (Rts2InfoValStream & _os, double JD);

		/**
		 * Return list and number of violated constraints.
		 */
		size_t getViolatedConstraints (double JD, ConstraintsList &violated);

		ConstraintsList getViolatedConstraints (double JD);

		/**
		 * Test if given constraint is violated by the target.
		 *
		 * @param cons     Constraint pointer
		 * @param JD       Julian Day for which constraint will be tested
		 * @return  true if constraint is violated for given target at given time
		 */
		bool isViolated (Constraint *cons, double JD);

		/**
		 * Return list and number of satisfied constraints.
		 */
		size_t getSatisfiedConstraints (double JD, ConstraintsList &satisfied);

		ConstraintsList getSatisfiedConstraints (double JD);

		/**
		 * Fill target list of altitude constraints.
		 */
		size_t getAltitudeConstraints (std::map <std::string, std::vector <ConstraintDoubleInterval> > &ac);

		size_t getAltitudeViolatedConstraints (std::map <std::string, std::vector <ConstraintDoubleInterval> > &ac);

		size_t getTimeConstraints (std::map <std::string, ConstraintPtr> &cons);

		/**
		 * Calculate constraint satifaction.
		 */
		void getSatisfiedIntervals (time_t from, time_t to, int length, int step, interval_arr_t &satisfiedIntervals);

		/**
		 * Return length (in seconds) of intervale when constraints will be satisfied. Check start
		 * at time from + length.
		 *
		 * @param from    ctime (seconds from 1/1/1970) of the interval to check from
		 * @param to      ctime (seconds from 1/1/1970) of check interval ends
		 * @param length  length of observing script in seconds
		 * @param step    step (in seconds) for steppign through check
		 */
		double getSatisfiedDuration (double from, double to, double length, double step);

		void getViolatedIntervals (time_t from, time_t to, int length, int step, interval_arr_t &satisfiedIntervals);

		/**
		 * Print observations at current target postion around given position.
		 */
		int printObservations (double radius, std::ostream & _os) { return printObservations (radius, ln_get_julian_from_sys (), _os); }

		int printObservations (double radius, double JD, std::ostream & _os);

		TargetSet getTargets (double radius);
		TargetSet getTargets (double radius, double JD);

		int printTargets (double radius, std::ostream & _os) { return printTargets (radius, ln_get_julian_from_sys (), _os); }

		int printTargets (double radius, double JD, std::ostream & _os);

		/**
		 * Print images from the observation.
		 *
		 * @param _os               output stream to print the images
		 * @param flags             output flags
		 * @param imageFormat       expression for printing extra image header data
		 */
		int printImages (std::ostream & _os, int flags = DISPLAY_ALL, const char *imageFormat = NULL) { return printImages (ln_get_julian_from_sys (), _os, flags, imageFormat); }

		int printImages (double JD, std::ostream & _os, int flags = DISPLAY_ALL, const char *imageFormat = NULL);

		/**
		 * Return calibration targets for given target
		 */
		TargetSet *getCalTargets () { return getCalTargets (ln_get_julian_from_sys ()); }

		virtual TargetSet *getCalTargets (double JD, double minaird = NAN);

		/**
		 * Write target metadata to image.
		 *
		 * @param image Image which will receive target metadata.
		 * @param JD    Date for which metadata will be written.
		 */
		virtual void writeToImage (rts2image::Image * image, double JD);

		/**
		 * Return target error box (in degrees).
		 */
		virtual double getErrorBox () { return NAN; }


		/**
		 * Return group constraint file.
		 */
		const char *getGroupConstraintFile ();

		/**
		 * Return location of constraint file.
		 */
		const char *getConstraintFile ();

		
		/**
		 * Load constraints and returns reference to their list.
		 */
		Constraints * getConstraints ();

		/**
		 * Check constraints for given date.
		 */
		virtual bool checkConstraints (double JD);

		/**
		 * Check if given ID is among watches for constraints
		 * associated to the target. Delete constrainst if it is
		 * associated. getConstraints method then load again
		 * constraints, if they are needed.
		 *
		 * @param watchID   ID of watch which was changed
		 */
		void revalidateConstraints (int watchID);

		/**
		 * Retrieve list of target labels.
		 */
		LabelsVector getLabels () { return labels.getTargetLabels (getTargetID ()); }

		void deleteLabels (int ltype) { labels.deleteTargetLabels (getTargetID (), ltype); }

		/**
		 * Add label to target. Might create new label if create parameter is set to true.
		 *
		 * @param label  character string with label identification
		 * @param ltype  label type
		 * @param create if true, new label will be created
		 */
		void addLabel (const char *label, int ltype, bool create) { labels.addLabel (getTargetID (), label, ltype, create); }

		/**
		 * Add label identified by label ID to the target.
		 *
		 * @param label_id   label ID.
		 */
		void addLabel (int label_id) { labels.addLabel (getTargetID (), label_id); }

		/**
		 * Test if a target is associated with a label.
		 *
		 * @param label_id   lable ID.
		 */
		bool hasLabel (int label_id) { LabelsVector tlabels = getLabels (); return tlabels.findLabelId (label_id) != tlabels.end (); }

	protected:
		char *target_comment;
		struct ln_lnlat_posn *observer;

		/**
		 * Observer altitude (in KM).
		 */
		double obs_altitude;

		/**
		 * Return script for camera exposure.
		 *
		 * @param camera_name   name of the camera
		 * @param script        script
		 *
		 * @throw rts2core::Error on an error
		 */
		virtual void getDBScript (const char *camera_name, std::string & script);

		// get called when target was selected to update bonuses etc..
		virtual int selectedAsGood ();

	private:
		// holds current target observation
		Observation * observation;

		std::vector <int> watchIDs;		// 

		void addWatch (const char *filename);

		int type;				// light, dark, flat, flat_dark
		std::string tar_info;

		int tar_telescope_mode;			// telescope mode - from modefiles, target will be set to it before movement start

		int startCalledNum;			// how many times startObservation was called - good to know for targets
		double airmassScale;

		time_t observationStart;

		// which changes behaviour based on how many times we called them before

		double minObsAlt;

		float tar_priority;
		float tar_bonus;
		time_t tar_bonus_time;
		time_t tar_next_observable;

		void writeAirmass (std::ostream & _os, double jd);

		Labels labels;

		// which constraints were sucessfully loaded
		int constraintsLoaded;

		char *constraintFile;
		char *groupConstraintFile;

		double satisfiedFrom;
		double satisfiedTo;
		double satisfiedProbedUntil;
};

/**
 * Class representing target with constant coordinates.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConstTarget:public Target
{
	public:
		ConstTarget ();
		ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		ConstTarget (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude, struct ln_equ_posn *pos);
		virtual void load ();
		virtual int saveWithID (bool overwrite, int tar_id);
		virtual void getPosition (struct ln_equ_posn *pos, double JD);

		/**
		 * Returns true if the target has nan position - this is usually case with scripted targets.
		 */
		bool hasNaNPosition () { return isnan (position.ra) || isnan (position.dec); }
		
		/**
		 * Retrieve target proper motion.
		 *
		 * @param pm   Target proper motion (in arcdeg/year)
		 */
		virtual void getProperMotion (struct ln_equ_posn *pm);

		virtual int getRST (struct ln_rst_time *rst, double jd, double horizon);
		virtual int compareWithTarget (Target * in_target, double grb_sep_limit);
		virtual void printExtra (Rts2InfoValStream & _os, double JD);

		void setPosition (double ra, double dec) { position.ra = ra; position.dec = dec; }
		void setProperMotion (double pm_ra, double pm_dec) { proper_motion.ra = pm_ra; proper_motion.dec = pm_dec; }
		void setProperMotion (struct ln_equ_posn *pm) { proper_motion.ra = pm->ra; proper_motion.dec = pm->dec; }

	protected:
		// get called when target was selected to update bonuses, target position etc..
		virtual int selectedAsGood ();
	private:
		struct ln_equ_posn position;
		struct ln_equ_posn proper_motion;
};

/**
 * Class representing dark callibration targets.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DarkTarget:public Target
{
	private:
		struct ln_equ_posn currPos;
	public:
		DarkTarget (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		virtual ~ DarkTarget (void);
		virtual bool getScript (const char *deviceName, std::string & buf);
		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int getRST (struct ln_rst_time *rst, double JD, double horizon) { return 1; }
		virtual moveType startSlew (struct ln_equ_posn * position, std::string &p1, std::string &p2, bool update_position, int plan_id = -1);
		virtual int isContinues () { return 1; }
};

class FlatTarget:public ConstTarget
{
	public:
		FlatTarget (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		virtual bool getScript (const char *deviceName, std::string & buf);
		virtual void load ();
		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int considerForObserving (double JD);
		virtual int isContinues () { return 1; }
		virtual void printExtra (Rts2InfoValStream & _os, double JD);
	private:
		void getAntiSolarPos (struct ln_equ_posn *pos, double JD);
};

/**
 * Possible calibration targets.
 */
class PosCalibration:public Target
{
	public:
		PosCalibration (int in_tar_id, double ra, double dec, char in_type_id, char *in_tar_name, struct ln_lnlat_posn *in_observer, double in_altitude, double JD):Target (in_tar_id, in_observer, in_altitude)
		{
			struct ln_hrz_posn hrz;

			object.ra = ra;
			object.dec = dec;
			ln_get_hrz_from_equ (&object, observer, JD, &hrz);
			currAirmass = ln_get_airmass (hrz.alt, getAirmassScale ());

			setTargetType (in_type_id);
			setTargetName (in_tar_name);
		}
		double getCurrAirmass () { return currAirmass; }

		virtual void getPosition (struct ln_equ_posn *in_pos, double JD)
		{
			in_pos->ra = object.ra;
			in_pos->dec = object.dec;
		}

		virtual int getRST (struct ln_rst_time *rst, double JD, double horizon)
		{
			struct ln_equ_posn pos;
			getPosition (&pos, JD);
			ln_get_object_next_rst_horizon (JD, observer, &pos, horizon, rst);
			return 0;
		}
	private:
		double currAirmass;
		char type_id;
		struct ln_equ_posn object;
};

class CalibrationTarget:public ConstTarget
{
	public:
		CalibrationTarget (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		virtual void load ();
		virtual int beforeMove ();
		virtual int endObservation (int in_next_id);
		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int considerForObserving (double JD);
		virtual int changePriority (int pri_change, time_t * time_ch) { return 0; }
		virtual float getBonus (double JD);
		virtual int isContinues () { return 2; }
	private:
		struct ln_equ_posn airmassPosition;
		time_t lastImage;
		int needUpdate;
};

/**
 * Modelling target. Points to random places on the sky to build pointing
 * model.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ModelTarget:public ConstTarget
{
	private:
		struct ln_hrz_posn hrz_poz;
		struct ln_equ_posn equ_poz;
		double ra_noise;
		double dec_noise;
		float alt_start;
		float alt_stop;
		float alt_step;
		float az_start;
		float az_stop;
		float az_step;
		float noise;
		int step;

		int alt_size;

		int modelStepType;

		int writeStep ();
		int getNextPosition ();
		int calPosition ();
	public:
		ModelTarget (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		virtual ~ ModelTarget (void);
		virtual void load ();
		virtual int beforeMove ();
		virtual moveType afterSlewProcessed ();
		virtual int endObservation (int in_next_id);
		virtual void getPosition (struct ln_equ_posn *pos, double JD);

		/**
	         * Returns minimal target altitude.
		 */
		virtual double getMinObsAlt ()
		{
			return -1;
		}
};

class OportunityTarget:public ConstTarget
{
	public:
		OportunityTarget (int in_tar_id, struct ln_lnlat_posn * in_obs, double in_altitude);
		virtual float getBonus (double JD);
		virtual int isContinues () { return 1; }

		virtual bool hasConstantPosition () { return true; }
};

class LunarTarget:public Target
{
	public:
		LunarTarget (int in_tar_id, struct ln_lnlat_posn * in_obs, double in_altitude);
		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int getRST (struct ln_rst_time *rst, double jd, double horizon);
};

/**
 * Swift Field of View - generate observations on Swift Field of View, based on
 * data received from GCN
 *
 */
class TargetSwiftFOV:public Target
{
	private:
		int oldSwiftId;
		int swiftId;
		struct ln_equ_posn swiftFovCenter;
		time_t swiftTimeStart;
		time_t swiftTimeEnd;
		double swiftOnBonus;
		char *swiftName;
		double swiftRoll;

		char *swiftLastTarName;
		int swiftLastTar;
		time_t swiftLastTarTimeStart;
		time_t swiftLastTarTimeEnd;
		struct ln_equ_posn swiftLastTarPos;
	public:
		TargetSwiftFOV (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		virtual ~ TargetSwiftFOV (void);

		virtual void load ();	 // find Swift pointing for observation
		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int getRST (struct ln_rst_time *rst, double JD, double horizon);
		virtual moveType afterSlewProcessed ();
								 // return 0, when target can be observed, otherwise modify tar_bonus..
		virtual int considerForObserving (double JD);
		virtual int beforeMove ();
		virtual float getBonus (double JD);
		virtual int isContinues ();

		virtual void printExtra (Rts2InfoValStream & _os, double JD);
};

class TargetIntegralFOV:public Target
{
	private:
		int oldIntegralId;
		int integralId;
		struct ln_equ_posn integralFovCenter;
		time_t integralTimeStart;
		double integralOnBonus;
	public:
		TargetIntegralFOV (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		virtual ~ TargetIntegralFOV (void);

		virtual void load ();	 // find Swift pointing for observation
		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int getRST (struct ln_rst_time *rst, double JD, double horizon);
		virtual moveType afterSlewProcessed ();
								 // return 0, when target can be observed, otherwise modify tar_bonus..
		virtual int considerForObserving (double JD);
		virtual int beforeMove ();
		virtual float getBonus (double JD);
		virtual int isContinues ();

		virtual void printExtra (Rts2InfoValStream & _os, double JD);
};

class TargetGps:public ConstTarget
{
	public:
		TargetGps (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		virtual float getBonus (double JD);
};

class TargetSkySurvey:public ConstTarget
{
	public:
		TargetSkySurvey (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		virtual float getBonus (double JD);
};

class TargetTerestial:public ConstTarget
{
	public:
		TargetTerestial (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		virtual int considerForObserving (double JD);
		virtual float getBonus (double JD);
		virtual moveType afterSlewProcessed ();
};

class Plan;

class TargetPlan:public Target
{
	public:
		TargetPlan (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		virtual ~ TargetPlan (void);

		virtual void load ();
		virtual void load (double JD);
		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int getRST (struct ln_rst_time *rst, double JD, double horizon);
		virtual int getObsTargetID ();
		virtual int considerForObserving (double JD);
		virtual float getBonus (double JD);
		virtual int isContinues ();
		virtual int beforeMove ();
		virtual moveType startSlew (struct ln_equ_posn *position, std::string &p1, std::string &p2, bool update_position, int plan_id = -1);
	
		virtual void printExtra (Rts2InfoValStream & _os, double JD);

	protected:
		virtual bool getScript (const char *camera_name, std::string & script);

	private:
		Plan * selectedPlan;
		Plan *nextPlan;

		// how long to look back for previous plan
		float hourLastSearch;
		float hourConsiderPlans;
		time_t nextTargetRefresh;
		void refreshNext ();
};

}

// print target information to stdout..
std::ostream & operator << (std::ostream & _os, rts2db::Target & target);
Rts2InfoValStream & operator << (Rts2InfoValStream & _os, rts2db::Target & target);

/**
 * Select target from database with given target ID.
 *
 * @param tar_id      ID of target which will be loaded
 * @param obs         observer position
 * @param altitude    observator altitude
 *
 * @return New target if it can be found. Otherwise will return NULL.
 */
rts2db::Target *createTarget (int tar_id, struct ln_lnlat_posn *obs, double altitude);

/**
 * Create target by name.
 *
 * @param  tar_name  target name
 * @param  obs       observer position
 *
 * @return New target if unique target can be found. For retriving set matching given name, please see TargetSet::findByName ().
 */
rts2db::Target *createTargetByName (const char *tar_name, struct ln_lnlat_posn * obs);

#endif							 /*! __RTS_TARGETDB__ */
