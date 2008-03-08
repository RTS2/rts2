#ifndef __RTS2__SKYBOTTARGET__
#define __RTS2__SKYBOTTARGET__

#include <string>
#include <libnova/libnova.h>
#include <list>

#include "../../utilsdb/target.h"

/*!
 * Holds information gathered from Simbad about target with given name
 * or around given location.
 */

class Rts2SkyBoTTarget:public EllTarget
{
	public:
		Rts2SkyBoTTarget (const char *in_name);

		virtual int load ();
};
#endif							 /* !__RTS2__SKYBOTTARGET__ */
