#ifndef __RTS2_APM_MIRROR__
#define __RTS2_APM_MIRROR__

#include "sensord.h"
#include "apm-filter.h"

#define MIRROR_OPENED	5
#define MIRROR_CLOSED	6
#define MIRROR_OPENING	7
#define MIRROR_CLOSING	8

namespace rts2sensord
{

/**
 * APM mirror cover driver.
 *
 * @author Stanislav Vitek <standa@vitkovi.net>
 */
class APMMirror : public Sensor
{
	public:
		APMMirror (int argc, char **argv, const char *sn, rts2filterd::APMFilter *in_filter);
		virtual int initHardware ();
		virtual int commandAuthorized (rts2core::Connection *conn);
		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);
		virtual int info ();
	protected:
		virtual int processOption (int in_opt);
	private:
		int mirror_state;
                rts2core::ConnAPM *connMirror;
		rts2filterd::APMFilter *filter;
                rts2core::ValueString *mirror;
                int open ();
                int close ();
                int sendUDPMessage (const char * _message);
};

}

#endif // __RTS2_APM_MIRROR__
