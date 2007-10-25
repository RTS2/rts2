#ifndef __RTS2_PLANSET__
#define __RTS2_PLANSET__

#include "rts2plan.h"

#include <list>
#include <ostream>
#include <string>
#include <time.h>

class Rts2PlanSet:public
std::list <
Rts2Plan >
{
	private:
		void
			load (std::string in_where);

	public:
		Rts2PlanSet ();
		Rts2PlanSet (int prop_id);
		Rts2PlanSet (time_t * t_from, time_t * t_to);

		virtual ~
			Rts2PlanSet (void);

		friend
			std::ostream &
			operator << (std::ostream & _os, Rts2PlanSet & plan_set);
};

std::ostream & operator << (std::ostream & _os, Rts2PlanSet & plan_set);
#endif							 /* !__RTS2_PLANSET__ */
