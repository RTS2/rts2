#ifndef __RTS2_CLIAPP__
#define __RTS2_CLIAPP__

#include "rts2app.h"

#include <libnova/libnova.h>

class Rts2CliApp:public Rts2App
{
protected:
  virtual int doProcessing () = 0;
  virtual void afterProcessing ();

public:
    Rts2CliApp (int in_argc, char **in_argv);

  virtual int run ();

  /**
   * Parses and initialize tm structure from char.
   *
   * String can contain either date, in that case it will be converted to night
   * starting on that date, or full date with time (hour, hour:min, or hour:min:sec).
   *
   * @return -1 on error, 0 on succes
   */
  int parseDate (const char *in_date, struct ln_date *out_time);
};

#endif /* !__RTS2_CLIAPP__ */
