#include "nmonitor.h"
#include "rts2nmsgwindow.h"

Rts2NMsgWindow::Rts2NMsgWindow ():Rts2NSelWindow (0, LINES - 19, COLS,
18, 1, 300, 500)
{
	msgMask = 0x07;
	setLineOffset (0);
	setSelRow (-1);
}


Rts2NMsgWindow::~Rts2NMsgWindow (void)
{
}


void
Rts2NMsgWindow::draw ()
{
	Rts2NSelWindow::draw ();
	werase (getWriteWindow ());
	maxrow = 0;
	int i = 0;
	for (std::list < Rts2Message >::iterator iter = messages.begin ();
		iter != messages.end () && maxrow < (padoff_y + getScrollHeight ());
		iter++, i++)
	{
		Rts2Message msg = *iter;
		if (!msg.passMask (msgMask))
			continue;
		if (maxrow < padoff_y)
		{
			maxrow++;
			continue;
		}
		switch (msg.getType ())
		{
			case MESSAGE_CRITICAL:
				wcolor_set (getWriteWindow (), CLR_FAILURE, NULL);
				break;
			case MESSAGE_ERROR:
				wcolor_set (getWriteWindow (), CLR_FAILURE, NULL);
				break;
			case MESSAGE_WARNING:
				wcolor_set (getWriteWindow (), CLR_WARNING, NULL);
				break;
			case MESSAGE_INFO:
				wcolor_set (getWriteWindow (), CLR_OK, NULL);
				break;
			case MESSAGE_DEBUG:
				wcolor_set (getWriteWindow (), CLR_TEXT, NULL);
				break;
		}
		struct tm tmesg;
		time_t t = msg.getMessageTimeSec ();
		localtime_r (&t, &tmesg);

		mvwprintw (getWriteWindow (), maxrow, 0, "%02i:%02i:%02i.%03i %3.3s %i %s",
			tmesg.tm_hour, tmesg.tm_min, tmesg.tm_sec, (int) (msg.getMessageTimeUSec () / 1000),
			msg.getMessageOName (), msg.getType (), msg.getMessageString ());

		wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
		maxrow++;
	}
	refresh ();
}


void
Rts2NMsgWindow::add (Rts2Message & msg)
{
	for (int tr = 0; tr < ((int) messages.size () - getScrollHeight ()); tr++)
	{
		messages.pop_front ();
	}
	messages.push_back (msg);
}


Rts2NMsgWindow & operator << (Rts2NMsgWindow & msgwin, Rts2Message & msg)
{
	msgwin.add (msg);
	return msgwin;
}
