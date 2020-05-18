/**
 * IRF90 Rotating Focuser from Planewave Instruments.
 *
 * Protocol details: https://planewave.com/files/tech/efa-protocol.html
 *
 * Copyright (C) 2019 Michael Mommert <mommermiscience@gmail.com>
 * based on Hedrick Focuser driver (planewave.cpp)
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
#include "connection/serial.h"

#define TICKS_PER_MICRON  207.288
#define GOTO2_THRESHOLD_TICKS  50000
#define FULL_TRACK_RATE  3955050

namespace rts2focusd
{

typedef enum { IDLE, MOVING_IN, MOVING_OUT, GOTOPOS2 } stateType;

/**
 * Class for Planewave IRF90 focuser. 
 *
 * @author Michael Mommert <mommermiscience@gmail.com>
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Shashikiran Ganesh <shashi@prl.res.in>
 * 
 */
class PWIIRF90:public Focusd
{
	public:
		PWIIRF90 (int argc, char **argv);
		~PWIIRF90 (void);
		virtual int init ();
		virtual int setTo (double num);
		virtual double tcOffset () {return 0.;};
		virtual int isFocusing ();
		virtual int info ();
		
	protected:
		virtual int processOption (int opt);
		virtual int initValues ();

		virtual int setValue (rts2core::Value *old_value,
							  rts2core::Value *new_value);
		virtual bool isAtStartPosition () { return false; }
		stateType state;
		
	private:
		void getValue(const char *name, rts2core::Value *val);
		//int findOptima ();
		
		int getPos();
		int getTemp();
		int setFan(int fancmod);
		int getFan();
		int GetTemperature(double *teltemperature, int tempsensor);
		
		int MoveFocuser(int focdir);
		int GotoPos2(long targetTicks);
		int IsGotoDone();

		const char *device_file;
		
		rts2core::ConnSerial *sconn;
		rts2core::ValueFloat *TempM1;
		rts2core::ValueFloat *TempAmbient;
		rts2core::ValueFloat *FocusState;
		rts2core::ValueBool *fanMode;
};

}

using namespace rts2focusd;

PWIIRF90::PWIIRF90 (int argc, char **argv):Focusd (argc, argv)
{
	sconn = NULL;
	device_file = "/dev/IRF";  // Lowell IRF90 default

	focType = std::string ("PWIIRF90");
	createTemperature ();

	createValue (TempM1, "temp_primary",
				 "[C] Primary mirror temperature", true);
	createValue (TempAmbient, "temp_ambient",
				 "[C] Ambient temperature in telescope", true);

	createValue (fanMode, "FANMODE", "Fan ON? : TRUE/FALSE", false,
				 RTS2_VALUE_WRITABLE);
		
	setFocusExtent (1000, 30000);  // focus range in microns

	state = IDLE;  // start in idle state
	
	createValue (FocusState, "focuser_state",
				 "0: moving in; 1: idle; 2: moving out, 3: goto", true);
	
	addOption ('f', NULL, 1, "device file (usually /dev/IRF)");
}

PWIIRF90::~PWIIRF90 ()
{
	delete sconn;
}

int PWIIRF90::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Focusd::processOption (opt);
	}

	return 0;
}

int PWIIRF90::initValues ()
{
	return Focusd::initValues ();
}

int PWIIRF90::setValue (rts2core::Value *old_value,
						rts2core::Value *new_value)
{
	if (old_value == fanMode)
	{
		setFan (new_value->getValueInteger());
		return 0;
	}

  	return Focusd::setValue (old_value, new_value);
}

int PWIIRF90::setFan(int fancmd)
{
	std::stringstream ss;
	std::string _buf;
	int i;
	unsigned int chksum;
	unsigned char sendstr[] = { 0x3b, 0x04, 0x20, 0x13, 0x27, 0x00, 0x00 };
	// have to update sendstr[5] (on/off) and sendstr[6] (checksum)
	unsigned char returnstr[] = { 0x00, 0x00, 0x00, 0x00,
								  0x00, 0x00, 0x00 };
	sendstr[5] = fancmd;
	
	// derive and update checksum byte
	// exclude start of message and chksum bytes
	for (i=1, chksum=0; i<(sizeof(sendstr)/sizeof(unsigned char)-1); i++)
		chksum += (unsigned int)sendstr[i];
	chksum = -chksum & 0xff;
	sendstr[6] = chksum;

	// // debug only
	// for (i=0; i<sizeof(sendstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)sendstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "send command "  << _buf << sendLog;
	
	// send command
	sconn->flushPortIO();
	sconn->setRTS();
	if (sconn->writePort((const char*)(&sendstr),
						 sizeof(sendstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "setFan: cannot send command"
								  << sendLog;

		sconn->clearRTS();
		return -1;
	}
	sconn->clearRTS();
	
	// read acknowledgement
	if (sconn->readPort((char*)returnstr, 7) == -1)
	{
		logStream (MESSAGE_ERROR) << "setFan: cannot read acknowledgement"
								  << sendLog;
		return -1;
	}

	// read reply
	if (sconn->readPort((char*)returnstr,
						sizeof(returnstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "setFan: cannot read return"
								  << sendLog;
		return -1;
	}

	// check checksum
	for (i=1, chksum=0; i<(sizeof(returnstr)/sizeof(char)-1); i++)
		chksum += (unsigned int)returnstr[i];
	chksum = -chksum & 0xff;

	// // debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(returnstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)returnstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received result "  << _buf << sendLog;
	
	if ((int)(unsigned char)returnstr[6] != chksum)
	{
		logStream (MESSAGE_ERROR) << "result checksum corrupt" << sendLog;
		return -1;
	}
	
	return 0;
}    

int PWIIRF90::getFan()
{
	std::stringstream ss;
	std::string _buf;
	int i;
	unsigned int chksum;
	unsigned char sendstr[] = { 0x3b, 0x03, 0x20, 0x13, 0x28, 0xa2 };
	unsigned char returnstr[] = { 0x00, 0x00, 0x00, 0x00,
								  0x00, 0x00, 0x00 };

	// // debug only
	// for (i=0; i<sizeof(sendstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)sendstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "send command "  << _buf << sendLog;
	
	// send command
	sconn->flushPortIO();
	sconn->setRTS();
	if (sconn->writePort((const char*)(&sendstr),
						 sizeof(sendstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "getFan: cannot send command"
								  << sendLog;
		sconn->clearRTS();
		return -1;
	}
	sconn->clearRTS();
		
	
	// read acknowledgement
	if (sconn->readPort((char*)returnstr, 6) == -1)
	{
		logStream (MESSAGE_ERROR) << "getFan: cannot read acknowledgement"
								  << sendLog;
		return -1;
	}

	// read reply
	if (sconn->readPort((char*)returnstr,
						sizeof(returnstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "getFan: cannot read return"
								  << sendLog;
		return -1;
	}

	// check checksum
	for (i=1, chksum=0; i<(sizeof(returnstr)/sizeof(unsigned char)-1); i++)
		chksum += (unsigned int)returnstr[i];
	chksum = -chksum & 0xff;

	// // debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(returnstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)returnstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received result "  << _buf << sendLog;
	
	if ((int)(unsigned char)returnstr[6] != chksum)
	{
		logStream (MESSAGE_ERROR) << "getFan: result checksum corrupt"
								  << sendLog;
		return -1;
	}
	
	if (returnstr[5] == 3)
		fanMode->setValueBool(0);  // fan off
	else if (returnstr[5] == 0)
		fanMode->setValueBool(1);  // fan on
	else
	{
		logStream (MESSAGE_ERROR) << "getFan: return value unclear"
								  << sendLog;
		return -1;
	}
	return 0;
}    

int PWIIRF90::GetTemperature(double *temp, int tempsensor=0)
{
	std::stringstream ss;
	std::string _buf;
	int i;
	unsigned int chksum;
	unsigned char sendstr[] = { 0x3b, 0x04, 0x20, 0x12, 0x26, 0x00, 0x00 };
	unsigned char returnstr[] = { 0x00, 0x00, 0x00, 0x00,
								  0x00, 0x00, 0x00, 0x00 };
	// have to update sendstr[5] (sensor) and sendstr[6] (checksum)
	sendstr[5] = tempsensor;
	
	// derive and update checksum byte
	// exclude start of message and chksum bytes
	for (i=1, chksum=0; i<(sizeof(sendstr)/sizeof(unsigned char)-1); i++)
		chksum += (unsigned int)sendstr[i];
	chksum = -chksum & 0xff;
	sendstr[6] = chksum;

	// debug only
	// for (i=0; i<sizeof(sendstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)sendstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "send command "  << _buf << sendLog;
	
	// send command
	sconn->flushPortIO();
	sconn->setRTS();
	if (sconn->writePort((const char*)(&sendstr),
						 sizeof(sendstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "GetTemperature: cannot send command"
								  << sendLog;
		sconn->clearRTS();
		return -1;
	}
	sconn->clearRTS();

	// read acknowledgement
	if (sconn->readPort((char*)returnstr, 7) == -1)
	{
		logStream (MESSAGE_ERROR)
			<< "GetTemperature: cannot read acknowledgement"
			<< sendLog;
		return -1;
	}

	// debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(returnstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)returnstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received acknowledgement "
	//                           << _buf << sendLog;

	// read reply
	if (sconn->readPort((char*)returnstr,
						sizeof(returnstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "GetTemperature: cannot read return"
								  << sendLog;
		return -1;
	}

	// check checksum
	for (i=1, chksum=0; i<(sizeof(returnstr)/sizeof(unsigned char)-1); i++)
		chksum += (unsigned int)returnstr[i];
	chksum = -chksum & 0xff;

	// // debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(returnstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)returnstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received result "  << _buf << sendLog;
	
	if ((int)(unsigned char)returnstr[7] != chksum)
	{
		logStream (MESSAGE_ERROR) << "result checksum corrupt" << sendLog;
		return -1;
	}
	
	// calculate temperature from result
	float _temp = (256*(int)(unsigned char)returnstr[6] +
				  (int)(unsigned char)returnstr[5]) / 16.0;

	*temp = _temp;
		
	return 0;
}

// set track rate of focuser
// focdir: -1=move in, 0=stop, 1=move out
int PWIIRF90::MoveFocuser(int focdir)
{
	int i;
	int chksum;
	std::stringstream ss;
	std::string _buf;
	unsigned char sendstr[] = { 0x3b, 0x06, 0x20, 0x12, 0x00,
					   0x00, 0x00, 0x00, 0x00 };
	unsigned char acknstr[] = { 0x3b, 0x06, 0x20, 0x12, 0x00,
					   0x00, 0x00, 0x00, 0x00 };

    // have to modify sendstr[4] with move direction, sendstr[5-7] with
	// encoded focusspd and sendstr[8] with chksum
	unsigned char returnstr[] = { 0x00, 0x00, 0x00, 0x00,
								  0x00, 0x00, 0x00 };
	unsigned long focusspd;
	
	// set focus (move) direction
	if (focdir > 0)
	{
		//logStream (MESSAGE_DEBUG) << "moving focuser out" << sendLog;
		sendstr[4] = 0x06;
		focusspd = (unsigned long)(FULL_TRACK_RATE);
	}
	else if (focdir < 0)
	{
		//logStream (MESSAGE_DEBUG) << "moving focuser out" << sendLog;
		sendstr[4] = 0x07;
		focusspd = (unsigned long)(FULL_TRACK_RATE);
	}
	else
	{
		if (state == MOVING_IN)
			sendstr[4] = 0x07;
		else if (state == MOVING_OUT)
			sendstr[4] = 0x06;
		else
			sendstr[4] = 0x06;
		focusspd = 0;		
	}
		
	// encode focusspd
	int b0, b1, b2;
	b0 = floor(focusspd/(256*256));
	focusspd -= 256*256*b0;
	b1 = floor(focusspd/256);
	b2 = focusspd-b1*256;

	// set focus speed
	sendstr[5] = b0;
	sendstr[6] = b1;
	sendstr[7] = b2;

	// derive and update checksum byte
	// exclude start of message and chksum bytes
	for (i=1, chksum=0; i<(sizeof(sendstr)/sizeof(unsigned char)-1); i++)
		chksum += (unsigned int)sendstr[i];
	chksum = -chksum & 0xff;
	sendstr[8] = (unsigned char)chksum;
	
	// // debug only
	// for (i=0; i<sizeof(sendstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)sendstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "send command "  << _buf << sendLog;

	// send command
	sconn->flushPortIO();
	sconn->setRTS();
	if (sconn->writePort((const char*)(&sendstr),
						 sizeof(sendstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "MoveFocus: cannot send command"
								  << sendLog;
		sconn->clearRTS();
		return -1;
	}
	sconn->clearRTS();

	// read acknowledgement
	if (sconn->readPort((char*)acknstr,
						sizeof(acknstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR)
			<< "MoveFocus: cannot read acknowledgement"
			<< sendLog;
		return -1;
	}

	if ((focdir > 0) && (focusspd > 0))
		state = MOVING_IN;
	else if ((focdir < 0) && (focusspd > 0))
		state = MOVING_OUT;
	else if (focusspd == 0)
		state = IDLE;
	else
	{
		logStream (MESSAGE_ERROR) << "current focuser state unknown"
								  << sendLog;
		return -1;
	}
	FocusState->setValueFloat(state);
	sendValueAll(FocusState);
	
	// // debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(acknstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)acknstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received acknowledgement "
	//                           << _buf << sendLog;

	// read reply
	if (sconn->readPort((char*)returnstr,
						sizeof(returnstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "MoveFocus: cannot read return"
								  << sendLog;
		return -1;
	}

	// check checksum
	for (i=1, chksum=0; i<(sizeof(returnstr)/sizeof(unsigned char)-1); i++)
		chksum += (unsigned int)returnstr[i];
	chksum = -chksum & 0xff;
	
	// // debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(returnstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)returnstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received result "  << _buf << sendLog;
	
	if ((int)(unsigned char)returnstr[6] != chksum)
	{
		logStream (MESSAGE_ERROR) << "MoveFocus: result checksum corrupt"
								  << sendLog;
		return -1;
	}
	
	
	return 0;
}


// order focuser to go to exact position using IRF90 command gotopos2
// don't use this function for long moves, it takes too long
int PWIIRF90::GotoPos2(long targetTicks)
{
	state = GOTOPOS2;
	FocusState->setValueFloat(state);
	sendValueAll(FocusState);
	
	int i;
	unsigned int chksum;
	std::stringstream ss;
	std::string _buf;
	unsigned char sendstr[] = { 0x3b, 0x06, 0x20, 0x12, 0x17,
					   0x00, 0x00, 0x00, 0x00 };
	unsigned char acknstr[] = { 0x3b, 0x06, 0x20, 0x12, 0x17,
					   0x00, 0x00, 0x00, 0x00 };

    // have to modify sendstr[5-7] with
	// encoded target position and sendstr[8] with chksum
	unsigned char returnstr[] = { 0x00, 0x00, 0x00, 0x00,
								  0x00, 0x00, 0x00 };
		
	// encode target position1
	int b0, b1, b2;
	b0 = floor(targetTicks/(256*256));
	targetTicks -= 256*256*b0;
	b1 = floor(targetTicks/256);
	b2 = targetTicks-b1*256;

	// set target position
	sendstr[5] = b0;
	sendstr[6] = b1;
	sendstr[7] = b2;

	// derive and update checksum byte
	// exclude start of message and chksum bytes
	for (i=1, chksum=0; i<(sizeof(sendstr)/sizeof(unsigned char)-1); i++)
	{
		chksum += (unsigned int)sendstr[i];
	}
	chksum = -chksum & 0xff;
	sendstr[8] = (unsigned char)chksum;
	
	// // debug only
	// for (i=0; i<sizeof(sendstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)sendstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "send command " << _buf << sendLog;

	// send command
	sconn->flushPortIO();
	sconn->setRTS();
	if (sconn->writePort((const char*)(&sendstr),
						 sizeof(sendstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "GotoPos2: cannot send command"
								  << sendLog;
		sconn->clearRTS();
		return -1;
	}
	sconn->clearRTS();

	// read acknowledgement
	if (sconn->readPort((char*)acknstr,
						sizeof(acknstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "GotoPos2: cannot read acknowledgement"
								  << sendLog;
		return -1;
	}

	
	// // debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(acknstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)acknstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received acknowledgement "
	//                           << _buf << sendLog;

	// read reply
	if (sconn->readPort((char*)returnstr,
						sizeof(returnstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) <<
			"GotoPos2: cannot read return"
								  << sendLog;
		return -1;
	}

	// check checksum
	for (i=1, chksum=0; i<(sizeof(returnstr)/sizeof(unsigned char)-1); i++)
		chksum += (unsigned int)returnstr[i];
	chksum = -chksum & 0xff;
	
	// // debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(returnstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)returnstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received result "  << _buf << sendLog;
	
	if ((int)(unsigned char)returnstr[6] != chksum)
	{
		logStream (MESSAGE_ERROR) << "GotoPos2: result checksum corrupt"
								  << sendLog;
		return -1;
	}
	
	return 0;
}


// checks whether previous gotoPos2 command has finished
int PWIIRF90::IsGotoDone()
{
	int i;
	unsigned int chksum;
	std::stringstream ss;
	std::string _buf;
	unsigned char sendstr[] = { 0x3b, 0x03, 0x20, 0x12, 0x13, 0xb8 };
	unsigned char acknstr[] = { 0x3b, 0x03, 0x20, 0x12, 0x13, 0xb8 };
	unsigned char returnstr[] = { 0x00, 0x00, 0x00, 0x00,
								  0x00, 0x00, 0x00 };

	// // debug only
	// for (i=0; i<sizeof(sendstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)sendstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "send command " << _buf << sendLog;

	// send command
	sconn->flushPortIO();
	sconn->setRTS();
	if (sconn->writePort((const char*)(&sendstr),
						 sizeof(sendstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) <<
			"IsGotoDone: cannot send command"
								  << sendLog;
		sconn->clearRTS();
		return -1;
	}
	sconn->clearRTS();

	// read acknowledgement
	if (sconn->readPort((char*)acknstr,
						sizeof(acknstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) <<
			"IsGotoDone: cannot read acknowledgement"
								  << sendLog;
		return -1;
	}
	
	// // debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(acknstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)acknstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received acknowledgement "
	//                           << _buf << sendLog;

	// read reply
	if (sconn->readPort((char*)returnstr,
						sizeof(returnstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) <<
			"IsGotoDone: cannot read return"
								  << sendLog;
		return -1;
	}

	// check checksum
	for (i=1, chksum=0; i<(sizeof(returnstr)/sizeof(unsigned char)-1); i++)
		chksum += (unsigned int)returnstr[i];
	chksum = -chksum & 0xff;

	// // debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(returnstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)returnstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received result "  << _buf << sendLog;
	
	if ((int)(unsigned char)returnstr[6] != chksum)
	{
		logStream (MESSAGE_ERROR) << "result checksum corrupt" << sendLog;
		return -1;
	}

	if (returnstr[5] == 0)
		return 0; // still running
	else if (returnstr[5] == 255)
		return 1; // done
	else
	{
		logStream (MESSAGE_ERROR) <<
			"IsGotoDone: unclear if goto has finished "  << sendLog;		
		return -1;
	}
}

// send focus to given position
int PWIIRF90::setTo (double num)
{
	target->setValueDouble(num);

	return 0;
}


// report state of the focuser
int PWIIRF90::isFocusing ()
{
	// update position
	if (getPos() < 0)
		return -1;

    // target and position in microns, convert to ticks
	double targetTicks = target->getValueDouble() * TICKS_PER_MICRON;
	double currentPosTicks = position->getValueDouble() * TICKS_PER_MICRON;

	// define focus direction	
	int focdir = 0;  
	if (targetTicks > currentPosTicks)
		focdir = 1;
	else
		focdir = -1;

	// GotoPos2 is done
	if (state == GOTOPOS2)
	{
		int isgotopos2done = IsGotoDone();

		if (isgotopos2done == -1)
		{
			logStream (MESSAGE_ERROR) << "IsFocusing: gotopos2 failed"
									  << sendLog;
			state = IDLE;
			FocusState->setValueFloat(state);
			sendValueAll(FocusState);
			return -1;
		}
		else if (isgotopos2done == 1)
		{
			logStream (MESSAGE_ERROR) << "arrived at target position" <<
				sendLog;
			state = IDLE;
			FocusState->setValueFloat(state);
			sendValueAll(FocusState);
			return -2;
		}
	}

	// focuser not at target; commence focusing
	/* logic here taken from IRF90 Python 2.7 library by Kevin Ivarsen
	    idea is to move focuser to within GOTO2_THRESHOLD_TICKS
		of target position, then use GOTO2 (IRF90 command) to 
		do the rest; GOTO2 is more precise, but also slower
	*/
	double targetBeforeGoto2 = targetTicks - focdir * GOTO2_THRESHOLD_TICKS;
	double remainingTicksBeforeStop = focdir * (targetBeforeGoto2 -
												currentPosTicks);

	// move towards the target position
	if ((remainingTicksBeforeStop > 0) && (state == IDLE))
	{
		MoveFocuser(focdir);
	}
	else if ((remainingTicksBeforeStop <= 0) && (state != GOTOPOS2))
	{
		// now we're close enough to use gotopos2
		GotoPos2(targetTicks);
	}

	return 1;

}


int PWIIRF90::getTemp ()
{
	double temp; 

	// primary mirror
	if (GetTemperature(&temp, 0) < 0)
	{
		logStream (MESSAGE_ERROR) <<
			"getTemp: could not read primary mirror temperature"
								  <<  sendLog;
		return -1;
	}
	TempM1->setValueFloat(temp);
	temperature->setValueFloat(temp);
	sendValueAll(TempM1);
	sendValueAll(temperature);	
    // FOC_TEMP is Primary Mirror Temperature ... 

	// ambient temperature
	if (GetTemperature(&temp, 1) < 0)
	{
		logStream (MESSAGE_ERROR) <<
			"getTemp: could not read ambient temperature" <<  sendLog;
		return -1;
	}
	TempAmbient->setValueFloat(temp);
	sendValueAll(TempAmbient);
	return 0;
}

int PWIIRF90::info ()
{
	getPos();
	getTemp();
	
	return Focusd::info();
}

int PWIIRF90::getPos ()
{
	std::stringstream ss;
	std::string _buf;
	int i;
	unsigned int chksum = 0;
	unsigned char sendstr[] = { 0x3b, 0x03, 0x20, 0x12, 0x1, 0xca };
	char returnstr[] = { 0x00, 0x00, 0x00, 0x00, 0x00,
						 0x00, 0x00, 0x00, 0x00 };
	
	// debug only
	// for (i=0; i<sizeof(sendstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)sendstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "send command "  << _buf << sendLog;

	// send command
	sconn->flushPortIO();
	sconn->setRTS();
	if (sconn->writePort((const char*)(&sendstr),
						 sizeof(sendstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "getPos: cannot send command"
								  << sendLog;
		sconn->clearRTS();
		return -1;
	}
	sconn->clearRTS();

	// read acknowledgement
	if (sconn->readPort(returnstr,
						sizeof(sendstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "getPos: cannot read acknowledgement"
								  << sendLog;
		return -1;
	}

	// debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(returnstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)returnstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received acknowledgement "
	//                           << _buf << sendLog;

	// read reply
	if (sconn->readPort(returnstr,
						sizeof(returnstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "getPos: cannot read return"
								  << sendLog;
		return -1;
	}

	// check checksum
	for (i=1, chksum=0; i<(sizeof(returnstr)/sizeof(unsigned char)-1); i++)
		chksum += (unsigned int)returnstr[i];
	chksum = -chksum & 0xff;

	// // debug only
	// ss.str(std::string());
	// for (i=0; i<sizeof(returnstr)/sizeof(unsigned char); ++i)
	// 	ss << std::hex << (int)returnstr[i] << " ";
	// _buf = ss.str();
	// logStream (MESSAGE_DEBUG) << "received result "  << _buf << sendLog;
	
	if ((int)(unsigned char)returnstr[8] != chksum)
	{
		logStream (MESSAGE_ERROR) << "result checksum corrupt" << sendLog;
		return -1;
	}
	
	int b0 = (unsigned char) returnstr[5];
  	int b1 = (unsigned char) returnstr[6];
  	int b2 = (unsigned char) returnstr[7];
  	float focus = (256*256*b0 + 256*b1 + b2)/TICKS_PER_MICRON;

	position->setValueInteger(focus);
	sendValueAll(position);
	return 0;
}

int PWIIRF90::init ()
{
	int ret;
	ret = Focusd::init ();
	
	if (ret) 
		return ret;
	
	sconn = new rts2core::ConnSerial(device_file, this, rts2core::BS19200,
									 rts2core::C8, rts2core::NONE, 5);
	sconn->setDebug(getDebug());
	sconn->init();

	sconn->flushPortIO();

	setIdleInfoInterval(10);

	// get temperature and position status
	info();

	// get fan status
	getFan();
	
	/* communications protocol:
	   ========================
	   messages are hex of the form:
	   { 0x3b (start of message byte), 
	     number of bytes (excluding 0x3b, nob, and checksum),
         source address,
         receiver address,
		 command,
		 *data,
		 checksum
       }

	   # Device addresses recognized by IRF90
	   (EFA is an electronic focus something, not important here)

	   Address = Enum(
		   PC = 0x20,       # User's computer
		   HC = 0x0D,       # EFA hand controller
		   FOC_TEMP = 0x12, # EFA focus motor and temp sensors
		   ROT_FAN = 0x13,  # EFA rotator motor and fan control
		   DELTA_T = 0x32   # Delta-T dew heater
	   )

	   # Temperature sensors.
	   # NOTE: Not all telescope configurations include all temp sensors!
	   TempSensor = Enum(
		   PRIMARY = 0,
		   AMBIENT = 1,
		   SECONDARY = 2,
		   BACKPLATE = 3,
		   M3 = 4
	   )

	   # Commands recognized by the EFA. See protocol documentation for more details.
	   Command = Enum(
		   MTR_GET_POS = 0x01,
		   MTR_GOTO_POS2 = 0x17,
		   MTR_OFFSET_CNT = 0x04,
		   MTR_GOTO_OVER = 0x13,
		   MTR_PTRACK = 0x06,
		   MTR_NTRACK = 0x07,
		   MTR_SLEWLIMITMIN = 0x1A,
		   MTR_SLEWLIMITMAX = 0x1B,
		   MTR_SLEWLIMITGETMIN = 0x1C,
		   MTR_SLEWLIMITGETMAX = 0x1D,
		   MTR_PMSLEW_RATE = 0x24,
		   MTR_NMSLEW_RATE = 0x25,
		   TEMP_GET = 0x26,
		   FANS_SET = 0x27,
		   FANS_GET = 0x28,
		   MTR_GET_CALIBRATION_STATE = 0x30,
		   MTR_SET_CALIBRATION_STATE = 0x31,
		   MTR_GET_STOP_DETECT = 0xEE,
		   MTR_STOP_DETECT = 0xEF,
		   MTR_GET_APPROACH_DIRECTION = 0xFC,
		   MTR_APPROACH_DIRECTION = 0xFD,
		   GET_VERSION = 0xFE,

	   )
	*/

	return 0;	
}


int main (int argc, char **argv)
{
	PWIIRF90 device (argc, argv);
	return device.run ();
}
