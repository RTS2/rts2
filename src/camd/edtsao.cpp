/* 
 * EDT controller driver
 * Copyright (C) 2007-2010 Petr Kubanek <petr@kubanek.net>
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

#include "connection/fork.h"
#include "valuearray.h"

#define OPT_NOTIMEOUT  OPT_LOCAL + 73
#define OPT_NOWRITE    OPT_LOCAL + 74
#define OPT_SIGNALFILE OPT_LOCAL + 75

/*
 * Depends on libedtsao.a, which is build from edtsao directory. This
 * contains unmodified files from EDT-SAO distribution, and should be
 * update regulary.
 */

namespace rts2camd
{

typedef enum {A_plus, A_minus, B, C, D, D_Harvard, P_Harvard, S_Harvard} edtAlgoType;

typedef enum {CHANNEL_4, CHANNEL_16} t_controllerType;

/**
 * Variable for EDT-SAO registers. It holds information
 * about which register the variable represents and which
 * algorithm is used to convert it to register hex value.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueEdt: public rts2core::ValueDoubleMinMax
{
	public:
		ValueEdt (std::string in_val_name);
		ValueEdt (std::string in_val_name, std::string in_description, bool writeToFits = true, int32_t flags = 0);
		~ValueEdt (void);

		/**
		 * Initialize EDT part of variable.
		 *
		 * @param in_reg   register prefix
		 * @param in_algo  type of algorithm used to convert float value to register value
		 */
		void initEdt (long in_reg, edtAlgoType in_algo, t_controllerType ct);

		/**
		 * Convert float value to register value.
		 *
		 * @param in_v float value which will be converted
		 *
		 * @return hex value of in_v
		 */
		long getHexValue (float in_v, t_controllerType controllerType);
	private:
		// EDT - SAO register. It will be shifted by 3 bytes left and ored with value calculated by algo
		long reg;
		// algorith used to compute hex suffix value
		edtAlgoType algo;
};

}

using namespace rts2camd;

ValueEdt::ValueEdt (std::string in_val_name):rts2core::ValueDoubleMinMax (in_val_name)
{
}

ValueEdt::ValueEdt (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):rts2core::ValueDoubleMinMax (in_val_name, in_description, writeToFits, flags)
{
}

ValueEdt::~ValueEdt (void)
{
}

void ValueEdt::initEdt (long in_reg, edtAlgoType in_algo, t_controllerType controllerType)
{
	reg = (in_reg << 12);
	algo = in_algo;

	// set v_min and v_max - minimal and maximal values
	switch (algo)
	{
		case A_plus:
			setMin (0);
			setMax (10);
			break;
		case A_minus:
			setMin (0);
			setMax (10);
			break;
		case B:
			switch (controllerType)
			{
				case CHANNEL_4:
					setMin (-5);
					setMax (5);
					break;
				case CHANNEL_16:
					setMin (-4);
					setMax (6);
					break;
			}
			break;
		case C:
			setMin (3.60);
			setMax (20);
			break;
		case D:
		case D_Harvard:
			setMin (0);
			switch (controllerType)
			{
				case CHANNEL_4:
					setMax (25.9);
					break;
				case CHANNEL_16:
					setMax (30);
					break;
			}
			break;
		case S_Harvard:
			setMin (0);
			setMax (10);
			break;
		case P_Harvard:
			setMin (0);
			setMax (12.5);
			break;
	}
}

long ValueEdt::getHexValue (float in_v, t_controllerType controllerType)
{
	long val = 0;
	// calculate value by algorithm
	switch (algo)
	{
		case A_plus:
			val = (long) (in_v * 409.5);
			break;
		case A_minus:
			val = (long) (fabs (in_v) * 409.5);
			break;
		case B:
			switch (controllerType)
			{
				case CHANNEL_4:
					val = (long) ((5 - in_v) * 409.5);
					break;
				case CHANNEL_16:
					val = (long) ((6 - in_v) * 409.5);
					break;
			}
			break;
		case C:
			switch (controllerType)
			{
				case CHANNEL_4:
					val = (long) (in_v * 204.7);
					break;
				case CHANNEL_16:
					val = (long) (((in_v - 1.25) / 20.0) * 4095);
					break;
			}
			break;
		case D:
			val = (long) (in_v * 136.5);
			break;
		case D_Harvard:
			val = (long) (((in_v - 1.38) / 30.0) * 4095);
			break;
		case S_Harvard:
			val = (long) (((in_v) / 10.0) * 4095);
			break;
		case P_Harvard:
			val = (long) (((in_v) / 12.5) * 4095);
			break;
	}
	if (val > 0xfff)
		val = 0xfff;
	if (val < 0)
		val = 0;
	return reg | val;
}

namespace rts2camd
{

/**
 * Possible types of EDTSEO commands for LSST controller.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
typedef enum 
{
	ZERO,
	READ,
	ADD,
	SKIP,
	FRD,
	JIGGLE,
	XFERC,
	XFERD,
	HEND,
	VEND = 0x10
} command_t;


/**
 * This is main control class for EDT-SAO CCD controller.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class EdtSao:public Camera
{
	public:
		EdtSao (int in_argc, char **in_argv);
		virtual ~ EdtSao (void);

		virtual int init ();
		virtual int initValues ();

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int processOption (int in_opt);
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual void beforeRun ();
		virtual void initBinnings ();

		virtual int startExposure ();
		virtual int stopExposure ();
		virtual long isExposing ();
		virtual int readoutStart ();

		virtual size_t suggestBufferSize () { return 0; }

		virtual int doReadout ();
		virtual int endReadout ();

	private:
		PdvDev * pd;
		char devname[16];
		int devunit;
		bool notimeout;
		int sdelay;

		t_controllerType controllerType;

		bool not_write;

		u_int status;

		rts2core::ValueFloat *shutterDelay;

		rts2core::ValueSelection *edtGain;
		rts2core::ValueInteger *parallelClockSpeed;
		// number of lines to skip in serial mode
		rts2core::ValueInteger *skipLines;
		
		// number of jigle lines
		rts2core::ValueInteger *jiggleLines;

		rts2core::ValueBool *dofcl;
		rts2core::ValueInteger *fclrNum;
		rts2core::ValueInteger *fclrFailed;

		rts2core::ValueBool *edtSplit;
		rts2core::ValueBool *edtUni;

		rts2core::ValueString *signalFile;
		rts2core::ValueString *signalFileDir;
		rts2core::ValueString *sigtosc;

		bool verbose;

		int edtwrite (unsigned long lval);

		int depth;

		bool shutter;
		bool overrun;

		u_char **bufs;
		int imagesize;

		int writeBinFile (const char *filename);

		/**
		 * Check if its a signal file (ending with .sig). If it is, it
		 * will first translate it to .bin file using sigtosc.pl.
		 */
		int writeSignalFile (const char *filename);

		int totalChannels;

		// perform camera-specific functions
		/** perform camera reset */
		void reset ();

		int setEDTSplit (bool on)
		{
			if (on)
				return edtwrite (SAO_SPLIT_ON);
			return edtwrite (SAO_SPLIT_OFF);
		}
		int setEDTUni (bool on)
		{
			if (on)
				return edtwrite (SAO_UNI_ON);
			return edtwrite (SAO_UNI_OFF);
		}

		/**
		 * Set A/D offset of given channel.
		 *
		 * @param ch      channel number
		 * @param offset  offset value
		 */
		int setADOffset (int ch, int offset);

		/**
		 * Enable/disable AD channel.
		 */
		void setChannel (int ch, bool enabled, bool last);

		void probe ();

		int fclr (int num);
		
		/**
		 * Repeat fclr until it is sucessfull.
		 */
		void fclr_r (int num);

		/** set high or low gain */
		int setEDTGain (bool high)
		{
			if (high)
				return edtwrite (SAO_GAIN_HIGH);
			return edtwrite (SAO_GAIN_LOW);
		}

		int setParallelClockSpeed (int new_clockSpeed);

		int setGrayScale (bool _grayScale, int board);

		int setGrayScale (bool _grayScale);

		// registers
		ValueEdt *vhi;
		ValueEdt *vlo;

		ValueEdt *phi;
		ValueEdt *plo;

		ValueEdt *shi;
		ValueEdt *slo;

		ValueEdt *rhi;
		ValueEdt *rlo;

		ValueEdt *rd;

		ValueEdt *od1r;
		ValueEdt *od2l;
		ValueEdt *og1r;
		ValueEdt *og2l;

		ValueEdt *dd;

		rts2core::IntegerArray *ADoffsets;

		int setEdtValue (ValueEdt * old_value, float new_value);
		int setEdtValue (ValueEdt * old_value, rts2core::Value * new_value);

		// write single command to controller
		void writeCommand (bool parallel, int addr, command_t command);
		void writeCommandEnd ();

		void writeSkip (bool parallel, int size, int &addr);
		/**
		 *
		 * @return Actuall size used.
		 */
		int writeReadPattern (bool parallel, int size, int bin, int &addr);

		// write to controller pattern file
		int writePattern ();

		// read only part of the chip
		int writePartialPattern ();

		// number of collumns
		rts2core::ValueInteger *chipWidth;

		// number of rows
		rts2core::ValueInteger *chipHeight;

		// number of rows to skip. If negative, rows will be moved up. Partial readout is not active if this equals to 0.
		rts2core::ValueInteger *partialReadout;

		// Grayscale off/on
		rts2core::ValueBool *grayScale;

		int lastSkipLines;
		int lastJiggleLines;
		int lastX;
		int lastY;
		int lastW;
		int lastH;
		int lastSplitMode;
		int lastPartialReadout;

		int sendChannel (int chan, u_char *buf, int chanorder, int totalchanel);
};

}

int EdtSao::edtwrite (unsigned long lval)
{
	unsigned long lsval = lval;
	if (ft_byteswap ())
		swap4 ((char *) &lsval, (char *) &lval, sizeof (lval));
	if (not_write)
	{
		std::cout << "edtwrite " << std::hex << lval << std::endl;
	}
	else
	{
		ccd_serial_write (pd, (u_char *) (&lsval), 4);
	}
	return 0;
}

int EdtSao::writeBinFile (const char *filename)
{
	// taken from edtwriteblk.c, heavily modified
	FILE *fp;
	u_int cbuf;
	u_int *cptr;
	int loops;
	int nwrite;

	std::string full_name;

	if (*filename != '/')
		full_name = signalFileDir->getValue () + std::string (filename);
	else
		full_name = std::string (filename);

	fp = fopen (full_name.c_str(), "r");
	if (!fp)
	{
		logStream (MESSAGE_ERROR) << "cannot open file " << full_name <<
			sendLog;
		return -1;
	}
	cptr = &cbuf;
	loops = 0;
	/*pdv_reset_serial(pd); */
	while ((nwrite = fread (&cbuf, 4, 1, fp)) > 0)
	{
		ccd_serial_write (pd, (u_char *) (cptr), nwrite * 4);
		if (verbose)
		{
			sao_print_command (cptr, nwrite);
		}
		loops++;
	}
	fclose (fp);
	logStream (MESSAGE_DEBUG) << "from " << full_name << " written " << loops << " serial commands." << sendLog;
	return 0;
}

int EdtSao::writeSignalFile (const char *filename)
{
	std::string full_name;

	if (*filename != '/')
		full_name = signalFileDir->getValue () + std::string (filename);
	else
		full_name = std::string (filename);

	size_t l = full_name.length ();
	if (l > 4 && full_name.substr (l-4) == ".sig")
	{
		// translate file..
		rts2core::ConnFork c (this, "/usr/local/bin/sigtosc.pl", false, false);
		c.addArg (full_name);
		// TODO replace .sig with .bin?
		c.addArg ("/tmp/rts2.bin");
		int ret = c.run ();
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "cannot translate " << full_name << sendLog;
			return ret;
		}
		logStream (MESSAGE_DEBUG) << "translated " << full_name << " to /tmp/rts2.bin" << sendLog;
		return writeBinFile ("/tmp/rts2.bin");
	}
	else
	{
		return writeBinFile (filename);
	}
}

void EdtSao::reset ()
{
	pdv_flush_fifo (pd);
	pdv_reset_serial (pd);
	edt_reg_write (pd, PDV_CMD, PDV_RESET_INTFC);
	edt_reg_write (pd, PDV_MODE_CNTL, 1);
	edt_msleep (10);
	edt_reg_write (pd, PDV_MODE_CNTL, 0);

	sleep (1);
}

int EdtSao::setADOffset (int ch, int offset)
{
	if (offset < 0 || offset > 0xfff)
	{
		logStream (MESSAGE_ERROR) << "A/D offset of channel " << ch << " is outside allowed range (0-0xfff) : " << offset << sendLog;
		return -1;
	}

	uint32_t o = 0x30080000;
	o |= ((ch % 4) << 20);
	o |= ((ch / 4) << 24);
	o |= (offset);
	if (edtwrite (o))
		return -1;

	logStream (MESSAGE_INFO) << "setting A/D offset on channel " << ch << " to " << offset << ": " << std::hex << o << sendLog;

	//ADoffsets->setValueInteger (ch, offset);

	return 0;
}

void EdtSao::setChannel (int ch, bool enabled, bool last)
{
	uint32_t o;
	// hardcoded new 16ch order
	int ch16[16] = {0x0, 0x4, 0x1, 0x5, 0x2, 0x6, 0x3, 0x7, 0x8, 0xc, 0x9, 0xd, 0xa, 0xe, 0xb, 0xf};
	switch (controllerType)
	{
		case CHANNEL_4:
			o = 0x51000000 | (ch << 8) | ch | (enabled ? 0x0040 : 0x0000) | (last ? 0x8000 : 0x0000);
			break;
		case CHANNEL_16:
			o = 0x51000000 | (ch << 8) | ch16[ch] | (enabled ? 0x0040 : 0x0000) | (last ? 0x8000 : 0x0000);
			break;
	}
	
	logStream (MESSAGE_DEBUG) << "channel " << ch << ": " << std::hex << o << sendLog;

	edtwrite (o);
}

void EdtSao::probe ()
{
	status = edt_reg_read (pd, PDV_STAT);
	shutter = status & PDV_CHAN_ID0;
	overrun = status & PDV_OVERRUN;
}

int EdtSao::fclr (int num)
{
	time_t now;
	time_t end_time;
	edt_reg_write (pd, PDV_CMD, PDV_RESET_INTFC);
	for (int i = 0; i < num; i++)
	{
		int j;
		edtwrite (0x4c000000);
		edtwrite (0x00000000);
		for (j = 0; j < 10; j++)
			probe ();
		time (&end_time);
		end_time += 10;
		while ((!overrun && (status & PDV_CHAN_ID1)) || overrun)
		{
			probe ();
			time (&now);
			if (now > end_time)
			{
				logStream (MESSAGE_ERROR)
					<< "timeout during fclr, phase 1. Overrun: " << overrun 
					<< " status & PDV_CHAN_ID1 " << (status & PDV_CHAN_ID1)
					<< " number " << num
					<< sendLog;
				return -1;
			}
		}
		time (&end_time);
		end_time += 7;
		while ((!overrun && !(status & PDV_CHAN_ID1)) || overrun)
		{
			probe ();
			time (&now);
			if (now > end_time)
			{
				logStream (MESSAGE_ERROR)
					<< "timeout during fclr, phase 2 "
					<< " number " << num
					<< sendLog;
				return 0;
			}
		}
		pdv_serial_wait (pd, 10, 4);
	}
	return 0;
}

void EdtSao::fclr_r (int num)
{
	fclrFailed->setValueInteger (0);
	sendValueAll (fclrFailed);

	while (fclr (num) != 0 && fclrFailed->getValueInteger () < 5)
	{
		logStream (MESSAGE_ERROR) << "cannot do fclr, trying again." << sendLog;
		fclrFailed->inc ();
		sendValueAll (fclrFailed);
		sleep (2);
	}
}

int EdtSao::setParallelClockSpeed (int new_speed)
{
	logStream (MESSAGE_DEBUG) << "setting parallel clock speed to " << new_speed << sendLog;
  	unsigned long ns = 0x46000000;	
	ns |= (new_speed & 0x00ff);
	return edtwrite (ns);
}

int EdtSao::setGrayScale (bool _grayScale, int board)
{
	logStream (MESSAGE_DEBUG) << "setting gray scale to " << _grayScale << " on board " << board << sendLog;
	edtwrite ((_grayScale ? 0x30c00000 : 0x30400000) | (board << 24));
	return 0;
}

int EdtSao::setGrayScale (bool _grayScale)
{
	switch (controllerType)
	{
		case CHANNEL_16:
			setGrayScale (_grayScale, 3);
			setGrayScale (_grayScale, 2);
			setGrayScale (_grayScale, 1);
		case CHANNEL_4:
			return setGrayScale (_grayScale, 0);
	}
	return 0;
}

void EdtSao::beforeRun ()
{
	// set gain to high
	int ret = setEDTGain (edtGain->getValueInteger () == 0);
	if (ret)
		exit (ret);

	Camera::beforeRun ();

	for (int i = 0; i < totalChannels; i++)
	{
		setADOffset (i, (*ADoffsets)[i]);
	}

	depth = 2;

	// do initialization
	reset ();

	ret = edtwrite (SAO_PARALER_SP);
	if (ret)
		exit (ret);

	ret = writeSignalFile (signalFile->getValue ());
	if (ret)
		exit (ret);

	ret = writeBinFile ("e2v_pidlesc.bin");
	if (ret)
		exit (ret);

	setSize (chipWidth->getValueInteger (), chipHeight->getValueInteger (), 0, 0);

	writePattern ();

	ret = setEDTSplit (edtSplit->getValueBool ());
	if (ret)
		exit (ret);
	ret = setEDTUni (edtUni->getValueBool ());
}

void EdtSao::initBinnings ()
{
	addBinning2D (1, 1);
	addBinning2D (2, 2);
	addBinning2D (3, 3);
	addBinning2D (4, 4);
	addBinning2D (8, 8);
	addBinning2D (16, 16);
	addBinning2D (1, 2);
	addBinning2D (1, 4);
	addBinning2D (1, 8);
	addBinning2D (1, 16);
	addBinning2D (2, 1);
	addBinning2D (4, 1);
	addBinning2D (8, 1);
	addBinning2D (16, 1);
}

void EdtSao::writeCommand (bool parallel, int addr, command_t command)
{
	u_char cmd[4];
	cmd[0] = 0x42;
	cmd[1] = command;
	if (parallel == false)
		addr |= 0x4000;
	cmd[2] = (addr & 0xff00) >> 8;
	cmd[3] = addr & 0x00ff;
	ccd_serial_write (pd, cmd, 4);
	if (verbose)
	{
		sao_print_command ((u_int *) cmd, 1);
	}
}

void EdtSao::writeCommandEnd ()
{
	u_char cmd[4];
	cmd[0] = cmd[1] = cmd[2] = cmd[3] = 0;
	ccd_serial_write (pd, cmd, 4);
}

void EdtSao::writeSkip (bool parallel, int size, int &addr)
{
	for (int i = 0; i < size; i++)
		writeCommand (parallel, addr++, SKIP);
}

int EdtSao::writeReadPattern (bool parallel, int size, int bin, int &addr)
{
	int i = 0;
	if (bin == 1)
	{
		for (; i < size; i++)
			writeCommand (parallel, addr++, READ);
		return size;
	}
	else
	{
		while (i + bin < size)
		{
			for (int j = 0; j < bin; j++, i++)
				writeCommand (parallel, addr++, ADD);
			writeCommand (parallel, addr++, READ);
		}
		int used_i = i;
		for (; i < size; i++)
			writeCommand (parallel, addr++, SKIP);
		return used_i;
	}
}

int EdtSao::writePattern ()
{
	// write parallel commands
	int addr = 0;
	if (skipLines->getValueInteger () == lastSkipLines
		&& jiggleLines->getValueInteger () == lastJiggleLines
	  	&& chipUsedReadout->getXInt () == lastX
		&& chipUsedReadout->getYInt () == lastY
		&& chipUsedReadout->getWidthInt () == lastW
		&& chipUsedReadout->getHeightInt () == lastH)
	{
		return 0;
	}

	lastSkipLines = skipLines->getValueInteger ();
	lastJiggleLines = jiggleLines->getValueInteger ();
	lastX = chipUsedReadout->getXInt ();
	lastY = chipUsedReadout->getYInt ();
	lastW = chipUsedReadout->getWidthInt ();
	lastH = chipUsedReadout->getHeightInt ();

	logStream (MESSAGE_DEBUG) << "starting to write pattern with hskip=" << skipLines->getValueInteger ()
		<< " jiggle=" << jiggleLines->getValueInteger ()
		<< " getUsedX=" << getUsedX () 
		<< " getUsedY=" << getUsedY ()
		<< " getUsedHeight=" << getUsedHeight ()
		<< " getUsedWidth=" << getUsedWidth ()
		<< sendLog;
	// CCD readout pattern. The two important commands are SKIP and READ. Both does what their name
	// suggest. The various parameters affecting which part of CCD will be read out are described bellow:
	//
	//
	//  _ and | - software boundaries (e.g. how controller generates signal)
	//  X       - real chip size (excluding prescan,..)
	//
	//   ______XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX___________     _   HEIGHT (rest)
	//   |     X                                           X           |
	//   |   SKIPPED (to clear full chip, can be dropped if needed)    |
	//   |_____X___________________________________________X___________|    _   getUsedHeight (ROI, WINDOW)
	//   |     X         |                          |      X           |    
	//   |PRE  X         |  ACTIVE (READ)           |  SKIPED          |
	//   |SCAN X         |    PIXELS = ROI          |  (end of window) |
	//   |     X         |   = IMAGE you should get |  (can be dropped)|
	//   |_____X_________|__________________________|______X___________|    _   getUsedY (ROI, WINDOW)
	//   |     X                                           X           |
	//   |   SKIPPED (for Window..0 when reading full chip)X           |
	//   |_____XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX___________|    _   0
	//
	//
	//   parameters affecting row
	//   |     |         |                          |                  |
	//   0   HSKIP    getUsedX (ROI, WINDOW)    getUsedWitdh (ROI,..) WIDTH  (rest)
	//
	//   Please note, that controller does not know geometry of the device. Everything
	//   is left on you - you must specify correct parameters.
	//   Of course getUsedX can be 0, WIDTH can be full chip width,.. - then you get image
	//   from full device. And if you set HSKIP to 0, you will receive full image.
	//   including both software (noise) and hardware (black) pixels.
	//
	//   You might modify parameters to get different signal geometry.
	//
	// -------------------------------
	// write paraller commands to read lines
	// note true as the first parameter to writeXXX - those are PARALLER (vertical) commands
	// start paraller section
	writeCommand (true, addr++, ZERO);

	// first, jiggle some pixels
	for (int i = 0; i < jiggleLines->getValueInteger (); i++)
		writeCommand (true, addr++, JIGGLE);

	// skip some lines if getUsedY - window - is not 0
	writeSkip (true, getUsedY (), addr);

	// write lines till getUsedHeight line
	setUsedHeight (writeReadPattern (true, getUsedHeight (), binningVertical () > 0 ? binningVertical () : 1, addr));
	// skip lines till end (top) of CCD
	writeSkip (true, getHeight () - getUsedHeight () - getUsedY (), addr);

	// end paraller (vertical) section
	writeCommand (true, addr++, VEND);

	// ---------------------------------------
	// write serial commands to read pixels
	// note false as the first parameter to writeXX - those aren't PARALLER commands
	addr = 0;
	// start serial section
	writeCommand (false, addr++, ZERO);
	// skip some lines - HSKIP
	writeSkip (false, skipLines->getValueInteger (), addr);

	// skip more lines, if we are in window mode
	writeSkip (false, getUsedX (), addr);
	// write read pixels
	chipUsedReadout->setWidth (writeReadPattern (false, getUsedWidth (), binningHorizontal () > 0 ? binningHorizontal () : 1, addr));
	// skip pixels till end of line
	writeSkip (false, getWidth () - getUsedWidth () - getUsedX (), addr);

	// end serial 
	writeCommand (false, addr++, HEND);
	// end pattern
	writeCommandEnd ();
	logStream (MESSAGE_DEBUG) << "pattern written" << sendLog;
	return 0;
}

int EdtSao::writePartialPattern ()
{
 	if (partialReadout->getValueInteger () == lastPartialReadout
		&& chipUsedReadout->getXInt () == lastX
		&& chipUsedReadout->getWidthInt () == lastW)
		return 0;

	int addr = 0;
	lastPartialReadout = partialReadout->getValueInteger ();
	lastX = chipUsedReadout->getXInt ();
	lastW = chipUsedReadout->getWidthInt ();

	writeCommand (true, addr++, ZERO);
	// read..
	int i;
	if (lastPartialReadout > 0)
	{
		for (i = 0; i < lastPartialReadout; i++)
			writeCommand (true, addr++, READ);
	}
	else
	{
		for (i = lastPartialReadout; i < 0; i++)
			writeCommand (true, addr++, FRD);
	}
	writeCommand (true, addr++, VEND);
	writeCommand (false, addr++, ZERO);
	int width;

	// skip in X axis..
	for (i = 0; i < getUsedX (); i++)
		writeCommand (false, addr++, SKIP);
	width = getUsedX () + getUsedWidth ();
	for (; i < width; i++)
		writeCommand (false, addr++, READ);
	for (; i < getWidth (); i++)
		writeCommand (false, addr++, SKIP);

	writeCommand (false, addr++, HEND);
	writeCommandEnd ();
	logStream (MESSAGE_DEBUG) << "partial pattern written" << sendLog;
	return 0;
}

int EdtSao::startExposure ()
{
	int ret;

	for (size_t i = 0; i < channels->size (); i++)
	{
		setChannel (i, (*channels)[i], i + 1 == channels->size ());
	}

	if (partialReadout->getValueInteger () != 0)
	{
		ret = writePartialPattern ();
		chipUsedReadout->setInts (chipUsedReadout->getXInt (), chipUsedReadout->getYInt (), chipUsedReadout->getWidthInt (), (int) (fabs (partialReadout->getValueInteger ())));
		dofcl->setValueBool (false);
	}
	else
	{
		if (lastPartialReadout != partialReadout->getValueInteger ())
		{
			lastPartialReadout = partialReadout->getValueInteger ();
			setSize (chipWidth->getValueInteger (), chipHeight->getValueInteger (), 0, 0);
			lastW = -1;
			dofcl->setValueBool (false);
		}
		// create and write pattern..
		ret = writePattern ();
	}
	sendValueAll (dofcl);
	if (ret)
		return ret;
	writeBinFile ("e2v_nidlesc.bin");
	usleep (USEC_SEC / 2);
	if (dofcl->getValueBool ())
		fclr_r (fclrNum->getValueInteger ());

//	writeBinFile ("e2v_freezesc.bin");

	// taken from expose.c
	/* set time */
	switch (controllerType)
	{
		case CHANNEL_16:
			if (getExposure () < 10.0)
			{
				edtwrite (0x500000c0);
				edtwrite (0x52000000 | (long) (getExposure () * 1000));
				break;
			}
			edtwrite (0x50000080);
		case CHANNEL_4:
			edtwrite (0x52000000 | (long) (getExposure () * 100));
			break;
	}

	switch (controllerType)
	{
		case CHANNEL_16:
			if (getExpType () == 1)
			{
				// dark exposure
				edtwrite (0x50030300);
				break;
			}
		case CHANNEL_4:
			edtwrite (0x50030000);		 /* open shutter */
			break;
	}

	return 0;
}

int EdtSao::stopExposure ()
{
	edtwrite (0x50020000);		 /* close shutter */
	return Camera::stopExposure ();
}

long EdtSao::isExposing ()
{
	int ret;
	ret = Camera::isExposing ();
	if (ret != -2)
		return ret;
	// taken from expose.c
	probe ();
	if ((!overrun && shutter) || overrun)
		return 100;
	pdv_serial_wait (pd, 100, 4);
	usleep (shutterDelay->getValueFloat () * USEC_SEC);
//	writeBinFile ("e2v_unfreezesc.bin");
	return -2;
}

int EdtSao::readoutStart ()
{
	int ret;
	int width, height;
	int db;
	int set_timeout = 0;
	int timeout_val = 7000;

	// taken from kepler.c
	pdv_set_header_dma (pd, 0);
	pdv_set_header_size (pd, 0);
	pdv_set_header_offset (pd, 0);

	width = getUsedWidth ();

	if (partialReadout->getValueInteger () != 0)
	{
		height = partialReadout->getValueInteger ();
		if (height < 0)
			height *= -1;
	}
	else
	{
		height = getUsedHeight ();
	}
	ret = pdv_setsize (pd, width * getUsedChannels (), height);
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "call to pdv_setsize failed" << sendLog;
		return -1;
	}
	depth = pdv_get_depth (pd);
	db = bits2bytes (depth);
	imagesize = chipUsedReadout->getWidthInt () * height * db;

	ret = Camera::readoutStart ();
	if (ret)
		return ret;

	/*
	 * SET TIMEOUT
	 * make about 1.5 * readout time + 2 sec
	 */
	if (!set_timeout)
		timeout_val = (chipUsedReadout->getWidthInt ()) * height / 1000 * timeout_val * 5 / 2000 + 2000 + jiggleLines->getValueInteger () * 10;
	pdv_set_timeout (pd, timeout_val);
	logStream (MESSAGE_DEBUG) << "timeout_val: " << timeout_val << " millisecs" << sendLog;

	/*
	 * ALLOCATE MEMORY for the image, and make sure it's aligned on a page
	 * boundary
	 */
	pdv_flush_fifo (pd);		 /* MC - add a flush */
	pdv_set_buffers (pd, 1, NULL);
	bufs = edt_buffer_addresses(pd);
	return 0;
}

int EdtSao::sendChannel (int chan, u_char *buf, int chanorder, int totalchanel)
{
	uint16_t *sb = new uint16_t[chipUsedSize ()];
	uint16_t *psb = sb;
	for (uint16_t *p = ((uint16_t *) buf) + chanorder; (u_char*) p < buf + chipByteSize () * totalchanel; p += totalchanel)
	{
		*psb = *p;
		psb++;
	}
	sendReadoutData ((char *) sb, getWriteBinaryDataSize (chanorder), chanorder);
	delete[] sb;
	return 0;
}

int EdtSao::doReadout ()
{
	int i, j;
	int ret;
	int x = -1;

	/*
	 *  ACQUIRE THE IMAGE
	 */
	logStream (MESSAGE_DEBUG)
		<< "reading image from " << pdv_get_cameratype (pd)
		<< " width " << pdv_get_width (pd)
		<< " height " << pdv_get_height (pd)
		<< " depth " << pdv_get_depth (pd)
		<< sendLog;

	markReadoutStart ();

	int tb = pdv_timeouts (pd);

	pdv_start_image (pd);

	// sendread
	edtwrite (0x4e000000);
	edtwrite (0x00000000);

	/* READ AND PROCESS ONE BUFFER = IMAGE */
	bufs[0] = pdv_wait_image (pd);

	updateReadoutSpeed (getReadoutPixels ());

	pdv_flush_fifo (pd);
	int ta = pdv_timeouts (pd);
	if (ta > tb)
	{
		logStream (MESSAGE_ERROR) << "got a timeout" << sendLog;
		return -1;
	}

	/* BYTE SWAP FOR UNIX */

	if (!ft_byteswap ())
	{
		for (j = 0; j < imagesize; j += 2)
		{
			u_char bufa = bufs[0][j];
			u_char bufb = bufs[0][j + 1];
			bufs[0][j] = bufb;
			bufs[0][j + 1] = bufa;
		}
	}

	/* PRINTOUT FIRST AND LAST PIXELS - DEBUG */

	if (verbose)
	{
		printf ("first 8 pixels\n");
		for (j = 0; j < 16; j += 2)
		{
			x = (bufs[0][j + 1] << 8) + bufs[0][j];
			printf ("%d ", x);
		}
		printf ("\nlast 8 pixels\n");
		for (j = imagesize - 16; j < imagesize; j += 2)
		{
			x = (bufs[0][j + 1] << 8) + bufs[0][j];
			printf ("%d ", x);
		}
		printf ("\n");
	}

	//pdv_free (bufs[0]);

	for (i = 0, j = 0; i < totalChannels; i++)
	{
		if ((*channels)[i])
		{
			ret = sendChannel (i, bufs[0], j, getUsedChannels ());
			if (ret < 0)
				return -1;
			j++;
		}
	}

	return -2;
}

int EdtSao::endReadout ()
{
	if (partialReadout->getValueInteger () == 0)
	{
		pdv_stop_continuous (pd);
		pdv_flush_fifo (pd);
		pdv_reset_serial (pd);
		edt_reg_write (pd, PDV_CMD, PDV_RESET_INTFC);
		writeBinFile ("e2v_pidlesc.bin");
	}
	return Camera::endReadout ();
}

EdtSao::EdtSao (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
	devname[0] = '\0';
	devunit = 0;
	pd = NULL;
	verbose = false;

	not_write = false;

	notimeout = 0;
	sdelay = 0;

	controllerType = CHANNEL_4;

	createExpType ();

	createDataChannels ();
	totalChannels = 4;

	addOption ('p', "devname", 1, "device name");
	addOption ('n', "devunit", 1, "device unit number");
	addOption ('W', NULL, 1, "chip width - number of collumns");
	addOption ('H', NULL, 1, "chip height - number of rows/lines");
	// add overscan pixel option
	addOption ('S', "hskip", 1, "number of lines to skip (overscan pixels)");
	addOption (OPT_NOTIMEOUT, "notimeout", 0, "don't timeout");
	addOption (OPT_NOWRITE, "nowrite", 0, "dry run - do not write to device");
	addOption ('s', "sdelay", 1, "serial delay");
	addOption ('v', "verbose", 0, "verbose report");
	addOption ('6', NULL, 0, "sixteen channel readout electronics");
	addOption (OPT_SIGNALFILE, "signalfile", 1, "default signal file");

	createValue (shutterDelay, "SHUTDEL", "[s] delay before readout (after closing shutter)", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	shutterDelay->setValueFloat (0);

	createValue (edtGain, "GAIN", "gain (high or low)", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	edtGain->addSelVal ("HIGH");
	edtGain->addSelVal ("LOW");
	edtGain->setValueInteger (0);

	createValue (signalFile, "SIGFILE", "signal (low-level) file", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (signalFileDir, "SIGFILEDIR", "default directory with signal files", true, RTS2_VALUE_WRITABLE);
	createValue (sigtosc, "SIGTOSC", "sigtosc.pl binary location", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL);

	createValue (edtSplit, "SPLIT", "split mode (on or off)", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF, CAM_WORKING);
	edtSplit->setValueBool (true);

	createValue (edtUni, "UNI", "uni mode (on or off)", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF, CAM_WORKING);
	edtUni->setValueBool (false);

	createValue (parallelClockSpeed, "PCLOCK", "parallel clock speed", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	parallelClockSpeed->setValueInteger (0);

	createValue (skipLines, "hskip", "number of lines to skip (as those contains bias values)", true, RTS2_VALUE_WRITABLE);
	skipLines->setValueInteger (0);

	createValue (jiggleLines, "jiggle", "number of jiggle lines", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	jiggleLines->setValueInteger (0);

	createValue (dofcl, "DOFCLR", "[bool] if fast clear was done", true, RTS2_VALUE_WRITABLE);
	createValue (fclrNum, "FCLR_NUM", "number of fast clears done before exposure", true, RTS2_VALUE_WRITABLE);
	fclrNum->setValueInteger (5);

	createValue (fclrFailed, "FCLR_FAILED", "number of failed FCLR attemps");
	fclrFailed->setValueInteger (0);

	createValue (chipWidth, "width", "chip width - number of collumns", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	chipWidth->setValueInteger (2024);

	createValue (chipHeight, "height", "chip height - number of rows", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	chipHeight->setValueInteger (520);

	createValue (partialReadout, "partial_readout", "read only part of image, do not clear the chip", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	partialReadout->setValueInteger (0);

	grayScale = NULL;

	createValue (vhi, "VHI", "[V] V high", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (vlo, "VLO", "[V] V low", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (phi, "PHI", "[V] P high", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (plo, "PLO", "[V] P low", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (shi, "SHI", "[V] S high", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (slo, "SLO", "[V] S low", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (rhi, "RHI", "[V] R high", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (rlo, "RLO", "[V] R low", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (rd, "RD", "[V] RD", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (od1r, "OD1_R", "[V] OD 1 right", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (od2l, "OD2_L", "[V] OD 2 left", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (og1r, "OG1_R", "[V] OG 1 right", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (og2l, "OG2_L", "[V] OG 2 left", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);
	createValue (dd, "DD", "[V] DD", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_NOTNULL, CAM_WORKING);

	// init last used modes - for writePattern
	lastSkipLines = lastJiggleLines = lastX = lastY = lastW = lastH = lastSplitMode = lastPartialReadout = -1;
}

EdtSao::~EdtSao (void)
{
	edt_close (pd);
}

int EdtSao::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'p':
			strncpy (devname, optarg, 16);
			if (devname[5] == 's')
				ccd_setdev (1);
			break;
		case 'n':
			devunit = atoi (optarg);
			break;
	        case 'H':
			chipHeight->setValueCharArr (optarg);
			break;
		case 'W':
			chipWidth->setValueCharArr (optarg);
			break;
	        case 'S':
	                skipLines -> setValueCharArr (optarg);
	                break;
		case OPT_NOTIMEOUT:
			notimeout = true;
			break;
		case OPT_NOWRITE:
			not_write = true;
			break;
		case 's':
			sdelay = atoi (optarg);
			break;
		case 'v':
			verbose = true;
			break;
		case '6':
			controllerType = CHANNEL_16;
			totalChannels = 16;
			break;
		case OPT_SIGNALFILE:
			signalFile->setValueCharArr (optarg);
			break;
		default:
			return Camera::processOption (in_opt);
	}
	return 0;
}

int EdtSao::setEdtValue (ValueEdt * old_value, float new_value)
{
	if (old_value->testValue (new_value) == false)
		return -2;
	old_value->setValueDouble (new_value);
	return edtwrite (old_value->getHexValue (new_value, controllerType));
}

int EdtSao::setEdtValue (ValueEdt * old_value, rts2core::Value * new_value)
{
	int ret = edtwrite (old_value->getHexValue (new_value->getValueFloat (), controllerType));
	logStream (MESSAGE_INFO) << "setting "<< old_value->getName () << " to " << new_value->getValueFloat () << sendLog;
	if (old_value == rd)
		sleep (1);
	return ret;
}

int EdtSao::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == chipHeight)
	{
		setSize (chipWidth->getValueInteger (), new_value->getValueInteger (), 0, 0);
		return 0;
	}
	if (old_value == chipWidth)
	{
		setSize (new_value->getValueInteger (), chipHeight->getValueInteger (), 0, 0);
		return 0;
	}
	if (old_value == vhi
	  	|| old_value == vlo
	  	|| old_value == phi
		|| old_value == plo
		|| old_value == shi
		|| old_value == slo
		|| old_value == rhi
		|| old_value == rlo
		|| old_value == rd
		|| old_value == od1r
		|| old_value == od2l
		|| old_value == og1r
		|| old_value == og2l
		|| old_value == dd
		)
	{
		return setEdtValue ((ValueEdt *) old_value, new_value);
	}
	if (old_value == edtGain)
	{
		return (setEDTGain (new_value->getValueInteger () == 0) == 0 ? 0 : -2);
	}
	if (old_value == parallelClockSpeed)
	{
		return (setParallelClockSpeed (new_value->getValueInteger ()) == 0 ? 0 : -2);
	}
	if (old_value == grayScale)
	{
		return (setGrayScale (((rts2core::ValueBool *) new_value)->getValueBool ())) == 0 ? 0 : -2;
	}
	if (old_value == edtSplit)
	{
		return (setEDTSplit (((rts2core::ValueBool *) new_value)->getValueBool ())) == 0 ? 0 : -2;
	}
	if (old_value == edtUni)
	{
		return (setEDTUni (((rts2core::ValueBool *) new_value)->getValueBool ())) == 0 ? 0 : -2;
	}
	if (old_value == signalFile)
	{
		return writeSignalFile (new_value->getValue ()) == 0 ? 0 : -2;
	}

	if (old_value == ADoffsets)
	{
		for (int i = 0; i < totalChannels; i++)
		{
			if ((*ADoffsets)[i] != (*((rts2core::IntegerArray *) new_value))[i])
			{
				int ret = setADOffset (i, (*((rts2core::IntegerArray *) new_value))[i]);
				if (ret)
					return -2;
			}
		}
		return 0;
	}
	return Camera::setValue (old_value, new_value);
}

int EdtSao::init ()
{
	int ret;
	ret = Camera::init ();
	if (ret)
		return ret;

	// open CCD
	pd = ccd_gopen (devname, devunit);
	if (pd == NULL)
	{
		logStream (MESSAGE_ERROR) << "cannot init " << devname << " unit " << devunit << sendLog;
		return -1;
	}

	if (notimeout)
		ccd_picture_timeout (pd, 0);

	ccd_set_serial_delay (pd, sdelay);

/*	switch (controllerType)
	{
		case CHANNEL_16: */
			createValue (grayScale, "gray_scale", "turns on simulated grayscale", true, RTS2_VALUE_WRITABLE);
			grayScale->setValueBool (false);
/*			break;
		case CHANNEL_4:
			break;
	} */

	createValue (ADoffsets, "ADO", "[ADU] channels A/D offsets", true, RTS2_VALUE_WRITABLE | RTS2_FITS_HEADERS, CAM_WORKING);

	dataChannels->setValueInteger (totalChannels);

	for (int i = 0; i < totalChannels; i++)
	{
		channels->addValue (true);
		ADoffsets->addValue (0);
	}

	setSize (chipWidth->getValueInteger (), chipHeight->getValueInteger (), 0, 0);

	writePattern ();

	return 0;
}

int EdtSao::initValues ()
{
	addConstValue ("DEVNAME", "device name", devname);
	addConstValue ("DEVNUM", "device unit number", devunit);
	addConstValue ("DEVNT", "device no timeout", notimeout);
	addConstValue ("SDELAY", "device serial delay", sdelay);

	vhi->initEdt (0xA0080, A_plus, controllerType);
	vlo->initEdt (0xA0188, A_minus, controllerType);
	phi->initEdt (0xA0084, P_Harvard, controllerType);
	plo->initEdt (0xA0184, P_Harvard, controllerType);
	shi->initEdt (0xA008C, S_Harvard, controllerType);
	slo->initEdt (0xA0180, S_Harvard, controllerType);
	rhi->initEdt (0xA0088, P_Harvard, controllerType);
	rlo->initEdt (0xA018C, P_Harvard, controllerType);
	rd->initEdt (0xA0384, C, controllerType);
	od1r->initEdt (0xA0388, D_Harvard, controllerType);
	od2l->initEdt (0xA038C, D_Harvard, controllerType);
	og1r->initEdt (0xA0288, B, controllerType);
	og2l->initEdt (0xA028C, B, controllerType);
	
	switch (controllerType)
	{
		case CHANNEL_4:
			dd->initEdt (0xA0380, C, controllerType);
			break;
		case CHANNEL_16:
			dd->initEdt (0xA0380, D, controllerType);
			break;
	}

	return Camera::initValues ();
}

int EdtSao::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("reset"))
	{
		if (!conn->paramEnd ())
			return -2;
		reset ();
		return 0;
	}
	return Camera::commandAuthorized (conn);
}

int main (int argc, char **argv)
{
	EdtSao device (argc, argv);
	return device.run ();
}
