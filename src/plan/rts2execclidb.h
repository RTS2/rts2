#ifndef __RTS2_EXECCLIDB__
#define __RTS2_EXECCLIDB__

#include "rts2execcli.h"

class Rts2DevClientCameraExecDb:public Rts2DevClientCameraExec
{
	protected:
		virtual void exposureStarted ();
	public:
		Rts2DevClientCameraExecDb (Rts2Conn * in_connection);
		virtual ~ Rts2DevClientCameraExecDb (void);
		virtual Rts2Image *createImage (const struct timeval *expStart);
		virtual void beforeProcess (Rts2Image * image);
};
#endif							 /* !__RTS2_EXECCLIDB__ */
