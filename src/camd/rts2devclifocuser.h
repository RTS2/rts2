#ifndef __RTS2_DEVCLI_FOCUSER__
#define __RTS2_DEVCLI_FOCUSER__

#define EVENT_FOCUSER_SET RTS2_LOCAL_EVENT + 750
#define EVENT_FOCUSER_STEP  RTS2_LOCAL_EVENT + 751
#define EVENT_FOCUSER_END_MOVE  RTS2_LOCAL_EVENT + 752
#define EVENT_FOCUSER_GET RTS2_LOCAL_EVENT + 753

#include "../utils/rts2devclient.h"

struct focuserMove
{
	char *focuserName;
	int value;
};

class Rts2DevClientFocusCamera:public Rts2DevClientFocus
{
	protected:
		virtual void focusingEnd ();
	public:
		Rts2DevClientFocusCamera (Rts2Conn * in_connection);
		virtual ~ Rts2DevClientFocusCamera (void);
		virtual void postEvent (Rts2Event * event);
		virtual void focusingFailed (int status);
};
#endif							 /*! __RTS2_DEVCLI_FOCUSER__ */
