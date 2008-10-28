/* 
 * Configuration file read routines.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

/**
 * @file
 * Holds RTS2 extension of Rts2ConfigRaw class. This class is used to acess
 * RTS2-specific values from the configuration file.
 */

#ifndef __RTS2_CONFIG__
#define __RTS2_CONFIG__

#include "rts2configraw.h"
#include "objectcheck.h"

#include <libnova/libnova.h>

/**
 * Represent full Config class, which includes support for Libnova types and
 * holds some special values. The phylosophy behind this class is to allow
 * quick access, without need to parse configuration file or to search for
 * configuration file string entries. Each value in configuration file should
 * be mapped to apporpriate privat member variable of this class, and public
 * access function should be provided.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2Block
 */
class Rts2Config:public Rts2ConfigRaw
{
	private:
		static Rts2Config *pInstance;
		struct ln_lnlat_posn observer;
		ObjectCheck *checker;
		int astrometryTimeout;
		double calibrationAirmassDistance;
		double calibrationLunarDist;
		int calibrationValidTime;
		int calibrationMaxDelay;
		float calibrationMinBonus;
		float calibrationMaxBonus;

		float swift_min_horizon;
		float swift_soft_horizon;

		bool grbd_follow_transients;
		int grbd_validity;

		std::vector <std::string> obs_requiredDevices;

		std::string obs_quePath;
		std::string obs_acqPath;
		std::string obs_archive;
		std::string obs_trash;
		std::string obs_flats;
		std::string obs_darks;
	protected:
		virtual int getSpecialValues ();

	public:
		/**
		 * Construct RTS2 class
		 */
		Rts2Config ();
		/**
		 * Deletes object checker.
		 */
		virtual ~ Rts2Config (void);

		/**
		 * Returns Rts2Config instance loaded by default application block.
		 *
		 * @callergraph
		 *
		 * @return Rts2Config instance.
		 */
		static Rts2Config *instance ();

		/**
		 * Returns airmass callibration distance, which is used for callibartion observations.
		 */
		double getCalibrationAirmassDistance ()
		{
			return calibrationAirmassDistance;
		}

		/**
		 * Returns min flux of the device, usefull when taking flat frames.
		 */
		int getDeviceMinFlux (const char *device, double &minFlux);

		/**
		 * Returns timeout time for astrometry processing call.
		 *
		 * @return Number of seconds of the timeout.
		 */
		int getAstrometryTimeout ()
		{
			return astrometryTimeout;
		}

		/**
		 * Return observing processing timeout.
		 *
		 * @see Rts2Config::getAstrometryTimeout()
		 *
		 * @return number of seconds for timeout.
		 */
		int getObsProcessTimeout ()
		{
			return astrometryTimeout;
		}

		/**
		 * Returns number of seconds for dark processing timeout.
		 *
		 * @see Rts2Config::getAstrometryTimeout()
		 */
		int getDarkProcessTimeout ()
		{
			return astrometryTimeout;
		}

		/**
		 * Returns number of seconds for flat processing timeout.
		 *
		 * @see Rts2Config::getAstrometryTimeout
		 */
		int getFlatProcessTimeout ()
		{
			return astrometryTimeout;
		}

		/**
		 * Returns minimal lunar distance for calibration targets. It's recorded in
		 * degrees, defaults to 20.
		 *
		 * @return Separation in degrees.
		 */
		double getCalibrationLunarDist ()
		{
			return calibrationLunarDist;
		}

		/**
		 * Calibration valid time. If we take at given airmass range image
		 * within last valid_time seconds, observing this target is
		 * not a high priority.
		 *
		 * @return Time in seconds for which calibration observation is valid.
		 */
		int getCalibrationValidTime ()
		{
			return calibrationValidTime;
		}

		/**
		 * Calibration maximal time in seconds. After that time, calibration
		 * observation will receive max_bonus bonus. Between
		 * valid_time and max_delay, bonus is calculated as: {min_bonus} + (({time from
		 * last image}-{valid_time})/({max_delay}-{valid_time}))*({max_bonus}-{min_bonus}).
		 * Default to 7200.
		 *
		 * @return Time in seconds.
		 */
		int getCalibrationMaxDelay ()
		{
			return calibrationMaxDelay;
		}

		/**
		 * Calibration minimal bonus. Calibration bonus will not be lower then this value. Default to 1.
		 */
		float getCalibrationMinBonus ()
		{
			return calibrationMinBonus;
		}

		/**
		 * Calibration maximal bonus. Calibration bonus will not be greater then this value. Default to 300.
		 */
		float getCalibrationMaxBonus ()
		{
			return calibrationMaxBonus;
		}

		/**
		 * Returns observer coordinates. Those are recored in configuration file.
		 *
		 * @callergraph
		 *
		 * @return ln_lnlat_posn structure, which contains observer coordinates.
		 */
		struct ln_lnlat_posn *getObserver ();
		ObjectCheck *getObjectChecker ();

		/**
		 * Returns value bellow which SWIFT target will not be considered for observation.
		 *
		 * @return Heigh bellow which Swift FOV will not be considered (in degrees).
		 *
		 * @callergraph
		 */
		float getSwiftMinHorizon ()
		{
			return swift_min_horizon;
		}

		/**
		 * Returns Swift soft horizon. Swift target, which was selected (because it
		 * is above Swift min_horizon), but which have altitude bellow soft horizon,
		 * will be assigned altitide of swift horizon.
		 *
		 * @callergraph
		 */
		float getSwiftSoftHorizon ()
		{
			return swift_soft_horizon;
		}

		/**
		 * If burst which is shown as know transient source and invalid
		 * GRBs should be observerd. Defaults to true.
		 *
		 * @return True if know sources and invalid GRBs should be
		 * followed.
		 */
		bool grbdFollowTransients ()
		{
			return grbd_follow_transients;
		}

		/**
		 * Duration for which GRB observation will be followed.
		 */
		int grbdValidity ()
		{
			return grbd_validity;
		}

		/**
		 * Get names of devices which are necessary for system
		 * operations.  Those devices are necessary for system being
		 * able to detect weather state and so to switch automatically
		 * to on mode.
		 *
		 * @return List of devices.
		 */
		std::vector <std::string> observatoryRequiredDevices ()
		{
			return obs_requiredDevices;
		}

		/**
		 * Return extension pattern for que images.
		 *
		 * @return Extension pattern (observatory/que_path entry in config file).
		 */
		std::string observatoryQuePath ()
		{
			return obs_quePath;
		}


		/**
		 * Return extension pattern for acqusition images.
		 *
		 * @return Extension pattern (observatory/acq_path entry in config file).
		 */
		std::string observatoryAcqPath ()
		{
			return obs_acqPath;
		}


		/**
		 * Return extension pattern for archive images.
		 *
		 * @return Extension pattern (observatory/archive_path entry in config file).
		 */
		std::string observatoryArchivePath ()
		{
			return obs_archive;
		}


		/**
		 * Return extension pattern for trash images.
		 *
		 * @return Extension pattern (observatory/trash_path entry in config file).
		 */
		std::string observatoryTrashPath ()
		{
			return obs_trash;
		}


		/**
		 * Return extension pattern for flat images.
		 *
		 * @return Extension pattern (observatory/flat_path entry in config file).
		 */
		std::string observatoryFlatPath ()
		{
			return obs_flats;
		}


		/**
		 * Return extension pattern for dark images.
		 *
		 * @return Extension pattern (observatory/dark_path entry in config file).
		 */
		std::string observatoryDarkPath ()
		{
			return obs_darks;
		}
};
#endif							 /* !__RTS2_CONFIG__ */
