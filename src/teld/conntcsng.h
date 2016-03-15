#ifndef __RTS2_CONNTCSNG__
#define __RTS2_CONNTCSNG__

#include "connection/tcp.h"

#define NGMAXSIZE    200

namespace rts2teld
{

class ConnTCSNG:public rts2core::ConnTCP
{
	public:
		ConnTCSNG (rts2core::Block *_master, const char *_hostname, int _port, const char *_obsID, const char *_subID);

		/**
		 * Request response from TCSNG. Returns pointer to buffer. Throws connection error if request cannot be made.
		 */
		const char * request (const char *req, int refNum);

		/**
		 * Returns sexadecimal value from telescope control system. Sexadecimal is packet as hhmmss.SS.
		 */
		double getSexadecimal (const char *req, int refNum);

	private:
		const char *obsID;   // observatory ID
		const char *subID;   // subsystem ID

		char ngbuf[NGMAXSIZE];
};

}

#endif //!__RTS2_CONNTCSNG__
