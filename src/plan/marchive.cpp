#include "../writers/rts2appdbimage.h"
#include "../writers/rts2imagedb.h"
#include "../utils/rts2config.h"

#include <iostream>

#include <list>

class Rts2MoveArchive:public Rts2AppDbImage
{
	protected:
		virtual int processImage (Rts2ImageDb * image)
		{
			int ret;
			double val;
			std::cout << "Processing " << image->getFileName () << "..";
			ret = image->getValue ("CRVAL1", val);
			if (!ret)
			{
				ret = image->toArchive ();
				std::cout << (ret ? "failed (not archive?)" : "archive") << std::endl;
			}
			else
			{
				ret = image->toTrash ();
				std::cout << (ret ? "failed (cannot move?)" : "trash") << std::endl;
			}
			return 0;
		}
	public:
		Rts2MoveArchive (int in_argc, char **in_argv):Rts2AppDbImage (in_argc,
			in_argv, true)
		{
			Rts2Config *config;
			config = Rts2Config::instance ();
		}
};

int
main (int argc, char **argv)
{
	Rts2MoveArchive app = Rts2MoveArchive (argc, argv);
	return app.run ();
}
