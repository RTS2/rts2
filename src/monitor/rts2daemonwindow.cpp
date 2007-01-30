#include "rts2daemonwindow.h"

Rts2DaemonWindow::Rts2DaemonWindow (CDKSCREEN * cdkscreen)
{

}

Rts2DaemonWindow::~Rts2DaemonWindow (void)
{

}

Rts2DeviceWindow::Rts2DeviceWindow (CDKSCREEN * cdkscreen, Rts2Conn * in_connection):Rts2DaemonWindow
  (cdkscreen)
{
  valueList =
    newCDKAlphalist (cdkscreen, 20, 1, LINES - 22, COLS - 20,
		     "<C></B/24>Value list", NULL, NULL, 0, '_', A_REVERSE,
		     TRUE, FALSE);
  connection = in_connection;
  draw ();
}

Rts2DeviceWindow::~Rts2DeviceWindow ()
{
  destroyCDKAlphalist (valueList);
}

void
Rts2DeviceWindow::drawValuesList ()
{
  Rts2DevClient *client = connection->getOtherDevClient ();
  if (client)
    drawValuesList (client);
}

void
Rts2DeviceWindow::drawValuesList (Rts2DevClient * client)
{
  int i = 0;
  char *new_list[client->valueSize ()];
  for (std::vector < Rts2Value * >::iterator iter = client->valueBegin ();
       iter != client->valueEnd (); iter++, i++)
    {
      Rts2Value *val = *iter;
      asprintf (&new_list[i], "%-20s|%30s", val->getName ().c_str (),
		val->getValue ());
    }
  setCDKAlphalistContents (valueList, (char **) new_list,
			   client->valueSize ());
  for (i = 0; i < client->valueSize (); i++)
    {
      free (new_list[i]);
    }
}

char *
Rts2DeviceWindow::injectKey (int key)
{
  return injectCDKEntry (valueList->entryField, key);
}

void
Rts2DeviceWindow::draw ()
{
  drawValuesList ();
  drawCDKAlphalist (valueList, TRUE);
}
