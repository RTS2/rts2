#ifndef __RTS2_APM_RELAY__
#define __RTS2_APM_RELAY__

/**
 * Driver for APM relays
 * Copyright (C) 2015 Stanislav Vitek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sensord.h"
#include "apm-filter.h"

#define RELAY1_OFF      1
#define RELAY1_ON       2
#define RELAY2_OFF      3
#define RELAY2_ON       4

namespace rts2sensord
{

/**
 * Driver for APM relays
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
