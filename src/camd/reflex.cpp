/* 
 * STA Reflex controller
 * Copyright (C) 2011,2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 *
 * includes code from STA ReflexControl and ReflexCapture
 * Copyright 2011 Semiconductor Technology Associates, Inc.  All rights reserved.
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
#include "iniparser.h"

#define OPT_DRY           OPT_LOCAL + 1
#define OPT_POWERUP       OPT_LOCAL + 2

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

// parameters expected at Reflex configuration file.
// please see man rts2-camd-reflex for details.
#define pLCLK          "LCLK"
#define pNCLEAR        "NCLEAR"

#define pROWSKIP       "ROWSKIP"
#define pROWREAD       "ROWREAD"
#define pROWOVER       "ROWOVER"

#define pPIXSKIP       "PIXSKIP"
#define pPIXREAD       "PIXREAD"
#define pPIXOVER       "PIXOVER"

#define pEXPO          "EXPO"
#define nEXPO          "NEXPO"
#define pEXPTIME       "EXPTIME"
#define pLIGHT         "LIGHT"

#define TWAIT          "TWAIT"

const int NUM_RING_BUFFERS = 2;

#else

#include <clallserial.h>

#endif


namespace rts2camd
{

typedef enum {
	CONVERSION_NONE,	// no conversion
	CONVERSION_MILI,	// /= 1000.0
	CONVERSION_MILI_LIMITS, // /= 1000.0, but with limits
	CONVERSION_mK, 		// convert from mK to degC
	CONVERSION_10000hex,	// 0x10000
	CONVERSION_65k		// 65535
} conversion_t;

/**
 * Register class. Holds register address in Reflex controller.
 */
class RRegister
{
	public:
		RRegister (rts2core::Value *_value, conversion_t _conv)
		{
			reflex_addr = 0;
			info_update = false;
			conv = _conv;
			value = _value;
		}

		void setRegisterAddress (uint32_t addr) { reflex_addr = addr; }
		void setInfoUpdate () { info_update = true; }
		conversion_t getConversionType () { return conv; }

		bool infoUpdate () { return info_update; }

		void setRegisterValue (uint32_t rval)
		{
			switch (getConversionType ())
			{
				case CONVERSION_mK:
					((rts2core::ValueFloat *) value)->setValueFloat (((int32_t) rval) / 1000.0 - 273.15);
					break;
				case CONVERSION_MILI:
					((rts2core::ValueFloat *) value)->setValueFloat (((int32_t) rval) / 1000.0);
					break;
				case CONVERSION_MILI_LIMITS:
					((rts2core::ValueDoubleMinMax *) value)->setValueDouble (((int32_t) rval) / 1000.0);
					break;
				case CONVERSION_10000hex:
					((rts2core::ValueFloat *) value)->setValueFloat (((double) rval) / 0x10000);
					break;
				case CONVERSION_65k:
					((rts2core::ValueFloat *) value)->setValueFloat (((int32_t) rval) / 65536.0);
					break;
				case CONVERSION_NONE:
					value->setValueInteger (rval);
					break;
			}
		}

		rts2core::Value *value;
	private:
		uint32_t reflex_addr;
		/**
		 * If value should be update on info call.
		 */
		bool info_update;
		conversion_t conv;
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
	unsigned int address;
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
 * http://www.sta-inc.net/reflex for controller details.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class Reflex:public Camera
{
	public:
		Reflex (int in_argc, char **in_argv);
		virtual ~Reflex (void);

		virtual int initValues ();

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int processOption (int in_opt);
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual void beforeRun ();
		virtual void initBinnings ();
		virtual void initDataTypes ();

		virtual int initHardware ();

		virtual int info ();

		virtual void signaledHUP ();

		virtual int startExposure ();
		virtual int stopExposure ();
		virtual long isExposing ();
		virtual int readoutStart ();

		virtual size_t suggestBufferSize () { return 0; }

		virtual int doReadout ();
		virtual int endReadout ();

	private:
		// if true, don't write anything
		bool dry_run;

		// if controller should be powered up after startup
		bool powerUp;

		// last tap parameters
		int last_taplength;
		int last_height;

		/**
		 * Register map. Index is register address.
		 */
		std::map <uint32_t, RRegister *> registers;

		/**
		 * High-level command to add register to configuration.
		 *
		 * @return 
		 */
		RRegister * createRegister (uint32_t address, const char *name, const char * desc, bool writable, bool infoupdate, bool hexa, conversion_t conv = CONVERSION_NONE, bool vdebug = false);

		int openInterface (int port);
		int closeInterface ();


		int CLRead (char *c);
		int CLReadLine (std::string &s, int timeout);
		int CLWriteLine (const char *s, int timeout);

		int CLFlush ();

		int CLCommand (const char *cmd, std::string &response, int timeout, bool log);
		int interfaceCommand (const char *cmd, std::string &response, int timeout, bool action, bool log = true);

		/**
		 * Access refelex registers.
		 */
		int readRegister (uint32_t addr, uint32_t& data);
		int writeRegister (uint32_t addr, uint32_t data);

		void setParameter (const char *pname, uint32_t value);

		/**
		 * Update value of all registers in memory.
		 */
		void rereadAllRegisters ();

		int sendChannel (int chan, u_char *buf, int chanorder, int totalchanel);

		// related to configuration file..
		const char *configFile;
		rts2core::IniParser *config;

		void reloadConfig (bool force);

		// parse states for program
		void parseStates ();
		// compile states, fill compiled member of RState
		void compileStates ();

		// parse parameters
		void parseParameters ();

		/**
		 * Find index of a parameter.
		 *
		 * @return parameter index, or -1 if parameter cannot be found
		 */
		int parameterIndex (const std::string pname);

		void defaultParameters ();

		// compile program
		void compile ();

		// load timing informations
		void loadTiming ();

		void createBoard (int bt);

		// create board values
		void createBoards ();

		// configure routines
		void configSystem ();

		// configure channels - taps - for given length
		void configureTaps (int taplength, int height, bool raw, bool hdrmode);

		void configBoard (int board);
		void configHDR (int board, bool raw, bool hdrmode);
		void configTEC ();

		RStates states;
		std::vector <std::string> program;
		std::vector <std::string> code;

		std::vector <std::pair <std::string, uint32_t> > parameters;

#ifdef CL_EDT
		rts2core::ValueInteger *edt_count;
		rts2core::ValueInteger *edt_bytes;
		int last_bytes;
		time_t next_bytes_check;
		double readout_started;
#endif
		rts2core::ValueBool *rawmode;
		rts2core::ValueSelection *baudRate;

		// holds board/adc pair for each channel
		// there are MAX_TAP_COUNT channels pairs (so MAX_TAP_COUNT * 2)
		std::pair <int, int> channeltap[MAX_TAP_COUNT * 2];

		// return board type for given board
		int boardType (int board) { return (registers[BOARD_TYPE_BP + board]->value->getValueInteger () >> 24) & 0xFF; }
		int boardRevision (int board) { return (registers[BOARD_TYPE_BP + board]->value->getValueInteger () >> 16) & 0xFF; }
#ifdef CL_EDT
		PdvDev *CLHandle;
#else
		hSerRef CLHandle;
#endif

		// indexes for counting boards
		int ad;
		int driver;
		int bias;

		// map board names to board indices
		std::map <std::string, int> bnames2number;
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
	dry_run = false;
	powerUp = false;
	last_taplength = -1;
	last_height = -1;

	ad = driver = bias = 0;

	createExpType ();

	createDataChannels ();
	setNumChannels (1);

#ifdef CL_EDT
	createValue (edt_count, "edt_count", "number of done images", false);
	createValue (edt_bytes, "edt_bytes", "bytes read from the image", false);
	last_bytes = 0;
#endif
	createValue (rawmode, "raw_mode", "RAW readout mode", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	rawmode->setValueBool (false);

	createValue (baudRate, "baud_rate", "CL baud rate", true, 0, CAM_WORKING);
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
	createRegister (0x00000003, "int.status_index", "a counter that increments every time the interface CPU polls the system for its status", false, true, false);
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

	createRegister (SYSTEM_CONTROL_ADDR, "sys.timing_lines", "Number of lines of timining data", true, true, false);

	configFile = NULL;
	config = NULL;

	addOption ('c', NULL, 1, "configuration file (.rcf)");
	addOption (OPT_DRY, "dry-run", 0, "don't perform any writes or commands");
	addOption (OPT_POWERUP, "power-up", 0, "auto power up controller");
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

void Reflex::initDataTypes ()
{
	addDataType (RTS2_DATA_USHORT);
	addDataType (RTS2_DATA_ULONG);
	setDataTypeWritable ();
}

int Reflex::startExposure ()
{
	int width, height;

	if (rawmode->getValueBool ())
	{
		width = getWidth ();
		height = getUsedHeight ();

		setUsedWidth (width * channels->size ());
	}
	else
	{
		width = getUsedWidth ();
		height = getUsedHeight ();
	}
	bool hdrmode = getDataType () == RTS2_DATA_ULONG;

#ifdef CL_EDT
	int ret;
	std::cout << "chan " << dataChannels->getValueInteger () << std::endl;
	if (rawmode->getValueBool ())
		ret = pdv_setsize (CLHandle, width * channels->size (), height);
	else if (hdrmode)
		ret = pdv_setsize (CLHandle, width * dataChannels->getValueInteger () * 2, height);
	else
		ret = pdv_setsize (CLHandle, width * dataChannels->getValueInteger (), height);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "call to pdv_setsize failed" << sendLog;
		return -1;
	}
	ret = pdv_auto_set_roi (CLHandle);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "call to pdv_auto_set failed" << sendLog;
		return -1;
	}
	pdv_flush_fifo (CLHandle);
	//ret = pdv_multibuf (CLHandle, 1); // NUM_RING_BUFFERS);
	ret = pdv_set_buffers (CLHandle, 1, NULL);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "pdv_multibuf call failed" << sendLog;
		return -1;
	}

  	pdv_set_timeout (CLHandle, 30000);

	pdv_start_image (CLHandle);
#endif
	std::string s;
	//interfaceCommand (">TH\r", s, 3000, true);

	setParameter (pEXPTIME, getExposure () * 1000);

	setParameter (pROWSKIP, chipTopY ());
	setParameter (pROWREAD, height);
	setParameter (pROWOVER, getHeight () - chipTopY () - height);

	setParameter (pPIXSKIP, chipTopX ());
	setParameter (pPIXREAD, width - 1);
	setParameter (pPIXOVER, getWidth () - chipTopX () - width);

	setParameter (pLIGHT, getExpType () ? 0 : 1);

	setParameter (pEXPO, 1);
	setParameter (nEXPO, 1);
	setParameter (pLCLK, 0);

	if (rawmode->getValueBool ())
	{
		setParameter (TWAIT, 1000);
	}
	else
	{
		setParameter (TWAIT, 100);
	}

	configureTaps (chipUsedReadout->getWidthInt (), chipUsedReadout->getHeightInt (), rawmode->getValueBool (), hdrmode);
	for (int i = 0; i < MAX_DAUGHTER_COUNT; i++)
		configHDR (i, rawmode->getValueBool (), hdrmode);

	interfaceCommand (">TP\r", s, 3000, true);

	return 0;
}

int Reflex::stopExposure ()
{
	return Camera::stopExposure ();
}

long Reflex::isExposing ()
{
	return Camera::isExposing ();
}

int Reflex::readoutStart ()
{
#ifdef CL_EDT
	readout_started = getNow () + 30;
	last_bytes = 0;
	next_bytes_check = getNow () + 5;
#endif
	return Camera::readoutStart ();
}

int Reflex::doReadout ()
{
#ifdef CL_EDT
	// still waiting for an image..
	if (edt_done_count (CLHandle) == 0)
	{
		double n = getNow ();
		if (readout_started < n)
		{
			logStream (MESSAGE_ERROR) << "timeout in doReadout" << sendLog;
			return -1;
		}
		edt_bytes->setValueInteger (edt_get_bytecount (CLHandle));
		if (next_bytes_check < n)
		{
			if (edt_bytes->getValueInteger () == last_bytes)
			{
				logStream (MESSAGE_ERROR) << "number of bytes received was not increased in last two seconds, aborting on " << last_bytes << " bytes read" << sendLog;
				return -1;
			}
			last_bytes = edt_bytes->getValueInteger ();
			next_bytes_check = n + 2;
			sendValueAll (edt_bytes);
		}
		return 100;
	}

	edt_bytes->setValueInteger (edt_get_bytecount (CLHandle));
	sendValueAll (edt_bytes);

	int ta = pdv_timeouts (CLHandle);
	//pdv_start_image (CLHandle);

	unsigned char * buf = pdv_wait_image (CLHandle);

	int tb = pdv_timeouts (CLHandle);

	updateReadoutSpeed (getReadoutPixels ());
	if (ta < tb)
	{
		logStream (MESSAGE_ERROR) << "timeout during readout" << sendLog;
		return -1;
	}

	// single channel raw data
	if (rawmode->getValueBool ())
	{
		int ret = sendReadoutData ((char *) buf, getWriteBinaryDataSize(0), 0);
		if (ret < 0)
			return -1;
	}
	else
	{
		size_t i,j;
		int lw = getUsedWidth () * 2;
		int dw = lw * channels->size ();
		// buffer for a single channel
		unsigned char chanbuf[lw * getUsedHeight ()];
		for (i = 0, j = 0; i < channels->size (); i++)
		{
			if ((*channels)[i])
			{
				// copy channel to channel buffer
				// line start pointer
				unsigned char *ls = buf + lw * i;
				unsigned char *ds = chanbuf;
				for (int l = 0; l < getUsedHeight (); l++)
				{
					memcpy (ds, ls, lw);
					ls += dw;
					ds += lw;
				}
				int ret = sendChannel (i, chanbuf, j, dataChannels->getValueInteger ());
				if (ret < 0)
					return -1;
				j++;
			}
		}
	}
#endif
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
		case OPT_DRY:
			dry_run = true;
			break;
		case OPT_POWERUP:
			powerUp = true;
			break;
		default:
			return Camera::processOption (in_opt);
	}
	return 0;
}

int Reflex::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	int ret;
	for (std::map <uint32_t, RRegister *>::iterator iter=registers.begin (); iter != registers.end (); iter++)
	{
		if (old_value == iter->second->value)
		{
			std::string s;
			switch (iter->first)
			{
				// special registers first
				case 0x00000004:
					ret = interfaceCommand ((new_value->getValueInteger () > 0 ? ">P1\r" : ">P0\r"), s, 5000, true) ? -2 : 0;
					if (ret)
						return -2;
					info ();
					if (new_value->getValueInteger () > 0)
						return interfaceCommand (">TS\r", s, 5000, true) ? -2 : 0;
					// don't start timing if powering down
					return 0;
				default:
					switch (iter->second->getConversionType ())
					{
						case CONVERSION_NONE:
							return writeRegister (iter->first, new_value->getValueInteger ()) ? -2 : 0;
						case CONVERSION_MILI:
						case CONVERSION_MILI_LIMITS:
							return writeRegister (iter->first, new_value->getValueFloat () * 1000.0) ? -2 : 0;
						case CONVERSION_10000hex:
							return writeRegister (iter->first, new_value->getValueFloat () * 0x10000) ? -2 : 0;
						case CONVERSION_65k:
							return writeRegister (iter->first, new_value->getValueFloat () * 65536.0) ? -2 : 0;
						case CONVERSION_mK:
							return writeRegister (iter->first, (new_value->getValueFloat () + 273.15) * 1000.0) ? -2 : 0;
					}
			}
		}
	}
	if (old_value == rawmode)
	{
		if (((rts2core::ValueBool *) new_value)->getValueBool ())
		{
			channels->setValueBool (0, true);
			for (size_t i = 1; i < channels->size (); i++)
				channels->setValueBool (i, false);
		}
		else
		{
			for (size_t i = 0; i < channels->size (); i++)
				channels->setValueBool (i, true);
			setUsedWidth (getWidth ());
		}
	}
	return Camera::setValue (old_value, new_value);
}

int Reflex::initHardware ()
{
	int ret;
	ret = openInterface (0);
	if (ret)
		return ret;

	rereadAllRegisters ();

	if (!configFile)
	{
		logStream (MESSAGE_WARNING) << "empty configuration file (missing -c option?), camera is assumed to be configured" << sendLog;
		return -1;
	}

	config = new rts2core::IniParser ();
	if (config->loadFile (configFile, true))
	{
		config = NULL;
		throw rts2core::Error ("cannot parse .rcf configuration file");
	}

	createBoards ();

	rereadAllRegisters ();

	reloadConfig (false);

	if (powerUp)
	{
		std::string s;
		ret = interfaceCommand (">P1\r", s, 5000, true);
		if (ret)
			return -1;
		ret = interfaceCommand (">TS\r", s, 5000, true) ? -1 : 0;
		if (ret)
			return -1;
	}

	/*ret = pdv_set_buffers (CLHandle, 1, NULL);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot initialize buffers" << sendLog;
		return -1;
	}*/

	setIdleInfoInterval (2);

	return 0;
}

int Reflex::info ()
{
#ifdef CL_EDT
	edt_count->setValueInteger (edt_done_count (CLHandle));
	edt_bytes->setValueInteger (edt_get_bytecount (CLHandle));
#endif
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
			iter->second->setRegisterValue (rval);
		}
	}
	return Camera::info ();
}

void Reflex::signaledHUP ()
{
	Camera::signaledHUP ();

	reloadConfig (true);
}

int Reflex::initValues ()
{
	return Camera::initValues ();
}

int Reflex::commandAuthorized (rts2core::Connection * conn)
{
	std::string s;
	if (conn->isCommand ("warmboot"))
	{
		if (!conn->paramEnd ())
			return -2;
		return interfaceCommand (">WARMBOOT\r", s, 100, true);
	}
	else if (conn->isCommand ("reboot"))
	{
		if (!conn->paramEnd ())
			return -2;
		return interfaceCommand (">REBOOT\r", s, 5000, true);
	}
	else if (conn->isCommand ("standby"))
	{
		if (!conn->paramEnd ())
			return -2;
		return interfaceCommand (">STANDBY\r", s, 5000, true);
	}
	else if (conn->isCommand ("resume"))
	{
		if (!conn->paramEnd ())
			return -2;
		return interfaceCommand (">RESUME\r", s, 5000, true);
	}
	else if (conn->isCommand ("clear"))
	{
		if (!conn->paramEnd ())
			return -2;
		return interfaceCommand (">E\r", s, 100, true);
	}
        else if (conn->isCommand ("configure_timing"))
        {
                if (!conn->paramEnd ())
                        return -2;
                return interfaceCommand (">T\r", s, 3000, true);
        }
        else if (conn->isCommand ("load_timing"))
        {
                if (!conn->paramEnd ())
                        return -2;
                return interfaceCommand (">TP\r", s, 3000, true);
        }
	else if (conn->isCommand ("apply"))
	{
		if (!conn->paramEnd ())
		{
			char* board;
			if (conn->paramNextString (&board) || !conn->paramEnd ())
				return -2;
			std::map <std::string, int>::iterator bit = bnames2number.find (std::string (board));
			if (bit == bnames2number.end ())
				return -2;
				
			char cmd[5] = ">Bx\r";
			cmd[2] = bit->second - 5 + '0';
			return interfaceCommand (cmd, s, 3000, true);
		}
		return interfaceCommand (">A\r", s, 3000, true);
	}
	return Camera::commandAuthorized (conn);
}

RRegister * Reflex::createRegister (uint32_t address, const char *name, const char * desc, bool writable, bool infoupdate, bool hexa, conversion_t conv, bool vdebug)
{
	if (registers.find (address) != registers.end ())
	{
		logStream (MESSAGE_ERROR) << "register with address " << std::hex << address << " already created! " << name << sendLog;
		exit (1);
	}

	int32_t flags = (writable ? RTS2_VALUE_WRITABLE : 0) | (vdebug ? RTS2_VALUE_DEBUG : 0);
	rts2core::Value *value;
	switch (conv)
	{
		case CONVERSION_MILI:
		case CONVERSION_mK:
		case CONVERSION_10000hex:
		case CONVERSION_65k:
			createValue ((rts2core::ValueFloat *&) value, name, desc, true, flags);
			break;
		case CONVERSION_MILI_LIMITS:
			createValue ((rts2core::ValueDoubleMinMax *&) value, name, desc, true, flags);
			break;
		case CONVERSION_NONE:
			if (hexa)
	 			flags |= RTS2_DT_HEX;
			createValue ((rts2core::ValueInteger *&) value, name, desc, true, flags);
			break;
	}

	RRegister *regval = new RRegister (value, conv);
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

int Reflex::interfaceCommand (const char *cmd, std::string &response, int timeout, bool action, bool log)
{
	if (action && dry_run)
	{
		logStream (MESSAGE_DEBUG) << "would send " << cmd << sendLog;
		return 0;
	}
	return CLCommand (cmd, response, timeout, log);
}

int Reflex::readRegister (unsigned addr, unsigned& data)
{
	char s[100];
	std::string ret;

	snprintf(s, 100, ">R%08X\r", addr);
	if (interfaceCommand (s, ret, 100, false, getDebug ()))
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

int Reflex::writeRegister (unsigned addr, unsigned data)
{
	char s[100];
	std::string ret;

	snprintf (s, 100, ">W%08X%08X\r", addr, data);
	if (interfaceCommand (s, ret, 100, true))
		return -1;
	return 0;
}

void Reflex::setParameter (const char *pname, uint32_t value)
{
	int pi = parameterIndex (pname);
	if (pi < 0)
		throw rts2core::Error (std::string ("cannot find parameter with name ") + pname);
	if (writeRegister (SYSTEM_CONTROL_ADDR + 1 + pi, value) < 0)
		throw rts2core::Error (std::string ("cannot set parameter ") + pname);

	// propagate changed parameter
	rts2core::Value *v = registers[SYSTEM_CONTROL_ADDR + 1 + pi]->value;
	v->setValueInteger (value);
	sendValueAll (v);
}

void Reflex::rereadAllRegisters ()
{
	// read all registers
	for (std::map <uint32_t, RRegister *>::iterator iter=registers.begin (); iter != registers.end (); iter++)
	{
		uint32_t rval;
		if (readRegister (iter->first, rval))
		{
			std::ostringstream err;
			err << "error reading register 0x" << std::setw (8) << std::setfill ('0') << std::hex << iter->first;
			throw rts2core::Error (err.str ());
		}
		iter->second->setRegisterValue (rval);
	}
}

int Reflex::sendChannel (int chan, u_char *buf, int chanorder, int totalchanel)
{
	return sendReadoutData ((char *) buf, getWriteBinaryDataSize (chanorder), chanorder);
}

void Reflex::reloadConfig (bool force)
{
	if (config == NULL || force)
	{
		config = new rts2core::IniParser ();
		if (config->loadFile (configFile, true))
		{
			config = NULL;
			throw rts2core::Error ("cannot parse .rcf configuration file");
		}
	}

	ad = driver = bias = 0;

	configSystem ();

	configTEC ();
	
	for (int i = 0; i < MAX_DAUGHTER_COUNT; i++)
		configBoard (i);

	parseStates ();
	compileStates ();

	parseParameters ();

	compile ();

	loadTiming ();

	std::string s;
	if (interfaceCommand (">A\r", s, 5000, true))
		throw rts2core::Error ("Failed to configure all after loading timing");

	rereadAllRegisters ();
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
		std::transform (newState.name.begin(), newState.name.end(), newState.name.begin(), (int(*)(int)) std::toupper);
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


/**
 * Eat spaces from beginning of tokens string.
 */
char nextChar (std::string &tokens)
{
	unsigned int i;
	for (i = 0; i < tokens.length (); i++)
	{
		if (!isspace (tokens[i]))
			break;
	}
	if (i > 0)
		tokens = tokens.substr (i);
	if (tokens.empty ())
		return 0;
	return tokens[0];
}

/**
 * Return next token; tokens are space separated entitites
 */
void getToken (std::string &tokens, std::string &token)
{
	unsigned int i;
	token = std::string ("");
	// eat white space..
	nextChar (tokens);
	for (i = 0; i < tokens.length (); i++)
	{
		if (!isalnum (tokens[i]))
			break;
		token.push_back(tokens[i]);
	}
	tokens = tokens.substr (i);
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
	ret << std::uppercase;
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
			ret << std::setw (2) << clockdata << std::setw (2) << keepdata;
			break;
		case BT_HS:
			// Slots for LVAL still exists, though FVAL is now sync and LVAL is don't care.  Realign remaining clocks.
			clockdata = (clockdata & 0x1) | ((clockdata & 0x3FE) << 1);
			keepdata = (keepdata & 0x1) | ((keepdata & 0x3FE) << 1);
		case BT_DRIVER:
			ret << std::setw (4) << clockdata << std::setw (4) << keepdata;
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
			iter->compiled += compileState (iter->daughter_values[i], boardType (BOARD_DAUGHTERS + i));
	}
}

void Reflex::parseParameters ()
{
	parameters.clear ();
	int pcount;
	if (config->getInteger ("TIMING", "PARAMETERS", pcount) || pcount < 0) 
		throw rts2core::Error ("cannot get number of parameter lines (TIMING/PARAMETERS");
	for (int l = 0; l < pcount; l++)
	{
		std::string line;
		std::string token;

		std::ostringstream ln;
		ln << "PARAMETER" << l;

		if (config->getString ("TIMING", ln.str ().c_str (), line))
			throw rts2core::Error ("cannot retrieve TIMING/" + ln.str ());
		getToken (line, token);
		if (token.empty () || token[0] == '#')
			continue;
		if (parameterIndex (token) >= 0)
			throw rts2core::Error ("Multiple declaration of parameter with name " + token);
		if (nextChar (line) != '=')
			throw rts2core::Error ("Parameters must be of the form NAME = VALUE");
		std::string pname = token;
		line = line.substr (1);
		getToken (line, token);
		char *endptr;
		uint32_t value = strtol (token.c_str (), &endptr,0);
		if (*endptr)
			throw rts2core::Error ("Invalid number " + token);
		std::transform (pname.begin(), pname.end(), pname.begin(), (int(*)(int)) std::toupper);
		parameters.push_back (std::pair <std::string, uint32_t> (pname, value));
	}
}

int Reflex::parameterIndex (const std::string pname)
{
	int ret = 0;
	for (std::vector <std::pair <std::string, uint32_t> >::iterator iter = parameters.begin (); iter != parameters.end (); iter++)
	{
		if (iter->first == pname)
			return ret;
		ret++;
	}
	return -1;
}

void Reflex::defaultParameters ()
{
	int i = 1;
	for (std::vector <std::pair <std::string, uint32_t> >::iterator iter = parameters.begin (); iter != parameters.end (); i++, iter++)
	{
		if (registers.find (SYSTEM_CONTROL_ADDR + i) == registers.end ())
		{
			std::ostringstream name, desc;
			name << "sys." << iter->first;
			desc << "20-bit value for timing core parameter " << i;
			createRegister (SYSTEM_CONTROL_ADDR + i, name.str ().c_str (), desc.str ().c_str (), true, true, false);
		}

		rts2core::Value *v = registers[SYSTEM_CONTROL_ADDR + i]->value;
		v->setValueInteger (iter->second);
		sendValueAll (v);
		writeRegister (SYSTEM_CONTROL_ADDR + i, iter->second);
	}
}

// compile program
void Reflex::compile ()
{
	program.clear ();
	// assign to program lines
	// the lines will be then converted to code..
	int linecount, lnum;
	unsigned int address = 0;
	
	if (config->getInteger ("TIMING", "LINES", linecount) || linecount < 0)
		throw rts2core::Error ("invalid number of lines (TIMING/LINES value in config file)");

	for (lnum = 0; lnum < linecount; lnum++)
	{
		std::ostringstream linename;
		linename << "LINE" << lnum;
		std::string pl;
		if (config->getString ("TIMING", linename.str ().c_str (), pl))
			throw rts2core::Error ("missing " + linename.str ());

		std::transform (pl.begin(), pl.end(), pl.begin(), (int(*)(int)) std::toupper);
		program.push_back (pl);
	}

	// name of label and its address..
	std::map <std::string, int> labels;

	// constant and its value
	std::map <std::string, uint32_t> constants;

	std::vector <std::string>::iterator iter;
	std::string token;
	// first compile step - get rid of comments, get constants and labels
	for (lnum = 0, iter = program.begin (); iter < program.end (); iter++, lnum++)
	{
		std::string line = *iter;
		if (getDebug ())
			logStream (MESSAGE_DEBUG) << "compile1 " << lnum << ": " << line << sendLog;

		getToken (line, token);
		// ignore empty lines
		if (token.empty () || token[0] == '#')
		{
			*iter = std::string ("");
		}
		// label..
		else if (!line.empty () && (line[0] == ':'))
		{
			if (labels.find (token) != labels.end ())
				throw rts2core::Error ("multiple declaration of label " + token);
			labels[token] = address;
			*iter = std::string ("");
		}
		// SET expression - constants
		else if (token == "SET")
		{
			getToken (line, token);
			if (token.empty ())
				throw rts2core::Error ("missing parameter name");
			if (constants.find (token) != constants.end ())
				throw rts2core::Error ("multiple declaration of constant " + token);
			char *endptr;
			constants[token] = strtol (line.c_str (), &endptr, 0);
			if (*endptr && !isspace (*endptr))
				throw rts2core::Error ("invalid number " + line);
			*iter = std::string ("");
		}
		else
		{
			// check state name
			if (states.findName (token) == states.end ())
				throw rts2core::Error ("cannot find state " + token);
			address++;
			continue;
		}
	}

	// assign address to states
	for (RStates::iterator siter = states.begin (); siter != states.end (); siter++)
	{
		siter->address = address;
		address++;
	}

	// second pass - emit code
	address = 0;

	code.clear ();

	for (lnum = 0, iter = program.begin (); iter != program.end (); iter++)
	{
		int opcode = 0;
		int condparam = 0;
		int jump = 0;
		int count = 0;
		int countparam = 0;
		int decparam = 0;

		bool ifflag = false;
		bool branchflag = false;

		if (iter->empty ())
			continue;
		address++;

		if (getDebug ())
			logStream (MESSAGE_DEBUG) << "compile2 " << lnum << ": " << *iter << sendLog;

		getToken (*iter, token);
		std::string compiled_state = states.findName (token)->compiled;

		if ((*iter)[0] != ';')
			goto encode;
		*iter = iter->substr (1);
		getToken (*iter, token);
		if (token.empty ())
			goto encode;
		if (token == "IF")
		{
			ifflag = true;
			// check for ! (not)
			if (nextChar (*iter) == '!')
			{
				*iter = iter->substr (1);
				opcode |= 0x10; // Branch if parameter is zero
			}
			else
			{
				opcode |= 0x18; // Branch if parameter is not zero
			}
			getToken (*iter, token);
			condparam = parameterIndex (token);
			if (condparam < 0)
				throw rts2core::Error ("unknow parameter " + token);
			getToken (*iter, token);
		}
		if (token == "GOTO")
		{
			branchflag = true;
			getToken (*iter, token);
			if (labels.find (token) == labels.end ())
				throw rts2core::Error ("unknown label " + token + " for GOTO");
			opcode |= 0x01;
			jump = labels[token];
		}
		else if (token == "CALL" || (states.findName (token) != states.end ()))
		{
			branchflag = true;
			if (token == "CALL")
			{
				getToken (*iter, token);
				if (labels.find (token) == labels.end ())
					throw rts2core::Error ("unknown label " + token + " for CALL");
				jump = labels[token];
			}
			else
			{
				jump = states.findName (token)->address;
			}
			count = 1; // default count if no count specified
			opcode |= 0x02; // CALL
			if (nextChar (*iter) == '(')
			{
				*iter = iter->substr (1);
				getToken (*iter, token);
				// argument is a constant
				if (constants.find (token) != constants.end ())
				{
					count = constants[token];
				}
				// argument is a parameter register
				else if ((countparam = parameterIndex (token)) >= 0)
				{
					opcode |= 0x20; // use parameter for count
				}
				else
				{
					countparam = 0;
					char *endptr;
					count = strtol (token.c_str (), &endptr, 0);
					if (*endptr)
						throw rts2core::Error ("invalid number " + token + " for call repeat index");
				}
				if ((count < 1) || (count > 1048575))
					throw rts2core::Error ("COUNT for CALL LABEL(COUNT) must be 1-1048575");
				if (nextChar (*iter) != ')')
					throw rts2core::Error ("missing closing parenthesis");
			}
		}
		else if (ifflag)
		{
			throw rts2core::Error ("IF not followed by GOTO or CALL");
		}
		else if (token == "RETURN")
		{
			branchflag = true;
			getToken (*iter, token);
			if (labels.find (token) == labels.end ())
				throw rts2core::Error ("unknow label " + token + " for RETURN");
			jump = labels[token];
			opcode |= 0x04; // RETURN
		}
		// check for parameter decrement
		if (branchflag)
		{
			if (nextChar (*iter) != ';')
				goto encode;
			getToken (*iter, token);
		}
		if (iter->empty () || token.empty ())
			goto encode;
		decparam = parameterIndex (token);
		if (decparam < 0)
			throw rts2core::Error ("unknow parameter " + token);
		if ((nextChar (*iter) != '-') || (nextChar (*iter) != '-'))
			throw rts2core::Error ("error parsing decrement parameter (should be " + token + "--)");
		opcode |= 0x40; // Decrement parameter
	encode:
		std::ostringstream os;
		os.fill ('0');
		os.setf (std::ios::hex, std::ios::basefield);
		os << std::uppercase << std::setw (2) << opcode << std::setw (2) << condparam << std::setw (2) << countparam << std::setw (2) << decparam << std::setw (4) << jump << std::setw (6) << count << compiled_state;
		code.push_back (os.str ());
	}
	// Encode states
	for (RStates::iterator siter = states.begin (); siter != states.end (); siter++)
	{
		std::ostringstream os;
		os.fill ('0');
		os.setf (std::ios::hex, std::ios::basefield);
		os << "04000000" << std::uppercase << std::setw (4) << siter->address << "000000" << siter->compiled;
		code.push_back (os.str ());
	}
}

// load timing informations
void Reflex::loadTiming ()
{
	std::string s;
	if (interfaceCommand (">TA000\r", s, 100, true))
		throw rts2core::Error ("Error seting timing load address");
	writeRegister (SYSTEM_CONTROL_ADDR, code.size ());
	for (std::vector <std::string>::iterator iter = code.begin (); iter != code.end (); iter++)
	{
		std::ostringstream os;
		os << ">T" << *iter << '\r';
		if (interfaceCommand (os.str ().c_str (), s, 100, true))
			throw rts2core::Error ("Error loading timing line");
	}

	defaultParameters ();

	if (interfaceCommand (">T\r", s, 3000, true))
		throw rts2core::Error ("Error applying timing");
}

void Reflex::createBoard (int bt)
{
	uint32_t ba;
	int i;
	std::ostringstream biss, bns;
	biss << "BOARD" << (bt - BOARD_TYPE_PB);
	std::string key = biss.str ();
	std::string bn = biss.str () + ".";

	switch ((registers[bt]->value->getValueInteger () >> 24) & 0xFF)
	{
		case BT_NONE:
			// empty board
			break;
		case BT_BPX6:
			ba = 0x00010000;
			createRegister (ba++, "back.temperature", "[C] backplane module temperature", false, true, false, CONVERSION_mK);
			createRegister (ba, "back.status", "backplane status", false, true, true);

			ba = 0x10010000;
				// control registers
			createRegister (ba++, "MCLK", "[Hz} master clock frequency (60 MHz - 100 MHz)", true, false, false);
			createRegister (ba++, "daughter_enable", "bit-field specifying which AD daughter modules the backplane should read", true, false, false);
			createRegister (ba++, "pair_count", "number of pairs of AD daughter modules the backplane should read", true, false, false);

			createRegister (ba++, "pair_0chA", "daughter module to read on Channel A of Pair 0", true, false, false, CONVERSION_NONE, true);
			createRegister (ba++, "pair_0chB", "daughter module to read on Channel B of Pair 0", true, false, false, CONVERSION_NONE, true);
			createRegister (ba++, "pair_1chA", "daughter module to read on Channel A of Pair 1", true, false, false, CONVERSION_NONE, true);
			createRegister (ba++, "pair_1chB", "daughter module to read on Channel B of Pair 1", true, false, false, CONVERSION_NONE, true);
			createRegister (ba++, "pair_2chA", "daughter module to read on Channel A of Pair 2", true, false, false, CONVERSION_NONE, true);
			createRegister (ba++, "pair_2chB", "daughter module to read on Channel B of Pair 2", true, false, false, CONVERSION_NONE, true);
			createRegister (ba++, "pair_3chA", "daughter module to read on Channel A of Pair 3", true, false, false, CONVERSION_NONE, true);
			createRegister (ba++, "pair_3chB", "daughter module to read on Channel B of Pair 3", true, false, false, CONVERSION_NONE, true);
			break;
		case BT_CLIF:
			ba = 0x00020000;
			createRegister (ba, "clif.temperature", "[C] camera link module temperature", false, true, false, CONVERSION_mK);

			ba = 0x10020000;
			createRegister (ba++, "clif.tap_count", "number of pairs per active pixel clock", true, false, false, CONVERSION_NONE, true);
			createRegister (ba++, "clif.loop_count", "number active pixel clocks per line", true, false, false, CONVERSION_NONE, true);

			for (i = 0; i < 32; i++)
			{
				std::ostringstream name, desc;
				name << "clif.tapA" << i << "_start";
				desc << "channel A tap " << i << " start address in line buffer";
				createRegister (ba++, name.str ().c_str (), desc.str ().c_str (), true, false, false, CONVERSION_NONE, true);
			}
			for (i = 0; i < 32; i++)
			{
				std::ostringstream name, desc;
				name << "clif.tapB" << i << "_start";
				desc << "channel B tap " << i << " start address in line buffer";
				createRegister (ba++, name.str ().c_str (), desc.str ().c_str (), true, false, false, CONVERSION_NONE, true);
			}

			for (i = 0; i < 32; i++)
			{
				std::ostringstream name, desc;
				name << "clif.tapA" << i << "_delta";
				desc << "channel A tap " << i << " delta address in line buffer";
				createRegister (ba++, name.str ().c_str (), desc.str ().c_str (), true, false, false, CONVERSION_NONE, true);
			}
			for (i = 0; i < 32; i++)
			{
				std::ostringstream name, desc;
				name << "clif.tapB" << i << "_delta";
				desc << "channel B tap " << i << " delta address in line buffer";
				createRegister (ba++, name.str ().c_str (), desc.str ().c_str (), true, false, false, CONVERSION_NONE, true);
			}
			createRegister (ba++, "clif.line_length", "line length in camera clock cycles", true, false, false);
			createRegister (ba++, "clif.precount", "dummy camera link clock cycles before LVAL high", true, false, false);
			createRegister (ba++, "clif.postcount", "dummy camera link clock cycles after LVAL low", true, false, false);
			createRegister (ba++, "clif.line_count", "number of camera link lines", true, false, false);
			createRegister (ba++, "clif.idle_count", "number of idle camera link lines between frames", true, false, false);
			createRegister (ba++, "clif.link_mode", "camera link mode", true, false, false);

			break;
		case BT_PA:
			ba = 0x00030000;
			createRegister (ba++, "pwrA.temperature", "[C] power module temperature", false, true, false, CONVERSION_mK);

			createRegister (ba++, "pwrA.p5VD_V", "[V] +5V digital supply voltage reading", false, true, false, CONVERSION_MILI);
			createRegister (ba++, "pwrA.p5VD_A", "[mA] +5V digital supply current reading", false, true, false);

			createRegister (ba++, "pwrA.p5VA_V", "[V] +5V analog supply voltage reading", false, true, false, CONVERSION_MILI);
			createRegister (ba++, "pwrA.p5VA_A", "[mA] +5V analog supply current reading", false, true, false);

			createRegister (ba++, "pwrA.m5VA_V", "[V] -5V analog supply voltage reading", false, true, false, CONVERSION_MILI);
			createRegister (ba, "pwrA.m5VA_A", "[mA] -5V analog supply current reading", false, true, false);
			break;
		case BT_PB:
			ba = 0x00040000;
			createRegister (ba++, "pwrB.temperature", "[C] power module temperature", false, true, false, CONVERSION_mK);

			createRegister (ba++, "pwrB.p30VA_V", "[V] +30V analog supply voltage reading", false, true, false, CONVERSION_MILI);
			createRegister (ba++, "pwrB.p30VA_A", "[mA] +30V analog supply current reading", false, true, false);

			createRegister (ba++, "pwrB.p15VA_V", "[V] +51V analog supply voltage reading", false, true, false, CONVERSION_MILI);
			createRegister (ba++, "pwrB.p15VA_A", "[mA] +15V analog supply current reading", false, true, false);

			createRegister (ba++, "pwrB.m15VA_V", "[V] -15V analog supply voltage reading", false, true, false, CONVERSION_MILI);
			createRegister (ba++, "pwrB.m15VA_A", "[mA] -15V analog supply current reading", false, true, false);

			createRegister (ba++, "pwrB.TEC_mon", "[C] monitored value of TEC setpoint", false, true, false, CONVERSION_mK);
			createRegister (ba, "pwrB.TEC_actual", "[C] current TEC temperature", false, true, false, CONVERSION_mK);

			ba = 0x10040000;
			createRegister (ba++, "pwrB.TEC_set", "[C] TEC setpoint (zero to disable)", true, false, false, CONVERSION_mK);
			break;
		case BT_AD8X120:
		case BT_AD8X100:
			ba = (bt - POWER) << 16;

			bns << "AD_" << ++ad;
			bnames2number[bns.str ()] = bt;
			bn = bns.str () + '.';

			createRegister (ba++, (bn + "temperature").c_str (), "[C] module temperature", false, true, false, CONVERSION_mK);
			createRegister (ba, (bn + "status").c_str (), "module status", false, true, true);

			// control registers
			ba = SYSTEM_CONTROL_ADDR | ((bt - POWER) << 16);
			createRegister (ba++, (bn + "clamp_low").c_str (), "[V] clamp voltage for negative side pf differential input", true, false, false, CONVERSION_MILI);
			createRegister (ba++, (bn + "clamp_high").c_str (), "[V] clamp voltage for positive side pf differential input", true, false, false, CONVERSION_MILI);

			createRegister (ba++, (bn + "raw_mode").c_str (), "CDS/raw mode", false, false, false);
			createRegister (ba++, (bn + "raw_channel").c_str (), "AD channel to stream in raw mode, 0-7", true, false, false);
			createRegister (ba++, (bn + "cds_gain").c_str (), "fixed point gain to apply in CDS mode (0x00010000 is gain 1.0)", true, false, true, CONVERSION_10000hex);
			createRegister (ba++, (bn + "cds_offset").c_str (), "fixed point offset to apply in CDS mode", true, false, true, ((registers[bt]->value->getValueInteger () >> 24) & 0xFF) == BT_AD8X100 ? CONVERSION_65k : CONVERSION_NONE);
			createRegister (ba++, (bn + "hdr_mode").c_str (), "Normal/HDR (32 bit) sample mode", true, false, false);

			createRegister (ba++, (bn + "cds_reset_start").c_str (), "starting sample to use for CDS reset level after pixel clock", true, false, false);
			createRegister (ba++, (bn + "cds_reset_stop").c_str (), "last sample to use for CDS reset level after pixel clock", true, false, false);
			createRegister (ba++, (bn + "cds_video_start").c_str (), "starting sample to use for CDS video level after pixel clock", true, false, false);
			createRegister (ba++, (bn + "cda_video_stop").c_str (), "last sample to use for CDS video level after pixel clock", true, false, false);
			break;
		case BT_DRIVER:
			{
				ba = (bt - POWER) << 16;

				bns << "DRV_" << ++driver;
				bnames2number[bns.str ()] = bt;
				bn = bns.str () + '.';

				createRegister (ba++, (bn + "temperature").c_str (), "[C] module temperature", false, true, false, CONVERSION_mK);
				createRegister (ba, (bn + "status").c_str (), "module status", false, true, true);

				ba = SYSTEM_CONTROL_ADDR | ((bt - POWER) << 16);
				createRegister (ba++, (bn + "driver_enable").c_str (), "bit-field indetifing which driver channels to enable", true, false, true);
				// used labels
				std::vector <std::string> labels;
				// labels handling
				rts2core::StringArray *vlabels;
				createValue (vlabels, (bn + "labels").c_str (), "driver board channel labels", false);

				for (i = 1; i < 13; i++)
				{
					std::ostringstream ln;
					ln << "LABEL" << i;
					std::string ll = std::string (config->getStringDefault (key.c_str (), ln.str ().c_str (), ""));
					std::ostringstream n;
					n << "ch" << i;
					if (ll[0] == '\0' || std::find (labels.begin (), labels.end (), std::string (ll)) != labels.end ())
						ll = n.str ();
					createRegister (ba++, (bn + ll + "_low").c_str (), ("[V] low level clock voltage for channel " + n.str ()).c_str (), true, false, false, CONVERSION_MILI_LIMITS);
					createRegister (ba++, (bn + ll + "_high").c_str (), ("[V] high level clock voltage for channel " + n.str ()).c_str (), true, false, false, CONVERSION_MILI_LIMITS);
					createRegister (ba++, (bn + ll + "_slew").c_str (), ("[V] clock slew rate for channel " + n.str ()).c_str (), true, false, false, CONVERSION_MILI_LIMITS);
					vlabels->addValue (ll);
					labels.push_back (std::string (ll));
				}
				break;
			}
		case BT_BIAS:
			{
				ba = (bt - POWER) << 16;

				bns << "BIAS_" << ++bias;
				bnames2number[bns.str ()] = bt;
				bn = bns.str () + '.';

				createRegister (ba++, (bn + "temperature").c_str (), "[C] module temperature", false, true, false, CONVERSION_mK);
				createRegister (ba++, (bn + "status").c_str (), "module status", false, true, true);
				// 8 low, 8 high labels
				std::string labels[16];
				rts2core::StringArray *vlabels;
				createValue (vlabels, (bn + "labels").c_str (), "BIAS board output labels", false);

				// labels
				char ln[4] = "LL1";
				char def_l[3] = "L1";
				for (i = 1; i < 9; i++)
				{
					ln[2] = i + '0';
					def_l[1] = i + '0';
					const char *ll = config->getStringDefault (key.c_str (), ln, "");
					if (ll[0] == '\0')
						ll = def_l;
					labels[i-1] = std::string (ll);
					vlabels->addValue (labels[i-1]);
				}
				ln[0] = def_l[0] = 'H';
				for (i = 1; i < 9; i++)
				{
					ln[2] = i + '0';
					def_l[1] = i + '0';
					const char *ll = config->getStringDefault (key.c_str (), ln, "");
					if (ll[0] == '\0')
						ll = def_l;
					labels[i+7] = std::string (ll);
					vlabels->addValue (labels[i+7]);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << labels[i-1] << "_Vmeas";
					comment << "[V] Low-voltage bias #" << i << " voltage";
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false, CONVERSION_MILI);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << labels[i+7] << "_Vmeas";
					comment << "[V] High-voltage bias #" << i << " voltage";
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false, CONVERSION_MILI);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << labels[i-1] << "_Cmeas";
					comment << "[uA] Low voltage bias #" << i << " current";
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << labels[i+7] << "_Cmeas";
					comment << "[uA] High-voltage bias #" << i << " current";
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), false, true, false);
				}

				ba = SYSTEM_CONTROL_ADDR | ((bt - POWER) << 16);
				createRegister (ba++, (bn + "bias_enable").c_str (), "bit field identifing which biash channels to enable", true, false, true);

				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << labels[i-1] << "_V";
					comment << "[V] DC bias level for LV " << i;
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), true, false, false, CONVERSION_MILI_LIMITS);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << labels[i+7] << "_V";
					comment << "[V] DC bias level for HV " << i;
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), true, false, false, CONVERSION_MILI_LIMITS);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << labels[i-1] << "_C";
					comment << "[uA] current limit for LV " << i;
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), true, false, false);
				}
				for (i = 1; i < 9; i++)
				{
					std::ostringstream vname, comment;
					vname << bn << labels[i+7] << "_C";
					comment << "[uA] current limit for HV " << i;
					createRegister (ba++, vname.str ().c_str (), comment.str ().c_str (), true, false, false);
				}
			}
			break;
		default:
			logStream (MESSAGE_ERROR) << "unknow board type " << std::hex << ((registers[bt]->value->getValueInteger () >> 24) & 0xFF) << sendLog;
	}
}

void Reflex::createBoards ()
{
	int numchan = 0;
	for (int bt = BOARD_TYPE_BP; bt < BOARD_TYPE_D8; bt++)
	{
		createBoard (bt);
		// count channels
		switch ((registers[bt]->value->getValueInteger () >> 24) & 0xFF)
		{
			case BT_AD8X100:
			case BT_AD8X120:
				numchan += 8;
				break;
		}
	}
	
	setNumChannels (numchan);
}

void Reflex::configSystem ()
{
	int i;
	std::string s;
	// System config
	double mclk;
	int pclk, trigin, taplength, height;

	if (! (registers[BOARD_TYPE_BP]->value->getValueInteger () && (BT_BPX6 << 24)))
		throw rts2core::Error ("Unknown backplane type");

//	// Disable polling to speed up configuration
//	if (interfaceCommand(">L0\r", s, 100, true, false))
//		throw rts2core::Error ("Error disabling polling")

	// Get target master clock frequency
	if (config->getDouble ("SYSTEM", "MASTERCLOCK", mclk, 0) || (mclk < 60.0) || (mclk > 100.0))
		throw rts2core::Error ("Invalid master clock setting (must be 60 - 100 MHz)");
	i = mclk * 1000000;

	// Set master clock register
	if (writeRegister (SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_MCLK, i))
		throw rts2core::Error ("Error writing Backplane master clock register");

	// Pixel clock divider
	if (boardRevision (BOARD_BP) > 1)
	{
		if (registers[BUILD_IF]->value->getValueInteger () >= 336)
		{
			if (config->getInteger ("SYSTEM", "PIXELCLOCK", pclk, 0) || (pclk < 0) || (pclk > 16))
				throw rts2core::Error ("Invalid pixel clock setting (must be 0 - 16)");
			pclk--;
			pclk = (pclk < 0 ? 0 : (pclk > 15 ? 15: pclk));
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_REVB_PCLK, pclk))
				throw rts2core::Error ("Error writing Backplane pixel clock register");
		}
		
		// Trigger in
		if (registers[BUILD_IF]->value->getValueInteger () >= 354)
		{
			config->getString ("SYSTEM", "TRIGGERIN", s, "Disabled");
			if (s == "Normal")
				trigin = 1;
			else if (s == "Inverted")
				trigin = 3;
			else
				trigin = 0;
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_REVB_TRIGIN, trigin))
				throw rts2core::Error ("Error writing Backplane trigger in register");
		}
	}

	// Set backplane deinterlacing

	// Get tap length in samples (subtract 1 since 0 = 1 sample, 1 = 2 samples, etc)
	if (config->getInteger ("BACKPLANE", "TAPLENGTH", taplength) || taplength <= 0)
		throw rts2core::Error ("Invalid tap length");

	// Get padding
	config->getInteger ("BACKPLANE", "PREPADDING", i, -1);
	if (boardRevision (BOARD_BP) == 1)
	{
		if (i < 0)
			throw rts2core::Error ("Invalid prepadding length");
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_PRE_COUNT, i))
			throw rts2core::Error ("Error writing Backplane deinterlacing prepadding");
	}
	else
	{
		if (i <= 0)
			throw rts2core::Error ("Invalid prepadding length");
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | INTERFACE_REVC_PRE_COUNT, i - 1))
			throw rts2core::Error ("Error writing Interface deinterlacing prepadding");
	}

	config->getInteger ("BACKPLANE", "POSTPADDING", i, -1);
	if (boardRevision (BOARD_BP) == 1)
	{
		if (i < 0)
			throw rts2core::Error ("Invalid postpadding length");
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_POST_COUNT, i))
			throw rts2core::Error ("Error writing Backplane deinterlacing postpadding");
	}
	else
	{
		if (i <= 0)
			throw rts2core::Error ("Invalid postpadding length");
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | INTERFACE_REVC_POST_COUNT, i - 1))
			throw rts2core::Error ("Error writing Interface deinterlacing postpadding");
	}

	if (config->getInteger ("BACKPLANE", "LINECOUNT", height) || height <= 0)
		throw rts2core::Error ("Invalid line count");

	// one channel size
	setSize (taplength, height, 0, 0);

	configureTaps (taplength, height, false, false);
}

void Reflex::configureTaps (int taplength, int height, bool raw, bool hdrmode)
{
	if (taplength == last_taplength && last_height == height)
		return;

	int i;
	std::string s;

	int board, adc, lines;
	char dir;

	bool tapenable[MAX_DAUGHTER_COUNT][MAX_ADC_COUNT];
	int tapstart[MAX_DAUGHTER_COUNT][MAX_ADC_COUNT];
	int tapdelta[MAX_DAUGHTER_COUNT][MAX_ADC_COUNT];

	// Prototype systems had deinterlacing engine on backplane
	if (boardRevision (BOARD_BP) == 1)
	{
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_TAP_LENGTH, taplength - 1))
			throw rts2core::Error ("Error writing Backplane deinterlacing tap length");
	}
	else
	{
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | INTERFACE_REVC_LOOP_COUNT, taplength - 1))
			throw rts2core::Error ("Error writing Interface deinterlacing tap length");
	}

	// All taps initially disabled
	for (board = 0; board < MAX_DAUGHTER_COUNT; board++)
		for (adc = 0; adc < MAX_ADC_COUNT; adc++)
		{
			tapenable[board][adc] = false;
			tapstart[board][adc] = 0;
			tapdelta[board][adc] = 0;
		}

	// Examine tap order
	bool cds = false;
	int tapcount = 0;

	if (config->getInteger ("BACKPLANE", "TAPCOUNT", lines) || lines < 0)
		throw rts2core::Error ("Invalid tap count");

	int ct = 0;

	if (raw)
	{
/*		if (s.length() != 4)
			throw rts2core::Error ("Invalid tap entry");
		if (cds)
			throw rts2core::Error ("Cannot mix CDS and raw taps");
		raw = true; */
		board = 5;
		if ((board < 0) || (board >= MAX_DAUGHTER_COUNT))
			throw rts2core::Error ("Invalid tap entry");
		if (tapenable[board][0])
			throw rts2core::Error ("Duplicate tap entry");
		tapenable[board][0] = true;
		channeltap[ct++] = std::pair <int, int> (board, 7);
		tapstart[board][0] = tapcount * taplength;
		tapdelta[board][0] = 1;

		tapcount++;
	}
	else
	{
		for (i = 0; i < lines; i++)
		{
			std::ostringstream os;
			os << "TAP" << i;
			config->getString ("BACKPLANE", os.str ().c_str (), s);
			std::transform (s.begin(), s.end(), s.begin(), (int(*)(int)) std::toupper);
			if (s.empty())
				continue;
			// Check for raw taps
			if (s.substr (0, 3) == "RAW")
			{
				throw rts2core::Error ("cannot configure RAW mode at config file - you should configure it with parameters");
			}
			// Normal CDS taps
			else
			{
			/*	if (raw)
					throw rts2core::Error ("Cannot mix CDS and raw taps"); */
				if (s.length() != 3)
					throw rts2core::Error ("Invalid tap entry");
				board = s[0] - 'A';
				if ((board < 0) || (board >= MAX_DAUGHTER_COUNT))
					throw rts2core::Error ("Invalid tap entry");
				cds = true;
				adc = s[1] - '1';
				if ((adc < 0) || (adc >= MAX_ADC_COUNT))
					throw rts2core::Error ("Invalid tap entry");
				dir = s[2];
				if ((dir != 'L') && (dir != 'R'))
					throw rts2core::Error ("Invalid tap entry");
				if (tapenable[board][adc])
					throw rts2core::Error ("Duplicate tap entry");
				tapenable[board][adc] = true;
				channeltap[ct++] = std::pair <int, int> (board, adc);
				if (dir == 'L')
				{
					tapstart[board][adc] = tapcount * taplength;
					tapdelta[board][adc] = 1;
				}
				else
				{
					tapstart[board][adc] =  (tapcount + 1) * taplength - 1;
					tapdelta[board][adc] = -1;
				}
			}
			tapcount++;
		}
	}
	if (tapcount == 0)
		throw rts2core::Error ("No taps selected");

	// Find which boards need to be enabled for reading
	int u = 0;
	int boardsusedcount = 0;
	int boardsused[MAX_DAUGHTER_COUNT];

	for (board = 0; board < MAX_DAUGHTER_COUNT; board++)
	{
		bool used = false;
		for (adc = 0; adc < MAX_ADC_COUNT; adc++)
			if (tapenable[board][adc])
				used = true;
		if (used)
		{
			u |= 1 << board;
			boardsused[boardsusedcount] = board;
			boardsusedcount++;
		}
	}
	if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_DAUGHTER_ENABLE, u))
		throw rts2core::Error ("Error writing Backplane daughter board enable bits");

	// Set number of daughter boards production backplane must read from, and read order
	if (boardRevision (BOARD_BP) > 1)
	{
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_REVB_TAP_COUNT, ((boardsusedcount - 1) / 2)))
			throw rts2core::Error ("Error writing Backplane deinterlacing tap loop count");
		for (i = 0; i < (boardsusedcount + 1) / 2; i++)
		{
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | (BACKPLANE_REVB_TAPA_SOURCE0 + i), 1 << boardsused[i * 2]))
				throw rts2core::Error ("Error writing Backplane deinterlacing tap sources");
			if ((i * 2 + 1) < boardsusedcount)
			{
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | (BACKPLANE_REVB_TAPB_SOURCE0 + i), 1 << boardsused[i * 2 + 1]))
					throw rts2core::Error ("Error writing Backplane deinterlacing tap sources");
			}
			else
			{
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | (BACKPLANE_REVB_TAPB_SOURCE0 + i), 0))
					throw rts2core::Error ("Error writing Backplane deinterlacing tap sources");
			}
		}
	}

	int loopcount;

	// Set tap engine loop count
	if (raw)
	{
		// In raw mode an ADC board streams a single channel continuously
		loopcount = (tapcount + 1) / 2;
	}

	//// Get HDR mode (0 = Normal 16 bits / pixel, 1 = HDR 32 bits / pixel)
	//config->getString ("BACKPLANE", "HDRMODE", s);
	//bool hdrmode = (s == "1");

	if (cds)
	{
		if (hdrmode)
			// In HDR CDS mode an ADC board sends groups of 16 samples (two 16 bit samples for each of 8 channels) - all 16 must be read, even if some taps are ignored/discarded
			loopcount = ((boardsusedcount + 1) / 2) * 16;
		else
			// In normal CDS mode an ADC board sends groups of 8 samples - all 8 must be read, even if some taps are ignored/discarded
			loopcount = ((boardsusedcount + 1) / 2) * 8;
	}
	// Subtract 1 from loop count since 0 = 1 loop, 1 = 2 loops, etc
	if (boardRevision (BOARD_BP) == 1)
	{
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_TAP_COUNT, loopcount - 1))
			throw rts2core::Error ("Error writing Backplane deinterlacing tap count");
	}
	else
	{
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | INTERFACE_REVC_TAP_COUNT, loopcount - 1))
			throw rts2core::Error ("Error writing Interface deinterlacing tap count");
	}

	int tap;
	unsigned tapaenable, tapbenable;
	int tapasource[MAX_TAP_COUNT], tapbsource[MAX_TAP_COUNT];
	int tapastart[MAX_TAP_COUNT], tapbstart[MAX_TAP_COUNT];
	int tapadelta[MAX_TAP_COUNT], tapbdelta[MAX_TAP_COUNT];

	int clmode, cllength, clspeed;

	// Calculate tap engine enables, sources, starts and deltas for prototype backplane
	if (boardRevision (BOARD_BP) == 1)
	{
		tap = 0;
		tapaenable = 0;
		tapbenable = 0;
		for (i = 0; i < MAX_PROTOTYPE_TAP_COUNT; i++)
		{
			tapasource[i] = 0;
			tapbsource[i] = 0;
		}
		for (i = 0; i < boardsusedcount; i++)
		{
			// Source of 1 is slot A, etc, while board is zero-based
			board = boardsused[i];
			if (raw)
			{
				// Even taps go to engine A, odd taps to engine B
				if (tap % 2 == 0)
				{
					tapasource[tap / 2] = board + 1;
					tapaenable |= 1 << (tap / 2);
					tapastart[tap / 2] = tapstart[board][0];
					tapadelta[tap / 2] = tapdelta[board][0];
				}
				else
				{
					tapbsource[tap / 2] = board + 1;
					tapbenable |= 1 << (tap / 2);
					tapbstart[tap / 2] = tapstart[board][0];
					tapbdelta[tap / 2] = tapdelta[board][0];
				}
				tap++;
			}
			if (cds)
			{
				for (adc = 0; adc < MAX_ADC_COUNT; adc++)
				{
					// Even boards go to engine A, odd taps to engine B
					if (i % 2 == 0)
					{
						tapasource[tap + adc] = board + 1;
						if (tapenable[board][adc])
							tapaenable |= 1 << (tap + adc);
						tapastart[tap + adc] = tapstart[board][adc];
						tapadelta[tap + adc] = tapdelta[board][adc];
					}
					else
					{
						tapbsource[tap + adc] = board + 1;
						if (tapenable[board][adc])
							tapbenable |= 1 << (tap + adc);
						tapbstart[tap + adc] = tapstart[board][adc];
						tapbdelta[tap + adc] = tapdelta[board][adc];
					}
				}
				// After filling 8 taps on engine A and B, move to the next set of 8
				if (i % 2)
					tap += 8;
			}
		}
		// Write tap engine A and B source words (8 tap sources per 32-bit control word)
		for (tap = 0; tap < MAX_PROTOTYPE_TAP_COUNT / 8; tap++)
		{
			unsigned a = 0;
			unsigned b = 0;
			int bit = 0;
			for (i = 0; i < 8; i++)
			{
				a |= tapasource[tap * 8 + i] << bit;
				b |= tapbsource[tap * 8 + i] << bit;
				bit += 4;
			}
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | (BACKPLANE_TAPA_7_0_SOURCE + tap), a))
				throw rts2core::Error ("Error writing Backplane deinterlacing tap A source");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | (BACKPLANE_TAPB_7_0_SOURCE + tap), b))
				throw rts2core::Error ("Error writing Backplane deinterlacing tap B source");
		}
		// Write tap engine A and B enables (each bit of 32-bit control word controls one tap enable)
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_TAPA_ENABLE, tapaenable))
			throw rts2core::Error ("Error writing Backplane deinterlacing tap A enables");
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_TAPB_ENABLE, tapbenable))
			throw rts2core::Error ("Error writing Backplane deinterlacing tap B enables");
		// Write tap engine A and B start and delta control words
		for (tap = 0; tap < loopcount; tap++)
		{
			// Only write start and delta for enabled taps
			if (tapaenable & (1 << tap))
			{
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | (BACKPLANE_TAPA_0_START + tap), tapastart[tap]))
					throw rts2core::Error ("Error writing Backplane deinterlacing tap A start");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | (BACKPLANE_TAPA_0_DELTA + tap), tapadelta[tap]))
					throw rts2core::Error ("Error writing Backplane deinterlacing tap A delta");
			}
			if (tapbenable & (1 << tap))
			{
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | (BACKPLANE_TAPB_0_START + tap), tapbstart[tap]))
					throw rts2core::Error ("Error writing Backplane deinterlacing tap B start");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | (BACKPLANE_TAPB_0_DELTA + tap), tapbdelta[tap]))
					throw rts2core::Error ("Error writing Backplane deinterlacing tap B delta");
			}
		}
		// Write number of 32-bit reads necessary to read an entire line (16-bit tap count x tap length / 2)
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_BP + 1) << 16) | BACKPLANE_READ_COUNT, tapcount * taplength / 2 - 1))
			throw rts2core::Error ("Error writing Backplane deinterlacing read count");
	}
	// Calculate tap engine enables, sources, starts and deltas for production system
	else
	{
		tap = 0;
		tapaenable = 0;
		tapbenable = 0;
		if (raw)
		{
			for (i = 0; i < boardsusedcount; i++)
			{
				board = boardsused[i];
				// Even taps go to engine A, odd taps to engine B
				if (tap % 2 == 0)
				{
					tapaenable |= 1 << (tap / 2);
					tapastart[tap / 2] = tapstart[board][0];
					tapadelta[tap / 2] = tapdelta[board][0];
				}
				else
				{
					tapbenable |= 1 << (tap / 2);
					tapbstart[tap / 2] = tapstart[board][0];
					tapbdelta[tap / 2] = tapdelta[board][0];
				}
				tap++;
			}
		}
		if (cds)
		{
			// HDR CDS
			if (hdrmode)
			{
				taplength *= 2;	// CameraLink lines must be twices as long
				for (adc = 0; adc < MAX_ADC_COUNT * 2; adc++)
				{
					for (i = 0; i < boardsusedcount + boardsusedcount % 2; i++)
					{
						if (i < boardsusedcount)
						{
							board = boardsused[i];
							// MSB 16 bit samples
							if (adc < MAX_ADC_COUNT)
							{
								// Even taps go to engine A, odd taps to engine B
								if (tap % 2 == 0)
								{
									if (tapenable[board][adc])
										tapaenable |= 1 << (tap / 2);
									tapastart[tap / 2] = tapstart[board][adc] * 2;
									tapadelta[tap / 2] = tapdelta[board][adc] * 2;
								}
								else
								{
									if (tapenable[board][adc])
										tapbenable |= 1 << (tap / 2);
									tapbstart[tap / 2] = tapstart[board][adc] * 2;
									tapbdelta[tap / 2] = tapdelta[board][adc] * 2;
								}
							}
							// LSB 16 bit samples
							else
							{
								// Even taps go to engine A, odd taps to engine B
								if (tap % 2 == 0)
								{
									if (tapenable[board][adc - MAX_ADC_COUNT])
										tapaenable |= 1 << (tap / 2);
									tapastart[tap / 2] = tapstart[board][adc - MAX_ADC_COUNT] * 2 + 1;
									tapadelta[tap / 2] = tapdelta[board][adc - MAX_ADC_COUNT] * 2;
								}
								else
								{
									if (tapenable[board][adc - MAX_ADC_COUNT])
										tapbenable |= 1 << (tap / 2);
									tapbstart[tap / 2] = tapstart[board][adc - MAX_ADC_COUNT] * 2 + 1;
									tapbdelta[tap / 2] = tapdelta[board][adc - MAX_ADC_COUNT] * 2;
								}
							}
						}
						tap++;
					}
				}
			}
			// Normal CDS
			else
			{
				for (adc = 0; adc < MAX_ADC_COUNT; adc++)
				{
					for (i = 0; i < boardsusedcount + boardsusedcount % 2; i++)
					{
						if (i < boardsusedcount)
						{
							board = boardsused[i];
							// Even taps go to engine A, odd taps to engine B
							if (tap % 2 == 0)
							{
								if (tapenable[board][adc])
									tapaenable |= 1 << (tap / 2);
								tapastart[tap / 2] = tapstart[board][adc];
								tapadelta[tap / 2] = tapdelta[board][adc];
							}
							else
							{
								if (tapenable[board][adc])
									tapbenable |= 1 << (tap / 2);
								tapbstart[tap / 2] = tapstart[board][adc];
								tapbdelta[tap / 2] = tapdelta[board][adc];
							}
						}
						tap++;
					}
				}
			}
		}
		// Write tap engine A and B enables (each bit of 32-bit control word controls one tap enable)
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | INTERFACE_REVC_TAPA_ENABLE, tapaenable))
			throw rts2core::Error ("Error writing Interface deinterlacing tap A enables");
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | INTERFACE_REVC_TAPB_ENABLE, tapbenable))
			throw rts2core::Error ("Error writing Interface deinterlacing tap B enables");
		// Write tap engine A and B start and delta control words
		for (tap = 0; tap < loopcount; tap++)
		{
			// Only write start and delta for enabled taps
			if (tapaenable & (1 << tap))
			{
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | (INTERFACE_REVC_TAPA_0_START + tap), tapastart[tap]))
					throw rts2core::Error ("Error writing Interface deinterlacing tap A start");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | (INTERFACE_REVC_TAPA_0_DELTA + tap), tapadelta[tap]))
					throw rts2core::Error ("Error writing Interface deinterlacing tap A delta");
			}
			if (tapbenable & (1 << tap))
			{
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | (INTERFACE_REVC_TAPB_0_START + tap), tapbstart[tap]))
					throw rts2core::Error ("Error writing Interface deinterlacing tap B start");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | (INTERFACE_REVC_TAPB_0_DELTA + tap), tapbdelta[tap]))
					throw rts2core::Error ("Error writing Interface deinterlacing tap B delta");
			}
		}
		// Get CameraLink mode (0 = Full, 1 = Base)
		config->getString ("BACKPLANE", "CLMODE", s);
		if (s == "Full")
			clmode = 0;
		else if (s == "Base")
			clmode = 1;
		else
			throw rts2core::Error ("Invalid CameraLink mode");
		if (clmode == 0)
			cllength = tapcount * taplength / 4 - 1;	// Full-mode is 64-bits per CameraLink clock
		else
			cllength = tapcount * taplength - 1;		// Base-mode is 16-bits per CameraLink clock
		// Write number of CameraLink pixels necessary to read an entire CameraLink line
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | INTERFACE_REVC_LINE_LENGTH, cllength))
			throw rts2core::Error ("Error writing Interface deinterlacing line length");
		// Write number of CameraLink lines per frame
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | INTERFACE_REVC_LINE_COUNT, height - 1))
			throw rts2core::Error ("Error writing Interface deinterlacing line count");

		// Write number of idle CameraLink lines between frames
		if (config->getInteger ("BACKPLANE", "IDLELINECOUNT", i) || i <= 0)
			throw rts2core::Error ("Invalid idle line count");
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | INTERFACE_REVC_IDLE_COUNT, i - 1))
			throw rts2core::Error ("Error writing Interface deinterlacing idle line count");
		// Write CameraLink mode
		if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | INTERFACE_REVC_CL_MODE, clmode))
			throw rts2core::Error ("Error writing Interface deinterlacing CameraLink mode");
		// Write CameraLink speed
		if (registers[BUILD_IF]->value->getValueInteger () > 356)
		{
			config->getString ("BACKPLANE", "CLSPEED", s);
			if (s == "80")
				clspeed = 0;
			else if (s == "40")
				clspeed = 1;
			else
				throw rts2core::Error ("Invalid CameraLink speed");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_IF + 1) << 16) | INTERFACE_REVC_CL_SPEED, clspeed))
				throw rts2core::Error ("Error writing Interface deinterlacing CameraLink speed");
		}
	}



	// Send configure system command
	if (interfaceCommand (">C\r", s, 3000, true))
		throw rts2core::Error ("Error configuring system");

//	if (interfaceCommand(">L1\r", s, 100, true, false))
//		throw rts2core::Error ("Error enabling polling")

	last_taplength = taplength;
	last_height = height;
}

void Reflex::configBoard (int board)
{
	std::string s, id;
	std::ostringstream ss_key;
	const char *key;
	int i, enable, bit, itemp, hdr;
	bool hven[8], lven[8];
	double dtemp;
	bool drven[12];
	double clamp, gain;

	if ((board < 0) || (board >= MAX_DAUGHTER_COUNT))
		throw rts2core::Error ("Invalid board number specified");

	// Silently ignore boards that aren't installed
	if (!boardType (board))
		return;
	ss_key << "BOARD" << (board + 1);

	std::string skey = ss_key.str ();
	key = skey.c_str ();

//	// Disable polling to speed up configuration
//	if (interfaceCommand(">L0\r", s, 100, true, false))
//		throw rts2core::Error ("Error disabling polling");

	// Bias board
	switch (boardType (BOARD_DAUGHTERS + board))
	{
		case BT_NONE:
			return;
		case BT_BIAS:
			// Set bias enable bits
			bit = 1;
			enable = 0;
			for (i = 0; i < 8; i++)
			{
				std::ostringstream s_subkey;
				s_subkey << (i + 1);
				std::string subkey = s_subkey.str ();
				config->getString (key, ("LE" + subkey).c_str (), s, "-");
				if ((s != "1") && (s != "0"))
					throw rts2core::Error ("Invalid bias enable setting (" + skey + "/LE" + subkey + ")");
				lven[i] = (s == "1");
				config->getString (key, ("HE" + subkey).c_str (), s, "-");
				if ((s != "1") && (s != "0"))
					throw rts2core::Error ("Invalid bias enable setting (" + skey + "/HE" + subkey + ")");
				hven[i] = (s == "1");
				if (lven[i])
					enable |= bit;
				if (hven[i])
					enable |= bit << 8;
				bit <<= 1;
			}
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | BIAS_ENABLE, enable))
				throw rts2core::Error ("Error setting bias enables");
			// Set voltages
			for (i = 0; i < 8; i++)
			{
				std::ostringstream s_subkey;
				s_subkey << (i + 1);
				std::string subkey = s_subkey.str ();
				if (config->getDouble (key, ("LV" + subkey).c_str (), dtemp))
					throw rts2core::Error ("Error parsing bias voltage (" + skey + "/LV" + subkey + ")");
				if ((dtemp < -13.0) || (dtemp > 13.0))
					 rts2core::Error ("Requested bias voltage out of range (" + skey + "/LV" + subkey + ")");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (BIAS_SET_LV1 + i), int (dtemp * 1000.0)))
					throw rts2core::Error ("Error setting bias voltages");
				if (config->getDouble (key, ("LC" + subkey).c_str (), dtemp))
					throw rts2core::Error ("Error parsing bias current limit (" + skey + "/LC" + subkey + ")");
				if ((dtemp < 0.0) || (dtemp > 100.0))
					throw rts2core::Error ("Requested bias current limit out of range (" + skey + "/LC" + subkey + ")");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (BIAS_SET_LC1 + i), int (dtemp * 1000.0)))
					throw rts2core::Error ("Error setting bias current limits");
				if (config->getInteger (key, ("LO" + subkey).c_str (), itemp))
					throw rts2core::Error ("Error parsing bias order (" + skey + "/LO" + subkey + ")");
				if ((itemp < 0) || (itemp > 10))
					throw rts2core::Error ("Requested bias order out of range (" + skey + "/LO" + subkey + ")");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (BIAS_ORDER_LV1 + i), itemp))
					throw rts2core::Error ("Error setting bias order");
				if (config->getDouble (key, ("HV" + subkey).c_str (), dtemp))
					throw rts2core::Error ("Error parsing bias voltage (" + skey + "/HV" + subkey + ")");
				if ((dtemp < 0.0) || (dtemp > 28.0))
					throw rts2core::Error ("Requested bias voltage out of range (" + skey + "/HV" + subkey + ")");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (BIAS_SET_HV1 + i), int (dtemp * 1000.0)))
					throw rts2core::Error ("Error setting bias voltages");
				if (config->getDouble (key, ("HC" + subkey).c_str (), dtemp))
					throw rts2core::Error ("Error parsing bias current limit (" + skey + "/HC" + subkey + ")");
				if ((dtemp < 0.0) || (dtemp > 100.0))
					throw rts2core::Error ("Requested bias current limit out of range (" + skey + "/HC" + subkey + ")");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (BIAS_SET_HC1 + i), int (dtemp * 1000.0)))
					throw rts2core::Error ("Error setting bias current limits");
				if (config->getInteger (key, ("HO" + subkey).c_str (), itemp))
					throw rts2core::Error ("Error parsing bias order (" + skey + "/HO" + subkey + ")");
				if ((itemp < 0) || (itemp > 10))
					throw rts2core::Error ("Requested bias order out of range (" + subkey + ")");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (BIAS_ORDER_HV1 + i), itemp))
					throw rts2core::Error ("Error setting bias order");
			}
			break;
		// Driver board
		case BT_DRIVER:
			// Set driver enable bits
			bit = 1;
			enable = 0;
			for (i = 0; i < 12; i++)
			{
				drven[i] = false;
				std::ostringstream s_subkey;
				s_subkey << "ENABLE" << (i + 1);
				std::string subkey = s_subkey.str ();
				config->getString (key, subkey.c_str (), s, "-");
				if ((s != "1") && (s != "0"))
					throw rts2core::Error ("Invalid driver enable setting (" + skey + "/" + subkey + ")");
				if (s == "1")
				{
					drven[i] = true;
					enable |= bit;
				}
				bit <<= 1;
			}
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | DRIVER_ENABLE, enable))
				throw rts2core::Error ("Error setting driver enables");

			// Set high and low clock voltages, and slew rates
			for (i = 0; i < 12; i++)
			{
				if (drven[i])
				{
					std::ostringstream s_subkey;
					s_subkey << (i + 1);
					std::string subkey = s_subkey.str ();
					if (config->getDouble (key, ("LOWLEVEL" + subkey).c_str (), dtemp))
						throw rts2core::Error ("Error parsing driver low level (" + skey + "/LOWLEVEL" + subkey + ")");
					if ((dtemp < -12.0) || (dtemp > 12.0))
						throw rts2core::Error ("Requested driver low level out of range (" + skey + "/LOWLEVEL" + subkey + ")");
					if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (DRIVER_DRV1_LOW + 3 * i), int (dtemp * 1000.0)))
						throw rts2core::Error ("Error setting driver low level (" + subkey + ")");
					if (config->getDouble (key, ("HIGHLEVEL" + subkey).c_str (), dtemp))
						throw rts2core::Error ("Error parsing driver high level (" + skey + "/HIGHLEVEL" + subkey + ")");
					if ((dtemp < -12.0) || (dtemp > 12.0))
						throw rts2core::Error ("Requested driver high level out of range (" + skey + "/HIGHLEVEL" + subkey + ")");
					if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (DRIVER_DRV1_HIGH + 3 * i), int (dtemp * 1000.0)))
						throw rts2core::Error ("Error setting driver high level (" + skey + "/HIGHLEVEL" + subkey + ")");
					if (config->getDouble (key, ("SLEWRATE" + subkey).c_str (), dtemp))
						throw rts2core::Error ("Error parsing driver slew rate (" + skey + "/SLEWRATE" + subkey + ")");
					if ((dtemp < 0.001) || (dtemp > 5000.0))
						throw rts2core::Error ("Requested driver slew rate out of range (" + skey + "/SLEWRATE" + subkey + ")");
					if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (DRIVER_DRV1_SLEW + 3 * i), int (dtemp * 1000.0)))
						throw rts2core::Error ("Error setting driver slew rate (" + skey + "/SLEWRATE" + subkey + ")");
				}
			}
			break;
		case BT_AD8X120:
			// Low level clamp
			if (config->getDouble (key, "CLAMPLOW", clamp))
				throw rts2core::Error ("Error parsing clamp low level (" + skey + "/CLAMPLOW)");
			if ((clamp < -3.0) || (clamp > 3.0))
				throw rts2core::Error ("Requested clamp low level out of range (" + skey + "/CLAMPLOW)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X120_CLAMP_LOW, int(clamp * 1000.0)))
				throw rts2core::Error ("Error setting AD clamp low level (" + skey + "/CLAMPLOW)");
			// High level clamp
			if (config->getDouble (key, "CLAMPHIGH", clamp))
				throw rts2core::Error ("Error parsing clamp high level (" + skey + "/CLAMPHIGH)");
			if ((clamp < -3.0) || (clamp > 3.0))
				throw rts2core::Error ("Requested clamp high level out of range (" + skey + "/CLAMPHIGH)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X120_CLAMP_HIGH, int (clamp * 1000.0)))
				throw rts2core::Error ("Error setting AD clamp high level (" + skey + "/CLAMPHIGH)");
			config->getString (key, "CDSMODE", s, "-");
			if (s == "CDS")
				itemp = 0;
			else if (s == "Raw")
				itemp = 1;
			else
				throw rts2core::Error ("Error parsing CDS mode (" + skey + "/CDSMODE)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X120_RAW_MODE, itemp))
				throw rts2core::Error ("Error setting AD CDS mode (" + skey + "/CDSMODE)");
			// Raw channel
			if (config->getInteger (key, "RAWCHANNEL", itemp))
				throw rts2core::Error ("Error parsing raw channel (" + skey + "/RAWCHANNEL)");
			if ((itemp < 1) || (itemp > 8))
				throw rts2core::Error ("Requested raw channel out of range (" + skey + "/RAWCHANNEL)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X120_RAW_CHANNEL, itemp - 1))
				throw rts2core::Error ("Error setting raw channel");
			// CDS Offset
			if (config->getInteger (key, "CDSOFFSET", itemp))
				throw rts2core::Error ("Error parsing CDS offset (" + skey + "/CDSOFFSET)");
			if ((itemp < 0) || (itemp > 65535))
				throw rts2core::Error ("Requested CDS offset out of range (" + skey + "/CDSOFFSET)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X120_CDS_OFFSET, itemp))
				throw rts2core::Error ("Error setting CDS offset");
			// SHP Toggle 1
			if (config->getInteger (key, "SHPTOGGLE1", itemp))
				throw rts2core::Error ("Error parsing SHP toggle 1 (" + skey + "/SHPTOGGLE1)");
			if ((itemp < 0) || (itemp > 65535))
				throw rts2core::Error ("Requested SHP toggle 1 out of range (" + skey + "/SHPTOGGLE1)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X120_SHP_TOGGLE1, itemp))
				throw rts2core::Error ("Error setting SHP toggle 1");
			// SHP Toggle 2
			if (config->getInteger (key, "SHPTOGGLE2", itemp))
				throw rts2core::Error ("Error parsing SHP toggle 2 (" + skey + "/SHPTOGGLE2)");
			if ((itemp < 0) || (itemp > 65535))
				throw rts2core::Error ("Requested SHP toggle 2 out of range (" + skey + "/SHPTOGGLE2)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X120_SHP_TOGGLE2, itemp))
				throw rts2core::Error ("Error setting SHP toggle 2");
			// SHD Toggle 1
			if (config->getInteger (key, "SHDTOGGLE1", itemp))
				throw rts2core::Error ("Error parsing SHD toggle 1 (" + skey + "/SHDTOGGLE1)");
			if ((itemp < 0) || (itemp > 65535))
				throw rts2core::Error ("Requested SHD toggle 1 out of range (" + skey + "/SHDTOGGLE1)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X120_SHD_TOGGLE1, itemp))
				throw rts2core::Error ("Error setting SHD toggle 1");
			// SHD Toggle 2
			if (config->getInteger (key, "SHDTOGGLE2", itemp))
				throw rts2core::Error ("Error parsing SHD toggle 2 (" + skey + "/SHDTOGGLE2)");
			if ((itemp < 0) || (itemp > 65535))
				throw rts2core::Error ("Requested SHD toggle 1 out of range (" + skey + "/SHDTOGGLE2)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X120_SHD_TOGGLE2, itemp))
				throw rts2core::Error ("Error setting SHD toggle 2");
			break;
		// AD8X100 board
		case BT_AD8X100:
			// Low level clamp
			if (config->getDouble (key, "CLAMPLOW", clamp))
				throw rts2core::Error ("Error parsing clamp low level (" + skey + "/CLAMPLOW)");
			if ((clamp < -3.0) || (clamp > 3.0))
				throw rts2core::Error ("Requested clamp low level out of range (" + skey + "/CLAMPLOW)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_CLAMP_LOW, int (clamp * 1000.0)))
				throw rts2core::Error ("Error setting AD clamp low level (" + skey + "/CLAMPLOW)");
			// High level clamp
			if (config->getDouble (key, "CLAMPHIGH", clamp))
				throw rts2core::Error ("Error parsing clamp high level (" + skey + "/CLAMPHIGH)");
			if ((clamp < -3.0) || (clamp > 3.0))
				throw rts2core::Error ("Requested clamp high level out of range (" + skey + "/CLAMPHIGH)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_CLAMP_HIGH, int (clamp * 1000.0)))
				throw rts2core::Error ("Error setting AD clamp high level (" + skey + "/CLAMPHIGH)");
			// CDS mode
			config->getString (key, "CDSMODE", s, "-");
			if (s == "CDS")
			{
				itemp = 0;
				hdr = 0;
			}
			else if (s == "Raw")
			{
				itemp = 1;
				hdr = 0;
			}
			else if (s == "HDR CDS")
			{
				itemp = 0;
				hdr = 1;
			}
			else
				throw rts2core::Error ("Error parsing CDS mode (" + skey + "/CDSMODE)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_RAW_MODE, itemp))
				throw rts2core::Error ("Error setting AD CDS mode (" + skey + "/CDSMODE)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_CDS_MODE, hdr))
				throw rts2core::Error ("Error setting AD CDS mode (" + skey + "/CDSMODE)");
			// Raw channel
			if (config->getInteger (key, "RAWCHANNEL", itemp))
				throw rts2core::Error ("Error parsing raw channel (" + skey + "/RAWCHANNEL)");
			if ((itemp < 1) || (itemp > 8))
				throw rts2core::Error ("Requested raw channel out of range (" + skey + "/RAWCHANNEL)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_RAW_CHANNEL, itemp - 1))
				throw rts2core::Error ("Error setting raw channel");
			// CDS Gain
			if (config->getDouble (key, "CDSGAIN", dtemp))
				throw rts2core::Error ("Error parsing CDS gain (" + skey + "/CDSGAIN)");
			if ((dtemp <= 0) || (dtemp > 65535))
				throw rts2core::Error ("Requested CDS gain out of range (" + skey + "/CDSGAIN)");
			itemp = dtemp * 65536.0;
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_CDS_GAIN, itemp))
				throw rts2core::Error ("Error setting CDS gain");
			gain = dtemp;
			// CDS Offset
			if (config->getDouble (key, "CDSOFFSET", dtemp))
				throw rts2core::Error ("Error parsing CDS offset (" + skey + "/CDSOFFSET)");
			// Scale offset by gain (offset is applied before gain on the AD board, but user usually wants offset after gain)
			dtemp /= gain;
			if ((dtemp < -32768) || (dtemp > 32767))
				throw rts2core::Error ("Requested CDS offset out of range (" + skey + "/CDSOFFSET)");
			itemp = dtemp * 65536.0;
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_CDS_OFFSET, itemp))
				throw rts2core::Error ("Error setting CDS offset");
			// SHP Toggle 1
			if (config->getInteger (key, "SHPTOGGLE1", itemp))
				throw rts2core::Error ("Error parsing SHP toggle 1 (" + skey + "/SHPTOGGLE1)");
			if ((itemp < 0) || (itemp > 65535))
				throw rts2core::Error ("Requested SHP toggle 1 out of range (" + skey + "/SHPTOGGLE1)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_SHP_TOGGLE1, itemp))
				throw rts2core::Error ("Error setting SHP toggle 1");
			// SHP Toggle 2
			if (config->getInteger (key, "SHPTOGGLE2", itemp))
				throw rts2core::Error ("Error parsing SHP toggle 2 (" + skey + "/SHPTOGGLE2)");
			if ((itemp < 0) || (itemp > 65535))
				throw rts2core::Error ("Requested SHP toggle 2 out of range (" + skey + "/SHPTOGGLE2)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_SHP_TOGGLE2, itemp))
				throw rts2core::Error ("Error setting SHP toggle 2");
			// SHD Toggle 1
			if (config->getInteger (key, "SHDTOGGLE1", itemp))
				throw rts2core::Error ("Error parsing SHD toggle 1 (" + skey + "/SHDTOGGLE1)");
			if ((itemp < 0) || (itemp > 65535))
				throw rts2core::Error ("Requested SHD toggle 1 out of range (" + skey + "/SHDTOGGLE1)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_SHD_TOGGLE1, itemp))
				throw rts2core::Error ("Error setting SHD toggle 1");
			// SHD Toggle 2
			if (config->getInteger (key, "SHDTOGGLE2", itemp))
				throw rts2core::Error ("Error parsing SHD toggle 2 (" + skey + "/SHDTOGGLE2)");
			if ((itemp < 0) || (itemp > 65535))
				throw rts2core::Error ("Requested SHD toggle 2 out of range (" + skey + "/SHDTOGGLE2)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_SHD_TOGGLE2, itemp))
				throw rts2core::Error ("Error setting SHD toggle 2");
			break;
		// HS board
		case BT_HS:
			if (config->getInteger (key, "RGRISINGEDGE", itemp))
				throw rts2core::Error ("Error parsing RG rising edge (" + skey + "/RGRISINGEDGE)");
			if ((itemp < 0) || (itemp > 63))
				throw rts2core::Error ("Requested RG rising edge out of range (" + skey + "/RGRISINGEDGE)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_RG_RISING_EDGE, itemp))
				throw rts2core::Error ("Error setting RG rising edge");
			if (config->getInteger (key, "RGFALLINGEDGE", itemp))
				throw rts2core::Error ("Error parsing RG falling edge (" + skey + "/RGFALLINGEDGE)");
			if ((itemp < 0) || (itemp > 63))
				throw rts2core::Error ("Requested RG falling edge out of range (" + skey + "/RGFALLINGEDGE)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_RG_FALLING_EDGE, itemp))
				throw rts2core::Error ("Error setting RG falling edge");
			for (i = 0; i < 6; i++)
			{
				std::ostringstream s_subkey;
				s_subkey << (i + 1);
				std::string subkey = s_subkey.str ();
				if (config->getInteger (key, ("S" + subkey + "RISINGEDGE").c_str (), itemp))
					throw rts2core::Error ("Error parsing S" + subkey + " rising edge (" + skey + "/S" + subkey + "RISINGEDGE)");
				if ((itemp < 0) || (itemp > 63))
					throw rts2core::Error ("Requested S" + subkey + " rising edge out of range (" + skey + "/S" + subkey + "RISINGEDGE)");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (HS_S1_RISING_EDGE + (i * 2)), itemp))
					throw rts2core::Error ("Error setting S" + subkey + " rising edge");
				if (config->getInteger (key, ("S" + subkey + "FALLINGEDGE").c_str (), itemp))
					throw rts2core::Error ("Error parsing S" + subkey + " falling edge (" + skey + "/S" + subkey + "FALLINGEDGE)");
				if ((itemp < 0) || (itemp > 63))
					throw rts2core::Error ("Requested S" + subkey + " falling edge out of range (" + skey + "/S" + subkey + "FALLINGEDGE)");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (HS_S1_FALLING_EDGE + (i * 2)), itemp))
					throw rts2core::Error ("Error setting S" + subkey + " falling edge");
			}
			if (config->getInteger (key, "LINELENGTH", itemp))
				throw rts2core::Error ("Error parsing line length (" + skey + "/LINELENGTH)");
			if ((itemp < 0) || (itemp > 8191))
				throw rts2core::Error ("Requested line length out of range (" + skey + "/LINELENGTH)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_LINE_LENGTH, itemp))
				throw rts2core::Error ("Error setting line length");
			if (config->getInteger (key, "CLPOBSTART", itemp))
				throw rts2core::Error ("Error parsing CLPOB start (" + skey + "/CLPOBSTART)");
			if ((itemp < 0) || (itemp > 8191))
				throw rts2core::Error ("Requested CLPOB start out of range (" + skey + "/CLPOBSTART)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_CLPOB_START, itemp))
				throw rts2core::Error ("Error setting CLPOB start");
			if (config->getInteger (key, "CLPOBEND", itemp))
				throw rts2core::Error ("Error parsing CLPOB end (" + skey + "/CLPOBEND)");
			if ((itemp < 0) || (itemp > 8191))
				throw rts2core::Error ("Requested CLPOB end out of range (" + skey + "/CLPOBEND)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_CLPOB_END, itemp))
				throw rts2core::Error ("Error setting CLPOB end");
			if (config->getInteger (key, "CLPOBLEVEL", itemp))
				throw rts2core::Error ("Error parsing CLPOB level (" + skey + "/CLPOBLEVEL)");
			if ((itemp < 0) || (itemp > 1023))
				throw rts2core::Error ("Requested CLPOB level out of range (" + skey + "/CLPOBLEVEL)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_CLPOB_LEVEL, itemp))
				throw rts2core::Error ("Error setting CLPOB level");

			for (i = 0; i < 6; i++)
			{
				std::ostringstream s_subkey;
				s_subkey << "S" << (i + 1) << "HIGH";
				std::string subkey = s_subkey.str ();
				if (config->getInteger (key, subkey.c_str (), itemp))
					throw rts2core::Error ("Error parsing S high flag (" + skey + "/" + subkey + ")");
				if ((itemp < 0) || (itemp > 1))
					throw rts2core::Error ("Requested high flag invalid (" + skey + "/" + subkey + ")");
				if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | (HS_S1_HIGH + i), itemp))
					throw rts2core::Error ("Error setting " + subkey + " high flag");
			}

			if (config->getInteger (key, "LVDSTEST", itemp))
				throw rts2core::Error ("Error parsing LVDS test flag (" + skey + "/LVDSTEST)");
			if ((itemp < 0) || (itemp > 1))
				throw rts2core::Error ("Requested LVDS test flag invalid (" + skey + "/LVDSTEST)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_LVDS_TEST, itemp))
				throw rts2core::Error ("Error setting LVDS test flag");
			if (config->getInteger (key, "TCLKDELAY", itemp))
				throw rts2core::Error ("Error parsing TCLK delay (" + skey + "/TCLKDELAY)");
			if ((itemp < 0) || (itemp > 15))
				throw rts2core::Error ("Requested TCLK delay out of range (" + skey + "/TCLKDELAY)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_TCLK_DELAY, itemp))
				throw rts2core::Error ("Error setting TCLK delay");
			if (config->getInteger (key, "DOUTPHASE", itemp))
					throw rts2core::Error ("Error parsing DOUT phase (" + skey + "/DOUTPHASE)");
			if ((itemp < 0) || (itemp > 63))
				throw rts2core::Error ("Requested DOUT phase out of range (" + skey + "/DOUTPHASE)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_DOUT_PHASE, itemp))
				throw rts2core::Error ("Error setting DOUT phase");
			if (config->getInteger (key, "SHPLOC", itemp))
				throw rts2core::Error ("Error parsing SHP location (" + skey + "/SHPLOC)");
			if ((itemp < 0) || (itemp > 63))
				throw rts2core::Error ("Requested SHP location out of range (" + skey + "/SHPLOC)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_SHP_LOC, itemp))
				throw rts2core::Error ("Error setting SHP location");
			if (config->getInteger (key, "SHPWIDTH", itemp))
				throw rts2core::Error ("Error parsing SHP width (" + skey + "/SHPWIDTH)");
			if ((itemp < 0) || (itemp > 63))
				throw rts2core::Error ("Requested SHP width out of range (" + skey + "/SHPWIDTH)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_SHP_WIDTH, itemp))
				throw rts2core::Error ("Error setting SHP width");
			if (config->getInteger (key, "SHDLOC", itemp))
				throw rts2core::Error ("Error parsing SHD location (" + skey + "/SHDLOC)");
			if ((itemp < 0) || (itemp > 63))
				throw rts2core::Error ("Requested SHD location out of range (" + skey + "/SHDLOC)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_SHD_LOC, itemp))
				throw rts2core::Error ("Error setting SHD location");
			config->getString (key, "CDSGAIN", s, "-");
			if (s == "-3")
				itemp = 0;
			else if (s == "0")
				itemp = 1;
			else if (s == "3")
				itemp = 2;
			else if (s == "6")
				itemp = 3;
			else
				throw rts2core::Error ("Error parsing CDS gain (" + skey + "/CDSGAIN)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_CDS_GAIN, itemp))
				throw rts2core::Error ("Error setting CDS gain");
			if (config->getDouble (key, "VGAGAIN", dtemp))
				throw rts2core::Error ("Error parsing VGA gain (" + skey + "/VGAGAIN)");
			itemp = int ((dtemp - 5.75) / 0.0358);
			if ((itemp < 0) || (itemp > 1023))
				throw rts2core::Error ("Requested VGA gain out of range (" + skey + "/VGAGAIN)");
			if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | HS_VGA_GAIN, itemp))
				throw rts2core::Error ("Error setting VGA gain");
			break;
		default:
			throw rts2core::Error ("Unknown board type (" + skey + ")");
	}

	// Apply board configuration
//	std::ostringstream cmd;
//	cmd << ">B" << (BOARD_DAUGHTERS + board) << "\r";
//	if (interfaceCommand(cmd.str ().c_str (), s, 3000, true))
//			throw rts2core::Error ("Error configuring board");
//	if (interfaceCommand(">L1\r", s, 100, true, false))
//		throw rts2core::Error ("Error enabling polling");
}

// configure HDR mode..
void Reflex::configHDR (int board, bool raw, bool hdrmode)
{
	uint32_t ba;
	char cmd[5] = ">Bx\r";
	std::string s;
	switch (boardType (BOARD_DAUGHTERS + board))
	{
		case BT_AD8X120:
			break;
		case BT_AD8X100:
			ba = SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_RAW_MODE;
			if (writeRegister(ba, raw ? 1 : 0))
				throw rts2core::Error ("Error setting AD CDS mode");
			registers[ba]->value->setValueInteger (raw ? 1 : 0);
			sendValueAll (registers[ba]->value);

			ba = SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_CDS_MODE;
			if (writeRegister(ba, hdrmode))
				throw rts2core::Error (std::string ("Error setting AD CDS mode"));
			registers[ba]->value->setValueInteger (hdrmode ? 1 : 0);
			sendValueAll (registers[ba]->value);
	
			if (raw)
			{	
				ba = SYSTEM_CONTROL_ADDR | ((BOARD_DAUGHTERS + board + 1) << 16) | AD8X100_RAW_CHANNEL;
				if (writeRegister(ba, 6))
					throw rts2core::Error ("Error setting raw channel");
				registers[ba]->value->setValueInteger (6);
				sendValueAll (registers[ba]->value);
			}

			cmd[2] = '0' + BOARD_DAUGHTERS + board;
			interfaceCommand (cmd, s, 3000, true);
			break;
	}
}

void Reflex::configTEC ()
{
	std::string s;
	double dtemp;
	int tecenable;

	// No TEC on rev A PowerB boards
	if (boardRevision (BOARD_PB) == 1)
		return;

//	// Disable polling to speed up configuration
//	if (interfaceCommand(">L0\r", s, 100, false))
//		throw rts2core::Error ("Error disabling polling")

	config->getString ("POWERB", "TECENABLE", s, "");
	tecenable = (s == "1");

	if (tecenable)
	{
		if (config->getDouble ("POWERB", "TECSETPOINT", dtemp))
			throw rts2core::Error ("Error parsing TEC setpoint (POWERB/TECSETPOINT)");
		if ((dtemp < -100.0) || (dtemp > 50.0))
			throw rts2core::Error ("Requested TEC setpoint out of range (POWERB/TECSETPOINT)");
		dtemp += 273.15;	// C to K
	}
	else
	{
		dtemp = 0.0;
	}
	if (writeRegister(SYSTEM_CONTROL_ADDR | ((BOARD_PB + 1) << 16) | POWERB_TEC_SETPOINT, int (dtemp * 1000.0)))
		throw rts2core::Error ("Error setting TEC setpoint");

	// Apply board configuration
//	if (interfaceCommand(">B3", s, 3000, true))
//		throw rts2core::Error ("Error configuring board");
	// Apply TEC command twice to allow for some settling
//	usleep(USEC_SEC / 2);
//	if (interfaceCommand(">B3", s, 3000, true))
//		throw rts2core::Error ("Error configuring board");
//	if (interfaceCommand(">L1\r", s, 100, true, false))
//		throw rts2core::Error ("Error enabling polling");
}

int main (int argc, char **argv)
{
	Reflex device (argc, argv);
	return device.run ();
}
