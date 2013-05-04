/* 
 * Configuration file read routines.
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

/**
 * @file
 * Holds RTS2 extension of IniParser class. This class is used to acess
 * RTS2-specific values from the configuration file.
 */

#ifndef __RTS2_CONFIG__
#define __RTS2_CONFIG__

#include "iniparser.h"
#include "objectcheck.h"

#include <libnova/libnova.h>
#include <algorithm>

namespace rts2core
{

/**
 * Represent full Config class, which includes support for Libnova types and
 * holds some special values. The philosophy behind this class is to allow
 * quick access, without need to parse configuration file or to search for
 * configuration file string entries. Each value in configuration file should
 * be mapped to apporpriate privat member variable of this class, and public
 * access function should be provided.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2Block
 */
class Configuration:public IniParser
{
	public:
		/**
		 * Construct RTS2 class. 
		 */
		Configuration (bool defaultSection = false);

		/**
		 * Deletes object checker.
		 */
		virtual ~ Configuration (void);

		/**
		 * Returns Configuration instance loaded by default application block.
		 *
		 * @callergraph
		 *
		 * @return Configuration instance.
		 */
		static Configuration *instance ();

		/**
		 * Store RA/DEC to FITS headers as sexadecimals.
		 */
		bool getStoreSexadecimals () { return storeSexadecimals; }

		/**
		 * Returns airmass callibration distance, which is used for callibartion observations.
		 */
		double getCalibrationAirmassDistance () { return calibrationAirmassDistance; }

		/**
		 * Returns min flux of the device, usefull when taking flat frames.
		 */
		int getDeviceMinFlux (const char *device, double &minFlux);

		/**
		 * Returns timeout time for astrometry processing call.
		 *
		 * @return Number of seconds of the timeout.
		 */
		int getAstrometryTimeout () { return astrometryTimeout; }

		/**
		 * Return observing processing timeout.
		 *
		 * @see Configuration::getAstrometryTimeout()
		 *
		 * @return number of seconds for timeout.
		 */
		int getObsProcessTimeout () { return astrometryTimeout; }

		/**
		 * Returns number of seconds for dark processing timeout.
		 *
		 * @see Configuration::getAstrometryTimeout()
		 */
		int getDarkProcessTimeout () { return astrometryTimeout; }

		/**
		 * Returns number of seconds for flat processing timeout.
		 *
		 * @see Configuration::getAstrometryTimeout
		 */
		int getFlatProcessTimeout () { return astrometryTimeout; }

		/**
		 * Returns minimal heigh for flat observations.
		 *
		 * @return Minmal flat heigh.
		 */
		double getMinFlatHeigh () { return minFlatHeigh; }

		/**
		 * Returns minimal lunar distance for calibration targets. It's recorded in
		 * degrees, defaults to 20.
		 *
		 * @return Separation in degrees.
		 */
		double getCalibrationLunarDist () { return calibrationLunarDist; }

		/**
		 * Calibration valid time. If we take at given airmass range image
		 * within last valid_time seconds, observing this target is
		 * not a high priority.
		 *
		 * @return Time in seconds for which calibration observation is valid.
		 */
		int getCalibrationValidTime () { return calibrationValidTime; }

		/**
		 * Calibration maximal time in seconds. After that time, calibration
		 * observation will receive max_bonus bonus. Between
		 * valid_time and max_delay, bonus is calculated as: {min_bonus} + (({time from
		 * last image}-{valid_time})/({max_delay}-{valid_time}))*({max_bonus}-{min_bonus}).
		 * Default to 7200.
		 *
		 * @return Time in seconds.
		 */
		int getCalibrationMaxDelay () { return calibrationMaxDelay; }

		/**
		 * Calibration minimal bonus. Calibration bonus will not be lower then this value. Default to 1.
		 */
		float getCalibrationMinBonus () { return calibrationMinBonus; }

		/**
		 * Calibration maximal bonus. Calibration bonus will not be greater then this value. Default to 300.
		 */
		float getCalibrationMaxBonus () { return calibrationMaxBonus; }

		/**
		 * Returns observer coordinates. Those are recored in configuration file.
		 *
		 * @callergraph
		 *
		 * @return ln_lnlat_posn structure, which contains observer coordinates.
		 */
		struct ln_lnlat_posn *getObserver ();

		/**
		 * Returns observatory longitude.
		 *
		 * @return Observatory longitude.
		 */
		const double getObservatoryLongitude ()	{ return getObserver ()->lng; }

		/**
		 * Returns observatory UT offset. The offset is used for
		 * calculation of time dividing two consecutive nights. If
		 * current time is after 12 + UT offset, then the night is current local day. If
		 * current time is before 12 + UT offset, then the night is preceeding local day.
		 */
		const float getUTOffset () { return utOffset; }

		/**
		 * Return observatory altitude.
		 */
		const double getObservatoryAltitude () { return observatoryAltitude; }

		ObjectCheck *getObjectChecker ();

		/**
		 * Returns value bellow which SWIFT target will not be considered for observation.
		 *
		 * @return Heigh bellow which Swift FOV will not be considered (in degrees).
		 *
		 * @callergraph
		 */
		const float getSwiftMinHorizon () { return swift_min_horizon; }

		/**
		 * Returns Swift soft horizon. Swift target, which was selected (because it
		 * is above Swift min_horizon), but which have altitude bellow soft horizon,
		 * will be assigned altitide of swift horizon.
		 *
		 * @callergraph
		 */
		const float getSwiftSoftHorizon () { return swift_soft_horizon; }

		/**
		 * If burst which is shown as know transient source and invalid
		 * GRBs should be observerd. Defaults to true.
		 *
		 * @return True if know sources and invalid GRBs should be
		 * followed.
		 */
		const bool grbdFollowTransients () { return grbd_follow_transients; }

		/**
		 * Duration for which GRB observation will be followed.
		 */
		const int grbdValidity () { return grbd_validity; }

		/**
		 * Get names of devices which are necessary for system
		 * operations.  Those devices are necessary for system being
		 * able to detect weather state and so to switch automatically
		 * to on mode.
		 *
		 * @return List of devices.
		 */
		std::vector <std::string> observatoryRequiredDevices () { return obs_requiredDevices; }

		/**
		 * Get names of devices which shall be ignored for astrometry
		 * updates.
		 *
		 * @return List of device names, which shall be ignored.
		 */
		std::vector <std::string> imgprocAstrometryDevices () { return imgproc_astrometryDevices; }

		/**
		 * Returns false if astrometry from this device should be ignored
		 * for corrections.
		 *
		 * @param name Device name.
		 *
		 * @return false if device astromery should be ignored
		 */
		bool isAstrometryDevice (const char *device_name)
		{
			return (imgproc_astrometryDevices.size () == 0 || std::find (imgproc_astrometryDevices.begin (), imgproc_astrometryDevices.end (), std::string (device_name)) != imgproc_astrometryDevices.end ());
		}

		/**
		 * Return base observatory path.
		 *
		 * @return observatory base path
		 */
		std::string observatoryBasePath () { return obs_basePath; }

		/**
		 * Return extension pattern for que images.
		 *
		 * @return Extension pattern (observatory/que_path entry in config file)
		 */
		std::string observatoryQuePath () { return obs_quePath; }

		/**
		 * Return extension pattern for acqusition images.
		 *
		 * @return Extension pattern (observatory/acq_path entry in config file).
		 */
		std::string observatoryAcqPath () { return obs_acqPath; }

		/**
		 * Return extension pattern for archive images.
		 *
		 * @return Extension pattern (observatory/archive_path entry in config file).
		 */
		std::string observatoryArchivePath () { return obs_archive; }

		/**
		 * Return extension pattern for trash images.
		 *
		 * @return Extension pattern (observatory/trash_path entry in config file).
		 */
		std::string observatoryTrashPath () { return obs_trash; }

		/**
		 * Return extension pattern for flat images.
		 *
		 * @return Extension pattern (observatory/flat_path entry in config file).
		 */
		std::string observatoryFlatPath () { return obs_flats; }

		/**
		 * Return extension pattern for dark images.
		 *
		 * @return Extension pattern (observatory/dark_path entry in config file).
		 */
		std::string observatoryDarkPath () { return obs_darks; }

		/**
		 * Return replacing string for an observatory.
		 *
		 * @return Replacing string.
		 */
		std::string observatoryHeaderReplace () { return obs_header_replace; }

		/**
		 * Returns vector of environment variables, which shall be recorded in FITS header.
		 *
		 * @param deviceName   Device for which environmental list is returned.
		 * @param ret Vector of environmental variables names.
		 */
		void deviceWriteEnvVariables (const char *deviceName, std::vector <std::string> &ret)
		{
			getStringVector (deviceName, "environment", ret);
		}

		/**
		 * Return UT noon of currently running night on the observatory
		 * site. Timezone shift is estimated from longitude.
		 */
		time_t getNight ();

		/**
		 * Returns time_t with date for night which includes given time.
		 *
		 * @param _in Time_t containing date for which night should be calculated.
		 * @return time_t, which converted to year month day with gmtime will get night date.
		 */
		time_t getNight (time_t _in)
		{
			return _in + time_t (Configuration::instance ()->getUTOffset () * (15.0 * 86400.0 / 360.0) - 86400 / 2);
		}

		/**
		 * Returns begining time in "night" time. Night time starts
		 * around noon on given day and run till next day noon.
		 *
		 * @param year  Year for which night will be calculated (2000+).
		 * @param month Month for which night will be calculated (1-12).
		 * @param day   Day for which night will be calculated (1-31).
		 *
		 * @return Time of night start - UT midi of day on which night starts.
		 */
		time_t getNight (int year, int month, int day);

		const char *getTargetDir () { return targetDir.c_str (); }

		bool getTargetConstraintsWithName () { return targetConstraintsWithName; }

		const char *getNightDir () { return nightDir.c_str (); }

		const char *getMasterConstraintFile () { return masterConsFile.c_str (); }

		/**
		 * Show milliseconds in time printouts.
		 */
		bool getShowMilliseconds () { return showMilliseconds; }

		void setShowMilliseconds (bool show) { showMilliseconds = show; }

	protected:
		virtual int getSpecialValues ();

	private:
		static Configuration *pInstance;
		struct ln_lnlat_posn observer;
		float utOffset;
		double observatoryAltitude;
		bool storeSexadecimals;
		ObjectCheck *checker;
		int astrometryTimeout;
		double minFlatHeigh;
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
		std::vector <std::string> imgproc_astrometryDevices;

		std::string obs_basePath;

		std::string obs_quePath;
		std::string obs_acqPath;
		std::string obs_archive;
		std::string obs_trash;
		std::string obs_flats;
		std::string obs_darks;

		std::string obs_header_replace;

		std::string targetDir;
		bool targetConstraintsWithName;
		std::string nightDir;
		std::string masterConsFile;

		bool showMilliseconds;
};

}
#endif							 /* !__RTS2_CONFIG__ */
