#include "rts2devcliwheel.h"
#include "../utils/rts2command.h"

Rts2DevClientFilterCamera::Rts2DevClientFilterCamera (Rts2Conn * conn):Rts2DevClientFilter
  (conn)
{
}

Rts2DevClientFilterCamera::~Rts2DevClientFilterCamera (void)
{
  getMaster ()->postEvent (new Rts2Event (EVENT_FILTER_MOVE_END));
}

void
Rts2DevClientFilterCamera::filterMoveEnd ()
{
  getMaster ()->postEvent (new Rts2Event (EVENT_FILTER_MOVE_END));
  Rts2DevClientFilter::filterMoveEnd ();
}

void
Rts2DevClientFilterCamera::filterMoveFailed (int status)
{
  getMaster ()->postEvent (new Rts2Event (EVENT_FILTER_MOVE_END));
  Rts2DevClientFilter::filterMoveFailed (status);
}

void
Rts2DevClientFilterCamera::postEvent (Rts2Event * event)
{
  struct filterStart *fs;
  switch (event->getType ())
    {
    case EVENT_FILTER_START_MOVE:
      fs = (filterStart *) event->getArg ();
      if (!strcmp (getName (), fs->filterName) && fs->filter >= 0)
	{
	  connection->queCommand (new Rts2CommandFilter (this, fs->filter));
	  fs->filter = -1;
	}
      break;
    case EVENT_FILTER_GET:
      fs = (filterStart *) event->getArg ();
      if (!strcmp (getName (), fs->filterName))
	fs->filter = getValueInteger ("filter");
      break;
    }
  Rts2DevClientFilter::postEvent (event);
}
