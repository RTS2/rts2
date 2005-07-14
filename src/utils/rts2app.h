#ifndef __RTS2_APP__
#define __RTS2_APP__

#include <vector>

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

public:
    Rts2App (int in_argc, char **in_argv);
    virtual ~ Rts2App ();

  int initOptions ();
  virtual int init ();

  virtual void help ();

  virtual int processOption (int in_opt);
  virtual int processArgs (const char *arg);	// for non-optional args
  int addOption (char in_short_option, char *in_long_option, int in_has_arg,
		 char *in_help_msg)
  {
    Rts2Option *an_option =
      new Rts2Option (in_short_option, in_long_option, in_has_arg,
		      in_help_msg);
      options.push_back (an_option);
      return 0;
  }

  virtual int run ();
};

#endif /* !__RTS2_APP__ */
