/* 
 * STA Reflex controller
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "camd.h"
#include "edtsao/interface.h"

#include "valuearray.h"

// select type of driver
#define CL_EDT
//#define CL_EURESYS

#ifdef CL_EURESYS
#include <terminal.h>
#define hSerRef void *
#elif defined(CL_EDT)
#include <edtinc.h>
#define CL_BAUDRATE_9600                        1
#define CL_BAUDRATE_19200                       2
#define CL_BAUDRATE_38400                       4
#define CL_BAUDRATE_57600                       8
#define CL_BAUDRATE_115200                      16
#define CL_BAUDRATE_230400                      32
#define CL_BAUDRATE_460800                      64
#define CL_BAUDRATE_921600                      128
#else
#include <clallserial.h>
#endif


namespace rts2camd
{

/**
 * Main control class for STA Reflex controller. See
 * http://www.sta-inc.net/reflex for controller details
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class Reflex:public Camera
{
	public:
		Reflex (int in_argc, char **in_argv);
		virtual ~Reflex (void);

		virtual int init ();
		virtual int initValues ();

		virtual int commandAuthorized (Rts2Conn * conn);

	protected:
		virtual int processOption (int in_opt);
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual void beforeRun ();
		virtual void initBinnings ();

		virtual int startExposure ();
		virtual int stopExposure ();
		virtual long isExposing ();
		virtual int readoutStart ();

		virtual long suggestBufferSize () { return -1; }

		virtual int doReadout ();
		virtual int endReadout ();

	private:
		int openInterface (int port);
		int closeInterface();

		rts2core::ValueSelection *baudRate;
#ifdef CL_EDT
		PdvDev *CLHandle;
#else
		hSerRef CLHandle;
#endif
};

}

using namespace rts2camd;

void Reflex::beforeRun ()
{
}

void Reflex::initBinnings ()
{
	addBinning2D (1, 1);
}

int Reflex::startExposure ()
{
	return 0;
}

int Reflex::stopExposure ()
{
	return Camera::stopExposure ();
}

long Reflex::isExposing ()
{
	return 0;
}

int Reflex::readoutStart ()
{
	return 0;
}

int Reflex::doReadout ()
{
	return -2;
}

int Reflex::endReadout ()
{
	return Camera::endReadout ();
}

Reflex::Reflex (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
	CLHandle = NULL;

	createValue (baudRate, "baud_rate", "CL baud rate", true, CAM_WORKING);
	baudRate->addSelVal ("9600", (rts2core::Rts2SelData *) CL_BAUDRATE_9600);
	baudRate->addSelVal ("19200", (rts2core::Rts2SelData *) CL_BAUDRATE_19200);
	baudRate->addSelVal ("38400", (rts2core::Rts2SelData *) CL_BAUDRATE_38400);
	baudRate->addSelVal ("57600", (rts2core::Rts2SelData *) CL_BAUDRATE_57600);
	baudRate->addSelVal ("115200", (rts2core::Rts2SelData *) CL_BAUDRATE_115200);
	baudRate->addSelVal ("230400", (rts2core::Rts2SelData *) CL_BAUDRATE_230400);
	baudRate->addSelVal ("460800", (rts2core::Rts2SelData *) CL_BAUDRATE_460800);
	baudRate->addSelVal ("921600", (rts2core::Rts2SelData *) CL_BAUDRATE_921600);
}

Reflex::~Reflex (void)
{
	edt_close (CLHandle);
}

int Reflex::processOption (int in_opt)
{
	switch (in_opt)
	{
		default:
			return Camera::processOption (in_opt);
	}
	return 0;
}

int Reflex::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	return Camera::setValue (old_value, new_value);
}

int Reflex::init ()
{
	return 0;
}

int Reflex::initValues ()
{
	return Camera::initValues ();
}

int Reflex::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("reset"))
	{
		if (!conn->paramEnd ())
			return -2;
		return 0;
	}
	return Camera::commandAuthorized (conn);
}

int Reflex::openInterface (int CLport)
{
	// Clear all previously open interfaces
	closeInterface();

	//int CLBaudrate = (int) (baudRate->getData ());
	int CLBaudrate = CL_BAUDRATE_115200;
#ifdef CL_EDT
	CLHandle = pdv_open ((const char *) (EDT_INTERFACE), 0);
	if (!CLHandle)
	{
		logStream (MESSAGE_ERROR) << "Error opening CameraLink port" << sendLog;
		return -1;
	}
	pdv_reset_serial (CLHandle);

	// Set baud rate and delimiters
	if (pdv_set_baud (CLHandle, CLBaudrate))
	{
		logStream (MESSAGE_ERROR) << "Error setting CameraLink baud rate" << sendLog;
		return -1;
	}
	pdv_set_serial_delimiters (CLHandle, "", "");
#else
	err = clSerialInit (CLport, &CLHandle);
	if (err != CL_ERR_NO_ERR)
	{
		CLHandle = 0;
		logStream (MESSAGE_ERROR) << "Error opening CameraLink port" << sendLog;
		return -1;
	}
	err = clSetBaudRate(CLHandle, CLBaudrate);
	if (err != CL_ERR_NO_ERR)
	{
		clSerialClose(CLHandle);
		CLHandle = 0;
		logStream (MESSAGE_ERROR) << "Error setting CameraLink baud rate" << sendLog;
		return -1;
	}
#endif
	return 0;
}

int Reflex::closeInterface()
{
	// Close any open CameraLink connection
	if (CLHandle)
	{
#ifdef CL_EDT
		pdv_close (CLHandle);
#else
		clSerialClose (CLHandle);
#endif
		CLHandle = 0;
	}
	return 0;
}

int main (int argc, char **argv)
{
	Reflex device (argc, argv);
	return device.run ();
}
