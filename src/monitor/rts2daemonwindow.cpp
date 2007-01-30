#include "rts2daemonwindow.h"

Rts2DaemonWindow::Rts2DaemonWindow (CDKSCREEN * cdkscreen)
{

}


Rts2DeviceWindow::Rts2DeviceWindow (CDKSCREEN * cdkscreen)
{
  valueList =
    newCDKAlphalist (cdkscreen, 20, 1, LINES - 22, COLS - 20,
		     "<C></B/24>Value list", NULL, NULL, 0, '_', A_REVERSE,
		     TRUE, FALSE);
  drawCDKAlphalist (valueList, TRUE);
}
