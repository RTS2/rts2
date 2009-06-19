/* 
 * EDT controller driver
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

// helper struct for splitModes
typedef struct SplitConf
{
	bool splitMode;
	bool uniMode;
	int chanNum;
	bool split;
	int ioPar1;
	int ioPar2;
};

const SplitConf splitConf[] =
{
	{false, false, 1, false, 0x51000040, 0x51008101},
	{false, true, 1, false, 0x51000000, 0x51008141},
	{true, false, 2, true, 0x51000040, 0x51008141}
};

typedef enum {A_plus, A_minus, B, C, D}
edtAlgoType;

/**
 * Special variable for EDT-SAO registers. It holds information
 * about which registers the variable represents.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueEdt: public Rts2ValueDoubleMinMax
{
	private:
		// EDT - SAO register. It will be shifted by 3 bytes left and ored with value calculated by algo
		long reg;
		// algorith used to compute hex suffix value
		edtAlgoType algo;
	public:
		ValueEdt (std::string in_val_name);
		ValueEdt (std::string in_val_name, std::string in_description,
			bool writeToFits = true, int32_t flags = 0);
		~ValueEdt (void);

		// init EDT part of variable
		void initEdt (long in_reg, edtAlgoType in_algo);

		/**
		 * @return -1 on error, otherwise hex value.
		 */
		long getHexValue (float in_v);
};

};

using namespace rts2camd;

ValueEdt::ValueEdt (std::string in_val_name)
:Rts2ValueDoubleMinMax (in_val_name)
{
}


ValueEdt::ValueEdt (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags)
:Rts2ValueDoubleMinMax (in_val_name, in_description, writeToFits, flags)
{

}


ValueEdt::~ValueEdt (void)
{
}


void
ValueEdt::initEdt (long in_reg, edtAlgoType in_algo)
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


long
ValueEdt::getHexValue (float in_v)
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
	private:
		PdvDev * pd;
		char devname[16];
		int devunit;
		bool notimeout;
		int sdelay;

		u_int status;

		// 0 - unsplit, 1 - left, 2 - right
		Rts2ValueSelection *splitMode;
		Rts2ValueSelection *edtGain;
		Rts2ValueInteger *parallelClockSpeed;
		// number of lines to skip in serial mode
		Rts2ValueInteger *skipLines;

		bool verbose;

		int edtwrite (unsigned long lval);

		int channels;
		int depth;

		bool shutter;
		bool overrun;

		int numbufs;

		u_char **bufs;
		int imagesize;
		int dsub;

		int writeBinFile (const char *filename);

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
		int setDAC ();

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

		// registers
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

		int setEdtValue (ValueEdt * old_value, float new_value);
		int setEdtValue (ValueEdt * old_value, Rts2Value * new_value);

		// write single command to controller
		void writeCommand (bool parallel, int addr, command_t command);
		void writeCommandEnd ();

		// write to controller pattern file
		int writePattern (const SplitConf *conf);

		// number of rows
		Rts2ValueInteger *chipHeight;

		int lastSkipLines;
		int lastX;
		int lastY;
		int lastW;
		int lastH;
		int lastSplitMode;

	protected:
		virtual int processOption (int in_opt);
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		virtual int initChips ();
		virtual int startExposure ();
		virtual int stopExposure ();
		virtual long isExposing ();
		virtual int readoutStart ();
		virtual int doReadout ();
		virtual void cancelPriorityOperations ();
		virtual int endReadout ();
	public:
		EdtSao (int in_argc, char **in_argv);
		virtual ~ EdtSao (void);

		virtual int init ();
		virtual int initValues ();
};

};

int
EdtSao::edtwrite (unsigned long lval)
{
	unsigned long lsval = lval;
	if (ft_byteswap ())
		swap4 ((char *) &lsval, (char *) &lval, sizeof (lval));
	ccd_serial_write (pd, (u_char *) (&lsval), 4);
	return 0;
}


int
EdtSao::writeBinFile (const char *filename)
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


void
EdtSao::reset ()
{
	pdv_flush_fifo (pd);
	pdv_reset_serial (pd);
	edt_reg_write (pd, PDV_CMD, PDV_RESET_INTFC);
	edt_reg_write (pd, PDV_MODE_CNTL, 1);
	edt_msleep (10);
	edt_reg_write (pd, PDV_MODE_CNTL, 0);

	sleep (1);
}


int
EdtSao::setDAC ()
{
	// values taken from ccdsetup script
	int ret;
	unsigned long edtVal[] =
	{
		0xa0384732,				 // RD = 9
		0x00000001,				 // sleep 1
		0xa0080800,				 // Vhi = 5
		0xa0084333,				 // Phi = 2
		0xa00887ff,				 // Rhi = 5
		0xa008c4cc,				 // Shi = 3
		0xa0180b32,				 // Slo = -7
		0xa0184e65,				 // Plo = -9
		0xa0188000,				 // Vlo = 0
		0xa018c7ff,				 // Rlo = -5
		0xa0288b32,				 // OG2 = -2
		0xa028cfff,				 // OG1 = -5
		0xa0380bfe,				 // DD = 15
		0xa0388ccc,				 // OD2 = 20
		0xa038cccc,				 // OD1 = 20
		0x30080100,				 // a/d offset channel 1
		0x30180100,				 // a/d offset channel 2
		0x30280200,				 // a/d offset channel 3
		0x30380200,				 // a/d offset channel 4
		0x51000040,				 // ioram channel order
		0x51000141,
		0x51000202,
		0x51008303,
		0x00000000
	};							 // end
	unsigned long *valp = edtVal;
	while (*valp != 0x00000000)
	{
		if (*valp == 0x000000001)
		{
			sleep (1);
		}
		else
		{
			ret = edtwrite (*valp);
			if (ret)
			{
				logStream (MESSAGE_ERROR) << "error writing value " << *valp <<
					sendLog;
				return -1;
			}
		}
		valp++;
	}

	setEdtValue (rd, 9);
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

	setEdtValue (dd, 15);

	return 0;
}


void
EdtSao::probe ()
{
	status = edt_reg_read (pd, PDV_STAT);
	shutter = status & PDV_CHAN_ID0;
	overrun = status & PDV_OVERRUN;
}


int
EdtSao::fclr (int num)
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


void
EdtSao::fclr_r (int num)
{
	while (fclr (num) != 0)
	{
		logStream (MESSAGE_ERROR)
			<< "Cannot do fclr, trying again." << sendLog;
		sleep (10);
	}
}


int
EdtSao::setParallelClockSpeed (int new_speed)
{
  	unsigned long ns = 0x46000000;	
	ns |= (new_speed & 0x00ff);
	return edtwrite (ns);
}


int
EdtSao::initChips ()
{
	int ret;

	channels = 2;
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

	setSize (2024, chipHeight->getValueInteger (), 0, 0);

	ret = setDAC ();
	return ret;
}


void
EdtSao::writeCommand (bool parallel, int addr, command_t command)
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

void
EdtSao::writeCommandEnd ()
{
	u_char cmd[4];
	cmd[0] = cmd[1] = cmd[2] = cmd[3] = 0;
	ccd_serial_write (pd, cmd, 4);
}


int
EdtSao::writePattern (const SplitConf *conf)
{
	// write parallel commands
	int addr = 0;
	if (skipLines->getValueInteger () == lastSkipLines
	  	&& chipUsedReadout->getXInt () == lastX
		&& chipUsedReadout->getYInt () == lastY
		&& chipUsedReadout->getWidthInt () == lastW
		&& chipUsedReadout->getHeightInt () == lastH
		&& splitMode->getValueInteger () == lastSplitMode)
	{
		return 0;
	}

	lastSkipLines = skipLines->getValueInteger ();
	lastX = chipUsedReadout->getXInt ();
	lastY = chipUsedReadout->getYInt ();
	lastW = chipUsedReadout->getWidthInt ();
	lastH = chipUsedReadout->getHeightInt ();
	lastSplitMode = splitMode->getValueInteger ();

	logStream (MESSAGE_DEBUG) << "starting to write pattern" << sendLog;
	writeCommand (true, addr++, ZERO);
	// read..
	int i;
	for (i = 0; i < getUsedY (); i++)
		writeCommand (true, addr++, SKIP);
	for (; i < getUsedY () + getUsedHeight (); i++)
		writeCommand (true, addr++, READ);
	for (; i < getHeight (); i++)
		writeCommand (true, addr++, SKIP);
	writeCommand (true, addr++, VEND);
	// write serial commandsa
	addr = 0;
	writeCommand (false, addr++, ZERO);
	for (i = 0; i < skipLines->getValueInteger (); i++)
		writeCommand (false, addr++, SKIP);
	int width;
	if (channels == 1)
	{
		// skip in X axis..
		for (i = 0; i < getUsedX (); i++)
			writeCommand (false, addr++, SKIP);
		width = getUsedX () + getUsedWidth ();
		for (; i < width; i++)
			writeCommand (false, addr++, READ);
		for (; i < getWidth (); i++)
			writeCommand (false, addr++, SKIP);
	}
	else
	{
		for (i = 0; i < getWidth () / 2; i++)
			writeCommand (false, addr++, READ);
	}
	writeCommand (false, addr++, HEND);
	writeCommandEnd ();
	logStream (MESSAGE_DEBUG) << "pattern written" << sendLog;
	return 0;
}


int
EdtSao::startExposure ()
{
	int ret;
	// taken from readout script
	// set split modes.. (0 - left, 1 - right, 2 - both)
	const SplitConf *conf = &splitConf[splitMode->getValueInteger ()];
	channels = conf->chanNum;
	ret = setEDTSplit (conf->splitMode);
	if (ret)
		return ret;
	ret = setEDTUni (conf->uniMode);
	if (ret)
		return ret;

	edtwrite (conf->ioPar1);	 //  channel order
	edtwrite (conf->ioPar2);

	// create and write pattern..
	ret = writePattern (conf);
	if (ret)
		return ret;
	writeBinFile ("e2v_nidlesc.bin");
	fclr_r (5);
//	if (channels != 1)
		writeBinFile ("e2v_freezesc.bin");

	// taken from expose.c
								 /* set time */
	edtwrite (0x52000000 | (long) (getExposure () * 100));

	edtwrite (0x50030000);		 /* open shutter */

	return 0;
}


int
EdtSao::stopExposure ()
{
	edtwrite (0x50020000);		 /* close shutter */
	return Camera::stopExposure ();
}


long
EdtSao::isExposing ()
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
//	if (channels != 1)
		writeBinFile ("e2v_unfreezesc.bin");
	return 0;
}


int
EdtSao::readoutStart ()
{
	int ret;
	int width, height;
	int i;
	int db;
	int set_timeout = 0;
	int timeout_val = 7000;
	int bufsize;

	numbufs = 4;
	dsub = 1;

	// taken from kepler.c
	pdv_set_header_dma (pd, 0);
	pdv_set_header_size (pd, 0);
	pdv_set_header_offset (pd, 0);

	/*
	 * SET SIZE VARIABLES FOR IMAGE
	 */
	if (channels == 1)
		width = getUsedWidth ();
	else
		width = getWidth () / 2;
	height = getUsedHeight ();
	ret = pdv_setsize (pd, width * channels * dsub, height);
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "startExposure failed in pdv_setsize" <<
			sendLog;
		return -1;
	}
	depth = pdv_get_depth (pd);
	db = bits2bytes (depth);
	imagesize = chipUsedReadout->getWidthInt () * chipUsedReadout->getHeightInt () * db;

	ret = Camera::readoutStart ();
	if (ret)
		return ret;

	/*
	 * SET TIMEOUT
	 * make about 1.5 * readout time + 2 sec
	 */
	if (!set_timeout)
		timeout_val = (chipUsedReadout->getWidthInt () / channels)
			* chipUsedReadout->getHeightInt () / 1000
			* timeout_val * 5 / 2000 + 2000;
	pdv_set_timeout (pd, timeout_val);
	logStream (MESSAGE_DEBUG) << "timeout_val: " << timeout_val << " millisecs" << sendLog;

	/*
	 * ALLOCATE MEMORY for the image, and make sure it's aligned on a page
	 * boundary
	 */
	imagesize = imagesize / dsub;
	pdv_flush_fifo (pd);		 /* MC - add a flush */
	numbufs = 1;
	bufsize = imagesize * dsub;
	if (verbose)
	{
		printf ("number of buffers: %d bufsize: %d\n", numbufs, bufsize);
		fflush (stdout);
	}
	bufs = (u_char **) malloc (numbufs * sizeof (u_char *));
	for (i = 0; i < numbufs; i++)
		bufs[i] = (u_char *) pdv_alloc (bufsize);
	pdv_set_buffers (pd, numbufs, bufs);
	return 0;
}


int
EdtSao::doReadout ()
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

	/* DO DOUBLE CORRELATED SUBTRACTION IF SPECIFIED */

	if (dsub > 1)
	{
		if (channels == 1)
		{
			for (j = 0, i = 0; j < imagesize; j += 2, i += 4)
			{
				dsx1 = (bufs[0][i + 1] << 8) + bufs[0][i];
				dsx2 = (bufs[0][i + 3] << 8) + bufs[0][i + 2];
				diff = dsx2 - dsx1;
				dataBuffer[j] = (u_char) ((short) diff);
				dataBuffer[j + 1] = (u_char) ((short) diff >> 8);
			}
		}
		else
		{
			for (j = 0, i = 0; j < imagesize; j += 4, i += 8)
			{
				dsx1 = (bufs[0][i + 1] << 8) + bufs[0][i];
				dsx2 = (bufs[0][i + 5] << 8) + bufs[0][i + 4];
				diff = dsx2 - dsx1;
				dataBuffer[j] = (u_char) ((int) diff);
				dataBuffer[j + 1] = (u_char) ((int) diff >> 8);
				dsx1 = (bufs[0][i + 3] << 8) + bufs[0][i + 2];
				dsx2 = (bufs[0][i + 7] << 8) + bufs[0][i + 6];
				diff = dsx2 - dsx1;
				dataBuffer[j + 2] = (u_char) ((int) diff);
				dataBuffer[j + 3] = (u_char) ((int) diff >> 8);
			}
		}
	}
	else
		for (j = 0; j < imagesize; j++)
			dataBuffer[j] = bufs[0][j];

	/* PRINTOUT FIRST AND LAST PIXELS - DEBUG */

	if (verbose)
	{
		printf ("first 8 pixels\n");
		for (j = 0; j < 16; j += 2)
		{
			x = (dataBuffer[j + 1] << 8) + dataBuffer[j];
			printf ("%d ", x);
		}
		printf ("\nlast 8 pixels\n");
		for (j = imagesize - 16; j < imagesize; j += 2)
		{
			x = (dataBuffer[j + 1] << 8) + dataBuffer[j];
			printf ("%d ", x);
		}
		printf ("\n");
	}

	for (i = 0; i < numbufs; i++)
		pdv_free (bufs[i]);		 /* free buf memory */

	// swap for split mode

	int width = chipUsedReadout->getWidthInt () / dsub;

	if (channels == 2)
	{
		int pixd = 0;
		int pixt;
		int row;

		int pixsize = chipUsedReadout->getWidthInt () * chipUsedReadout->getHeightInt ();
		uint16_t *dx = (uint16_t *) malloc (pixsize * 4);

		logStream (MESSAGE_DEBUG) << "split mode" << sendLog;
		while (pixd < pixsize)
		{						 /* do all the even pixels */
			row = pixd / width;
			pixt = pixd / 2 + row * width / 2;
			i = pixd * 2;
			j = pixt;
			if (j > pixsize)
			{
				printf ("j: %i i: %i pixd: %i pixels: %i\n", j, i, pixd,
					pixsize);
			}
			else
			{
				dx[j] = (dataBuffer[i + 1] << 8) + dataBuffer[i];
			}
			pixd += 2;
		}
		pixd = 1;
		while (pixd < pixsize)
		{						 /* do all the odd pixels */
			row = pixd / width;
			pixt = (3 * row * width / 2 + width) - (pixd + 1) / 2;
			i = pixd * 2;
			j = pixt;
			if (j > pixsize)
			{
				printf ("j: %i i: %i pixd: %i pixels: %i\n", j, i, pixd,
					pixsize);
			}
			else
			{
				dx[j] = (dataBuffer[i + 1] << 8) + dataBuffer[i];
			}
			pixd += 2;
		}

		memcpy (dataBuffer, dx, pixsize * 2);

		free (dx);

		/* split mode - assumes 16 bit/pixel and 2 channels */
		// do it in place, without allocating second memory
		/*dx = (uint16_t *) dataBuffer;
		   for (int r = 0; r < chipUsedReadout->height; r++)
		   {
		   int fp, fp2;
		   uint16_t v;
		   // split first half
		   for (int c = 0; c < chipUsedReadout->width / 4; c++)
		   {
		   // swap values
		   fp = r * chipUsedReadout->width + c;
		   fp2 = fp + chipUsedReadout->width / 4;
		   v = dx[fp];
		   dx[fp] = dx[fp2];
		   dx[fp2] = v;
		   }
		   // and then do second half..
		   for (int c = chipUsedReadout->width / 2;
		   c <
		   chipUsedReadout->width / 2 + chipUsedReadout->width / 4;
		   c++)
		   {
		   // swap values
		   fp = r * chipUsedReadout->width + c;
		   fp2 = fp + chipUsedReadout->width / 4;
		   v = dx[fp];
		   dx[fp] = dx[fp2];
		   dx[fp2] = v;
		   }
		   } */
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "single channel" << sendLog;
		// /* already stored loend
		/*for (i = 0; i < imagesize; i += 2)
		   {
		   dataBuffer[i] = (dataBuffer[i + 1] << 8) + dataBuffer[i];
		   } */
	}

	ret = sendReadoutData (dataBuffer, getWriteBinaryDataSize ());
	if (ret < 0)
		return -1;
	return -2;
}


void
EdtSao::cancelPriorityOperations ()
{
	Camera::cancelPriorityOperations ();
}


int
EdtSao::endReadout ()
{
	pdv_stop_continuous (pd);
	pdv_flush_fifo (pd);
	pdv_reset_serial (pd);
	edt_reg_write (pd, PDV_CMD, PDV_RESET_INTFC);
	writeBinFile ("e2v_pidlesc.bin");
	return Camera::endReadout ();
}


EdtSao::EdtSao (int in_argc, char **in_argv):
Camera (in_argc, in_argv)
{
	devname[0] = '\0';
	devunit = 0;
	pd = NULL;
	verbose = false;

	notimeout = 0;
	sdelay = 0;

	createExpType ();

	addOption ('p', "devname", 1, "device name");
	addOption ('n', "devunit", 1, "device unit number");
	addOption ('H', NULL, 1, "chip height - number of rows");
	// add overscan pixel option
	addOption ('S', "hskip", 1, "number of lines to skip (overscan pixels)");
	addOption (OPT_NOTIMEOUT, "notimeout", 0, "don't timeout");
	addOption ('s', "sdelay", 1, "serial delay");
	addOption ('v', "verbose", 0, "verbose report");

	createValue (splitMode, "SPL_MODE", "split mode of the readout", true, 0,
		CAM_WORKING, true);
	splitMode->setValueInteger (0);

	// add possible split modes
	splitMode->addSelVal ("LEFT");
	splitMode->addSelVal ("RIGHT");
	splitMode->addSelVal ("BOTH");

	createValue (edtGain, "GAIN", "gain (high or low)", true, 0,
		CAM_WORKING, true);

	edtGain->addSelVal ("HIGH");
	edtGain->addSelVal ("LOW");

	edtGain->setValueInteger (0);

	createValue (parallelClockSpeed, "PCLOCK", "parallel clock speed", true, 0, CAM_WORKING, true);
	parallelClockSpeed->setValueInteger (6);

	createValue (skipLines, "hskip", "number of lines to skip (as those contains bias values)", true);
	skipLines->setValueInteger (0);

	createValue (chipHeight, "height", "chip height - number of rows", true, 0, CAM_WORKING, true);
	chipHeight->setValueInteger (520);

	createValue (phi, "PHI", "P high", true, 0, CAM_WORKING, true);
	phi->initEdt (0xA0084, A_plus);

	createValue (plo, "PLO", "P low", true, 0, CAM_WORKING, true);
	plo->initEdt (0xA0184, A_minus);

	createValue (shi, "SHI", "S high", true, 0, CAM_WORKING, true);
	shi->initEdt (0xA008C, A_plus);

	createValue (slo, "SLO", "S low", true, 0, CAM_WORKING, true);
	slo->initEdt (0xA0180, A_minus);

	createValue (rhi, "RHI", "R high", true, 0, CAM_WORKING, true);
	rhi->initEdt (0xA0088, A_plus);

	createValue (rlo, "RLO", "R low", true, 0, CAM_WORKING, true);
	rlo->initEdt (0xA018C, A_minus);

	createValue (rd, "RD", "RD", true, 0, CAM_WORKING, true);
	rd->initEdt (0xA0384, C);

	createValue (od1r, "OD1_R", "OD 1 right", true, 0, CAM_WORKING, true);
	od1r->initEdt (0xA0388, D);

	createValue (od2l, "OD2_L", "OD 2 left", true, 0, CAM_WORKING, true);
	od2l->initEdt (0xA038C, D);

	createValue (og1r, "OG1_R", "OG 1 right", true, 0, CAM_WORKING, true);
	og1r->initEdt (0xA0288, B);

	createValue (og2l, "OG2_L", "OG 2 left", true, 0, CAM_WORKING, true);
	og2l->initEdt (0xA028C, B);

	createValue (dd, "DD", "DD", true, 0, CAM_WORKING, true);
	dd->initEdt (0xA0380, C);

	// init last used modes - for writePattern
	lastSkipLines = lastX = lastY = lastW = lastH = lastSplitMode = -1;
}


EdtSao::~EdtSao (void)
{
	edt_close (pd);
}


int
EdtSao::processOption (int in_opt)
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
			chipHeight->setValueInteger (atoi (optarg));
			break;
	        case 'S':
	                skipLines -> setValueInteger (atoi (optarg));
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
		default:
			return Camera::processOption (in_opt);
	}
	return 0;
}


int
EdtSao::setEdtValue (ValueEdt * old_value, float new_value)
{
	if (old_value->testValue (new_value) == false)
		return -2;
	old_value->setValueDouble (new_value);
	return edtwrite (old_value->getHexValue (new_value));
}


int
EdtSao::setEdtValue (ValueEdt * old_value, Rts2Value * new_value)
{
	return edtwrite (old_value->getHexValue (new_value->getValueFloat ()));
}


int
EdtSao::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == splitMode)
	{
		return 0;
	}
	if (old_value == phi
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
		return (setEDTGain (new_value->getValueInteger () == 0));
	}
	if (old_value == parallelClockSpeed)
	{
		return (setParallelClockSpeed (new_value->getValueInteger ()) == 0 ? 0 : -2);
	}
	return Camera::setValue (old_value, new_value);
}


int
EdtSao::init ()
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

	return initChips ();
}


int
EdtSao::initValues ()
{
	addConstValue ("DEVNAME", "device name", devname);
	addConstValue ("DEVNUM", "device unit number", devunit);
	addConstValue ("DEVNT", "device no timeout", notimeout);
	addConstValue ("SDELAY", "device serial delay", sdelay);
	return Camera::initValues ();
}


int
main (int argc, char **argv)
{
	EdtSao device = EdtSao (argc, argv);
	return device.run ();
}
