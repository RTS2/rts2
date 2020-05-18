/* 
 * Planewave IRF90 rotator
 * Copyright (C) 2019 Michael Mommert <mommermiscience@gmail.com>
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

#include "rotad.h"
#include "connection/serial.h"

#define ROTATOR_TICKS_PER_DEG 36408.0

namespace rts2rotad
{

/**
 * Planewave IRF90 rotator
 *
 * @author Michael Mommert <mommermiscience@gmail.com>
 */
class PWIIRF90R:public Rotator
{
	public:
		PWIIRF90R(int argc, char **argv);
		~PWIIRF90R(void); 
		virtual int init();
		virtual int info();
	protected:
		virtual void setTarget (double tv);
		virtual long isRotating ();
	private:
		const char *device_file;
		rts2core::ConnSerial *sconn;
};

}

using namespace rts2rotad;

PWIIRF90R::PWIIRF90R (int argc, char **argv):Rotator (argc, argv)
{
	sconn = NULL;
	device_file = "/dev/IRF";  // Lowell IRF90 default
}


PWIIRF90R::~PWIIRF90R ()
{
	//delete sconn;
}

int PWIIRF90R::init ()
{
	int ret;
	ret = Rotator::init ();
	
	if (ret) 
		return ret;
	
	sconn = new rts2core::ConnSerial(device_file, this, rts2core::BS19200,
									 rts2core::C8, rts2core::NONE, 5);
	sconn->setDebug(getDebug());
	sconn->init();

	sconn->flushPortIO();

	setIdleInfoInterval(10);
	
	logStream (MESSAGE_ERROR) << "I am initialized!" << sendLog;

	info();
	
	return 0;
}

int PWIIRF90R::info()
{
	// retrieve current position
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
	if (sconn->writePort((const char*)(&sendstr),
						 sizeof(sendstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "getFan: cannot send command"
								  << sendLog;
		return -1;
	}
		
	
	// read acknowledgement
	if (sconn->readPort((char*)returnstr, 6) == -1)
	{
		logStream (MESSAGE_ERROR) << "getFan: cannot read acknowledgement"
								  << sendLog;
		return -1;
	}

	// switch RTS bit to receive results
	sconn->switchRTS();

	// read reply
	if (sconn->readPort((char*)returnstr,
						sizeof(returnstr)/sizeof(unsigned char)) == -1)
	{
		logStream (MESSAGE_ERROR) << "getFan: cannot read return"
								  << sendLog;
		sconn->switchRTS();
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
		sconn->switchRTS();
		logStream (MESSAGE_ERROR) << "getFan: result checksum corrupt"
								  << sendLog;
		return -1;
	}
	
	// switch RTS bit to enable requests again
	sconn->switchRTS();

	logStream (MESSAGE_ERROR) << "getFan: fan is " << (unsigned int)returnstr[6] << sendLog;

	return 0;

}

void PWIIRF90R::setTarget (double vt)
{
	
}

long PWIIRF90R::isRotating ()
{
	double diff = getDifference ();
	std::cout << "diff " << diff << " " << getCurrentPosition () << " " << getTargetPosition () << std::endl;
	if (fabs (diff) > 1)
	{
		setCurrentPosition (ln_range_degrees ((diff > 0) ? getCurrentPosition () - 1 : getCurrentPosition () + 1));
		return USEC_SEC * 0.2;
	}
	setCurrentPosition (getTargetPosition ());
	return -2;
}

int main (int argc, char **argv)
{
	PWIIRF90R device (argc, argv);
	return device.run ();
}
