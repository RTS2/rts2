#include "rts2commandstatusinfo.h"

Rts2CommandStatusInfo::Rts2CommandStatusInfo (Rts2Centrald * master, Rts2ConnCentrald * in_central_conn):Rts2Command
  (master)
{
  central_conn = in_central_conn;
  setCommand ("status_info");
}

int
Rts2CommandStatusInfo::commandReturnOK ()
{
  central_conn->updateStatusWait ();
  return RTS2_COMMAND_KEEP;
}

int
Rts2CommandStatusInfo::commandReturnFailed ()
{
  central_conn->updateStatusWait ();
  return RTS2_COMMAND_KEEP;
}
