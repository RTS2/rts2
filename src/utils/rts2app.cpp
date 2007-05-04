#include "rts2app.h"

#include "config.h"

#include <iostream>
#include <signal.h>
#include <sstream>
#include <errno.h>
#include <libnova/libnova.h>

#include <sys/types.h>
#include <unitsd.h>

static Rts2App *masterApp = NULL;

Rts2App *
getMasterApp ()
{
  return masterApp;
}

Rts2LogStream
logStream (messageType_t in_messageType)
{
  Rts2LogStream ls (masterApp, in_messageType);
  return ls;
}

Rts2App::Rts2App (int in_argc, char **in_argv):
Rts2Object ()
{
  argc = in_argc;
  argv = in_argv;

  end_loop = false;

  addOption ('h', "help", 0, "write this help");
  addOption ('V', "version", 0, "show program version and license");
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

void
signalHUP (int sig)
{
  masterApp->sigHUP (sig);
}

void
killSignal (int sig)
{
  signal (SIGHUP, exit);
  signal (SIGINT, exit);
  signal (SIGTERM, exit);
  if (masterApp)
    {
      if (masterApp->getEndLoop ())
	{
	  exit (0);
	}
      masterApp->endRunLoop ();
    }
}

int
Rts2App::init ()
{
  masterApp = this;
  signal (SIGHUP, signalHUP);
  signal (SIGINT, killSignal);
  signal (SIGTERM, killSignal);

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
    case 'V':
      std::cout << "Part of RTS2 version: " << VERSION << std::endl
	<< std::endl
	<<
	"(C) 2001-2007 Petr Kubanek and others, see AUTHORS file for complete list"
	<< std::endl << std::
	endl << "This program comes with ABSOLUTELY NO WARRANTY; for details"
	<< std::
	endl <<
	"see http://www.gnu.org.  This is free software, and you are welcome"
	<< std::
	endl <<
	"to redistribute it under certain conditions; see http://www.gnu.org"
	<< std::endl << "for them." << std::endl << std::endl;
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
  while (!getEndLoop ())
    {
      std::cout << desc << " [";
      if (val == INT_MIN)
	std::cout << " new id ";
      else
	std::cout << val;
      std::cout << "]: ";
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
  while (!getEndLoop ())
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
  while (!getEndLoop ())
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

bool Rts2App::askForBoolean (const char *desc, bool val)
{
  char
    temp[20];
  while (!getEndLoop ())
    {
      std::cout << desc << " (y/n) [" << (val ? "y" : "n") << "]: ";
      std::cin.getline (temp, 20);
      // use default value
      if (strlen (temp) == 0)
	break;
      switch (*temp)
	{
	case 'Y':
	case 'y':
	  val = true;
	  break;
	case 'N':
	case 'n':
	  val = false;
	  break;
	default:
	  std::cout << "Invalid string!" << std::endl;
	  std::cin.clear ();
	  std::cin.ignore (2000, '\n');
	  continue;
	}
      break;
    }
  std::cout << desc << ": " << (val ? "y" : "n") << std::endl;
  return val;
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

void
Rts2App::sendMessage (messageType_t in_messageType,
		      const char *in_messageString)
{
  Rts2Message msg = Rts2Message ("app", in_messageType, in_messageString);
  std::cerr << msg.toString () << std::endl;
}

void
Rts2App::sendMessage (messageType_t in_messageType, std::ostringstream & _os)
{
  sendMessage (in_messageType, _os.str ().c_str ());
}

Rts2LogStream Rts2App::logStream (messageType_t in_messageType)
{
  Rts2LogStream
  ls (this, in_messageType);
  return ls;
}

int
Rts2App::sendMailTo (const char *subject, const char *text,
		     const char *in_mailAddress)
{
  int ret;
  char *cmd;
  FILE *mailFile;

  // fork so we will not inhibit calling process..
  ret = fork ();
  if (ret == -1)
    {
      logStream (MESSAGE_ERROR) << "Rts2Block::sendMail fork: " <<
	strerror (errno) << sendLog;
      return -1;
    }
  if (ret != 0)
    {
      return 0;
    }
  forkedInstance ();
  asprintf (&cmd, "/usr/bin/mail -s '%s' '%s'", subject, in_mailAddress);
  mailFile = popen (cmd, "w");
  if (!mailFile)
    {
      logStream (MESSAGE_ERROR) << "Rts2Block::sendMail popen: " <<
	strerror (errno) << sendLog;
      exit (0);
    }
  fprintf (mailFile, "%s", text);
  pclose (mailFile);
  free (cmd);
  exit (0);
}

void
Rts2App::sigHUP (int sig)
{
  endRunLoop ();
}
