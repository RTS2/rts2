#include "nmonitor.h"
#include "rts2nmsgbox.h"

Rts2NMsgBox::Rts2NMsgBox (const char *in_query, const char *in_buttons[],
int in_butnum):
Rts2NWindow (COLS / 2 - 25, LINES / 2 - 15, 50, 5)
{
	query = in_query;
	buttons = in_buttons;
	butnum = in_butnum;
	exitState = 0;
}


Rts2NMsgBox::~Rts2NMsgBox (void)
{
}


keyRet
Rts2NMsgBox::injectKey (int key)
{
	switch (key)
	{
		case KEY_RIGHT:
			exitState++;
			break;
		case KEY_LEFT:
			exitState--;
			break;
		case KEY_EXIT:
		case K_ESC:
			exitState = -1;
			return RKEY_ESC;
		case KEY_ENTER:
		case K_ENTER:
			return RKEY_ENTER;
		default:
			return Rts2NWindow::injectKey (key);
	}
	if (exitState < 0)
		exitState = 0;
	if (exitState >= butnum)
		exitState = butnum - 1;
	return RKEY_HANDLED;
}


void
Rts2NMsgBox::draw ()
{
	Rts2NWindow::draw ();
	mvwprintw (window, 1, 2, "%s", query);
	for (int i = 0; i < butnum; i++)
	{
		if (i == exitState)
			wattron (window, A_REVERSE);
		else
			wattroff (window, A_REVERSE);
		mvwprintw (window, 2, 2 + i * 30 / 2, buttons[i]);
	}
	wattroff (window, A_REVERSE);
	refresh ();
}
