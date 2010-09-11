#ifndef __RTS2_PROP__
#define __RTS2_PROP__

#include "target.h"
#include "planset.h"

#include <iostream>

namespace rts2db
{

class Rts2Prop
{
	public:
		Rts2Prop ();
		Rts2Prop (int in_prop_id);
		virtual ~ Rts2Prop (void);

		int load ();
		int save ();

		Target *getTarget ();
		PlanSet *getPlanSet ();

		friend std::ostream & operator << (std::ostream & _os, Rts2Prop prop) { return _os; }

	private:
		int prop_id;
		int tar_id;
		int user_id;

		Target *target;
		PlanSet *planSet;
		void init ();
};

}

#endif							 /* !__RTS2_PROP__ */
