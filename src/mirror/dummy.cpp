#include "mirror.h"
#include <iostream>

class Rts2DevMirrorDummy:public Rts2DevMirror
{
	public:
		Rts2DevMirrorDummy (int in_argc, char **in_argv):Rts2DevMirror (in_argc,
			in_argv)
		{
		}
		virtual int endOpen ()
		{
			sleep (10);
			return Rts2DevMirror::endOpen ();
		}
		virtual int isOpened ()
		{
			return -2;
		}
		virtual int isClosed ()
		{
			return -2;
		}
		virtual int endClose ()
		{
			sleep (10);
			return Rts2DevMirror::endClose ();
		}
};

int
main (int argc, char **argv)
{
	Rts2DevMirrorDummy device = Rts2DevMirrorDummy (argc, argv);
	return device.run ();
}
