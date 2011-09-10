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

#include <algorithm>

#include "rts2configraw.h"
#include "utilsfunc.h"
#include "app.h"

#ifdef DEBUG_EXTRA
#include <iostream>
#endif							 /* DEBUG_EXTRA */

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <libnova/libnova.h>

std::ostream & operator << (std::ostream & _os, Rts2ConfigValue val)
{
	_os << val.valueName;
	if (val.valueSuffix.length () > 0)
		_os << "." << val.valueSuffix;
	_os << " = " << val.value;
	return _os;
}

Rts2ConfigSection::Rts2ConfigSection (const char *name)
{
	sectName = std::string (name);
	blockedBy = NULL;
	#ifdef DEBUG_EXTRA
	std::cout << "[" << name << "]" << std::endl;
	#endif						 /* DEBUG_EXTRA */
}

Rts2ConfigSection::~Rts2ConfigSection (void)
{
	delete blockedBy;
}

Rts2ConfigValue * Rts2ConfigSection::getValue (const char *valueName, bool verbose)
{
	std::string name (valueName);
	for (Rts2ConfigSection::iterator iter = begin (); iter != end (); iter++)
	{
		Rts2ConfigValue *val = &(*iter);
		if (val->isValue (name))
			return val;
	}
	if (verbose && find (missingValues.begin (), missingValues.end (), name) == missingValues.end ())
	{
	  	// check if that wasn't reported..
		logStream (MESSAGE_WARNING) << "cannot find value '" << name <<
			"' in section '" << sectName << "'." << sendLog;
		missingValues.push_back (name);
	}
	return NULL;
}

void Rts2ConfigSection::createBlockedBy (std::string val)
{
	delete blockedBy;
	blockedBy = new std::vector < std::string >;
	std::string * deviceName = NULL;
	for (size_t i = 0; i < val.size (); i++)
	{
		if (val[i] == ' ')
		{
			if (deviceName != NULL)
			{
				blockedBy->push_back (*deviceName);
				delete deviceName;
				deviceName = NULL;
			}
		}
		else
		{
			if (deviceName == NULL)
				deviceName = new std::string ();
			deviceName->push_back (val[i]);
		}
	}
	if (deviceName)
	{
		blockedBy->push_back (*deviceName);
	}
	else
	{
		if (blockedBy->size () == 0)
		{
			delete blockedBy;
			blockedBy = NULL;
		}
	}
}

const bool Rts2ConfigSection::containedInBlockedBy (const char *querying_device)
{
	if (blockedBy == NULL)
		return true;
	return find (blockedBy->begin (), blockedBy->end (),
		std::string (querying_device)) != blockedBy->end ();
}

void Rts2ConfigRaw::clearSections ()
{
	for (Rts2ConfigRaw::iterator iter = begin (); iter != end (); iter++)
	{
		delete *iter;
	}
	missingSections.clear ();
	clear ();
}

int Rts2ConfigRaw::parseConfigFile (const char *filename, bool parseFullLine)
{
	int ln = 0;
	Rts2ConfigSection *sect = NULL;
	std::string line;
	while (!configStream->fail ())
	{
		getline (*configStream, line);
		ln++;
		if (configStream->fail ())
			break;
		std::string::iterator top = line.begin ();
		while (isspace (*top) && top != line.end ())
			top++;
		// ignore blank lines and comments
		if (top == line.end () || *top == ';' || *top == '#')
			continue;
		// start new section..
		if (*top == '[')
		{
			size_t el = line.find (']');
			if (el == std::string::npos)
			{
				logStream (MESSAGE_ERROR) <<
					"cannot find end delimiter for section '" << line <<
					"' on line " << ln << " infile " << filename << "." << sendLog;
				return -1;
			}
			if (sect)
				push_back (sect);
			sect = new Rts2ConfigSection (line.substr (top - line.begin () + 1, el-1).c_str ());
		}
		else
		{
			if (!sect)
			{
				logStream (MESSAGE_ERROR) << "value without section on line " << ln << ": " << line <<  sendLog;
				return -1;
			}
			// now create value..
			enum { NAME, SUFF, VAL, VAL_QUT, VAL_END }
			pstate = NAME;
			std::string valName;
			std::string valSuffix;
			std::string val;
			std::string::iterator es = top;
			for (; es != line.end () && pstate != VAL_END; es++)
			{
				switch (pstate)
				{
					case NAME:
						if (*es == '.')
						{
							valName = line.substr (top - line.begin (), es - top);
							pstate = SUFF;
							top = es + 1;
						}
						else if (*es == '=' || isspace (*es))
						{
							pstate = VAL;
							valName = line.substr (top - line.begin (), es - top);
							while (*es != '=' && es != line.end ())
								es++;
							es++;
							while (es != line.end () && isspace (*es))
								es++;
							if (*es == '"')
							{
								pstate = VAL_QUT;
								top = es + 1;
							}
							else
							{
								top = es;
							}
						}
						break;
					case SUFF:
						if (*es == '=' || isspace (*es))
						{
							valSuffix = line.substr (top - line.begin (), es - top);
							pstate = VAL;
							while (*es != '=' && es != line.end ())
								es++;
							es++;
							while (es != line.end () && isspace (*es))
								es++;
							if (*es == '"')
							{
								pstate = VAL_QUT;
								top = es + 1;
							}
							else
							{
								top = es;
							}
						}
						break;
					case VAL:
						if (isspace (*es))
						{
							pstate = VAL_END;
							if (parseFullLine)
							{
								es  = line.end ();
								while (isspace (*es))
									es--;
							}	
							val = line.substr (top - line.begin (), es - top);
							es = line.end ();
						}
						break;
					case VAL_QUT:
						if (*es == '"')
						{
							pstate = VAL_END;
							if (top == es)
								val = std::string ("");
							else
								val = line.substr (top - line.begin (), es - top);
							es = line.end ();
						}
						break;
					case VAL_END:
						break;
				}
			}
			if (pstate == VAL_QUT)
			{
				logStream (MESSAGE_ERROR) << "missing \" on line " << ln
					<< " of file " << filename << sendLog;
				return -1;
			}
			if (pstate == VAL)
			{
				val = line.substr (top - line.begin (), es - top);
				pstate = VAL_END;
			}
			if (pstate != VAL_END)
			{
				logStream (MESSAGE_ERROR) << "invalid configuration line " << ln 
					<< " of file " << filename << sendLog;
				return -1;
			}
			#ifdef DEBUG_EXTRA
			std::cout << "Pushing " << valName << " " << valSuffix << " " << val << std::endl;
			#endif				 /* DEBUG_EXTRA */
			sect->push_back (Rts2ConfigValue (valName, valSuffix, val));
			if (valName == "blocked_by")
			{
				sect->createBlockedBy (val);
			}
		}
	}
	if (sect)
		push_back (sect);
	return 0;
}

Rts2ConfigRaw::Rts2ConfigRaw ()
{
	verboseEntry = true;
	configStream = NULL;
}

Rts2ConfigRaw::~Rts2ConfigRaw (void)
{
}

int Rts2ConfigRaw::loadFile (const char *filename, bool parseFullLine)
{
	clearSections ();

	if (!filename)
		// default
		filename = RTS2_PREFIX "/etc/rts2/rts2.ini";
	configStream = new std::ifstream ();
	configStream->open (filename);
	if (configStream->fail ())
	{
		logStream (MESSAGE_ERROR) << "cannot open configuration file '" << filename <<
			"'." << sendLog;
		delete configStream;
		return -1;
	}
	// fill sections
	if (parseConfigFile (filename, parseFullLine))
	{
		delete configStream;
		return -1;
	}

	if (getSpecialValues ())
	{
		delete configStream;
		return -1;
	}

	delete configStream;

	return 0;
}

Rts2ConfigSection * Rts2ConfigRaw::getSection (const char *section, bool verbose)
{
	std::string name (section);
	for (Rts2ConfigRaw::iterator iter = begin (); iter != end (); iter++)
	{
		Rts2ConfigSection *sect = *iter;
		if (sect->isSection (name))
			return sect;
	}
	if (verbose && find (missingSections.begin (), missingSections.end (), name) == missingSections.end ())
	{
		logStream (MESSAGE_ERROR) << "cannot find section '" << section << "'." << sendLog;
		missingSections.push_back (name);
	}
	return NULL;
}

Rts2ConfigValue * Rts2ConfigRaw::getValue (const char *section, const char *valueName)
{
	Rts2ConfigSection *sect = getSection (section, verboseEntry);
	if (!sect)
	{
		return NULL;
	}
	return sect->getValue (valueName, verboseEntry);
}

int Rts2ConfigRaw::getString (const char *section, const char *valueName, std::string & buf)
{
	Rts2ConfigValue *val = getValue (section, valueName);
	if (!val)
	{
		return -1;
	}
	buf = val->getValue ();
	return 0;
}

int Rts2ConfigRaw::getString (const char *section, const char *valueName, std::string & buf, const char *defVal)
{
	int ret;
 	clearVerboseEntry ();
	ret = getString (section, valueName, buf);
	if (ret)
 	{
		buf = std::string (defVal);
 	}
 	setVerboseEntry ();
	return ret;
}

const char * Rts2ConfigRaw::getStringDefault (const char *section, const char *valueName, const char *defVal)
{
	clearVerboseEntry ();
	Rts2ConfigValue *val = getValue (section, valueName);
	if (!val)
	{
		return defVal;
	}
	setVerboseEntry ();
	return val->getValue ().c_str ();
}

int Rts2ConfigRaw::getStringVector (const char *section, const char *valueName, std::vector<std::string> & value, bool verbose)
{
	std::string valbuf;
	int ret;
	bool oldVerbose = verboseEntry;
	verboseEntry = verbose;
	ret = getString (section, valueName, valbuf);
	verboseEntry = oldVerbose;
	if (ret)
		return ret;
	value = SplitStr (valbuf, std::string (" "));
	return 0;
}

int Rts2ConfigRaw::getInteger (const char *section, const char *valueName, int &value)
{
	std::string valbuf;
	char *retv;
	int ret;
	ret = getString (section, valueName, valbuf);
	if (ret)
		return ret;
	value = strtol (valbuf.c_str (), &retv, 0);
	if (*retv != '\0')
	{
		logStream (MESSAGE_ERROR) << "cannot convert " << valbuf
			<< " in section [" << section
			<< "] value '" << valueName 
			<< "' to float number. Please check configuration file." << sendLog;
		return -1;
	}
	return 0;
}

int Rts2ConfigRaw::getIntegerDefault (const char *section, const char *valueName, int defVal)
{
	int ret;
	int value;
	clearVerboseEntry ();
	ret = getInteger (section, valueName, value);
	setVerboseEntry ();
	if (ret)
		return defVal;
	return value;
}

int Rts2ConfigRaw::getFloat (const char *section, const char *valueName, float &value)
{
	std::string valbuf;
	char *retv;
	int ret;
	ret = getString (section, valueName, valbuf);
	if (ret)
		return ret;
#ifdef HAVE_STRTOF	
	value = strtof (valbuf.c_str (), &retv);
#else	
	value = strtod (valbuf.c_str (), &retv);
#endif	
	if (*retv != '\0')
	{
		logStream (MESSAGE_ERROR) << "cannot convert " << valbuf
			<< " in section [" << section
			<< "] value '" << valueName 
			<< "' to float number. Please check configuration file." << sendLog;
		return -1;
	}
	return 0;
}

int Rts2ConfigRaw::getFloat (const char *section, const char *valueName, float &value, float defVal)
{
	int ret;
	clearVerboseEntry ();
	ret = getFloat (section, valueName, value);
	if (ret)
	{
		value = defVal;
	}
	setVerboseEntry ();
	return ret;
}

double Rts2ConfigRaw::getDoubleDefault (const char *section, const char *valueName, double val)
{
	int ret;
	double value;
	clearVerboseEntry ();
	ret = getDouble (section, valueName, value);
	setVerboseEntry ();
	if (ret)
		return val;
	return value;
}

int Rts2ConfigRaw::getDouble (const char *section, const char *valueName, double &value)
{
	std::string valbuf;
	char *retv;
	int ret;
	ret = getString (section, valueName, valbuf);
	if (ret)
		return ret;
	value = strtod (valbuf.c_str (), &retv);
	if (*retv != '\0')
	{
		logStream (MESSAGE_ERROR) << "cannot convert " << valbuf
			<< " in section [" << section
			<< "] value '" << valueName 
			<< "' to float number. Please check configuration file." << sendLog;
		return -1;
	}
	return 0;
}

int Rts2ConfigRaw::getDouble (const char *section, const char *valueName, double &value, double defVal)
{
  	int ret;
	clearVerboseEntry ();
	ret = getDouble (section, valueName, value);
	if (ret)
	{
		value = defVal;
	}
	setVerboseEntry ();
	return ret;
}

bool Rts2ConfigRaw::getBoolean (const char *section, const char *valueName, bool def)
{
	std::string valbuf;
	int ret;
	clearVerboseEntry ();
	ret = getString (section, valueName, valbuf);
	setVerboseEntry ();
	if (ret)
	{
		return def;
	}
	if (valbuf == "y" || valbuf == "yes" || valbuf == "true")
		return true;
	if (valbuf == "n" || valbuf == "no" || valbuf == "false")
		return false;
	return def;
}

const bool Rts2ConfigRaw::blockDevice (const char *device_name, const char *querying_device)
{
	Rts2ConfigSection *sect = getSection (device_name, false);
	if (!sect)
		return true;
	return sect->containedInBlockedBy (querying_device);
}
