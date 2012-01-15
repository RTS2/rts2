/* 
 * Utility to remove images from RTS2 database.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "utilsfunc.h"
#include "../../lib/rts2db/appdb.h"
#include "../../lib/rts2fits/imagedb.h"

#include <iostream>
#include <list>

class Rts2DeleteApp: public rts2db::AppDb
{
	public:
		Rts2DeleteApp (int argc, char **argv);
		virtual ~Rts2DeleteApp (void);

	protected:
		virtual int processOption (int in_opt);
		virtual int processArgs (const char *in_arg);

		virtual int doProcessing ();

	private:
		int findImages ();
		int deleteImage (const char *in_name);

		int dont_delete;
		std::list <const char *> imageNames;
};

int Rts2DeleteApp::findImages ()
{
	// find images in db and run delete on them..
	return 0;
}

int Rts2DeleteApp::deleteImage (const char *in_name)
{
	int ret;
	rts2image::ImageDb *image;
	std::cout << "Delete image " << in_name << "..";
	if (dont_delete)
	{
		ret = 0;
	}
	else
	{
		image = new rts2image::ImageDb ();
		image->openFile (in_name);
		ret = image->deleteImage ();
		delete image;
	}
	if (ret)
	{
		std::cout << "error (" << errno << " - " << strerror (errno) << ")" << std::endl;
	}
	else
	{
		std::cout << "OK" << std::endl;
	}
	return ret;
}

Rts2DeleteApp::Rts2DeleteApp (int argc, char **argv) : rts2db::AppDb (argc, argv)
{
	addOption ('n', "notmod", 1, "don't delete anything, just show what will be done");
	dont_delete = 0;
}

Rts2DeleteApp::~Rts2DeleteApp (void)
{
	imageNames.clear ();
}

int Rts2DeleteApp::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'n':
			dont_delete = 1;
			break;
		default:
			return rts2db::AppDb::processOption (in_opt);
	}
	return 0;
}

int Rts2DeleteApp::processArgs (const char *in_arg)
{
	imageNames.push_back (in_arg);
	return 0;
}

int Rts2DeleteApp::doProcessing ()
{
	int ret;
	if (imageNames.size () != 0)
	{
		std::list <const char *>::iterator img_iter;
		ret = 0;
		for (img_iter = imageNames.begin (); img_iter != imageNames.end (); img_iter++)
		{
			const char *an_name = *img_iter;
			ret = deleteImage (an_name);
			if (ret)
				return ret;
		}
		return ret;
	}
	return findImages ();
}

int main (int argc, char **argv)
{
	Rts2DeleteApp app = Rts2DeleteApp (argc, argv);
	return app.run ();
}
