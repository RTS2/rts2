#ifndef __RTS2_OPTION__
#define __RTS2_OPTION__

#include <getopt.h>
#include <malloc.h>

// custom configuration values

#define OPT_VERSION	999
#define OPT_DATABASE	1000
#define OPT_CONFIG	1001

class Rts2Option
{
  int short_option;
  char *long_option;
  int has_arg;
  char *help_msg;
public:
    Rts2Option (int in_short_option, char *in_long_option, int in_has_arg,
		char *in_help_msg)
  {
    short_option = in_short_option;
    long_option = in_long_option;
    has_arg = in_has_arg;
    help_msg = in_help_msg;
  }
  void help ();
  void getOptionChar (char **end_opt);
  void getOptionStruct (struct option *options)
  {
    options->name = long_option;
    options->has_arg = has_arg;
    options->flag = NULL;
    options->val = short_option;
  }
};

#endif /* !__RTS2_OPTION__ */
