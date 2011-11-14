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
 * Register class. Holds register address in Reflex controller.
 */
class RRegister:public rts2core::ValueInteger
{
	public:
		RRegister (std::string in_val_name):rts2core::ValueInteger (in_val_name)
		{
			reflex_addr = 0;
			info_update = false;
		}
		RRegister (std::string in_val_name, std::string in_description, bool writeToFits = true, int32_t flags = 0):rts2core::ValueInteger (in_val_name, in_description, writeToFits, flags)
		{
			reflex_addr = 0;
			info_update = false;
		}

		void setRegisterAddress (uint32_t addr) { reflex_addr = addr; }
		void setInfoUpdate () { info_update = true; }

		bool infoUpdate () { return info_update; }

	private:
		uint32_t reflex_addr;
		/**
		 * If value should be update on info call.
		 */
		bool info_update;
};

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

		virtual int initValues ();

		virtual int commandAuthorized (Rts2Conn * conn);

	protected:
		virtual int processOption (int in_opt);
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual void beforeRun ();
		virtual void initBinnings ();

		virtual int initHardware ();

		virtual int info ();

		virtual int startExposure ();
		virtual int stopExposure ();
		virtual long isExposing ();
		virtual int readoutStart ();

		virtual long suggestBufferSize () { return -1; }

		virtual int doReadout ();
		virtual int endReadout ();

	private:
		/**
		 * Register map. Index is register address.
		 */
		std::map <uint32_t, RRegister *> registers;

		/**
		 * High-level command to add register to configuration.
		 *
		 * @return 
		 */
		RRegister * createRegister (uint32_t address, const char *name, const char * desc, bool writable, bool infoupdate);

		int openInterface (int port);
		int closeInterface ();


		int CLRead (char *c);
		int CLReadLine (std::string &s, int timeout);
		int CLWriteLine (const char *s, int timeout);

		int CLFlush ();

		int CLCommand (const char *cmd, std::string &response, int timeout, bool log);
		int interfaceCommand (const char *cmd, std::string &response, int timeout, bool log = true);

		/**
		 * Access refelex registers.
		 */
		int readRegister(uint32_t addr, uint32_t& data);
		int writeRegister(uint32_t addr, uint32_t data);

		rts2core::ValueSelection *baudRate;
#ifdef CL_EDT
		PdvDev *CLHandle;
#else
		hSerRef CLHandle;
#endif
};

}

using namespace rts2camd;

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

	// interface registers
	createRegister (0x00000000, "int.error_code", "holds the error code fomr the most recent error", false, true);
	createRegister (0x00000001, "int.error_source", "holds an integer identifying the source of the most recent error", false, true);
	createRegister (0x00000002, "int.error_line", "holds the line number in the interface CPUs", false, true);
	createRegister (0x00000003, "int.status_index", "a counter that increments every time the interface CPU polss the system for its status", false, true);
	createRegister (0x00000004, "int.power", "current CCD power state", true, true);
	createRegister (0x00000005, "int.backplane_type", "board type field for the backplane board", false, false);
	createRegister (0x00000006, "int.interface_type", "board type field for the interface board", false, false);
}

Reflex::~Reflex (void)
{
	for (std::map <uint32_t, RRegister *>::iterator iter = registers.begin (); iter != registers.end (); iter++)
		delete iter->second;
	
	registers.clear ();

#ifdef CL_EDT
	edt_close (CLHandle);
#endif
}

void Reflex::beforeRun ()
{
	Camera::beforeRun ();
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

int Reflex::initHardware ()
{
	int ret;
	ret = openInterface (0);
	if (ret)
		return ret;
	return 0;
}

int Reflex::info ()
{
	for (std::map <uint32_t, RRegister *>::iterator iter=registers.begin (); iter != registers.end (); iter++)
	{
		if (iter->second->infoUpdate ())
		{
			uint32_t rval;
			if (readRegister (iter->first, rval))
			{
				logStream (MESSAGE_ERROR) << "error reading register " << iter->first << sendLog;
				return -1;
			}
			iter->second->setValueInteger (rval);
		}
	}
	return Camera::info ();
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

RRegister * Reflex::createRegister (uint32_t address, const char *name, const char * desc, bool writable, bool infoupdate)
{
	RRegister *regval;
	createValue (regval, name, desc, true, writable ? (RTS2_VALUE_WRITABLE | RTS2_DT_HEX)  : RTS2_DT_HEX);
	regval->setRegisterAddress (address);
	if (infoupdate)
		regval->setInfoUpdate ();

	registers[address] = regval;
	return regval;
}

int Reflex::openInterface (int CLport)
{
	// Clear all previously open interfaces
	closeInterface ();

	//int CLBaudrate = (int) (baudRate->getData ());
	int CLBaudrate = 115200;
#ifdef CL_EDT
	CLHandle = pdv_open (EDT_INTERFACE, 0);
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
	int err = clSerialInit (CLport, &CLHandle);
	if (err != CL_ERR_NO_ERR)
	{
		CLHandle = 0;
		logStream (MESSAGE_ERROR) << "Error opening CameraLink port" << sendLog;
		return -1;
	}
	err = clSetBaudRate (CLHandle, CLBaudrate);
	if (err != CL_ERR_NO_ERR)
	{
		clSerialClose (CLHandle);
		CLHandle = 0;
		logStream (MESSAGE_ERROR) << "Error setting CameraLink baud rate" << sendLog;
		return -1;
	}
#endif
	return 0;
}

int Reflex::closeInterface ()
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

int Reflex::CLRead (char *c)
{
#ifdef CL_EDT
	// Try to read a single character
	return !pdv_serial_read_nullterm (CLHandle, c, 1, false);
#else
#ifdef CL_EURESYS
	unsigned long count = 1;
#else
	uint32_t count = 1;
#endif

	// Try to read a single character
	if ((clSerialRead(CLHandle, (int8_t *)c, &count, 1) == CL_ERR_NO_ERR) && (count == 1))
		return 0;
	// Error
	return 1;
#endif
}

int Reflex::CLReadLine (std::string &s, int timeout)
{
	char c[2];
	time_t t;
	bool bTimeout = false;
	bool bEOL = false;

	s.clear();
	time (&t);
	while (!bTimeout)
	{
		// Check for a timeout
		if ( getNow () - t > timeout )
			bTimeout = true;
		// Check for a new character
		if ( CLRead(c) )
		{
			usleep (USEC_SEC * 0.001);
			continue;
		}
		// End of line?
		if (*c == '\r')
		{
			bEOL = true;
			break;
		}
		c[1] = '\0';
		// Record character
		s.append (c);
	}
	if (bEOL)
	{
		if ((s.length() < 1) || (s[0] != '<'))
			return -1;
		return 0;	// Success
	}
	return -1;	// Error, no EOL or receive timed out
}

int Reflex::CLWriteLine (const char *s, int timeout)
{
#ifdef CL_EURESYS
	unsigned long count;
#else
	uint32_t count;
#endif
	// Write string
	count = strlen(s);
#ifdef CL_EDT
	if (pdv_serial_binary_command (CLHandle, s, count))
		return -1;
	return 0;
#else
	int err = clSerialWrite (CLHandle, (int8_t *) s, &count, timeout);
	if ((err == CL_ERR_NO_ERR) && (count == strlen (s)))
		return 0;
	else
		return -1;
#endif
}

int Reflex::CLFlush ()
{
#ifdef CL_EDT
#else
#ifdef CL_EURESYS
	if (clFlushInputBuffer(CLHandle) != CL_ERR_NO_ERR)
#else
	if (clFlushPort(CLHandle) != CL_ERR_NO_ERR)
#endif
		return 1;
#endif
	return 0;
}

int Reflex::CLCommand (const char *cmd, std::string &response, int timeout, bool log)
{
	int err;

	if (CLFlush())
	{
		logStream (MESSAGE_ERROR) << "Error flushing CameraLink serial port" << sendLog;
		return -1;
	}
	if (CLWriteLine (cmd, timeout))
	{
		logStream (MESSAGE_ERROR) << "Error writing to CameraLink serial port" << sendLog;
		return -1;
	}
	if (log)
		logStream (MESSAGE_DEBUG) << "send " << cmd << sendLog;
	err = CLReadLine (response, timeout);
	if (log)
		logStream (MESSAGE_DEBUG) << "read " << response << sendLog;

	if (err)
	{
		logStream (MESSAGE_ERROR) << "Error reading from CameraLink serial port" << sendLog;
		return -1;
	}
	return 0;
}

int Reflex::interfaceCommand (const char *cmd, std::string &response, int timeout, bool log)
{
	return CLCommand (cmd, response, timeout, log);
}

int Reflex::readRegister(unsigned addr, unsigned& data)
{
	char s[100];
	std::string ret;

	snprintf(s, 100, ">R%08X\r", addr);
	if (interfaceCommand (s, ret, 100))
		return -1;
	if (ret.length() != 9)
		return 1;
	if (!from_string (data, ret.substr (1), std::hex))
	{
		logStream (MESSAGE_ERROR) << "received invalid register value: " << ret << sendLog;
		return -1;
	}
	return 0;
}

int Reflex::writeRegister(unsigned addr, unsigned data)
{
	char s[100];
	std::string ret;

	snprintf (s, 100, ">W%08X%08X\r", addr, data);
	if (interfaceCommand (s, ret, 100))
		return -1;
	return 0;
}

int main (int argc, char **argv)
{
	Reflex device (argc, argv);
	return device.run ();
}
