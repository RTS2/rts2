#include "rts2devclifocuser.h"
#include "../utils/rts2command.h"

Rts2DevClientFocusCamera::Rts2DevClientFocusCamera (Rts2Conn * in_connection):Rts2DevClientFocus
(in_connection)
{
}


Rts2DevClientFocusCamera::~Rts2DevClientFocusCamera (void)
{
}


void
Rts2DevClientFocusCamera::postEvent (Rts2Event * event)
{
	struct focuserMove *fm;
	fm = (focuserMove *) event->getArg ();
	switch (event->getType ())
	{
		case EVENT_FOCUSER_SET:
		case EVENT_FOCUSER_STEP:
			if (!strcmp (getName (), fm->focuserName))
			{
				if (event->getType () == EVENT_FOCUSER_SET)
					connection->
						queCommand (new Rts2CommandSetFocus (this, fm->value));
				else
					connection->
						queCommand (new Rts2CommandChangeFocus (this, fm->value));
				// we process message
				fm->focuserName = NULL;
			}
			break;
		case EVENT_FOCUSER_GET:
			if (!strcmp (getName (), fm->focuserName))
			{
				fm->value = getConnection ()->getValueInteger ("FOC_POS");
				fm->focuserName = NULL;
			}
			break;
	}
	Rts2DevClientFocus::postEvent (event);
}


void
Rts2DevClientFocusCamera::focusingEnd ()
{
	getMaster ()->postEvent (new Rts2Event (EVENT_FOCUSER_END_MOVE));
	Rts2DevClientFocus::focusingEnd ();
}


void
Rts2DevClientFocusCamera::focusingFailed (int status)
{
	getMaster ()->postEvent (new Rts2Event (EVENT_FOCUSER_END_MOVE));
	Rts2DevClientFocus::focusingFailed (status);
}
