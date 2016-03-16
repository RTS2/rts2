#ifndef __RTS2_CONNTCSNG__
#define __RTS2_CONNTCSNG__

#include "connection/tcp.h"

#define NGMAXSIZE    200

#define TCSNG_RA_AZ          0x01
#define TCSNG_DEC_AL         0x02
#define TCSNG_FOCUS          0x04
#define TCSNG_DOME           0x08

namespace rts2core
{

class ConnTCSNG:public ConnTCP
{
	public:
		ConnTCSNG (rts2core::Block *_master, const char *_hostname, int _port, const char *_obsID, const char *_subID);

		/**
		 * Request response from TCSNG. Returns pointer to buffer. Throws connection error if request cannot be made.
		 */
		const char * runCommand (const char *cmd, const char *req);
		const char * request (const char *val);
		const char * command (const char *cmd);

		/**
		 * Returns sexadecimal value from telescope control system. Sexadecimal is packet as hhmmss.SS.
		 */
		double getSexadecimalHours (const char *req);
		double getSexadecimalTime (const char *req);
		double getSexadecimalAngle (const char *req);

		int getReqCount () { return reqCount; }

	private:
		const char *obsID;   // observatory ID
		const char *subID;   // subsystem ID

		char ngbuf[NGMAXSIZE];

		int reqCount;
};

}

#endif //!__RTS2_CONNTCSNG__
