#ifndef __RTS2__SIMBADTARGET__
#define __RTS2__SIMBADTARGET__

#include <string>
#include <libnova/libnova.h>
#include <list>

#include "../../utilsdb/target.h"

/*!
 * Holds information gathered from Simbad about target with given name
 * or around given location.
 */

class Rts2SimbadTarget:public ConstTarget
{
	private:
		// target name
		std::list < std::string > aliases;
		std::string references;
		std::string simbadType;
		struct ln_equ_posn propMotions;
		float simbadBMag;
	public:
		Rts2SimbadTarget (const char *in_name);

		virtual int load ();

		int getPosition (struct ln_equ_posn *pos)
		{
			return Target::getPosition (pos);
		}

		virtual void printExtra (std::ostream & _os);

		std::list < std::string > getAliases ()
		{
			return aliases;
		}
};
#endif							 /* !__RTS2__SIMBADTARGET__ */
