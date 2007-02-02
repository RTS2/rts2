#ifndef __RTS2_NMENU__
#define __RTS2_NMENU__

#include "rts2daemonwindow.h"

#include <vector>

class Rts2NAction
{
private:
  const char *text;
  int code;
public:
    Rts2NAction (const char *in_text, int in_code)
  {
    text = in_text;
    code = in_code;
  }
  virtual ~ Rts2NAction (void)
  {
  }
  void draw (WINDOW * master_window, int y);
};

class Rts2NSubmenu:public Rts2NSelWindow
{
private:
  std::vector < Rts2NAction * >actions;
  const char *text;
public:
    Rts2NSubmenu (WINDOW * master_window, const char *in_text);
    virtual ~ Rts2NSubmenu (void);
  void addAction (Rts2NAction * in_action)
  {
    actions.push_back (in_action);
  }
  void createAction (const char *in_text, int in_code)
  {
    addAction (new Rts2NAction (in_text, in_code));
    grow (strlen (in_text) + 2, 1);
  }
  void draw (WINDOW * master_window);
  void drawSubmenu ();
};

class Rts2NMenu:public Rts2NWindow
{
private:
  std::vector < Rts2NSubmenu * >submenus;
  std::vector < Rts2NSubmenu * >::iterator selSubmenuIter;
  Rts2NSubmenu *selSubmenu;
  int top_x;
public:
    Rts2NMenu (WINDOW * master_window);
    virtual ~ Rts2NMenu (void);
  virtual int injectKey (int key);
  virtual void draw ();
  void addSubmenu (Rts2NSubmenu * in_submenu);
};

#endif /* !__RTS2_NMENU__ */
