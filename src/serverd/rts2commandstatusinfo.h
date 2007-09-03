#ifndef __RTS2_COMMAND_STATUSINFO__
#define __RTS2_COMMAND_STATUSINFO__

#include "../utils/rts2command.h"
#include "centrald.h"

class Rts2CommandStatusInfo:public Rts2Command
{
private:
  Rts2ConnCentrald * central_conn;
public:
  Rts2CommandStatusInfo (Rts2Centrald * master, Rts2ConnCentrald * conn);
  virtual int commandReturnOK ();
  virtual int commandReturnFailed ();
};

#endif /*! __RTS2_COMMAND_STATUSINFO__ */
