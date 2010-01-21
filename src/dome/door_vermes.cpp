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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>
#include <stdlib.h>

#include "dome.h"
#include "vermes.h" 
#include "door_vermes.h" 
#include "move-door_vermes.h"
#include "ssd650v_comm_vermes.h"
#include "oak_comm_vermes.h"

extern int doorState;
extern int motor_on_off_state_door ;
extern int evt_door ;
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
	Rts2ValueBool *open_door;
	Rts2ValueBool *close_door;

	/**
	 * Update status of end sensors.
	 *
	 * @return -1 on failure, 0 on success.
	 */
	int updateDoorStatus ();
	/**
	 * Update status messages.
	 *
	 * @return 0 on success.
	 */
	int updateDoorStatusMessage ();

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
	virtual void valueChanged (Rts2Value * changed_value) ;
    };
}

using namespace rts2dome;

int DoorVermes::updateDoorStatus ()
{
    // if bad return -1 ;
    return 0 ;
}
int DoorVermes::updateDoorStatusMessage ()
{
    switch (doorState){
	case DS_UNDEF:
	    doorStateMessage->setValueString("undefined") ;
	    break;
	case DS_STOPPED_CLOSED:
	    doorStateMessage->setValueString("stopped, closed") ;
	    break;
	case DS_STOPPED_FULLY_OPEN:
	    doorStateMessage->setValueString("stopped, open") ;
	    break;
	case DS_STOPPED_PARTIALLY_OPEN:
	    doorStateMessage->setValueString("stopped, partially open") ;
	    break;
	case DS_START_OPEN:
	    doorStateMessage->setValueString("opening") ;
	    break;
	case DS_START_CLOSE:
	    doorStateMessage->setValueString("closing") ;
	    break;
	case DS_RUNNING_OPEN:
	    doorStateMessage->setValueString("opening") ;
	    break;
	case DS_RUNNING_CLOSE:
	    doorStateMessage->setValueString("closing") ;
	    break;
	case DS_STOPPING_OPENING:
	    doorStateMessage->setValueString("stopping opening") ;
	    break;
	case DS_STOPPING_CLOSING:
	    doorStateMessage->setValueString("stopping closing") ;
	    break;
	case DS_EMERGENCY_ENDSWITCH:
	    doorStateMessage->setValueString("hard end switches ON") ;
	    break;
	default:
	    doorStateMessage->setValueString("door state is undefined") ;
    }

    // if bad return -1 ;
    return 0 ;
}

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
void DoorVermes::valueChanged (Rts2Value * changed_value)
{
  if (changed_value == open_door) {
    if( open_door->getValueBool()) {
      logStream (MESSAGE_DEBUG) << "DorrVermes::valueChanged opening door" << sendLog ;
      evt_door= EVNT_DS_CMD_OPEN ;
    } else {
      logStream (MESSAGE_ERROR) << "DorrVermes::valueChanged use CLOSE_DOOR to close the door" << sendLog ;
    }
    return ;
  } else if (changed_value == close_door) {
    if( close_door->getValueBool()) {
      logStream (MESSAGE_DEBUG) << "DorrVermes::valueChanged closing door" << sendLog ;
      evt_door= EVNT_DS_CMD_CLOSE ;
    } else {
      logStream (MESSAGE_ERROR) << "DorrVermes::valueChanged use OPEN_DOOR to open the door" << sendLog ;
    }
    return ;
  }
  Dome::valueChanged (changed_value);
}

int
DoorVermes::init ()
{

    int ret;
    pthread_t  move_door_id;

    ret = Dome::init ();
    if (ret)
	return ret;

    logStream (MESSAGE_ERROR) << "Connect to SSD650v" << sendLog;
    // ssd650v frequency inverter
    if(connectSSD650vDevice(SSD650V_CMD_CONNECT)) {
	logStream (MESSAGE_ERROR) << "Vermes::initValues a general failure on SSD650V connection occured" << sendLog ;
    }
    if(( ret=motor_off()) != SSD650V_MS_STOPPED ) {
	fprintf( stderr, "DoorVermes::init something went wrong with SSD650V (OFF)\n") ;
	motor_on_off_state_door= SSD650V_MS_UNDEFINED ;
    } else {
	motor_on_off_state_door= SSD650V_MS_STOPPED ;
    }
    logStream (MESSAGE_ERROR) << "Connect to OAK" << sendLog;
    // Toradex Oak digitial inputs, and thread which stopps door
    connectOakDiginDevice(OAKDIGIN_CONNECT) ;
    
    updateDoorStatusMessage() ;
    if( doorState != DS_STOPPED_CLOSED)
    {
	fprintf( stderr, "DoorVermes::init door is not closed, exiting\n") ;
	exit(1) ;
    }
    // thread only starts door
    int *value ;
    ret = pthread_create( &move_door_id, NULL, move_door, value) ;

    return 0;  
}

int
DoorVermes::info ()
{
    logStream (MESSAGE_DEBUG) << "DoorVermes::info"<< sendLog ;
    updateDoorStatusMessage() ;
    int ret;
    if (ret)
	return -1;


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
    
    if ( doorState== DS_STOPPED_FULLY_OPEN)
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
    
    if ( doorState== -1)
	return -1;
    return 0; // wildi ToDo check that
}

long
DoorVermes::isClosed ()
{
    logStream (MESSAGE_DEBUG) << "DoorVermes::isClosed"<< sendLog ;
    
    if ( doorState== -1)
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

    createValue (doorStateMessage, "DOORSTATE", "door state as clear text", false);

    createValue (raining, "RAIN", "if it's raining", true);
    raining->setValueBool (false);

    createValue (open_door, "OPEN_DOOR", "true opens door", false, RTS2_VALUE_WRITABLE);
    createValue (close_door, "CLOSE_DOOR", "true closes door", false, RTS2_VALUE_WRITABLE);


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
