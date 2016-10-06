#include "mirror.h"
#include <iostream>

namespace rts2mirror
{

class Dummy:public Mirror
{
	public:
		Dummy (int in_argc, char **in_argv):Mirror (in_argc, in_argv)
		{
			addPosition ("A");
			addPosition ("B");
			addPosition ("C");
		}
		virtual int movePosition (int pos)
		{
			return 0;
		}
		virtual int isMoving ()
		{
			return -2;
		}
};

}

using namespace rts2mirror;

int main (int argc, char **argv)
{
	Dummy device = Dummy (argc, argv);
	return device.run ();
}
