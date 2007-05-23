#include "rts2cliapp.h"

Rts2CliApp::Rts2CliApp (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
}

void
Rts2CliApp::afterProcessing ()
{
}

int
Rts2CliApp::run ()
{
  int ret;
  ret = init ();
  if (ret)
    return ret;
  ret = doProcessing ();
  afterProcessing ();
  return ret;
}
