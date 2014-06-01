/*!
 * @file Photometer deamon.
 *
 * @author petr
 */
#define FILTER_STEP  33

#include "phot.h"

#include <time.h>

using namespace rts2phot;

class Dummy:public Photometer
{
	public:
		Dummy (int argc, char **argv);

		virtual int scriptEnds ();

		virtual long getCount ();

		virtual int init ()
		{
			photType = "Dummy";
			return Photometer::init ();
		}

		virtual int homeFilter ();
		virtual int startFilterMove (int new_filter);
		virtual long isFilterMoving ();
		virtual int enableMove ();
		virtual int disableMove ();
	protected:
		virtual int startIntegrate ();
	private:
		int filterCount;
};

Dummy::Dummy (int in_argc, char **in_argv):Photometer (in_argc, in_argv)
{
}

int Dummy::scriptEnds ()
{
	startFilterMove (0);
	return Photometer::scriptEnds ();
}

long Dummy::getCount ()
{
	sendCount (random (), req_time, false);
	return (long (req_time * USEC_SEC));
}

int Dummy::homeFilter ()
{
	return 0;
}

int Dummy::startIntegrate ()
{
	return 0;
}

int Dummy::startFilterMove (int new_filter)
{
	filter->setValueInteger (new_filter);
	filterCount = 10;
	return Photometer::startFilterMove (new_filter);
}

long Dummy::isFilterMoving ()
{
	if (filterCount <= 0)
		return -2;
	filterCount--;
	return USEC_SEC / 10;
}

int Dummy::enableMove ()
{
	return 0;
}

int Dummy::disableMove ()
{
	return 0;
}

int main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}
