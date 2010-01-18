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


// WILDI MUST GO AWAY
int is_synced            = -1 ;   // ==SYNCED if target_az reached
int cupola_tracking_state= -1 ; 
int motor_on_off_state   = -1 ;
int barcodereader_state ;
double barcodereader_az ;
double barcodereader_dome_azimut_offset= -253.6 ; // wildi ToDo: make an option
double target_az ;
// WILDI MUST GO AWAY



#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>

#include "dome.h"
#include "vermes.h" 
#include "door_vermes.h" 
#include "move-door_vermes.h"
#include "ssd650v_comm_vermes.h"
#include "oak_comm_vermes.h"


int doorState;

namespace rts2dome {
/*
 * Class to control of Obs. Vermes cupola door. 
 *
 * @author Markus Wildi <markus.wildi@one-arcsec.org>
 */
    class DoorVermes: public Dome 
    {
    private:
	Rts2ValueString  *doorStateMessage;
	Rts2ValueBool *raining;


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


int
DoorVermes::processOption (int _opt)
{
    switch (_opt) {
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
    logStream (MESSAGE_ERROR) << "Connect to OAK" << sendLog;

    connectOakDiginDevice(OAKDIGIN_CONNECT) ;
    
    return 0;  
}

int
DoorVermes::info ()
{
    logStream (MESSAGE_DEBUG) << "DoorVermes::info"<< sendLog ;
    int ret;
    if (ret)
	return -1;

    switch (doorState){
	case  DOOR_CLOSED:
	    doorStateMessage->setValueString("door is closed") ;
	    break;
	case DOOR_OPEN:
	    doorStateMessage->setValueString("door is open") ;
	    break;
	case DOOR_CLOSING:
	    doorStateMessage->setValueString("door is closing") ;
	    break;
	case DOOR_OPENING:
	    doorStateMessage->setValueString("door is opening") ;
	    break;
	case DOOR_UNDEFINED:
	default:
	    doorStateMessage->setValueString("door state is undefined") ;
    }



    return Dome::info ();
}

int
DoorVermes::startOpen ()
{
    logStream (MESSAGE_DEBUG) << "DoorVermes::startOpen"<< sendLog ;
    
    if (!isGoodWeather ())
	return -1;

    return 0; // wildi ToDo check that
}

long
DoorVermes::isOpened ()
{
    logStream (MESSAGE_DEBUG) << "DoorVermes::isOpened"<< sendLog ;
    
    if ( doorState== DOOR_OPENING)
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
    
    doorStateMessage->setValueString("door is closing") ;
    if ( doorState== DOOR_CLOSING)
	return -1;
    return 0; // wildi ToDo check that
}

long
DoorVermes::isClosed ()
{
    logStream (MESSAGE_DEBUG) << "DoorVermes::isClosed"<< sendLog ;
    
    if ( doorState==DOOR_CLOSING)
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
    // if it's raining
    if (raining->getValueBool () == true)
	return false;
    return Dome::isGoodWeather ();
}

DoorVermes::DoorVermes (int argc, char **argv): Dome (argc, argv)
{
    //	comediFile = "/dev/comedi0";

    createValue (doorStateMessage, "DOORSTATE", "dor state messages as tect", false);

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
