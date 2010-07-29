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

#define OPT_NOTIMEOUT  OPT_LOCAL + 3

/*
 * Depends on libedtsao.a, which is build from edtsao directory. This
 * contains unmodified files from EDT-SAO distribution, and should be
 * update regulary.
 */

namespace rts2camd
{

typedef enum {A_plus, A_minus, B, C, D} edtAlgoType;

/**
 * Variable for EDT-SAO registers. It holds information
 * about which register the variable represents and which
 * algorithm is used to convert it to register hex value.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueEdt: public Rts2ValueDoubleMinMax
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
		void initEdt (long in_reg, edtAlgoType in_algo);

		/**
		 * Convert float value to register value.
		 *
		 * @param in_v float value which will be converted
		 *
		 * @return hex value of in_v
		 */
		long getHexValue (float in_v);
	private:
		// EDT - SAO register. It will be shifted by 3 bytes left and ored with value calculated by algo
		long reg;
		// algorith used to compute hex suffix value
		edtAlgoType algo;
};

}

using namespace rts2camd;

ValueEdt::ValueEdt (std::string in_val_name):Rts2ValueDoubleMinMax (in_val_name)
{
}

ValueEdt::ValueEdt (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):Rts2ValueDoubleMinMax (in_val_name, in_description, writeToFits, flags)
{

}

ValueEdt::~ValueEdt (void)
{
}

void ValueEdt::initEdt (long in_reg, edtAlgoType in_algo)
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
			setMin (-10);
			setMax (0);
			break;
		case B:
			setMin (-5);
			setMax (5);
			break;
		case C:
			setMin (0);
			setMax (20);
			break;
		case D:
			setMin (0);
			setMax (25.9);
			break;
	}
}

long ValueEdt::getHexValue (float in_v)
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
			val = (long) ((5 - in_v) * 409.5);
			break;
		case C:
			val = (long) (in_v * 204.7);
			break;
		case D:
			val = (long) (in_v * 158.0);
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

		virtual int scriptEnds ();

	protected:
		virtual int processOption (int in_opt);
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		virtual int initChips ();
		virtual void initBinnings ();

		virtual int startExposure ();
		virtual int stopExposure ();
		virtual long isExposing ();
		virtual int readoutStart ();

		virtual long suggestBufferSize () { return -1; }

		virtual int doReadout ();
		virtual int endReadout ();

	private:
		PdvDev * pd;
		char devname[16];
		int devunit;
		bool notimeout;
		int sdelay;

		enum {CHANNEL_4, CHANNEL_16} controllerType;

		u_int status;

		Rts2ValueSelection *edtGain;
		Rts2ValueInteger *parallelClockSpeed;
		// number of lines to skip in serial mode
		Rts2ValueInteger *skipLines;

		Rts2ValueBool *edtSplit;
		Rts2ValueBool *edtUni;

		bool verbose;

		int edtwrite (unsigned long lval);

		int depth;

		bool shutter;
		bool overrun;

		u_char **bufs;
		int imagesize;

		int writeBinFile (const char *filename);

		Rts2ValueBool **channels;
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
		 * Setup D/A controller. This takes system defaults. It is
		 * called only if camera reset was specified on command line,
		 * as one of the parameters.
		 */
		int setDAC ();

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

		Rts2ValueInteger **ADoffsets;

		int setEdtValue (ValueEdt * old_value, float new_value);
		int setEdtValue (ValueEdt * old_value, Rts2Value * new_value);

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
		Rts2ValueInteger *chipWidth;

		// number of rows
		Rts2ValueInteger *chipHeight;

		// number of rows to skip. If negative, rows will be moved up. Partial readout is not active if this equals to 0.
		Rts2ValueInteger *partialReadout;

		// Grayscale off/on
		Rts2ValueBool *grayScale;

		int lastSkipLines;
		int lastX;
		int lastY;
		int lastW;
		int lastH;
		int lastSplitMode;
		int lastPartialReadout;

		int sendChannel (int chan, u_char *buf, int chanorder, int totalchanel);
};

};

int EdtSao::edtwrite (unsigned long lval)
{
	unsigned long lsval = lval;
	if (ft_byteswap ())
		swap4 ((char *) &lsval, (char *) &lval, sizeof (lval));
	std::cerr << "edtwrite 0x" << std::hex << lval << std::endl;
	ccd_serial_write (pd, (u_char *) (&lsval), 4);
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

	std::string full_name = std::string ("/home/ccdtest/bin/") + std::string (filename);

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
	logStream (MESSAGE_DEBUG) << "From " << full_name
		<< " written " << loops << " serial commands."
		<< sendLog;
	return 0;
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

int EdtSao::setDAC ()
{
	// values taken from ccdsetup script

/*	setEdtValue (rd, 9);
	sleep (1);

	setEdtValue (phi, 2);
	setEdtValue (plo, -9);

	setEdtValue (shi, 3);
	setEdtValue (slo, -7);

	setEdtValue (rhi, 5);
	setEdtValue (rlo, -5);

	setEdtValue (od1r, 20);
	setEdtValue (od2l, 20);

	setEdtValue (og1r, -5);
	setEdtValue (og2l, -2);

	setEdtValue (dd, 15); */

	return 0;
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

	ADoffsets[ch]->setValueInteger (offset);

	return 0;
}

void EdtSao::setChannel (int ch, bool enabled, bool last)
{
	uint32_t o = 0x51000000 | (ch << 8) | ch | (enabled ? 0x0040 : 0x0000) | (last ? 0x8000 : 0x0000);
	
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
	while (fclr (num) != 0)
	{
		logStream (MESSAGE_ERROR)
			<< "Cannot do fclr, trying again." << sendLog;
		sleep (10);
	}
}

int EdtSao::setParallelClockSpeed (int new_speed)
{
  	unsigned long ns = 0x46000000;	
	ns |= (new_speed & 0x00ff);
	return edtwrite (ns);
}

int EdtSao::setGrayScale (bool _grayScale, int board)
{
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
}

int EdtSao::initChips ()
{
	int ret;

	depth = 2;

	// do initialization
	reset ();

	ret = setEDTGain (true);
	if (ret)
		return ret;
	ret = edtwrite (SAO_PARALER_SP);
	if (ret)
		return ret;

	ret = writeBinFile ("e2vsc.bin");
	if (ret)
		return ret;

	ret = writeBinFile ("e2v_pidlesc.bin");
	if (ret)
		return ret;

	setSize (chipWidth->getValueInteger (), chipHeight->getValueInteger (), 0, 0);

	ret = setDAC ();

	channels = new Rts2ValueBool*[totalChannels];
	ADoffsets = new Rts2ValueInteger*[totalChannels];

	char *chname = new char[8];
	char *daoname = new char[7];

	for (int i = 0; i < totalChannels; i++)
	{
		sprintf (chname, "CHAN_%02d", i + 1);
		createValue (channels[i], chname, "channel on/off", true, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE, CAM_WORKING);
		channels[i]->setValueBool (true);

		sprintf (daoname, "DAO_%02d", i + 1);
		createValue (ADoffsets[i], daoname, "[ADU] D/A offset", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
		setADOffset (i, 0x100);
	}

	delete[] daoname;
	delete[] chname;

	dataChannels->setValueInteger (totalChannels);

	ret = setEDTSplit (edtSplit->getValueBool ());
	if (ret)
		return ret;
	ret = setEDTUni (edtUni->getValueBool ());

	return ret;
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
	  	&& chipUsedReadout->getXInt () == lastX
		&& chipUsedReadout->getYInt () == lastY
		&& chipUsedReadout->getWidthInt () == lastW
		&& chipUsedReadout->getHeightInt () == lastH)
	{
		return 0;
	}

	lastSkipLines = skipLines->getValueInteger ();
	lastX = chipUsedReadout->getXInt ();
	lastY = chipUsedReadout->getYInt ();
	lastW = chipUsedReadout->getWidthInt ();
	lastH = chipUsedReadout->getHeightInt ();

	logStream (MESSAGE_DEBUG) << "starting to write pattern" << sendLog;
	writeCommand (true, addr++, ZERO);
	
	writeSkip (true, getUsedY (), addr);
	chipUsedReadout->setHeight (writeReadPattern (true, getUsedHeight (), binningVertical (), addr));
	writeSkip (true, getHeight () - getUsedHeight () - getUsedY (), addr);

	writeCommand (true, addr++, VEND);
	// write serial commands
	addr = 0;
	writeCommand (false, addr++, ZERO);
	writeSkip (false, skipLines->getValueInteger (), addr);

	writeSkip (false, getUsedX (), addr);
	chipUsedReadout->setWidth (writeReadPattern (false, getUsedWidth (), binningHorizontal (), addr));
	writeSkip (false, getWidth () - getUsedWidth () - getUsedX (), addr);

	writeCommand (false, addr++, HEND);
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

	dataChannels->setValueInteger (0);

	// write channels and their order..
	for (int i = 0; i < totalChannels; i++)
	{
		setChannel (i, channels[i]->getValueBool (), i + 1 == totalChannels);
		if (channels[i]->getValueBool ())
			dataChannels->inc ();
	}

	sendValueAll (dataChannels);

	bool dofcl = true;

	if (partialReadout->getValueInteger () != 0)
	{
		ret = writePartialPattern ();
		chipUsedReadout->setInts (chipUsedReadout->getXInt (), chipUsedReadout->getYInt (), chipUsedReadout->getWidthInt (), (int) (fabs (partialReadout->getValueInteger ())));
		dofcl = false;
	}
	else
	{
		if (lastPartialReadout != partialReadout->getValueInteger ())
		{
			lastPartialReadout = partialReadout->getValueInteger ();
			setSize (chipWidth->getValueInteger (), chipHeight->getValueInteger (), 0, 0);
			lastW = -1;
			dofcl = false;
		}
		// create and write pattern..
		ret = writePattern ();
	}
	if (ret)
		return ret;
	writeBinFile ("e2v_nidlesc.bin");
	if (dofcl == true)
		fclr_r (5);
	writeBinFile ("e2v_freezesc.bin");

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
	if (ret)
		return ret;
	// taken from expose.c
	probe ();
	if ((!overrun && shutter) || overrun)
		return 100;
	pdv_serial_wait (pd, 100, 4);
	writeBinFile ("e2v_unfreezesc.bin");
	return 0;
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
	ret = pdv_setsize (pd, width * dataChannels->getValueInteger (), height);
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "startExposure failed in pdv_setsize" << sendLog;
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
		timeout_val = (chipUsedReadout->getWidthInt ()) * height / 1000 * timeout_val * 5 / 2000 + 2000;
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
	u_int dsx1, dsx2;
	short diff;
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
	int tb = pdv_timeouts (pd);

	pdv_start_image (pd);

	// sendread
	edtwrite (0x4e000000);
	edtwrite (0x00000000);

	/* READ AND PROCESS ONE BUFFER = IMAGE */
	bufs[0] = pdv_wait_image (pd);

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

	pdv_free (bufs[0]);

	// swap for split mode

	int width = chipUsedReadout->getWidthInt ();

	for (i = 0, j = 0; i < totalChannels; i++)
	{
		if (channels[i]->getValueBool ())
		{
			ret = sendChannel (i, bufs[0], j, dataChannels->getValueInteger ());
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
	addOption ('s', "sdelay", 1, "serial delay");
	addOption ('v', "verbose", 0, "verbose report");
	addOption ('6', NULL, 0, "sixteen channel readout electronics");

	createValue (edtGain, "GAIN", "gain (high or low)", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	edtGain->addSelVal ("HIGH");
	edtGain->addSelVal ("LOW");

	createValue (edtSplit, "SPLIT", "split mode (on or off)", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF, CAM_WORKING);
	edtSplit->setValueBool (true);

	createValue (edtSplit, "UNI", "uni mode (on or off)", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF, CAM_WORKING);
	edtSplit->setValueBool (false);

	edtGain->setValueInteger (0);

	createValue (parallelClockSpeed, "PCLOCK", "parallel clock speed", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	parallelClockSpeed->setValueInteger (0);

	createValue (skipLines, "hskip", "number of lines to skip (as those contains bias values)", true, RTS2_VALUE_WRITABLE);
	skipLines->setValueInteger (0);

	createValue (chipWidth, "width", "chip width - number of collumns", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	chipWidth->setValueInteger (2024);

	createValue (chipHeight, "height", "chip height - number of rows", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	chipHeight->setValueInteger (520);

	createValue (partialReadout, "partial_readout", "read only part of image, do not clear the chip", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	partialReadout->setValueInteger (0);

	grayScale = NULL;

	createValue (vhi, "VHI", "[V] V high", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	vhi->initEdt (0xA0080, A_plus);

	createValue (vlo, "VLO", "[V] V low", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	vlo->initEdt (0xA0188, A_minus);

	createValue (phi, "PHI", "[V] P high", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	phi->initEdt (0xA0084, A_plus);

	createValue (plo, "PLO", "[V] P low", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	plo->initEdt (0xA0184, A_minus);

	createValue (shi, "SHI", "[V] S high", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	shi->initEdt (0xA008C, A_plus);

	createValue (slo, "SLO", "[V] S low", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	slo->initEdt (0xA0180, A_minus);

	createValue (rhi, "RHI", "[V] R high", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	rhi->initEdt (0xA0088, A_plus);

	createValue (rlo, "RLO", "[V] R low", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	rlo->initEdt (0xA018C, A_minus);

	createValue (rd, "RD", "[V] RD", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	rd->initEdt (0xA0384, C);

	createValue (od1r, "OD1_R", "[V] OD 1 right", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	od1r->initEdt (0xA0388, D);

	createValue (od2l, "OD2_L", "[V] OD 2 left", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	od2l->initEdt (0xA038C, D);

	createValue (og1r, "OG1_R", "[V] OG 1 right", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	og1r->initEdt (0xA0288, B);

	createValue (og2l, "OG2_L", "[V] OG 2 left", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	og2l->initEdt (0xA028C, B);

	createValue (dd, "DD", "[V] DD", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	dd->initEdt (0xA0380, C);

	// init last used modes - for writePattern
	lastSkipLines = lastX = lastY = lastW = lastH = lastSplitMode = lastPartialReadout = -1;
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
	return edtwrite (old_value->getHexValue (new_value));
}

int EdtSao::setEdtValue (ValueEdt * old_value, Rts2Value * new_value)
{
	int ret = edtwrite (old_value->getHexValue (new_value->getValueFloat ()));
	logStream (MESSAGE_INFO) << "setting "<< old_value->getName () << " to " << new_value->getValueFloat () << sendLog;
	if (old_value == rd)
		sleep (1);
	return ret;
}

int EdtSao::setValue (Rts2Value * old_value, Rts2Value * new_value)
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
		return (setEDTGain (new_value->getValueInteger () == 0) ? 0 : -2);
	}
	if (old_value == parallelClockSpeed)
	{
		return (setParallelClockSpeed (new_value->getValueInteger ()) == 0 ? 0 : -2);
	}
	if (old_value == grayScale)
	{
		return (setGrayScale (((Rts2ValueBool *) new_value)->getValueBool ())) == 0 ? 0 : -2;
	}
	if (old_value == edtSplit)
	{
		return (setEDTSplit (((Rts2ValueBool *) new_value)->getValueBool ())) == 0 ? 0 : -2;
	}
	if (old_value == edtUni)
	{
		return (setEDTUni (((Rts2ValueBool *) new_value)->getValueBool ())) == 0 ? 0 : -2;
	}

	for (int i = 0; i < totalChannels; i++)
	{
		if (old_value == ADoffsets[i])
			return (setADOffset (i, new_value->getValueInteger ())) == 0 ? 0 : -2;
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
		logStream (MESSAGE_ERROR) << "cannot init " << devname << " unit " <<
			devunit << sendLog;
		return -1;
	}

	if (notimeout)
		ccd_picture_timeout (pd, 0);

	ccd_set_serial_delay (pd, sdelay);

	setParallelClockSpeed (parallelClockSpeed->getValueInteger ());

	switch (controllerType)
	{
		case CHANNEL_16:
			createValue (grayScale, "gray_scale", "turns on simulated grayscale", true, RTS2_VALUE_WRITABLE);
			grayScale->setValueBool (false);
			break;
		case CHANNEL_4:
			break;
	}

	return initChips ();
}

int EdtSao::initValues ()
{
	addConstValue ("DEVNAME", "device name", devname);
	addConstValue ("DEVNUM", "device unit number", devunit);
	addConstValue ("DEVNT", "device no timeout", notimeout);
	addConstValue ("SDELAY", "device serial delay", sdelay);
	return Camera::initValues ();
}

int EdtSao::scriptEnds ()
{
	edtGain->setValueInteger (0);
	setEDTGain (0);
	sendValueAll (edtGain);

	for (int i = 0; i < totalChannels; i++)
	{
		channels[i]->setValueBool (true);
		sendValueAll (channels[i]);
	}

	parallelClockSpeed->setValueInteger (6);
	setParallelClockSpeed (parallelClockSpeed->getValueInteger ());
	sendValueAll (parallelClockSpeed);

	return Camera::scriptEnds ();
}

int main (int argc, char **argv)
{
	EdtSao device = EdtSao (argc, argv);
	return device.run ();
}
