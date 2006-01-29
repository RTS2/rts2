#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2config.h"

#include <ctype.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

Rts2Config *
  Rts2Config::pInstance = NULL;

Rts2Config::Rts2Config ()
{
  fp = NULL;
  observer.lat = 0;
  observer.lng = 0;
  checker = NULL;
}

Rts2Config::~Rts2Config ()
{
  if (checker)
    delete checker;
  if (fp)
    fclose (fp);
  fp = NULL;
}

Rts2Config *
Rts2Config::instance ()
{
  if (!pInstance)
    pInstance = new Rts2Config ();
  return pInstance;
}

int
Rts2Config::loadFile (char *filename)
{
  char horizont_file[250];
  if (!filename)
    // default
    filename = "/etc/rts2/rts2.ini";
  if (fp)
    fclose (fp);
  fp = fopen (filename, "r");
  if (fp == NULL)
    {
      syslog (LOG_ERR, "Rts2Config::loadFile cannot open '%s'", filename);
      return -1;
    }
  // get some commonly used values
  observer.lat = 0;
  observer.lng = 0;
  getDouble ("observatory", "longtitude", observer.lng);
  getDouble ("observatory", "latitude", observer.lat);
  // load horizont file..
  getString ("observatory", "horizont", horizont_file, 250);
  checker = new ObjectCheck (horizont_file);
  return 0;
}

int
Rts2Config::getString (const char *section, const char *param, char *buf,
		       int bufl)
{
#define BUF_SIZE 256
  int getsect;
  char tbuf[BUF_SIZE];
  char *ss;
  char *es;
  int ret;

  *buf = '\0';

  if (!fp)
    return 0;

  rewind (fp);

  getsect = 0;
  while (fgets (tbuf, BUF_SIZE, fp) != NULL)
    {
      ss = tbuf;
      while (*ss && isblank (*ss))
	ss++;
      if (*ss != '[')
	continue;
      if (*ss != '\0')
	{
	  while (*ss && isblank (*ss))
	    ss++;
	  ret = strncasecmp (ss + 1, section, strlen (section));
	  if (!ret)
	    {
	      getsect = 1;
	      break;
	    }
	}
    }

  if (!getsect)
    {
      return -1;
    }

  while (fgets (tbuf, BUF_SIZE, fp) != NULL)
    {
      ss = tbuf;
      while (*ss && isblank (*ss))
	ss++;
      if (*ss == ';')
	continue;
      if (*ss == '[')
	return -1;
      // did we have parameter?
      if (strncasecmp (ss, param, strlen (param)) == 0)
	{
	  // find '='
	  ss += strlen (param);
	  while (*ss && *ss != '=')
	    ss++;
	  if (*ss != '=')
	    continue;
	  ss++;
	  // copy value..ignore blanks, if we find ", thread them as string delimeter, otherwise use only non-blanks..
	  while (*ss && isblank (*ss))
	    ss++;
	  if (*ss == '"')
	    {
	      ss++;
	      es = ss;
	      int len = 0;
	      while (*es && *es != '"')
		{
		  es++;
		  len++;
		  if (len == bufl)
		    break;
		}
	      if (*es != '"')
		return -1;
	      strncpy (buf, ss, len);
	      buf[len] = '\0';
	    }
	  else
	    {
	      es = ss;
	      int len = 0;
	      while (*es && !isblank (*es) && *es != '\r' && *es != '\n')
		{
		  es++;
		  len++;
		  if (len == bufl)
		    break;
		}
	      strncpy (buf, ss, len);
	      buf[len] = '\0';
	    }
	  return 0;
	}
    }
  return -1;
#undef BUF_SIZE
}

int
Rts2Config::getInteger (const char *section, const char *param, int &value)
{
  char valbuf[100];
  char *retv;
  int ret;
  ret = getString (section, param, valbuf, 100);
  if (ret)
    return ret;
  value = strtol (valbuf, &retv, 0);
  if (*retv != '\0')
    {
      value = 0;
      return -1;
    }
  return 0;
}

int
Rts2Config::getFloat (const char *section, const char *param, float &value)
{
  char valbuf[100];
  char *retv;
  int ret;
  ret = getString (section, param, valbuf, 100);
  if (ret)
    return ret;
  value = strtof (valbuf, &retv);
  if (*retv != '\0')
    {
      value = 0;
      return -1;
    }
  return 0;
}

int
Rts2Config::getDouble (const char *section, const char *param, double &value)
{
  char valbuf[100];
  char *retv;
  int ret;
  ret = getString (section, param, valbuf, 100);
  if (ret)
    return ret;
  value = strtod (valbuf, &retv);
  if (*retv != '\0')
    {
      value = 0;
      return -1;
    }
  return 0;
}

int
Rts2Config::getBoolean (const char *section, const char *param)
{
  char valbuf[100];
  int ret;
  ret = getString (section, param, valbuf, 100);
  if (ret)
    return 0;
  if (strcasecmp (valbuf, "y") == 0
      || strcasecmp (valbuf, "yes") == 0 || strcasecmp (valbuf, "true") == 0)
    return 1;
  return 0;
}

void
Rts2Config::getBoolean (const char *section, const char *param, bool & value)
{
  char valbuf[100];
  int ret;
  ret = getString (section, param, valbuf, 100);
  if (ret)
    return;
  if (strcasecmp (valbuf, "y") == 0
      || strcasecmp (valbuf, "yes") == 0 || strcasecmp (valbuf, "true") == 0)
    value = true;
  else
    value = false;
}

struct ln_lnlat_posn *
Rts2Config::getObserver ()
{
  return &observer;
}

ObjectCheck *
Rts2Config::getObjectChecker ()
{
  return checker;
}
