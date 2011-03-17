/**
 * Apogee filter wheel (USB based) driver.
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "filterd.h"
#include "ApnFilterWheel.h"

namespace rts2filterd
{

class Alta:public Filterd
{
	public:
		Alta (int in_argc, char **in_argv);
		virtual ~ Alta (void);
	protected:
		virtual int processOption (int in_opt);
		virtual int init (void);

		virtual int getFilterNum (void);
		virtual int setFilterNum (int new_filter);

		virtual int homeFilter ();

	private:
		CApnFilterWheel *dev;

		unsigned long filter_count;
		Apn_Filter filterType;
};

}

using namespace rts2filterd;

Alta::Alta (int in_argc, char **in_argv):Filterd (in_argc, in_argv)
{
  	dev = NULL;
	filterType = Apn_Filter_Unknown;

	addOption ('t', NULL, 1, "APN filter type (see Apogee.h for list; 0-5)");
}

Alta::~Alta (void)
{
	if (dev)
	{
		dev->Close ();
		delete dev;
	}
}

int Alta::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 't':
			filterType = atoi (optarg);
			break;
		default:
			return Filterd::processOption (in_opt);
	}
	return 0;
}

int Alta::init (void)
{
	int ret;
	ret = Filterd::init ();
	if (ret)
		return ret;
	
	dev = new CApnFilterWheel ();
	ret = dev->Init (filterType, 1);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "cannot init filter wheel driver" << sendLog;
		return -1;
	}
	ret = dev->GetMaxPositions (&filter_count);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "cannot retrieve maximal numer of filter wheel positions" << sendLog;
		return -1;
	}
	return 0;
}

int Alta::getFilterNum (void)
{
	bool ret;
	unsigned long f;
	ret = dev->GetPosition (&f);
	f--;
	if (!ret)
		return -1;
	return (int) f;
}

int Alta::setFilterNum (int new_filter)
{
	bool ret;
	if (new_filter < -1 || new_filter >= filter_count)
		return -1;

	ret = dev->SetPosition (new_filter+1);
	if (!ret)
		return -1;
	return Filterd::setFilterNum (new_filter);
}

int Alta::homeFilter ()
{
	return setFilterNum (1);
}

int main (int argc, char **argv)
{
	Alta device = Alta (argc, argv);
	return device.run ();
}
