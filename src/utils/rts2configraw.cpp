#include "rts2configraw.h"
#include "rts2app.h"
#include <strtok.h>

#ifdef DEBUG_EXTRA
#include <iostream>
#endif /* DEBUG_EXTRA */

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
#ifdef DEBUG_EXTRA
  std::cout << "[" << name << "]" << std::endl;
#endif /* DEBUG_EXTRA */
}

Rts2ConfigValue *
Rts2ConfigSection::getValue (const char *param)
{
  std::string name (param);
  for (Rts2ConfigSection::iterator iter = begin (); iter != end (); iter++)
    {
      Rts2ConfigValue *val = &(*iter);
      if (val->isValue (name))
	return val;
    }
  logStream (MESSAGE_WARNING) << "Cannot find value '" << param <<
    "' in section '" << sectName << "'." << sendLog;
  return NULL;
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
	  { NAME, SUFF, VAL, VAL_QUT, VAL_END } pstate = NAME;
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
#endif /* DEBUG_EXTRA */
	  sect->push_back (Rts2ConfigValue (valName, valSuffix, val));
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
Rts2ConfigRaw::getValue (const char *section, const char *param)
{
  Rts2ConfigSection *sect = getSection (section);
  if (!sect)
    {
      return NULL;
    }
  return sect->getValue (param);
}

int
Rts2ConfigRaw::getString (const char *section, const char *param,
			  std::string & buf)
{
  Rts2ConfigValue *val = getValue (section, param);
  if (!val)
    {
      return -1;
    }
  buf = val->getValue ();
  return 0;
}

int
Rts2ConfigRaw::getInteger (const char *section, const char *param, int &value)
{
  std::string valbuf;
  char *retv;
  int ret;
  ret = getString (section, param, valbuf);
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
Rts2ConfigRaw::getFloat (const char *section, const char *param, float &value)
{
  std::string valbuf;
  char *retv;
  int ret;
  ret = getString (section, param, valbuf);
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
Rts2ConfigRaw::getDouble (const char *section, const char *param)
{
  std::string valbuf;
  char *retv;
  int ret;
  ret = getString (section, param, valbuf);
  if (ret)
    return nan ("f");
  return strtod (valbuf.c_str (), &retv);
}

int
Rts2ConfigRaw::getDouble (const char *section, const char *param,
			  double &value)
{
  std::string valbuf;
  char *retv;
  int ret;
  ret = getString (section, param, valbuf);
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
  Rts2ConfigRaw::getBoolean (const char *section, const char *param, bool def)
{
  std::string valbuf;
  int ret;
  ret = getString (section, param, valbuf);
  if (ret)
    return def;
  if (strcasecmp (valbuf.c_str (), "y") == 0
      || strcasecmp (valbuf.c_str (), "yes") == 0
      || strcasecmp (valbuf.c_str (), "true") == 0)
    return true;
  return false;
}
