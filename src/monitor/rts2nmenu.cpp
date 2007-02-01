#include "rts2nmenu.h"

Rts2NMenu::Rts2NMenu (WINDOW * master_window):Rts2NWindow (master_window, 0, 0, 1, 1,
	     0)
{
}

Rts2NMenu::~Rts2NMenu (void)
{
}

int
Rts2NMenu::injectKey (int key)
{
  return 0;
}

void
Rts2NMenu::draw ()
{
  Rts2NWindow::draw ();
  werase (window);
  refresh ();
}
