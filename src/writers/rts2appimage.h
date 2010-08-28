#ifndef __RTS2_APP_IMAGE__
#define __RTS2_APP_IMAGE__

#include "../utils/rts2cliapp.h"
#include "rts2image.h"

#include <list>

class Rts2AppImage:public Rts2CliApp
{
	protected:
		std::list < const char *>imageNames;
		bool readOnly;
		virtual int processImage (Rts2Image * image) { return 0; }
	public:
		Rts2AppImage (int in_argc, char **in_argv, bool in_readOnly):Rts2CliApp (in_argc, in_argv)
		{
			readOnly = in_readOnly;
		}
		virtual ~ Rts2AppImage (void)
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
				Rts2Image *image = new Rts2Image ();
				image->openImage (an_name, false, readOnly);
				ret = processImage (image);
				delete image;
				if (ret)
					return ret;
			}
			return ret;
		}
};
#endif							 /* !__RTS2_APP_IMAGE__ */
