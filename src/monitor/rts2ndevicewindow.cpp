#include "nmonitor.h"
#include "rts2ndevicewindow.h"

#include "../utils/timestamp.h"
#include "../utils/rts2displayvalue.h"


Rts2NDeviceWindow::Rts2NDeviceWindow (WINDOW * master_window, Rts2Conn * in_connection):Rts2NSelWindow
  (master_window, 10, 1, COLS - 10,
   LINES - 25)
{
  connection = in_connection;
  draw ();
}

Rts2NDeviceWindow::~Rts2NDeviceWindow ()
{
}

void
Rts2NDeviceWindow::drawValuesList ()
{
  Rts2DevClient *client = connection->getOtherDevClient ();
  if (client)
    drawValuesList (client);
}

void
Rts2NDeviceWindow::drawValuesList (Rts2DevClient * client)
{
  struct timeval tv;
  gettimeofday (&tv, NULL);
  double now = tv.tv_sec + tv.tv_usec / USEC_SEC;
  for (std::vector < Rts2Value * >::iterator iter = client->valueBegin ();
       iter != client->valueEnd (); iter++)
    {
      Rts2Value *val = *iter;
      // customize value display
      std::ostringstream _os;
      if (val->getWriteToFits ())
	wcolor_set (getWriteWindow (), CLR_FITS, NULL);
      else
	wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
      switch (val->getValueType ())
	{
	case RTS2_VALUE_TIME:
	  _os << LibnovaDateDouble (val->
				    getValueDouble ()) << " (" <<
	    TimeDiff (now, val->getValueDouble ()) << ")";
	  wprintw (getWriteWindow (), "%-20s|%30s\n",
		   val->getName ().c_str (), _os.str ().c_str ());
	  break;
	case RTS2_VALUE_BOOL:
	  wprintw (getWriteWindow (), "%-20s|%30s\n",
		   val->getName ().c_str (),
		   ((Rts2ValueBool *) val)->
		   getValueBool ()? "true" : "false");
	  break;
	default:
	  wprintw (getWriteWindow (), "%-20s|%30s\n",
		   val->getName ().c_str (), getDisplayValue (val).c_str ());
	}
      maxrow++;
    }
}

void
Rts2NDeviceWindow::printValueDesc (Rts2Value * val)
{
  mvwprintw (window, getHeight () - 1, 2, "D: \"%s\"",
	     val->getDescription ().c_str ());
}

void
Rts2NDeviceWindow::draw ()
{
  Rts2NWindow::draw ();
  werase (getWriteWindow ());
  maxrow = 1;
  printState (connection);
  drawValuesList ();
  int s = getSelRow ();
  if (s >= 1 && connection->getOtherDevClient ())
    printValueDesc (connection->getOtherDevClient ()->valueAt (s - 1));
  refresh ();
}
