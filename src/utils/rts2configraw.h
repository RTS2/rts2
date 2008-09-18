/* 
 * Configuration file read routines.
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

#ifndef __RTS2_CONFIG_RAW__
#define __RTS2_CONFIG_RAW__
/**
 * @file
 * Classes for access to configuration file.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */

#include <fstream>
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#define SEP " "

/**
 * This class represent section value. Section value have name, possible sufix
 * which is separated from name by ".", and value, which is after value name
 * and suffix followed by "=" sign.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ConfigValue
{
	private:
		std::string valueName;
		std::string valueSuffix;
		std::string value;
	public:
		Rts2ConfigValue (std::string in_valueName, std::string in_valueSuffix,
			std::string in_value)
		{
			valueName = in_valueName;
			valueSuffix = in_valueSuffix;
			value = in_value;
		}
		
		/**
		 * Check for value name.
		 *
		 * @param name Name which will be checked.
		 *
		 * @return True if value name is equal to name.
		 */
		bool isValue (std::string name)
		{
			return (valueName == name && valueSuffix.length () == 0);
		}

		/**
		 * Return value name.
		 *
		 * @return Value name.
		 */
		std::string getValueName ()
		{
			return valueName;
		}

		/**
		 * Return value suffix. Value suffix is used for .min and .max
		 * entris in mode files.
		 *
		 * @return Value suffix.
		 */
 		std::string getSuffix ()
 		{
 			return valueSuffix;
 		}

		/**
		 * Return value as string.
		 *
		 * @return Value as string.
		 */
		std::string getValue ()
		{
			return value;
		}

		/**
		 * Return value as double number.
		 *
		 * @return Value as double number.
		 */
		double getValueDouble ()
		{
			return atof (value.c_str ());
		}



		friend std::ostream & operator << (std::ostream & _os, Rts2ConfigValue val);
};

std::ostream & operator << (std::ostream & _os, Rts2ConfigValue val);

/**
 * This class represent section of configuration file.
 * Section start with [section_name] and ends with next section.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ConfigSection:public std::list <Rts2ConfigValue >
{
	private:
		std::string sectName;
		std::vector <std::string> *blockedBy;
	public:
		Rts2ConfigSection (const char *name);
		~Rts2ConfigSection (void);

		/**
		 * Return true if section have given name.
		 *
		 * @param name Name which we are looking for.
		 *
		 * @return True if section have name name.
		 */
		bool isSection (std::string name)
		{
			return sectName == name;
		}

		std::string getName ()
		{
			return sectName;
		}
		/**
		 * Look for value with given name.
		 *
		 * @param valueName Name of value.
		 * @param verbose   When true (default), warning message will be printed for missing value.
		 *
		 * @return NULL if value cannot be found, otherwise return Rts2ConfigValue valid pointer.
		 */
		Rts2ConfigValue *getValue (const char *valueName, bool verbose = true);

		/**
		 * Create blockedBy vector from string.
		 *
		 * @param val Value which containst name of blocked devices.
		 */
		void createBlockedBy (std::string val);

		/**
		 * Query if querying_device is among "blocked_by" devices listed for this section.
		 *
		 * @param querying_device Name of device which initiates query.
		 *
		 * @return True if device is listed or blocked_by entry does not exists, false if not.
		 *
		 * @callergraph
		 */
		const bool containedInBlockedBy (const char *querying_device);
};

/**
 * This class represent whole config file. It have methods to reload config
 * file, get list of sections, and access sections.
 *
 * It is raw as it does not contain any call to Libnova function. For full
 * config, which depends on Libnova functions and provides ObjectChecker,
 * \see Rts2Config.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */

class Rts2ConfigRaw: public std::vector < Rts2ConfigSection * >
{
	private:
		bool verboseEntry;

		void setVerboseEntry ()
		{
			verboseEntry = true;
		}

		void clearVerboseEntry ()
		{
			verboseEntry = false;
		}

		void clearSections ();
		int parseConfigFile (const char *filename);

		// sections which are know to be missing
		std::vector <std::string> missingSections;

		Rts2ConfigSection *getSection (const char *section, bool verbose=true);
		Rts2ConfigValue *getValue (const char *section, const char *valueName);
	protected:
		std::ifstream * configStream;
		virtual int getSpecialValues ()
		{
			return 0;
		}

	public:
		Rts2ConfigRaw ();
		virtual ~ Rts2ConfigRaw (void);
		int loadFile (const char *filename = NULL);
		/**
		 * Get string from configuration file.
		 *
		 * @param section    Section name.
		 * @param valueName  Value name.
		 * @param buf        Return buffer.
		 *
		 * @return -1 on error, 0 on sucess.
		 */
		int getString (const char *section, const char *valueName, std::string & buf);

		int getString (const char *section, const char *valueName, std::string & buf, const char* defVal);

		/**
		 * Return string value as vector of strings. Vector items are separated by space.
		 * If value cannot be found, -1 is returned and vector remains empty.
		 *
		 * @param section    Section name.
		 * @param valueName  Value name.
		 * @param value      Returned string vector.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int getStringVector (const char *section, const char *valueName, std::vector<std::string> & value);

		int getInteger (const char *section, const char *valueName, int &value);

		int getInteger (const char *section, const char *valueName, int &value, int defVal);

		/**
		 * Return float value. Print warning message if value cannot be find in configuration file.
		 *
		 * @param section    Section name.
		 * @param valueName  Value name.
		 * @param value      Returned value.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int getFloat (const char *section, const char *valueName, float &value);

		/**
		 * Return float value. Use supplied default value if value cannot be find in configuration file.
		 *
		 * @param section    Section name.
		 * @param valueName  Value name.
		 * @param value      Returned value.
		 * @param defVal     Default value, used if value cannot be found.
		 *
		 * @return -1 when default value was used, 0 on success. Never prints any error messages, only
		 * 	debug log entry is created.
		 */
		int getFloat (const char *section, const char *valueName, float &value, float defVal);

		/**
		 * Return double configuration value.
		 */
		double getDouble (const char *section, const char *valueName);

		int getDouble (const char *section, const char *valueName, double &value);

		/**
		 * Return double configuration value. Use supplied default value if value cannot be find in configuration file.
		 *
		 * @param section    Section name.
		 * @param valueName  Value name.
		 * @param value      Returned value.
		 * @param defVal     Default value, used if value cannot be found.
		 *
		 * @return -1 when default value was used, 0 on success. Never prints any error messages, only
		 * 	debug log entry is created.
		 */
		int getDouble (const char *section, const char *valueName, double &value, double defVal);

		/**
		 *
		 * @param section    Section name.
		 * @param valueName  Value name.
		 * @param def        Default value.
		 *
		 * @return Configuration valeu.
		 */
		bool getBoolean (const char *section, const char *valueName, bool def);

		/**
		 * Query if device can be blocked by another device. This function return true in following two cases:
		 *
		 * <ul>
		 *   <li>there is not blocked_by entry in section with name device_name
		 *   <li>there is blocked_by entry in section with name device_name, and querying_device is among listed devices
		 * </ul>
		 *
		 * @param device_name Quering device name. This device ask, if querying_device block it.
		 * @param querying_device Queried device.
		 *
		 * @return True if querying_device can block device with device_name.
		 *
		 * @callergraph
		 */
		const bool blockDevice (const char *device_name, const char *querying_device);
};
#endif							 /*! __RTS2_CONFIG_RAW__ */
