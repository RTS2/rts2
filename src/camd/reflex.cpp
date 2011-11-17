/* 
 * STA Reflex controller
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 * portions from STA code, Copyright 2011 Semiconductor Technology Associates, Inc.  All rights reserved.
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

#include <iomanip>

#include "camd.h"
#include "valuearray.h"
#include "rts2configraw.h"

// only constants; class is kept in reflex.cpp
#include "reflex.h"

// select type of driver
#define CL_EDT
//#define CL_EURESYS

#ifdef CL_EURESYS

#include <terminal.h>
#define hSerRef void *

#elif defined(CL_EDT)

#include <edtinc.h>
#define CL_BAUDRATE_9600                        9600
#define CL_BAUDRATE_19200                       19200
#define CL_BAUDRATE_38400                       38400
#define CL_BAUDRATE_57600                       57600
#define CL_BAUDRATE_115200                      115200
#define CL_BAUDRATE_230400                      230400
#define CL_BAUDRATE_460800                      460800
#define CL_BAUDRATE_921600                      921600

const int NUM_RING_BUFFERS = 2;

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
 * Reflex state. Describes how to change controller outputs.
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
struct RState
{
	std::string name;

	std::string value_backplane;
	std::string value_interface;

	std::string id_backplane;
	std::string id_interface;	

	std::string daughter_values[MAX_DAUGHTER_COUNT];
	std::string daughter_ids[MAX_DAUGHTER_COUNT];

	// state compiled to be loaded into timing core
	std::string compiled;
};

/**
 * Holds all states, enables quick search on them.
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
class RStates:public std::list <RState>
{
	public:
		RStates ():std::list <RState> () {}

		RStates::iterator findName (std::string sname);
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

		virtual void signaledHUP ();

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
		RRegister * createRegister (uint32_t address, const char *name, const char * desc, bool writable, bool infoupdate, bool hexa);

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

		// related to configuration file..
		const char *configFile;
		Rts2ConfigRaw *config;

		void reloadConfig ();

		// parse states for program
		void parseStates ();

		// compile states, fill compiled member of RState
		void compileStates ();

		// compile program
		void compile ();

		// load timing informations
		void loadTiming ();

		// create board values
		void createBoards ();

		RStates states;

		rts2core::ValueSelection *baudRate;
#ifdef CL_EDT
		PdvDev *CLHandle;
#else
		hSerRef CLHandle;
#endif
};

}

using namespace rts2camd;


RStates::iterator RStates::findName (std::string sname)
{
	RStates::iterator iter;
	for (iter = begin (); iter != end (); iter++)
	{
		if (iter->name == sname)
			return iter;
	}
	return iter;
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

	// interface registers
	createRegister (0x00000000, "int.error_code", "holds the error code fomr the most recent error", false, true, true);
	createRegister (0x00000001, "int.error_source", "holds an integer identifying the source of the most recent error", false, true, false);
	createRegister (0x00000002, "int.error_line", "holds the line number in the interface CPUs", false, true, false);
	createRegister (0x00000003, "int.status_index", "a counter that increments every time the interface CPU polss the system for its status", false, true, false);
	createRegister (0x00000004, "int.power", "current CCD power state", true, true, false);

	createRegister (0x00000005, "int.backplane_type", "board type field for the backplane board", false, false, true);
	createRegister (0x00000006, "int.interface_type", "board type field for the interface board", false, false, true);
	createRegister (0x00000007, "int.powerA_type", "PowerA board type", false, false, true);
	createRegister (0x00000008, "int.powerB_type", "PowerB board type", false, false, true);
	createRegister (0x00000009, "int.daughter1_type", "Daughter 1 board type", false, false, true);
	createRegister (0x0000000A, "int.daughter2_type", "Daughter 2 board type", false, false, true);
	createRegister (0x0000000B, "int.daughter3_type", "Daughter 3 board type", false, false, true);
	createRegister (0x0000000C, "int.daughter4_type", "Daughter 4 board type", false, false, true);
	createRegister (0x0000000D, "int.daughter5_type", "Daughter 5 board type", false, false, true);
	createRegister (0x0000000E, "int.daughter6_type", "Daughter 6 board type", false, false, true);
	createRegister (0x0000000F, "int.daughter7_type", "Daughter 7 board type", false, false, true);
	createRegister (0x00000010, "int.daughter8_type", "Daughter 8 board type", false, false, true);

	createRegister (0x00000011, "int.backplane_ROM", "Backplane ROM ID", false, false, true);
	createRegister (0x00000012, "int.interface_ROM", "Backplane ROM ID", false, false, true);
	createRegister (0x00000013, "int.powerA_ROM", "PowerA ROM ID", false, false, true);
	createRegister (0x00000014, "int.powerB_ROM", "PowerB ROM ID", false, false, true);
	createRegister (0x00000015, "int.daughter1_ROM", "Daughter 1 ROM ID", false, false, true);
	createRegister (0x00000016, "int.daughter2_ROM", "Daughter 2 ROM ID", false, false, true);
	createRegister (0x00000017, "int.daughter3_ROM", "Daughter 3 ROM ID", false, false, true);
	createRegister (0x00000018, "int.daughter4_ROM", "Daughter 4 ROM ID", false, false, true);
	createRegister (0x00000019, "int.daughter5_ROM", "Daughter 5 ROM ID", false, false, true);
	createRegister (0x0000001A, "int.daughter6_ROM", "Daughter 6 ROM ID", false, false, true);
	createRegister (0x0000001B, "int.daughter7_ROM", "Daughter 7 ROM ID", false, false, true);
	createRegister (0x0000001C, "int.daughter8_ROM", "Daughter 8 ROM ID", false, false, true);

	createRegister (0x0000001D, "int.backplane_build", "Backplane firmware build number", false, false, false);
	createRegister (0x0000001E, "int.interface_build", "Backplane firmware build number", false, false, false);
	createRegister (0x0000001F, "int.powerA_build", "PowerA firmware build number", false, false, false);
	createRegister (0x00000020, "int.powerB_build", "PowerB firmware build number", false, false, false);
	createRegister (0x00000021, "int.daughter1_build", "Daughter 1 firmware build number", false, false, false);
	createRegister (0x00000022, "int.daughter2_build", "Daughter 2 firmware build number", false, false, false);
	createRegister (0x00000023, "int.daughter3_build", "Daughter 3 firmware build number", false, false, false);
	createRegister (0x00000024, "int.daughter4_build", "Daughter 4 firmware build number", false, false, false);
	createRegister (0x00000025, "int.daughter5_build", "Daughter 5 firmware build number", false, false, false);
	createRegister (0x00000026, "int.daughter6_build", "Daughter 6 firmware build number", false, false, false);
	createRegister (0x00000027, "int.daughter7_build", "Daughter 7 firmware build number", false, false, false);
	createRegister (0x00000028, "int.daughter8_build", "Daughter 8 firmware build number", false, false, false);

	createRegister (0x00000029, "int.backplane_flags", "Backplane feature flags", false, false, true);
	createRegister (0x0000002A, "int.interface_flags", "Backplane feature flags", false, false, true);
	createRegister (0x0000002B, "int.powerA_flags", "PowerA feature flags", false, false, true);
	createRegister (0x0000002C, "int.powerB_flags", "PowerB feature flags", false, false, true);
	createRegister (0x0000002D, "int.daughter1_flags", "Daughter 1 feature flags", false, false, true);
	createRegister (0x0000002E, "int.daughter2_flags", "Daughter 2 feature flags", false, false, true);
	createRegister (0x0000002F, "int.daughter3_flags", "Daughter 3 feature flags", false, false, true);
	createRegister (0x00000030, "int.daughter4_flags", "Daughter 4 feature flags", false, false, true);
	createRegister (0x00000031, "int.daughter5_flags", "Daughter 5 feature flags", false, false, true);
	createRegister (0x00000032, "int.daughter6_flags", "Daughter 6 feature flags", false, false, true);
	createRegister (0x00000033, "int.daughter7_flags", "Daughter 7 feature flags", false, false, true);
	createRegister (0x00000034, "int.daughter8_flags", "Daughter 8 feature flags", false, false, true);

	createRegister (0x10000000, "sys.timing_lines", "Number of lines of timining data", true, true, false);
	for (int i = 0; i < 64; i++)
	{
		std::ostringstream name, desc;
		name << "sys.tp" << i;
		desc << "20-bit value for timing core parameter " << i;
		createRegister (0x10000001 + i, name.str ().c_str (), desc.str ().c_str (), true, true, true);
	}

	configFile = NULL;
	config = NULL;

	addOption ('c', NULL, 1, "configuration file (.rcf)");
}

Reflex::~Reflex (void)
{
	registers.clear ();

	delete config;

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
	int width, height;
	
	width = getUsedWidth ();
	height = getUsedHeight ();

#ifdef CL_EDT
	int ret = pdv_setsize (CLHandle, width * dataChannels->getValueInteger (), height);
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "call to pdv_setsize failed" << sendLog;
		return -1;
	}
	return 0;
#endif
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
		case 'c':
			configFile = optarg;
			break;
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

	// read all registers
	for (std::map <uint32_t, RRegister *>::iterator iter=registers.begin (); iter != registers.end (); iter++)
	{
		uint32_t rval;
		if (readRegister (iter->first, rval))
		{
			logStream (MESSAGE_ERROR) << "error reading register 0x" << std::setw (8) << std::setfill ('0') << std::hex << iter->first << sendLog;
			return -1;
		}
		iter->second->setValueInteger (rval);
	}

	createBoards ();

	reloadConfig ();
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
				logStream (MESSAGE_ERROR) << "error reading register 0x" << std::setw (8) << std::setfill ('0') << std::hex << iter->first << sendLog;
				return -1;
			}
			iter->second->setValueInteger (rval);
		}
	}
	return Camera::info ();
}

void Reflex::signaledHUP ()
{
	Camera::signaledHUP ();
	reloadConfig ();
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

RRegister * Reflex::createRegister (uint32_t address, const char *name, const char * desc, bool writable, bool infoupdate, bool hexa)
{
	RRegister *regval;
	int32_t flags = (writable ? RTS2_VALUE_WRITABLE : 0) | (hexa ? RTS2_DT_HEX : 0);
	createValue (regval, name, desc, true, flags);
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
	CLHandle = pdv_open ((char *) EDT_INTERFACE, 0);
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
	pdv_set_serial_delimiters (CLHandle, (char *) "", (char *) "");
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
	if (interfaceCommand (s, ret, 100, getDebug ()))
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

void Reflex::reloadConfig ()
{
	if (!configFile)
	{
		logStream (MESSAGE_WARNING) << "empty configuration file (missing -c option?), camera is assumed to be configured" << sendLog;
		return;
	}

	delete config;

	config = new Rts2ConfigRaw ();
	if (config->loadFile (configFile))
	{
		config = NULL;
		throw rts2core::Error ("cannot parse .rcf configuration file");
	}

	parseStates ();
	compileStates ();
}

void Reflex::parseStates ()
{
	if (config == NULL)
		throw rts2core::Error ("empty configuration file");
	states.clear ();
	int count;
	int ret = config->getInteger ("TIMING", "STATES", count);
	if (ret || count < 0)
		throw rts2core::Error ("invalid state count");

	for (int i = 0; i < count; i++)
	{
		RState newState;
		std::ostringstream n;
		n << "STATE" << i;

		ret = config->getString ("TIMING", (n.str () + "NAME").c_str (), newState.name);
		if (ret)
			throw rts2core::Error ("cannot get state name");
		if (states.findName (newState.name) != states.end())
			throw rts2core::Error ("duplicate state name " + newState.name);

		if (config->getString ("TIMING", (n.str () + "\\BACKPLANE\\ID").c_str (), newState.id_backplane)
			|| config->getString ("TIMING", (n.str () + "\\INTERFACE\\ID").c_str (), newState.id_interface)
			|| config->getString ("TIMING", (n.str () + "\\BACKPLANE\\CLOCKS").c_str (), newState.value_backplane)
			|| config->getString ("TIMING", (n.str () + "\\INTERFACE\\CLOCKS").c_str (), newState.value_interface))
			throw rts2core::Error ("cannot find backplane/interface id/clocks");
		
		for (int b = 0; b < MAX_DAUGHTER_COUNT; b++)
		{
			std::ostringstream bn;
			bn << n.str () << "\\BOARD" << (b + 1);

			config->getString ("TIMING", (bn.str () + "\\ID").c_str (), newState.daughter_ids[b], "");
			config->getString ("TIMING", (bn.str () + "\\CLOCKS").c_str (), newState.daughter_values[b], "");
		}

		states.push_back (newState);
	}
}

std::string compileState (std::string value, int bt)
{
	std::ostringstream ret;
	ret.fill ('0');
	ret.setf (std::ios::hex, std::ios::basefield);
	
	unsigned count = 0;
	switch (bt)
	{
		case BT_BPX6:
		case BT_CLIF:
			count = 8;
			break;
		case BT_NONE:
		case BT_BIAS:
			return ret.str ();
		case BT_DRIVER:
			count = 12;
			break;
		case BT_AD8X120:
			count = 5;
			break;
		case BT_AD8X100:
			count = 4;
			break;
		case BT_HS:
			count = 10;
			break;
		default:
			throw rts2core::Error ("unknow board type while compiling states");
	}
	if (value.length () != count)
	{
		std::ostringstream err;
		err << "invalid length of value for board type " << std::hex << bt << ": expected " << count << " characters, string is " << value;
		throw rts2core::Error (err.str ());
	}
	uint16_t clockdata = 0;
	uint16_t keepdata = 0;
	int bit = 1;
	for (unsigned  clock = 0; clock < count; clock++)
	{
		switch (value[clock])
		{
			case '0':
				break;
			case '1':
				clockdata |= bit;
				break;
			case '2':
				keepdata |= bit;
				break;
		}
		bit <<= 1;
	}
	// perform special handling of states..
	switch (bt)
	{
		case BT_AD8X100:
			// Old AD8X120 is CLAMP, FVAL, LVAL, DVAL, PCLK
			// For new AD8X100, FVAL is now SYNC, LVAL is don't care.  Realign DVAL and PCLK to match old timing
			clockdata = (clockdata & 0x3) | ((clockdata & 0xC) << 1);
			keepdata = (keepdata & 0x3) | ((keepdata & 0xC) << 1);
		case BT_BPX6:
		case BT_CLIF:
		case BT_AD8X120:
			ret.width (2);
			ret << clockdata << keepdata;
			break;
		case BT_HS:
			// Slots for LVAL still exists, though FVAL is now sync and LVAL is don't care.  Realign remaining clocks.
			clockdata = (clockdata & 0x1) | ((clockdata & 0x3FE) << 1);
			keepdata = (keepdata & 0x1) | ((keepdata & 0x3FE) << 1);
		case BT_DRIVER:
			ret.width (4);
			ret << clockdata << keepdata;
			break;
	}
	return ret.str ();
}

void Reflex::compileStates ()
{
	for (RStates::iterator iter = states.begin (); iter != states.end (); iter++)
	{
		iter->compiled = compileState (iter->value_backplane, BT_BPX6) + compileState (iter->value_interface, BT_CLIF);
		for (uint32_t i = 0; i < MAX_DAUGHTER_COUNT; i++)
			iter->compiled += compileState (iter->daughter_values[i], registers[BOARD_TYPE_D1 + i]->getValueInteger () << 24);
	}
}

// compile program
void Reflex::compile ()
{

}

// load timing informations
void Reflex::loadTiming ()
{

}

void Reflex::createBoards ()
{
	uint32_t ba;
	int i;

	for (int bt = BOARD_TYPE_BP; bt < BOARD_TYPE_D8; bt++)
	{
		std::ostringstream biss;
		biss << "board" << (bt - BOARD_TYPE_PB) << ".";
		std::string bn = biss.str ();
		switch (registers[bt]->getValueInteger () >> 24)
		{
			case BT_NONE:
				// empty board
				break;
			case BT_BPX6:
				ba = 0x00010000;
				createRegister (ba++, "back.temperature", "[mK] backplane module temperature", false, true, false);
				createRegister (ba, "back.status", "backplane status", false, true, true);

				ba = 0x10010000;
				// control registers
				createRegister (ba++, "MCLK", "[Hz} master clock frequency (60 MHz - 100 MHz)", true, true, false);
				createRegister (ba++, "daughter_enable", "bit-field specifying which AD daughter modules the backplane should read", true, true, false);
				createRegister (ba++, "pair_count", "number of pairs of AD daughter modules the backplane should read", true, true, false);

				createRegister (ba++, "pair_0chA", "daughter module to read on Channel A of Pair 0", true, true, false);
				createRegister (ba++, "pair_0chB", "daughter module to read on Channel B of Pair 0", true, true, false);
				createRegister (ba++, "pair_1chA", "daughter module to read on Channel A of Pair 1", true, true, false);
				createRegister (ba++, "pair_1chB", "daughter module to read on Channel B of Pair 1", true, true, false);
				createRegister (ba++, "pair_2chA", "daughter module to read on Channel A of Pair 2", true, true, false);
				createRegister (ba++, "pair_2chB", "daughter module to read on Channel B of Pair 2", true, true, false);
				createRegister (ba++, "pair_3chA", "daughter module to read on Channel A of Pair 3", true, true, false);
				createRegister (ba++, "pair_3chB", "daughter module to read on Channel B of Pair 3", true, true, false);
				break;
			case BT_CLIF:
				ba = 0x00020000;
				createRegister (ba, "clif.temperature", "[mK] camera link module temperature", false, true, false);

				ba = 0x10020000;
				createRegister (ba++, "clif.tap_count", "number of pairs per active pixel clock", true, true, false);
				createRegister (ba++, "clif.loop_count", "number active pixel clocks per line", true, true, false);

				for (i = 0; i < 32; i++)
				{
					std::ostringstream name, desc;
					name << "clif.tapA" << i << "_start";
					desc << "channel A tap " << i << "start address in line buffer";
					createRegister (ba++, name.str ().c_str (), desc.str ().c_str (), true, true, false);
				}
				for (i = 0; i < 32; i++)
				{
					std::ostringstream name, desc;
					name << "clif.tapB" << i << "_start";
					desc << "channel B tap " << i << "start address in line buffer";
					createRegister (ba++, name.str ().c_str (), desc.str ().c_str (), true, true, false);
				}

				for (i = 0; i < 32; i++)
				{
					std::ostringstream name, desc;
					name << "clif.tapA" << i << "_delta";
					desc << "channel A tap " << i << "delta address in line buffer";
					createRegister (ba++, name.str ().c_str (), desc.str ().c_str (), true, true, false);
				}
				for (i = 0; i < 32; i++)
				{
					std::ostringstream name, desc;
					name << "clif.tapB" << i << "_delta";
					desc << "channel B tap " << i << "delta address in line buffer";
					createRegister (ba++, name.str ().c_str (), desc.str ().c_str (), true, true, false);
				}
				createRegister (ba++, "clif.line_length", "line length in camera clock cycles", true, true, false);
				createRegister (ba++, "clif.precount", "dummy camera link clock cycles before LVAL high", true, true, false);
				createRegister (ba++, "clif.postcount", "dummy camera link clock cycles after LVAL low", true, true, false);
				createRegister (ba++, "clif.line_cunt", "number of camera link lines", true, true, false);
				createRegister (ba++, "clif.idle_ount", "number of idle camera link lines between frames", true, true, false);
				createRegister (ba++, "clif.link_mode", "camera link mode", true, true, false);

				break;
			case BT_PA:
				ba = 0x00030000;
				createRegister (ba++, "pwrA.temperature", "[mK] power module temperature", false, true, false);

				createRegister (ba++, "pwrA.p5VD_V", "[mV] +5V digital supply voltage reading", false, true, false);
				createRegister (ba++, "pwrA.p5VD_A", "[mA] +5V digital supply current reading", false, true, false);

				createRegister (ba++, "pwrA.p5VA_V", "[mV] +5V analog supply voltage reading", false, true, false);
				createRegister (ba++, "pwrA.p5VA_A", "[mA] +5V analog supply current reading", false, true, false);

				createRegister (ba++, "pwrA.m5VA_V", "[mV] -5V analog supply voltage reading", false, true, false);
				createRegister (ba, "pwrA.m5VA_A", "[mA] -5V analog supply current reading", false, true, false);
				break;
			case BT_PB:
				ba = 0x00040000;
				createRegister (ba++, "pwrB.temperature", "[mK] power module temperature", false, true, false);

				createRegister (ba++, "pwrB.p30VA_V", "[mV] +30V analog supply voltage reading", false, true, false);
				createRegister (ba++, "pwrB.p30VA_A", "[mA] +30V analog supply current reading", false, true, false);

				createRegister (ba++, "pwrB.p15VA_V", "[mV] +51V analog supply voltage reading", false, true, false);
				createRegister (ba++, "pwrB.p15VA_A", "[mA] +15V analog supply current reading", false, true, false);

				createRegister (ba++, "pwrB.m15VA_V", "[mV] -15V analog supply voltage reading", false, true, false);
				createRegister (ba++, "pwrB.m15VA_A", "[mA] -15V analog supply current reading", false, true, false);

				createRegister (ba++, "pwrB.TEC_set", "[mK] monitored value of TEC setpoint", false, true, false);
				createRegister (ba, "pwrB.TEC_actual", "[mK] current TEC temperature", false, true, false);

				ba = 0x10040000;
				createRegister (ba++, "pwrB.TEC_set", "[mK] TEC setpoint (zero to disable)", true, true, false);
				break;
			case BT_AD8X120:
			case BT_AD8X100:
				ba = (bt - POWER) << 16;
				createRegister (ba++, (bn + "temperature").c_str (), "[mK] module temperature", false, true, false);
				createRegister (ba, (bn + "status").c_str (), "module status", false, true, true);

				// control registers
				ba = 0x10000000 | ((bt - POWER) < 16);
				createRegister (ba++, (bn + "clamp_low").c_str (), "[mV] clamp voltage for negative side pf differential input", true, true, false);
				createRegister (ba++, (bn + "clamp_high").c_str (), "[mV] clamp voltage for positive side pf differential input", true, true, false);

				createRegister (ba++, (bn + "raw_mode").c_str (), "CDS/raw mode", true, true, false);
				createRegister (ba++, (bn + "raw_channel").c_str (), "AD channel to stream in raw mode, 0-7", true, true, false);
				createRegister (ba++, (bn + "cds_gain").c_str (), "[M] fixed point gain to apply in CDS mode (0x00010000 is gain 1.0)", true, true, true);
				createRegister (ba++, (bn + "cds_offset").c_str (), "fixed point offset to apply in CDS mode", true, true, false);
				createRegister (ba++, (bn + "hdr_mode").c_str (), "Normal/HDR (32 bit) sample mode", true, true, false);

				createRegister (ba++, (bn + "cds_reset_start").c_str (), "starting sample to use for CDS reset level after pixel clock", true, true, false);
				createRegister (ba++, (bn + "cds_reset_stop").c_str (), "last sample to use for CDS reset level after pixel clock", true, true, false);
				createRegister (ba++, (bn + "cds_video_start").c_str (), "starting sample to use for CDS video level after pixel clock", true, true, false);
				createRegister (ba++, (bn + "cda_video_stop").c_str (), "last sample to use for CDS video level after pixel clock", true, true, false);
				break;
			case BT_DRIVER:
				ba = (bt - POWER) << 16;
				createRegister (ba++, (bn + "temperature").c_str (), "[mK] module temperature", false, true, false);
				createRegister (ba, (bn + "status").c_str (), "module status", false, true, true);

				ba = 0x10000000 | ((bt - POWER) << 16);
				createRegister (ba++, (bn + "driver_enable").c_str (), "bit-field indetifing which driver channels to enable", true, true, true);

				for (i = 1; i < 13; i++)
				{
					std::ostringstream n;
					n << i;
					createRegister (ba++, (bn + "ch" + n.str () + "_low").c_str (), ("[mV] low level clock voltage for channel " + n.str ()).c_str (), true, true, false);
					createRegister (ba++, (bn + "ch" + n.str () + "_high").c_str (), ("[mV] high level clock voltage for channel " + n.str ()).c_str (), true, true, false);
					createRegister (ba++, (bn + "ch" + n.str () + "_slew").c_str (), ("[mV] clock slew rate for channel " + n.str ()).c_str (), true, true, false);
				}
				break;
			case BT_BIAS:
				ba = (bt - POWER) << 16;
				createRegister (ba++, (bn + "temperature").c_str (), "[mK] module temperature", false, true, false);
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << "meas_LV" << i;
					comment << "[mv] Low-voltage bias #" << i << " voltage";
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << "meas_HV" << i;
					comment << "[mv] High-voltage bias #" << i << " voltage";
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << "meas_LC" << i;
					comment << "[uA] Low voltage bias #" << i << " current";
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << "meas_HC" << i;
					comment << "[uA] High-voltage bias #" << i << " current";
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false);
				}

				ba = 0x10000000 | ((bt - POWER) << 16);
				createRegister (ba++, (bn + "bias_enable").c_str (), "bit field identifing which biash channels to enable", true, true, true);

				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << "LV" << i;
					comment << "[mv] DC bias level for LV " << i;
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << "HV" << i;
					comment << "[mv] DC bias level for HV " << i;
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << "LC" << i;
					comment << "[uA] current limit for LV " << i;
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << "HC" << i;
					comment << "[uA] current limit for HV " << i;
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false);
				}
				break;
			default:
				logStream (MESSAGE_ERROR) << "unknow board type " << std::hex << (registers[bt]->getValueInteger () >> 24) << sendLog;
		}
	}
}

int main (int argc, char **argv)
{
	Reflex device (argc, argv);
	return device.run ();
}
