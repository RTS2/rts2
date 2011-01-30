#ifndef __RTS2_MIRROR__
#define __RTS2_MIRROR__

#include "../utils/device.h"

class Rts2DevMirror:public rts2core::Device
{
	public:
		Rts2DevMirror (int argc, char **argv);
		virtual ~ Rts2DevMirror (void);
		virtual int idle ();

		virtual int startOpen ()
		{
			maskState (MIRROR_MASK, MIRROR_A_B, "moving A->B");
			return 0;
		}
		virtual int isOpened ()
		{
			return -1;
		}
		virtual int endOpen ()
		{
			maskState (MIRROR_MASK, MIRROR_B, "moved A->B");
			return 0;
		}

		virtual int startClose ()
		{
			maskState (MIRROR_MASK, MIRROR_B_A, "moving B->A");
			return 0;
		}
		virtual int isClosed ()
		{
			return -1;
		}
		virtual int endClose ()
		{
			maskState (MIRROR_MASK, MIRROR_A, "moved B->A");
			return 0;
		}

		// return 1, when mirror is (still) moving
		virtual int isMoving ()
		{
			return ((getState () & MIRROR_MASK) == MIRROR_A_B
				|| (getState () & MIRROR_MASK) == MIRROR_B_A);
		}

		virtual int ready ()
		{
			return -1;
		}

		int startOpen (Rts2Conn * conn);
		int startClose (Rts2Conn * conn);

		virtual int commandAuthorized (Rts2Conn * conn);
};
#endif							 /* ! __RTS2_MIRROR__ */
