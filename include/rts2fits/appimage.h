#ifndef __RTS2_APP_IMAGE__
#define __RTS2_APP_IMAGE__

#include "cliapp.h"
#include "image.h"

#include <list>

namespace rts2image
{

class AppImageCore:public rts2core::CliApp
{
	public:
		AppImageCore (int in_argc, char **in_argv, bool in_readOnly):rts2core::CliApp (in_argc, in_argv)
		{
			readOnly = in_readOnly;
		}
		virtual ~ AppImageCore (void)
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
			for (img_iter = imageNames.begin (); img_iter != imageNames.end (); img_iter++)
			{
				const char *an_name = *img_iter;
				Image *image = new Image ();
				image->openFile (an_name, readOnly, false);
				ret = processImage (image);
				delete image;
				if (ret)
					return ret;
			}
			return ret;
		}

	protected:
		std::list < const char *>imageNames;
		bool readOnly;
		virtual int processImage (Image * image) { return 0; }
};

}

#endif							 /* !__RTS2_APP_IMAGE__ */
