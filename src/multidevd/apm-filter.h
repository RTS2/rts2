/* 
 * APM filter wheel (ESA TestBed telescope).
 * Copyright (C) 2014 - 2015 Stanislav Vitek <standa@vitkovi.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __RTS2_APM_FILTER__
#define __RTS2_APM_FILTER__

#include "filterd.h"
#include "apm-multidev.h"

namespace rts2filterd
{

/**
 * Class for APM filter wheel.
 *
 * @author Standa Vitek <standa@vitkovi.net>
 */
class APMFilter : public Filterd, rts2multidev::APMMultidev
{
        public:
                APMFilter (const char *name, rts2core::ConnAPM *conn);

	protected:
                virtual int getFilterNum (void);
                virtual int setFilterNum (int new_filter);
                virtual int homeFilter ();

        private:
                int sendUDPMessage (const char * in_message);
                int filterNum;
                int filterSleep;
};

}

#endif // !__RTS2_APM_FILTER__
