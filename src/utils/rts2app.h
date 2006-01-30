#ifndef __RTS2_APP__
#define __RTS2_APP__

#include <vector>
#include <time.h>

#include "rts2object.h"
#include "rts2option.h"

class Rts2App:public Rts2Object
{
private:
  std::vector < Rts2Option * >options;

  /**
   * Prints help message, describing all options
   */
  void helpOptions ();

  int argc;
  char **argv;

protected:
    virtual int processOption (int in_opt);
  virtual int processArgs (const char *arg);	// for non-optional args
  int addOption (char in_short_option, char *in_long_option, int in_has_arg,
		 char *in_help_msg);

  /**
   * Ask user for integer.
   */
  int askForInt (const char *desc, int &val);
  int askForString (const char *desc, std::string & val);

public:  Rts2App (int in_argc, char **in_argv);
    virtual ~ Rts2App ();

  int initOptions ();
  virtual int init ();

  virtual void help ();

  virtual int run ();

  /**
   * Parses and initialize tm structure from char.
   *
   * String can contain either date, in that case it will be converted to night
   * starting on that date, or full date with time (hour, hour:min, or hour:min:sec).
   *
   * @return -1 on error, 0 on succes
   */
  int parseDate (const char *in_date, struct tm *out_time);
};

// send mail to recepient; requires /usr/bin/mail binary
int sendMailTo (const char *subject, const char *text,
		const char *mailAddress);

#endif /* !__RTS2_APP__ */
