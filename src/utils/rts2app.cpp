#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2app.h"

#include <iostream>
#include <sstream>
#include <syslog.h>
#include <libnova/libnova.h>

Rts2App::Rts2App (int in_argc, char **in_argv):
Rts2Object ()
{
  argc = in_argc;
  argv = in_argv;

  addOption ('h', "help", 0, "write this help");
}

Rts2App::~Rts2App ()
{
  std::vector < Rts2Option * >::iterator opt_iter;

  for (opt_iter = options.begin (); opt_iter != options.end (); opt_iter++)
    {
      delete (*opt_iter);
    }
  options.clear ();
}

int
Rts2App::initOptions ()
{
  int c;
  int ret;

  std::vector < Rts2Option * >::iterator opt_iter;

  Rts2Option *opt;

  struct option *long_option, *an_option;

  long_option =
    (struct option *) malloc (sizeof (struct option) * (options.size () + 1));

  char *opt_char = new char[options.size () * 3 + 1];

  char *end_opt = opt_char;

  an_option = long_option;

  for (opt_iter = options.begin (); opt_iter != options.end (); opt_iter++)
    {
      opt = (*opt_iter);
      opt->getOptionStruct (an_option);
      opt->getOptionChar (&end_opt);
      an_option++;
    }

  *end_opt = '\0';

  an_option->name = NULL;
  an_option->has_arg = 0;
  an_option->flag = NULL;
  an_option->val = 0;

  /* get attrs */
  while (1)
    {
      c = getopt_long (argc, argv, opt_char, long_option, NULL);

      if (c == -1)
	break;
      ret = processOption (c);
      if (ret)
	return ret;
    }

  delete[]opt_char;

  while (optind < argc)
    {
      ret = processArgs (argv[optind++]);
      if (ret)
	return ret;
    }

  return 0;
}

int
Rts2App::init ()
{
  return initOptions ();
}

void
Rts2App::helpOptions ()
{
  std::vector < Rts2Option * >::iterator opt_iter;
  for (opt_iter = options.begin (); opt_iter != options.end (); opt_iter++)
    {
      (*opt_iter)->help ();
    }
}

void
Rts2App::help ()
{
  std::cout << "Options" << std::endl;
  helpOptions ();
}

int
Rts2App::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'h':
    case 0:
      help ();
      exit (EXIT_SUCCESS);
    case '?':
      break;
    default:
      std::
	cout << "Unknow option: " << in_opt << "(" << (char) in_opt << ")" <<
	std::endl;
      exit (EXIT_FAILURE);
    }
  return 0;
}

int
Rts2App::processArgs (const char *arg)
{
  return -1;
}

int
Rts2App::addOption (char in_short_option, char *in_long_option,
		    int in_has_arg, char *in_help_msg)
{
  Rts2Option *an_option =
    new Rts2Option (in_short_option, in_long_option, in_has_arg,
		    in_help_msg);
  options.push_back (an_option);
  return 0;
}

int
Rts2App::askForInt (const char *desc, int &val)
{
  char temp[200];
  while (1)
    {
      std::cout << desc << " [" << val << "]: ";
      std::cin.getline (temp, 200);
      std::string str_val (temp);
      if (str_val.empty ())
	break;
      std::istringstream is (str_val);
      is >> val;
      if (!is.fail ())
	break;
      std::cout << "Invalid number!" << std::endl;
      std::cin.clear ();
      std::cin.ignore (2000, '\n');
    }
  std::cout << desc << ": " << val << std::endl;
  return 0;
}

int
Rts2App::askForDouble (const char *desc, double &val)
{
  char temp[200];
  while (1)
    {
      std::cout << desc << " [" << val << "]: ";
      std::cin.getline (temp, 200);
      std::string str_val (temp);
      if (str_val.empty ())
	break;
      std::istringstream is (str_val);
      is >> val;
      if (!is.fail ())
	break;
      std::cout << "Invalid number!" << std::endl;
      std::cin.clear ();
      std::cin.ignore (2000, '\n');
    }
  std::cout << desc << ": " << val << std::endl;
  return 0;
}

int
Rts2App::askForString (const char *desc, std::string & val)
{
  char temp[201];
  while (1)
    {
      std::cout << desc << " [" << val << "]: ";
      std::cin.getline (temp, 200);
      // use default value
      if (strlen (temp) == 0)
	break;
      val = std::string (temp);
      if (!std::cin.fail ())
	break;
      std::cout << "Invalid string!" << std::endl;
      std::cin.clear ();
      std::cin.ignore (2000, '\n');
    }
  std::cout << desc << ": " << val << std::endl;
  return 0;
}

int
Rts2App::askForChr (const char *desc, char &out)
{
  char temp[201];
  std::cout << desc;
  std::cin.getline (temp, 200);
  out = *temp;
  return 0;
}

int
Rts2App::run ()
{
  std::cout << "Empty run methods!" << std::endl;
  return 0;
}

int
Rts2App::parseDate (const char *in_date, struct tm *out_time)
{
  char *ret;
  char *ret2;
  out_time->tm_isdst = 0;
  out_time->tm_hour = out_time->tm_min = out_time->tm_sec = 0;
  ret = strptime (in_date, "%Y-%m-%d", out_time);
  if (ret && ret != in_date)
    {
      // we end with is T, let's check if it contains time..
      if (*ret == 'T')
	{
	  ret2 = strptime (ret, "T%H:%M:%S", out_time);
	  if (ret2 && *ret2 == '\0')
	    return 0;
	  ret2 = strptime (ret, "T%H:%M", out_time);
	  if (ret2 && *ret2 == '\0')
	    return 0;
	  ret2 = strptime (ret, "T%H", out_time);
	  if (ret2 && *ret2 == '\0')
	    return 0;
	  return -1;
	}
      // only year..
      return 0;
    }
  return -1;
}

int
Rts2App::parseDate (const char *in_date, double &JD)
{
  struct tm tm_date;
  struct ln_date l_date;
  int ret;
  ret = parseDate (in_date, &tm_date);
  if (ret)
    return ret;
  ln_get_date_from_tm (&tm_date, &l_date);
  JD = ln_get_julian_day (&l_date);
  return 0;
}

int
sendMailTo (const char *subject, const char *text, const char *mailAddress,
	    Rts2Object * master)
{
  int ret;
  char *cmd;
  FILE *mailFile;

  // fork so we will not inhibit calling process..
  ret = fork ();
  if (ret == -1)
    {
      syslog (LOG_ERR, "Rts2Block::sendMail fork: %m");
      return -1;
    }
  if (ret != 0)
    {
      return 0;
    }
  if (master)
    master->forkedInstance ();
  asprintf (&cmd, "/usr/bin/mail -s '%s' '%s'", subject, mailAddress);
  mailFile = popen (cmd, "w");
  if (!mailFile)
    {
      syslog (LOG_ERR, "Rts2Block::sendMail popen: %m");
      exit (0);
    }
  fprintf (mailFile, "%s", text);
  pclose (mailFile);
  free (cmd);
  exit (0);
}
