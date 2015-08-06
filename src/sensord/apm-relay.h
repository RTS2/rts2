#ifndef __RTS2_APM_RELAY__
#define __RTS2_APM_RELAY__

#include "sensord.h"
#include "apm-filter.h"

#define RELAY1_OFF      1
#define RELAY1_ON       2
#define RELAY2_OFF      3
#define RELAY2_ON       4

namespace rts2sensord
{

/**
 * APM relays driver.
 *     
 * @author Stanislav Vitek <standa@vitkovi.net>
 */
class APMRelay : public Sensor
{
        public:
                APMRelay (int argc, char **argv, const char *sn, rts2filterd::APMFilter *in_filter);
                virtual int initHardware ();
                virtual int commandAuthorized (rts2core::Connection *conn);
                virtual int info ();

        protected:
                virtual int processOption (int in_opt);
        private:
                int relay1_state, relay2_state;
                rts2core::ConnAPM *connRelay;
                rts2filterd::APMFilter *filter;
                rts2core::ValueString *relay1, *relay2;
                int relayOn (int n);
                int relayOff (int n);
                int sendUDPMessage (const char * _message);
};

}

#endif // __RTS2_APM_RELAY__
