#ifndef __RTS2_APP_DB_IMAGE__
#define __RTS2_APP_DB_IMAGE__

#include "rts2db/appdb.h"
#include "imagedb.h"

#include <list>

namespace rts2image
{

class AppDbImage:public rts2db::AppDb
{
	public:
		AppDbImage (int in_argc, char **in_argv, bool in_readOnly):rts2db::AppDb (in_argc, in_argv)
		{
			readOnly = in_readOnly;
		}
		virtual ~ AppDbImage (void)
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
				try
				{
					rts2image::ImageDb *imagedb = new rts2image::ImageDb ();
					imagedb->openFile (an_name, readOnly, false);
					rts2image::ImageDb *image = getValueImageType (imagedb);
					ret = processImage (image);
					delete image;
				}
				catch (rts2core::Error err)
				{
					std::cerr << err << std::endl;
					ret = -1;
				}
				if (ret)
					return ret;
			}
			return ret;
		}

	protected:
		std::list < const char *>imageNames;
		bool readOnly;
		virtual int processImage (__attribute__ ((unused)) rts2image::ImageDb * image)
		{
			return 0;
		}
};

}
#endif							 /* !__RTS2_APP_DB_IMAGE__ */
