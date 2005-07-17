#ifndef __RTS2_CONN_NOSEND__
#define __RTS2_CONN_NOSEND__

#include "rts2block.h"

class Rts2ConnNoSend:public Rts2Conn
{

public:
  Rts2ConnNoSend (Rts2Block * in_master);
  virtual ~ Rts2ConnNoSend (void);

  virtual int send (char *message);
};

#endif /* !__RTS2_CONN_NOSEND__ */
