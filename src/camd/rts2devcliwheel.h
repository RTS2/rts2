#ifndef __RTS2_DEVCLI_CAM_WHEEL__
#define __RTS2_DEVCLI_CAM_WHEEL__

#include "../utils/rts2devclient.h"

#define EVENT_FILTER_START_MOVE RTS2_LOCAL_EVENT + 650
#define EVENT_FILTER_MOVE_END RTS2_LOCAL_EVENT + 651
#define EVENT_FILTER_GET  RTS2_LOCAL_EVENT + 652

struct filterStart
{
	char *filterName;
	int filter;
};

class Rts2DevClientFilterCamera:public Rts2DevClientFilter
{
	protected:
		virtual void filterMoveEnd ();
	public:
		Rts2DevClientFilterCamera (Rts2Conn * conn);
		virtual ~ Rts2DevClientFilterCamera (void);
		virtual void filterMoveFailed (int status);
		virtual void postEvent (Rts2Event * event);
};
#endif							 /* !__RTS2_DEVCLI_CAM_WHEEL__ */
