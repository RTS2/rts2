#ifndef __RTS2_NCOMWIN__
#define __RTS2_NCOMWIN__

#include "rts2daemonwindow.h"

class Rts2NComWin:public Rts2NWindow
{
private:
  WINDOW * comwin;
  WINDOW *statuspad;
public:
    Rts2NComWin (WINDOW * master_window);
    virtual ~ Rts2NComWin (void);
  virtual int injectKey (int key);
  virtual void draw ();
  virtual void refresh ();

  virtual void clear ()
  {
    werase (comwin);
    Rts2NWindow::clear ();
  }

  virtual void setCursor ();

  virtual WINDOW *getWriteWindow ()
  {
    return comwin;
  }

  void getWinString (char *buf, int n)
  {
    mvwinnstr (comwin, 0, 0, buf, n);
  }
  void printCommand (char *cmd)
  {
    mvwprintw (statuspad, 0, 0, "%s", cmd);
  }

  void commandReturn (Rts2Command * cmd, int cmd_status);
};

#endif /* !__RTS2_NCOMWIN__ */
