#ifndef __RTS2_OPTION__
#define __RTS2_OPTION__

#include <getopt.h>
#include <stdio.h>

class Rts2Option
{
  char short_option;
  char *long_option;
  int has_arg;
  char *help_msg;
public:
    Rts2Option (char in_short_option, char *in_long_option, int in_has_arg,
		char *in_help_msg)
  {
    short_option = in_short_option;
    long_option = in_long_option;
    has_arg = in_has_arg;
    help_msg = in_help_msg;
  }
  void help ()
  {
    printf ("\t-%c|--%-15s  %s\n", short_option, long_option, help_msg);
  }
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
