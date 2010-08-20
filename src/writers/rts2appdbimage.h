#ifndef __RTS2_APP_DB_IMAGE__
#define __RTS2_APP_DB_IMAGE__

#include "../utilsdb/rts2appdb.h"
#include "rts2imagedb.h"

#include <list>

class Rts2AppDbImage:public Rts2AppDb
{
	protected:
		std::list < const char *>imageNames;
		bool readOnly;
		virtual int processImage (Rts2ImageDb * image)
		{
			return 0;
		}
	public:
		Rts2AppDbImage (int in_argc, char **in_argv, bool in_readOnly):Rts2AppDb (in_argc, in_argv)
		{
			readOnly = in_readOnly;
		}
		virtual ~ Rts2AppDbImage (void)
		{
			imageNames.clear ();
		}
		virtual int processArgs (const char *in_arg)
		{
			imageNames.push_back (in_arg);
			return 0;
		}

		virtual int doProcessing ()
		{
			int ret = 0;
			std::list < const char *>::iterator img_iter;
			for (img_iter = imageNames.begin (); img_iter != imageNames.end ();
				img_iter++)
			{
				const char *an_name = *img_iter;
				Rts2ImageDb *image;
				try
				{
					Rts2ImageDb *imagedb = new Rts2ImageDb (an_name, false, readOnly);
					image = getValueImageType (imagedb);
				}
				catch (rts2core::Error err)
				{
					std::cerr << err << std::endl;
				}
				ret = processImage (image);
				delete image;
				if (ret)
					return ret;
			}
			return ret;
		}
};
#endif							 /* !__RTS2_APP_DB_IMAGE__ */
