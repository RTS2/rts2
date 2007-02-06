#ifndef __RTS2_NMSGWINDOW__
#define __RTS2_NMSGWINDOW__

#include "rts2daemonwindow.h"
#include "../utils/rts2message.h"

#include <list>

class Rts2NMsgWindow:public Rts2NSelWindow
{
private:
  std::list < Rts2Message > messages;
public:
  Rts2NMsgWindow (WINDOW * master_window);
  virtual ~ Rts2NMsgWindow (void);
  virtual void draw ();
  friend Rts2NMsgWindow & operator << (Rts2NMsgWindow & msgwin,
				       Rts2Message & msg);

  void add (Rts2Message & msg);
};

Rts2NMsgWindow & operator << (Rts2NMsgWindow & msgwin, Rts2Message & msg);

#endif /* !__RTS2_NMSGWINDOW__ */
