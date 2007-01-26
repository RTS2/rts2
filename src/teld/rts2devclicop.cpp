#include "rts2devclicop.h"
#include "../utils/rts2command.h"

#include <math.h>
#include <libnova/libnova.h>

Rts2DevClientCupolaTeld::Rts2DevClientCupolaTeld (Rts2Conn * conn):Rts2DevClientCupola
  (conn)
{
}

Rts2DevClientCupolaTeld::~Rts2DevClientCupolaTeld (void)
{
  getMaster ()->postEvent (new Rts2Event (EVENT_COP_SYNCED));
}

void
Rts2DevClientCupolaTeld::syncEnded ()
{
  getMaster ()->postEvent (new Rts2Event (EVENT_COP_SYNCED));
  Rts2DevClientCupola::syncEnded ();
}

void
Rts2DevClientCupolaTeld::syncFailed (int status)
{
  getMaster ()->postEvent (new Rts2Event (EVENT_COP_SYNCED));
  Rts2DevClientCupola::syncFailed (status);
}

void
Rts2DevClientCupolaTeld::postEvent (Rts2Event * event)
{
  struct ln_equ_posn *dome_position;
  switch (event->getType ())
    {
    case EVENT_COP_START_SYNC:
      dome_position = (struct ln_equ_posn *) event->getArg ();
      connection->
	queCommand (new
		    Rts2CommandCupolaMove (this, dome_position->ra,
					   dome_position->dec));
      dome_position->ra = nan ("f");
      break;
    }
  Rts2DevClientCupola::postEvent (event);
}
