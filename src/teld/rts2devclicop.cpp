#include "rts2devclicop.h"
#include "../utils/rts2command.h"

#include <math.h>
#include <libnova/libnova.h>

Rts2DevClientCopulaTeld::Rts2DevClientCopulaTeld (Rts2Conn * conn):Rts2DevClientCopula
  (conn)
{
}

Rts2DevClientCopulaTeld::~Rts2DevClientCopulaTeld (void)
{
  getMaster ()->postEvent (new Rts2Event (EVENT_COP_SYNCED));
}

void
Rts2DevClientCopulaTeld::syncEnded ()
{
  getMaster ()->postEvent (new Rts2Event (EVENT_COP_SYNCED));
  Rts2DevClientCopula::syncEnded ();
}

void
Rts2DevClientCopulaTeld::syncFailed (int status)
{
  getMaster ()->postEvent (new Rts2Event (EVENT_COP_SYNCED));
  Rts2DevClientCopula::syncFailed (status);
}

void
Rts2DevClientCopulaTeld::postEvent (Rts2Event * event)
{
  struct ln_equ_posn *dome_position;
  switch (event->getType ())
    {
    case EVENT_COP_START_SYNC:
      dome_position = (struct ln_equ_posn *) event->getArg ();
      connection->
	queCommand (new
		    Rts2CommandCopulaMove (this, dome_position->ra,
					   dome_position->dec));
      dome_position->ra = nan ("f");
      break;
    }
  Rts2DevClientCopula::postEvent (event);
}
