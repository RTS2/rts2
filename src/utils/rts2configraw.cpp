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

#include "rts2configraw.h"
#include "rts2app.h"
#include <strtok.h>

#ifdef DEBUG_EXTRA
#include <iostream>
#endif							 /* DEBUG_EXTRA */

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

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


Rts2ConfigValue *
Rts2ConfigSection::getValue (const char *valueName)
{
	std::string name (valueName);
	for (Rts2ConfigSection::iterator iter = begin (); iter != end (); iter++)
	{
		Rts2ConfigValue *val = &(*iter);
		if (val->isValue (name))
			return val;
	}
	logStream (MESSAGE_WARNING) << "Cannot find value '" << name <<
		"' in section '" << sectName << "'." << sendLog;
	return NULL;
}


void
Rts2ConfigSection::createBlockedBy (std::string val)
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
		blockedBy->push_back (*deviceName);
}


const bool
Rts2ConfigSection::containedInBlockedBy (const char *querying_device)
{
	if (blockedBy == NULL)
		return true;
	return find (blockedBy->begin (), blockedBy->end (),
		std::string (querying_device)) != blockedBy->end ();
}


void
Rts2ConfigRaw::clearSections ()
{
	for (Rts2ConfigRaw::iterator iter = begin (); iter != end (); iter++)
	{
		delete *iter;
	}
	clear ();
}


int
Rts2ConfigRaw::parseConfigFile ()
{
	int ln = 1;
	Rts2ConfigSection *sect = NULL;
	std::string line;
	while (!configStream->fail ())
	{
		char top;
		*configStream >> top;
		while (isspace (top))
			*configStream >> top;
		if (configStream->fail ())
			break;
		// ignore comments..
		if (top == ';' || top == '#')
		{
			configStream->ignore (2000, '\n');
			ln++;
			continue;
		}
		// start new section..
		if (top == '[')
		{
			getline (*configStream, line);
			size_t el = line.find (']');
			if (el == std::string::npos)
			{
				logStream (MESSAGE_ERROR) <<
					"Cannot find end delimiter for section '" << line <<
					"' on line " << ln << "." << sendLog;
				return -1;
			}
			if (sect)
				push_back (sect);
			sect = new Rts2ConfigSection (line.substr (0, el).c_str ());
		}
		else
		{
			if (!sect)
			{
				logStream (MESSAGE_ERROR) << "Value without section on line " <<
					ln << " " << top << "." << sendLog;
				return -1;
			}
			configStream->unget ();
			getline (*configStream, line);
			// now create value..
			enum
			{ NAME, SUFF, VAL, VAL_QUT, VAL_END }
			pstate = NAME;
			std::string valName;
			std::string valSuffix;
			std::string val;
			size_t beg, es;
			for (es = 0; es < line.length () && pstate != VAL_END; es++)
			{
				switch (pstate)
				{
					case NAME:
						if (line[es] == '.')
						{
							valName = line.substr (0, es);
							pstate = SUFF;
							beg = es + 1;
						}
						else if (line[es] == '=' || isspace (line[es]))
						{
							pstate = VAL;
							valName = line.substr (0, es);
							while (line[es] != '=' && es < line.length ())
								es++;
							es++;
							while (es < line.length () && isspace (line[es]))
								es++;
							beg = es;
							if (line[es] == '"')
							{
								es++;
								pstate = VAL_QUT;
								beg = es;
							}
						}
						break;
					case SUFF:
						if (line[es] == '=' || isspace (line[es]))
						{
							valSuffix = line.substr (beg, es - beg);
							pstate = VAL;
							while (line[es] != '=' && es < line.length ())
								es++;
							es++;
							while (es < line.length () && isspace (line[es]))
								es++;
							beg = es;
							if (line[es] == '"')
							{
								es++;
								pstate = VAL_QUT;
								beg = es;
							}
						}
						break;
					case VAL:
						if (isspace (line[es]))
						{
							pstate = VAL_END;
							val = line.substr (beg, es - beg);
							es = line.length ();
						}
						break;
					case VAL_QUT:
						if (line[es] == '"')
						{
							pstate = VAL_END;
							val = line.substr (beg, es - beg);
							es = line.length ();
						}
						break;
					case VAL_END:
						es++;
						break;
				}
			}
			if (pstate == VAL_QUT)
			{
				logStream (MESSAGE_ERROR) << "Missing \" on line " << ln << "."
					<< sendLog;
				return -1;
			}
			if (pstate == VAL)
			{
				val = line.substr (beg, es - beg);
				pstate = VAL_END;
			}
			if (pstate != VAL_END)
			{
				logStream (MESSAGE_ERROR) << "Invalid configuration line " << ln
					<< "." << sendLog;
				return -1;
			}
			#ifdef DEBUG_EXTRA
			std::
				cout << "Pushing " << valName << " " << valSuffix << " " << val <<
				std::endl;
			#endif				 /* DEBUG_EXTRA */
			sect->push_back (Rts2ConfigValue (valName, valSuffix, val));
			if (valName == "blocked_by")
			{
				sect->createBlockedBy (val);
			}
		}
		ln++;
	}
	if (sect)
		push_back (sect);
	return 0;
}


Rts2ConfigRaw::Rts2ConfigRaw ()
{
	configStream = NULL;
}


Rts2ConfigRaw::~Rts2ConfigRaw (void)
{
}


int
Rts2ConfigRaw::loadFile (char *filename)
{
	clearSections ();

	if (!filename)
		// default
		filename = "/etc/rts2/rts2.ini";
	configStream = new std::ifstream ();
	configStream->open (filename);
	if (configStream->fail ())
	{
		logStream (MESSAGE_ERROR) << "Cannot open config file '" << filename <<
			"'." << sendLog;
		delete configStream;
		return -1;
	}
	// fill sections
	if (parseConfigFile ())
	{
		delete configStream;
		return -1;
	}

	getSpecialValues ();

	delete configStream;

	return 0;
}


Rts2ConfigSection *
Rts2ConfigRaw::getSection (const char *section)
{
	std::string name (section);
	for (Rts2ConfigRaw::iterator iter = begin (); iter != end (); iter++)
	{
		Rts2ConfigSection *sect = *iter;
		if (sect->isSection (name))
			return sect;
	}
	logStream (MESSAGE_ERROR) << "Cannot find section '" << section << "'." <<
		sendLog;
	return NULL;
}


Rts2ConfigValue *
Rts2ConfigRaw::getValue (const char *section, const char *valueName)
{
	Rts2ConfigSection *sect = getSection (section);
	if (!sect)
	{
		return NULL;
	}
	return sect->getValue (valueName);
}


int
Rts2ConfigRaw::getString (const char *section, const char *valueName,
std::string & buf)
{
	Rts2ConfigValue *val = getValue (section, valueName);
	if (!val)
	{
		return -1;
	}
	buf = val->getValue ();
	return 0;
}


int
Rts2ConfigRaw::getInteger (const char *section, const char *valueName,
int &value)
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
		value = 0;
		return -1;
	}
	return 0;
}


int
Rts2ConfigRaw::getFloat (const char *section, const char *valueName,
float &value)
{
	std::string valbuf;
	char *retv;
	int ret;
	ret = getString (section, valueName, valbuf);
	if (ret)
		return ret;
	value = strtof (valbuf.c_str (), &retv);
	if (*retv != '\0')
	{
		value = 0;
		return -1;
	}
	return 0;
}


double
Rts2ConfigRaw::getDouble (const char *section, const char *valueName)
{
	std::string valbuf;
	char *retv;
	int ret;
	ret = getString (section, valueName, valbuf);
	if (ret)
		return nan ("f");
	return strtod (valbuf.c_str (), &retv);
}


int
Rts2ConfigRaw::getDouble (const char *section, const char *valueName,
double &value)
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
		value = 0;
		return -1;
	}
	return 0;
}


bool
Rts2ConfigRaw::getBoolean (const char *section, const char *valueName,
bool def)
{
	std::string valbuf;
	int ret;
	ret = getString (section, valueName, valbuf);
	if (ret)
		return def;
	if (valbuf == "y" || valbuf == "yes" || valbuf == "true")
		return true;
	if (valbuf == "n" || valbuf == "no" || valbuf == "false")
		return false;
	return def;
}


const bool
Rts2ConfigRaw::blockDevice (const char *device_name,
const char *querying_device)
{
	Rts2ConfigSection *sect = getSection (device_name);
	if (!sect)
		return true;
	return sect->containedInBlockedBy (querying_device);
}
