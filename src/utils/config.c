#define _GNU_SOURCE

#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct config_entry
{
  char *name;
  char type;
  void *value;
  struct config_entry *next;
}
 *config = NULL;

void
free_config ()
{
  struct config_entry *cfg = config, *last;
  config = NULL;
  while (cfg)
    {
      last = cfg;
      cfg = cfg->next;
      free (last->name);
      free (last->value);
      free (last);
    }
}

int
read_config (char *filename)
{
#define BUF_SIZE	100
  char buf[BUF_SIZE];
  char *value, *name;
  struct config_entry *cfg;
  FILE *f = fopen (filename, "r");
  if (!f)
    return -1;
  free_config ();
  config = cfg =
    (struct config_entry *) malloc (sizeof (struct config_entry));
  cfg->next = NULL;
  while (fgets (buf, 100, f))
    {
      char *bp = buf;
      int state = 0;
      int name_len = 0, value_len = 0;
      value = NULL;
      while (*bp)		// put out comments
	{
	  if (*bp == '#')
	    *bp = 0;
	  else
	    {
	      switch (state)
		{
		case 0:
		  if (!isblank (*bp))
		    {
		      name = bp;
		      state = 1;
		      name_len = 1;
		    }
		  break;
		case 1:
		  if (isblank (*bp))
		    {
		      *bp = 0;
		      state = 2;
		    }
		  else
		    name_len++;
		  break;
		case 2:
		  if (*bp == '=')
		    {
		      *bp = 0;
		      value = bp;
		      state = 3;
		    }
		  break;
		case 3:
		  if (*bp == '"')
		    {
		      value = bp + 1;
		      state = 6;
		    }
		  else if (!isblank (*bp))
		    {
		      value = bp;
		      state = 4;
		      value_len++;
		    }
		  break;
		case 4:
		  if (isblank (*bp) || isspace (*bp))
		    {
		      *bp = 0;
		      state = 5;
		    }
		  else
		    value_len++;
		  break;
		case 5:
		  if (!(isblank (*bp) || isspace (*bp)))
		    state = 7;
		  break;
		case 6:
		  if (*bp == '"')
		    {
		      *bp = 0;
		      state = 8;
		    }
		  else
		    value_len++;
		  break;
		}
	      bp++;
	    }
	}

      if (state == 5 || state == 8)
	{
	  double val;
	  char *end_val;
	  val = strtod (value, &end_val);
	  if (*end_val || state == 8)
	    {
	      cfg->value = (char *) malloc (value_len + 1);
	      strcpy (cfg->value, value);
	      cfg->type = CFG_STRING;
	    }
	  else
	    {
	      cfg->value = (double *) malloc (sizeof (double));
	      *(double *) cfg->value = val;
	      cfg->type = CFG_DOUBLE;
	    }
	  cfg->next =
	    (struct config_entry *) malloc (sizeof (struct config_entry));
	  cfg->next->name = NULL;
	  cfg->name = (char *) malloc (name_len + 1);
	  strcpy (cfg->name, name);
	  cfg = cfg->next;
	}
    }
  return 0;
}

struct config_entry *
get_entry (char *name)
{
  struct config_entry *cfg = config;
  while (cfg && cfg->name)
    {
      if (!strcasecmp (cfg->name, name))
	return cfg;
      cfg = cfg->next;
    }
  return NULL;
}

int
get_string (char *name, char **val)
{
  struct config_entry *cfg = get_entry (name);
  if (cfg && cfg->type == CFG_STRING)
    {
      *val = cfg->value;
      return 0;
    }
  return -1;
}

char *
get_string_default (char *name, char *def)
{
  char *val;
  if (get_string (name, &val) == -1)
    return def;
  return val;
}

int
get_double (char *name, double *val)
{
  struct config_entry *cfg = get_entry (name);
  if (cfg && cfg->type == CFG_DOUBLE)
    {
      *val = *(double *) cfg->value;
      return 0;
    }
  return -1;
}

double
get_double_default (char *name, double def)
{
  double val;
  if (get_double (name, &val) == -1)
    return def;
  return val;
}

struct config_entry *
get_device_entry (char *device, char *name)
{
  struct config_entry *cfg = config;
  int dev_len = strlen (device);
  while (cfg && cfg->name)
    {
      if (!strncasecmp (cfg->name, device, dev_len))
	if (cfg->name[dev_len] == '.'
	    && !strcasecmp (&cfg->name[dev_len + 1], name))
	  return cfg;
      cfg = cfg->next;
    }
  // return null, if device doesn't exist
  return get_entry ("name");
}

extern int
get_device_string (char *device, char *name, char **val)
{
  struct config_entry *cfg = get_device_entry (device, name);
  if (cfg && cfg->type == CFG_STRING)
    {
      *val = cfg->value;
      return 0;
    }
  return -1;
}

extern char *
get_device_string_default (char *device, char *name, char *def)
{
  char *val;
  if (get_device_string (device, name, &val) == -1)
    return get_string_default (name, def);
  return val;
}

extern int
get_device_double (char *device, char *name, double *val)
{
  struct config_entry *cfg = get_device_entry (device, name);
  if (cfg && cfg->type == CFG_DOUBLE)
    {
      *val = *(double *) cfg->value;
      return 0;
    }
  return -1;
}

extern double
get_device_double_default (char *device, char *name, double def)
{
  double val;
  if (get_device_double (device, name, &val) == -1)
    return get_double_default (name, def);
  return val;
}
