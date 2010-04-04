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
		char mt;
		switch (msg.getType ())
		{
			case MESSAGE_CRITICAL:
				mt = 'C';
				wcolor_set (getWriteWindow (), CLR_FAILURE, NULL);
				break;
			case MESSAGE_ERROR:
				mt = 'E';
				wcolor_set (getWriteWindow (), CLR_FAILURE, NULL);
				break;
			case MESSAGE_WARNING:
				mt = 'W';
				wcolor_set (getWriteWindow (), CLR_WARNING, NULL);
				break;
			case MESSAGE_INFO:
				mt = 'i';
				wcolor_set (getWriteWindow (), CLR_OK, NULL);
				break;
			case MESSAGE_DEBUG:
				mt = 'd';
				wcolor_set (getWriteWindow (), CLR_TEXT, NULL);
				break;
		}
		struct tm tmesg;
		time_t t = msg.getMessageTimeSec ();
		localtime_r (&t, &tmesg);

		mvwprintw (getWriteWindow (), maxrow, 0, "%02i:%02i:%02i.%03i %c %s %s",
			tmesg.tm_hour, tmesg.tm_min, tmesg.tm_sec, (int) (msg.getMessageTimeUSec () / 1000),
			mt, msg.getMessageOName (), msg.getMessageString ());

		wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
		maxrow++;
	}

	wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
	mvwvline (getWriteWindow (), 0, 12, ACS_VLINE, (maxrow > getHeight ()? maxrow : getHeight ()));
	mvwaddch (window, 0, 13, ACS_TTEE);
	mvwaddch (window, getHeight () - 1, 13, ACS_BTEE);

	mvwvline (getWriteWindow (), 0, 14, ACS_VLINE, (maxrow > getHeight ()? maxrow : getHeight ()));
	mvwaddch (window, 0, 15, ACS_TTEE);
	mvwaddch (window, getHeight () - 1, 15, ACS_BTEE);


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
