/**
 * Copyright (C) 2018 Ronan Cunniffe
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
 * along with this program; if not, wwriteReadrite to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "focusd.h"
#include "connection/serial.h"

#define STEPS_PER_MM	5667.0			//HW resolution
#define STEPS_PER_PST	(STEPS_PER_MM * 0.01)	//0 - 30mm -> 0 - 3000 pst)
#define MECH_MIN_PST 0
#define MECH_MAX_PST 3000
#define SERIAL_TIMEOUT_DECIS 10		// Yeah, deciseconds.  Not my fault.

#define AAF_POLL_HW 750
/*
 * Protocol details.
 * 
 * ASA provided a windows client (which we sniffed), and a Linux SDK (which
 * (a) did maximally stupid things like spontaneously create logging
 * directories, and (b) segfaulted.
 *
 * This is a preliminary driver based on the reverse-engineered protocol, rather
 * than the PITA non-working SDK.
 *
 * Rather than use 0x0a, they use '#' and '$' as start and end tokens.
 * The focuser only transmits in reply to a host command.  Not all host
 * commands trigger replies, and the focuser does not echo (so you can manually
 * type commands, but only blind)
 *
 * Actually, focuser replies *do* append 0x0a - this may be a bug (if their
 * code skips to the next start token, they won't notice it).
 *
 * Function		Send			Receive
 * Get Position		#P081 F$		#P081 F <steps>$
 * Set Position		#P092 F <steps>$	<no reply>
 * Get Temperature	#S051 F$		$S052 F <temp>$
 * Get Status		#S001 F$		$S004 F XXX$
 *
 * For the AAF3, the resolution is ~6400 steps/mm.
 * The temperature scaling is not (yet) clear, we get 645 or 642 for
 * temperatures actually somewhere near 25-28.
 *
 * Known status codes are:
 * NSU	Uninitialised?
 * NSO	OK, not moving
 * NCO	OK, moving
 */
namespace rts2focusd
{

/**
 * Class for ASA AAF focuser (and possibly others)
 *
 * This is based on reverse-engineered protocol.  See source code for details.
 * 
 *
 * @author Ronan Cunniffe
 */


class AAF:public Focusd
{
	public:
		AAF (int argc, char **argv);
		~AAF (void);

//		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int info ();
		virtual int setTo (double num);
		virtual double tcOffset () {return 0.;};
		virtual int isFocusing ();

		virtual bool isAtStartPosition ();
		virtual int processOption (int in_opt);
		virtual int initHardware ();
		virtual int initValues ();
	private:
		const char *dev_name;
		rts2core::ConnSerial *serialport;
		
		int status;

		// high-level I/O functions
		int getPos ();
		int getTemp ();
		int getStatus ();
};

};

using namespace rts2focusd;

AAF::AAF (int argc, char **argv):Focusd (argc, argv)
{
	dev_name=NULL;
	setFocusExtent(MECH_MIN_PST, MECH_MAX_PST);
	addOption ('f', "dev_name", 1, "device file (usually /dev/ttySx");
}

AAF::~AAF ()
{
	delete serialport;
}

int AAF::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dev_name = optarg;
			break;
		default:
			return Focusd::processOption (in_opt);
	}
	return 0;
}


/*!
 * Initialise focuser on given port
 *
 *
 * @return 0 on success, otherwise -1 & set errno
 */

int AAF::initHardware ()
{
	int ret;


	if (dev_name == NULL)
	{
		logStream(MESSAGE_ERROR) << "Cannot initialise AAF focuser without a device name." << sendLog;
		return -1;
	}

	serialport = new rts2core::ConnSerial (dev_name, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, SERIAL_TIMEOUT_DECIS);
//	serialport->setDebug (true);
	ret = serialport->init ();
	if (ret)
		return ret;

	// check this is actually an AAF device.
	ret = getStatus();
	if (ret!=0)
	{
		logStream(MESSAGE_ERROR) << dev_name << " probably isn't an AAF focuser." << sendLog;
		return -1;
	}

// Ok, send the init messages.  Not absolutely necessary, but copied anyway
// from Windows transcript.
 	serialport->writePort("#P031 F$",8);
	serialport->writePort("#E002 F 1$",10);
	serialport->writePort("#P132 F 195$",12);
	serialport->writePort("#E012 F 50$",11);
	serialport->writePort("#E022 F 50$",11);

	logStream(MESSAGE_INFO) << "AAF on " << dev_name << " initialized ok." << sendLog;
	return 0;
}


/*
 * int AAF::commandAuthorized (rts2core::Connection * conn)
{
	return Focusd::commandAuthorized(conn);
}
*/

bool AAF::isAtStartPosition()
{
	// *FIXME* Not sure exactly how this is used.
	return false;
}

int AAF::getPos ()
{
//	logStream(MESSAGE_INFO) << "Getting position." << sendLog;
	char rbuf[16];
	memset((void *)rbuf,0,16);

	if (serialport->writeRead ("#P081 F$", 8, rbuf, 16, '\n') < 1)
	{
		serialport->flushPortIO ();
		logStream (MESSAGE_ERROR) << "Position query failed." << sendLog;
		return -1;
	}
	unsigned rblen = strlen(rbuf);
	if ((!strncmp("#P081 F ",rbuf,16)) || (rbuf[rblen-1]=='$'))
	{
		logStream (MESSAGE_ERROR) << "Garbled reply to position query: " << rbuf << sendLog;
		return -1;
	}
	rbuf[rblen-1] = '\0';

	char *startptr = rbuf + 8; // skip '#P081 F '
	char *endptr;
	long int steps = strtol(startptr, &endptr, 10);
	if (endptr == startptr)
	{
		logStream (MESSAGE_ERROR) << "Expected integer position, got " << startptr << "." << sendLog;
		return -1;
	}


	position->setValueInteger((int)(steps/STEPS_PER_PST));
	sendValueAll(position);
	return 0;
}

int AAF::getTemp ()
{
//	logStream(MESSAGE_INFO) << "Getting temperature." << sendLog;
	char rbuf[16];
	memset((void *)rbuf,0,16);
	if (serialport->writeRead ("#S051 F$", 8, rbuf, 13, '\n') < 1)
	{
		return -1;
	}
	temperature->setValueFloat (atof ((rbuf + 8)));
	sendValueAll(temperature);

	//logStream(MESSAGE_INFO) << "Temperature = " << temperature->getValueFloat() << sendLog;
	return 0;
}

int AAF::getStatus ()
{
//	logStream(MESSAGE_INFO) << "Getting status." << sendLog;
	char rbuf[16];
	memset((void *)rbuf,0,16);
	if (serialport->writeRead ("#S001 F$", 8, rbuf, 13, '\n') < 1)
	{
		return -1;
	}
	rbuf[11]='\0';
//	logStream(MESSAGE_INFO) << "Status code " << &rbuf[8] << sendLog;
	
	if (rbuf[8] != 'N')
	{
		logStream(MESSAGE_INFO) << "Unexpected status ch1 code '" << rbuf[8] << "'.  Expected 'N'" << sendLog;
		return -1;
	}

	switch (rbuf[9])
	{
		case 'C' : 	//constant
		case 'A' : 	//accelerating
		case 'D' : 	//guess....
			getPos ();
			setIdleInfoInterval(0.1);
			return 1;
		case 'S' : 	//stationary
			setIdleInfoInterval(20);
			return 0;
	}
	logStream(MESSAGE_INFO) << "Unexpected status ch2 code " << rbuf[9] << sendLog;

	return -1;
}


int AAF::initValues ()
{
	focType = std::string ("ASA_AAF");
	createTemperature();
	return Focusd::initValues ();
}

int AAF::info ()
{
	int ret;
	ret = getPos ();
	if (ret)
		return ret;
	ret = getTemp ();
	if (ret)
		return ret;

	ret = getStatus();
	if (ret)
		return ret;
	
	return Focusd::info ();
}

/*!
 * Send command
 */
int AAF::setTo (double pseudosteps)
{
	/* *FIXME* can this be called while focuser is still moving? */
	char command[20];
	int ret;

	unsigned steps = (unsigned)(pseudosteps * STEPS_PER_PST);

//	logStream(MESSAGE_INFO) << "Setting focuser to " << steps << " (" << (float)(pseudosteps*0.01) << "mm)" << sendLog;

	// *FIXME* This code cannot be called, because the lower level focuser
	// code checks before calling here, and explodes with "command ended with -2, invalid blahblah. Bah.
	if ((pseudosteps > MECH_MAX_PST) || (pseudosteps < MECH_MIN_PST))
	{
		logStream(MESSAGE_WARNING) << "Denied request to move focuser to " << pseudosteps << ". Limits are " << MECH_MIN_PST << "-" << MECH_MAX_PST << "mm." << sendLog;
		return -1;

	}

	sprintf (command, "#P092 F %u$", steps);

	// logStream(MESSAGE_INFO) << "Sending " << command << "." << sendLog;

	ret = serialport->writePort(command, strlen(command));

	usleep(USEC_SEC/100);
	return ret;
}

int AAF::isFocusing ()
{
	int ret = getStatus ();
	
	switch (ret)
	{
		case 0 : return -2;
		case 1 : return 0;
	}
	return -1;
}


int main (int argc, char **argv)
{
	AAF device (argc, argv);
	return device.run ();
}
