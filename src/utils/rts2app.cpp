#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2app.h"

#include <iostream>

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

  char *opt_char = new char[options.size () * 2 + 1];

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
Rts2App::run ()
{
  std::cout << "Empty run methods!" << std::endl;
  return 0;
}
