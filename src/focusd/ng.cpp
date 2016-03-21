/**
 * NG focuser driver (part of TCS NG)
 * Copyright (C) 2016 Scott Swindell
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

#include "focusd.h"
#include "connection/tcsng.h"
#define OPT_FOC_STEPS    OPT_LOCAL + 1001
#define FOC_MOTION_BIT 0x4

#define TCSNG_NO_MOVE_CALLED   0
#define TCSNG_MOVE_CALLED      1
#define TCSNG_MOVING           2

namespace rts2focusd
{

/**
 * Class for NG focuser.
 *
 * @author Scott Swindell
 */
class NG:public Focusd
{
	public:
		NG (int argc, char **argv);
		~NG (void);
		virtual int setTo (double num);
		virtual double tcOffset () {return 0.;};
		virtual int isFocusing ();
		virtual int endFocusing();
		virtual int info();

	protected:
		virtual int processOption (int opt);
		virtual int initValues ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);
		virtual int initHardware();
		virtual bool isAtStartPosition () { return true; }
		int writePosition(int num);
		virtual int writePosition(double num){ writePosition((int) num); return 0; }
	
	private:
	
		const char * obsid;
		rts2core::ValueFloat *focSteps;
		rts2core::ValueDouble *focMin;
		rts2core::ValueDouble *focMax;
		rts2core::ValueInteger *moveState;
		rts2core::ConnTCSNG *ngconn;
		HostString *host;
		long timeStamp;
};

}

using namespace rts2focusd;

NG::NG (int argc, char **argv):Focusd (argc, argv)
{
	focType = std::string ("NG");
	host=NULL;

	createValue (focMin, "foc_min", "minimal focuser value", false, RTS2_VALUE_WRITABLE);
	createValue (focMax, "foc_max", "maximal focuser value", false, RTS2_VALUE_WRITABLE);
	createValue (moveState, "moveState", "focuser move state", false);

	addOption ('f', "ipAddress", 1, "Ip Address of the focuser");
	addOption ('n', "obsid", 1, "TCS NG observatory name");
	addOption (OPT_FOC_STEPS, "focstep", 1, "initial value of focuser steps");
}

NG::~NG ()
{
	delete ngconn;
}

int NG::initHardware()
{
	if (host == NULL)
	{
		logStream(MESSAGE_ERROR) << "You must use the -f option to set the focuser." << sendLog;
		return -1;
	}
	if (obsid == NULL)
	{
		logStream(MESSAGE_ERROR) << "You must use the -n option to set the obsid." << sendLog;
		return -1;
	}
	ngconn = new rts2core::ConnTCSNG (this, host->getHostname (), host->getPort (), obsid, "TCS");
	ngconn->setDebug( true );
	return 0;
}

int NG::processOption (int opt)
{
	switch (opt)
	{
		
		case 'f':
			host = new HostString (optarg, "5750");
			break;
		case 'n':
			obsid = optarg;
			break;
		default:
			return Focusd::processOption (opt);
	}
	return 0;
}

int NG::initValues ()
{
	
	position->setValueDouble (ngconn->getInteger("FOCUS"));
	//defaultPosition->setValueDouble (0);
	//temperature->setValueFloat (100);
	return Focusd::initValues ();
}

int NG::info()
{
	position->setValueDouble( (double) ngconn->getInteger("FOCUS") );
	
	return 0;
}

int NG::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == focMin)
	{
		setFocusExtent (new_value->getValueDouble (), focMax->getValueDouble ());
		return 0;
	}
	else if (old_value == focMax)
	{
		setFocusExtent (focMin->getValueDouble (), new_value->getValueDouble ());
		return 0;
	}
	return Focusd::setValue (old_value, new_value);
}

int NG::writePosition( int num )
{

	logStream( MESSAGE_INFO ) << "write position called" << sendLog;
	//setTo()
	return 0;
}

int NG::setTo (double num)
{
	char cmdBuf[50];
	sprintf(cmdBuf, "FOCUS %i", (int) num);
	ngconn->command(cmdBuf);
	timeStamp = getNow();
	moveState->setValueInteger(TCSNG_MOVE_CALLED);
	return 0;
}

int NG::isFocusing ()
{
	int rtn = USEC_SEC/10;
	bool _isMoving = (ngconn->getInteger("MOTION") & FOC_MOTION_BIT) >> 2;
	
	switch( moveState->getValueInteger() )
	{
		case TCSNG_NO_MOVE_CALLED:
			if( _isMoving )
				rtn = -1;	
			break;
		case TCSNG_MOVE_CALLED:
			if( _isMoving )
				moveState->setValueInteger(TCSNG_MOVING);
		case TCSNG_MOVING:
			if( !_isMoving )
			{
				moveState->setValueInteger(TCSNG_NO_MOVE_CALLED);
				rtn = -2;
			}
	}
	
	position->setValueDouble( (double) ngconn->getValueInteger("FOCUS") );
	logStream(MESSAGE_INFO) << "isFocusing Called moveState is " << moveState->getValueInteger() << " _isMoving is "<< _isMoving <<" deltaT is " << getNow() - timeStamp <<  sendLog; 
	return rtn;
}

int NG::endFocusing()
{

	info();
	return 0;
}

int main (int argc, char **argv)
{
	NG device (argc, argv);
	return device.run ();
}
