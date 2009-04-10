/* 
 * Driver for Hlohovec (Slovakia) 50cm telescope.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "tgdrive.h"
#include "telescope.h"

#define OPT_RA            OPT_LOCALHOST + 2201
#define OPT_DEC           OPT_LOCALHOST + 2202

using namespace rts2teld;

namespace rts2teld
{

/**
 * Class for Hlohovec (50cm) telescope.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Hlohovec:public Telescope
{
	private:
		TGDrive *raDrive;
		TGDrive *decDrive;

		const char *devRA;
		const char *devDEC;

		Rts2ValueInteger *ra_rPos;
		Rts2ValueInteger *ra_dPos;
		Rts2ValueInteger *dec_rPos;
		Rts2ValueInteger *dec_dPos;
	protected:
		virtual void usage ();
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();

		virtual int startMove ();
		virtual int stopMove ();
		virtual int endMove ();
		virtual int startPark ();
		virtual int endPark ();

		virtual int setValue (Rts2Value *old_value, Rts2Value *new_value);
		
	public:
		Hlohovec (int argc, char **argv);
		virtual ~Hlohovec ();
};

}


void
Hlohovec::usage ()
{
	std::cout << "\t" << getAppName () << " --ra /dev/ttyS0 --dec /dev/ttyS1" << std::endl;
}


int
Hlohovec::processOption (int opt)
{
	switch (opt)
	{
		case OPT_RA:
			devRA = optarg;
			break;
		case OPT_DEC:
			devDEC = optarg;
			break;
		default:
			return Telescope::processOption (opt);
	}
	return 0;
}


int
Hlohovec::init ()
{
	int ret;
	ret = Telescope::init ();
	if (ret)
		return ret;

	if (devRA == NULL)
	{
		logStream (MESSAGE_ERROR) << "RA device file was not specified." << sendLog;
		return -1;
	}

	if (devDEC == NULL)
	{
		logStream (MESSAGE_ERROR) << "DEC device file was not specified." << sendLog;
	}

	raDrive = new TGDrive (devRA, this);
	raDrive->setDebug ();
	raDrive->setLogAsHex ();
	ret = raDrive->init ();
	if (ret)
		return ret;

	raDrive->write2b (0x1ED, 2);

	if (devDEC != NULL)
	{
		decDrive = new TGDrive (devDEC, this);
		decDrive->setDebug ();
		decDrive->setLogAsHex ();
		ret = decDrive->init ();
		if (ret)
			return ret;
	}

	return 0;
}


int
Hlohovec::info ()
{
	ra_dPos->setValueInteger (raDrive->read4b (TGA_TARPOS));
	ra_rPos->setValueInteger (raDrive->read4b (TGA_CURRPOS));

	if (decDrive)
	{
		dec_dPos->setValueInteger (decDrive->read4b (TGA_TARPOS));
		dec_rPos->setValueInteger (decDrive->read4b (TGA_CURRPOS));
	}

	return Telescope::info ();
}


int
Hlohovec::startMove ()
{
	return 0;
}


int
Hlohovec::stopMove ()
{
	return 0;
}


int
Hlohovec::endMove ()
{
	return 0;
}


int
Hlohovec::startPark ()
{
	return 0;
}


int
Hlohovec::endPark ()
{
	return 0;
}


int
Hlohovec::setValue (Rts2Value *old_value, Rts2Value *new_value)
{
	if (old_value == ra_dPos)
	{
		try
		{
			raDrive->write4b (TGA_TARPOS, new_value->getValueInteger ());
			return 0;
		}
		catch (TGDriveError e)
		{
			return -2;
		}
	}
	return Telescope::setValue (old_value, new_value);
}


Hlohovec::Hlohovec (int argc, char **argv):Telescope (argc, argv)
{
	raDrive = NULL;
	decDrive = NULL;

	devRA = NULL;
	devDEC = NULL;

	createValue (ra_dPos, "AX_RA_T", "Target RA position", true);
	createValue (ra_rPos, "AX_RA_C", "Current RA position", true);
	createValue (dec_rPos, "AX_DEC_T", "Target DEC position", true);
	createValue (dec_dPos, "AX_DEC_C", "Current DEC position", true);

	addOption (OPT_RA, "ra", 1, "RA drive serial device");
	addOption (OPT_DEC, "dec", 1, "DEC drive serial device");
}


Hlohovec::~Hlohovec ()
{
	delete raDrive;
	delete decDrive;
}


int
main (int argc, char **argv)
{
	Hlohovec device = Hlohovec (argc, argv);
	return device.run ();
}
