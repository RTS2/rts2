#include "nmonitor.h"
#include "rts2ndevicewindow.h"

#include "../utils/timestamp.h"
#include "../utils/rts2displayvalue.h"


Rts2NDeviceWindow::Rts2NDeviceWindow (Rts2Conn * in_connection):Rts2NSelWindow
  (10, 1, COLS - 10,
   LINES - 25)
{
  connection = in_connection;
  valueBox = NULL;
  draw ();
}

Rts2NDeviceWindow::~Rts2NDeviceWindow ()
{
}

void
Rts2NDeviceWindow::printState ()
{
  wattron (window, A_REVERSE);
  if (connection->getErrorState ())
    wcolor_set (window, CLR_FAILURE, NULL);
  else if (connection->havePriority ())
    wcolor_set (window, CLR_OK, NULL);
  mvwprintw (window, 0, 2, "%s %s (%i) priority: %s", connection->getName (),
	     connection->getStateString ().c_str (), connection->getState (),
	     connection->havePriority ()? "yes" : "no");
  wcolor_set (window, CLR_DEFAULT, NULL);
  wattroff (window, A_REVERSE);
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

  maxrow = 0;

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
      // ultra special handling of SCRIPT value
      if (val->getValueDisplayType () == RTS2_DT_SCRIPT)
	{
	  wprintw (getWriteWindow (), "%-20s ", val->getName ().c_str ());
	  wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
	  char *valStart = val->getValue ();
	  char *valTop = valStart;
	  int scriptPosition = client->getValueInteger ("scriptPosition");
	  int scriptEnd =
	    client->getValueInteger ("scriptLen") + scriptPosition;
	  while (*valTop && (valTop - valStart < scriptPosition))
	    {
	      waddch (getWriteWindow (), *valTop);
	      valTop++;
	    }
	  wcolor_set (getWriteWindow (), CLR_SCRIPT_CURRENT, NULL);
	  while (*valTop && (valTop - valStart < scriptEnd))
	    {
	      waddch (getWriteWindow (), *valTop);
	      valTop++;
	    }
	  wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
	  while (*valTop)
	    {
	      waddch (getWriteWindow (), *valTop);
	      valTop++;
	    }
	  waddch (getWriteWindow (), '\n');
	  continue;
	}
      switch (val->getValueType ())
	{
	case RTS2_VALUE_TIME:
	  _os << LibnovaDateDouble (val->
				    getValueDouble ()) << " (" <<
	    TimeDiff (now, val->getValueDouble ()) << ")";
	  wprintw (getWriteWindow (), "%-20s %30s\n",
		   val->getName ().c_str (), _os.str ().c_str ());
	  break;
	case RTS2_VALUE_BOOL:
	  wprintw (getWriteWindow (), "%-20s %30s\n",
		   val->getName ().c_str (),
		   ((Rts2ValueBool *) val)->
		   getValueBool ()? "true" : "false");
	  break;
	default:
	  wprintw (getWriteWindow (), "%-20s %30s\n",
		   val->getName ().c_str (), getDisplayValue (val).c_str ());
	}
      maxrow++;
    }
  wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
  mvwvline (getWriteWindow (), 0, 20, ACS_VLINE,
	    (maxrow > getHeight ()? maxrow : getHeight ()));
  mvwaddch (window, 0, 21, ACS_TTEE);
  mvwaddch (window, getHeight () - 1, 21, ACS_BTEE);
}

Rts2Value *
Rts2NDeviceWindow::getSelValue ()
{
  int s = getSelRow ();
  if (s >= 0 && connection->getOtherDevClient ())
    return connection->getOtherDevClient ()->valueAt (s);
  return NULL;
}

void
Rts2NDeviceWindow::printValueDesc (Rts2Value * val)
{
  wattron (window, A_REVERSE);
  mvwprintw (window, getHeight () - 1, 2, "D: \"%s\"",
	     val->getDescription ().c_str ());
  wattroff (window, A_REVERSE);
}

void
Rts2NDeviceWindow::endValueBox ()
{
  delete valueBox;
  valueBox = NULL;
}

void
Rts2NDeviceWindow::createValueBox ()
{
  int s = getSelRow ();
  if (s >= 0 && connection->getOtherDevClient ())
    {
      Rts2Value *val = connection->getOtherDevClient ()->valueAt (s);
      if (!val)
	return;
      switch (val->getValueType ())
	{
	case RTS2_VALUE_BOOL:
	  valueBox =
	    new Rts2NValueBoxBool (this, (Rts2ValueBool *) val, 21, s - 1);
	  break;
	case RTS2_VALUE_DOUBLE:
	  valueBox =
	    new Rts2NValueBoxDouble (this, (Rts2ValueDouble *) val, 21, s);
	  break;
	default:
	  break;
	}
    }
}

keyRet Rts2NDeviceWindow::injectKey (int key)
{
  keyRet
    ret;
  switch (key)
    {
    case KEY_F (6):
      if (valueBox)
	endValueBox ();
      createValueBox ();
      return RKEY_HANDLED;
    }
  if (valueBox)
    {
      ret = valueBox->injectKey (key);
      if (ret == RKEY_ENTER)
	{
	  if (getSelValue ())
	    valueBox->sendValue (connection);
	  endValueBox ();
	  return RKEY_HANDLED;
	}
      return ret;
    }
  return Rts2NSelWindow::injectKey (key);
}

void
Rts2NDeviceWindow::draw ()
{
  Rts2NWindow::draw ();
  werase (getWriteWindow ());
  drawValuesList ();
  printState ();
  Rts2Value *val = getSelValue ();
  if (val != NULL)
    printValueDesc (val);
  refresh ();
  if (valueBox)
    {
      valueBox->draw ();
    }
}
