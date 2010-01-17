/* 
 * Dome door driver for Obs. Vermes.
 * Copyright (C) 2010 Markus Wildi
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "dome.h"
#include "vermes.h" 
#include "door_vermes.h" 
#include "move-door_vermes.h"
#include "ssd650v_comm_vermes.h"

namespace rts2dome
{

/**
 * Class to control of Obs. Vermes cupola door. 
 *
 * @author Markus Wildi <markus.wildi@one-arcsec.org>
 */
class DoorVermes: public Dome
{
	private:
		Rts2ValueBool *swOpenLeft;
		Rts2ValueBool *swCloseLeft;

		Rts2ValueBool *raining;

		/**
		 * Update status of end sensors.
		 *
		 * @return -1 on failure, 0 on success.
		 */
		int updateStatus ();

		/**
		 * Pull on roof trigger.
		 *
		 * @return -1 on failure, 0 on success.
		 */
		int roofChange ();

	protected:
		virtual int processOption (int _opt);
		virtual int init ();
		virtual int info ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

		virtual bool isGoodWeather ();

	public:
		DoorVermes (int argc, char **argv);
		virtual ~DoorVermes ();
};

}

using namespace rts2dome;



int DoorVermes::updateStatus ()
{
  logStream (MESSAGE_DEBUG) << "DoorVermes::updateStatus"<< sendLog ;
	swOpenLeft->setValueBool (false);
	swCloseLeft->setValueBool (false);
	return 0;
}


int
DoorVermes::roofChange ()
{
  logStream (MESSAGE_DEBUG) << "DoorVermes::roofChange"<< sendLog ;
  usleep (100); // wildi ToDo
	return 0;
}


int
DoorVermes::processOption (int _opt)
{
	switch (_opt)
	{
// 		case 'c':
// 			comediFile = optarg;
// 			break;
		default:
			return Dome::processOption (_opt);
	}
	return 0;
}


int
DoorVermes::init ()
{
	int ret;
	ret = Dome::init ();
	if (ret)
		return ret;

// 	comediDevice = comedi_open (comediFile);
// 	if (comediDevice == NULL)
// 	{
// 		logStream (MESSAGE_ERROR) << "Cannot open comedi port" << comediFile << sendLog;
// 		return -1;
// 	}


	return 0;
}


int
DoorVermes::info ()
{
  logStream (MESSAGE_DEBUG) << "DoorVermes::info"<< sendLog ;
	int ret;
	ret = updateStatus ();
	if (ret)
		return -1;

	if (swCloseLeft->getValueBool())
		setState (DOME_CLOSED, "Dome is closed");
	else if (swOpenLeft->getValueBool ())
	  	setState (DOME_OPENED, "Dome is opened");

	return Dome::info ();
}


int
DoorVermes::startOpen ()
{
  logStream (MESSAGE_DEBUG) << "DoorVermes::startOpen"<< sendLog ;

	if (!isGoodWeather ())
		return -1;

	return roofChange ();
}


long
DoorVermes::isOpened ()
{
  logStream (MESSAGE_DEBUG) << "DoorVermes::isOpened"<< sendLog ;

	int ret;
	ret = updateStatus ();
	if (ret)
		return -1;
	if (swOpenLeft->getValueBool())
		return USEC_SEC;
	return -2;
}


int
DoorVermes::endOpen ()
{
  logStream (MESSAGE_DEBUG) << "DoorVermes::endOpen"<< sendLog ;

	return 0;
}


int
DoorVermes::startClose ()
{
  logStream (MESSAGE_DEBUG) << "DoorVermes::startClose"<< sendLog ;

	int ret;
	ret = updateStatus ();
	if (ret)
		return -1;
	
	swCloseLeft->setValueBool(true) ; // wildi ToDo: fake
	if (swCloseLeft->getValueBool())
		return -1;
	
	return roofChange ();
}


long
DoorVermes::isClosed ()
{
  logStream (MESSAGE_DEBUG) << "DoorVermes::isClosed"<< sendLog ;

	int ret;
	ret = updateStatus ();
	if (ret)
		return -1;
	if (swCloseLeft->getValueBool ())
		return USEC_SEC;
	return -2;
}


int
DoorVermes::endClose ()
{
  logStream (MESSAGE_DEBUG) << "DoorVermes::endClose"<< sendLog ;

	return 0;
}


bool
DoorVermes::isGoodWeather ()
{
	if (getIgnoreMeteo () == true)
		return true;
	int ret = updateStatus ();
	if (ret)
		return false;
	// if it's raining
	if (raining->getValueBool () == true)
		return false;
	return Dome::isGoodWeather ();
}


DoorVermes::DoorVermes (int argc, char **argv): Dome (argc, argv)
{
  //	comediFile = "/dev/comedi0";

	createValue (swOpenLeft, "sw_open_left", "left open end switch", false);
	createValue (swCloseLeft, "sw_close_left", "left close end switch", false);

	createValue (raining, "RAIN", "if it's raining", true);

	raining->setValueBool (false);

	addOption ('c', NULL, 1, "path to comedi device");
}


DoorVermes::~DoorVermes ()
{

}


int
main (int argc, char **argv)
{
	DoorVermes device = DoorVermes (argc, argv);
	return device.run ();
}
