/*
 * Dummy camera for testing purposes.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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
#include "utilsfunc.h"
#include "rts2fits/image.h"

#define OPT_WIDTH        OPT_LOCAL + 1
#define OPT_HEIGHT       OPT_LOCAL + 2
#define OPT_DATA_SIZE    OPT_LOCAL + 3
#define OPT_CHANNELS     OPT_LOCAL + 4
#define OPT_REMOVE_TEMP  OPT_LOCAL + 5
#define OPT_INFOSLEEP    OPT_LOCAL + 6
#define OPT_READSLEEP    OPT_LOCAL + 7
#define OPT_FRAMETRANS   OPT_LOCAL + 8
#define OPT_GENTYPE      OPT_LOCAL + 9

namespace rts2camd
{

/**
 * Class for a dummy camera.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Dummy:public Camera
{
	public:
		Dummy (int in_argc, char **in_argv):Camera (in_argc, in_argv)
		{
			createTempCCD ();

			showTemp = true;

			supportFrameT = false;
			infoSleep = 0;

			createValue (callReadout, "call_readout", "call emulated readout; for tests of non-reading CCDs", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
			callReadout->setValueBool (true);

			createValue (readoutSleep, "readout", "readout sleep in sec", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
			readoutSleep->setValueDouble (0);

			createValue (callReadoutSize, "readout_size", "[pixels] number of pixels send on a single read", false, RTS2_VALUE_WRITABLE);
			callReadoutSize->setValueLong (100000);

			createValue (expMin, "exp_min", "[s] minimal exposure time", false, RTS2_VALUE_WRITABLE);
			createValue (expMax, "exp_max", "[s] maximal exposure time", false, RTS2_VALUE_WRITABLE);
			tempMin = tempMax = NULL;

			createValue (genType, "gen_type", "data generation algorithm", true, RTS2_VALUE_WRITABLE);
			genType->addSelVal ("random");
			genType->addSelVal ("linear");
			genType->addSelVal ("linear shifted");
			genType->addSelVal ("flats dusk");
			genType->addSelVal ("flats dawn");
			genType->addSelVal ("astar");
			genType->addSelVal ("field");
			genType->setValueInteger (6);

			createValue (fitsTransfer, "fits_transfer", "write FITS file directly in camera", false, RTS2_VALUE_WRITABLE);
			fitsTransfer->setValueBool (false);

			createValue (astar_num, "astar_num", "number of artificial stars", false, RTS2_VALUE_WRITABLE);
			astar_num->setValueInteger (1);

			createValue (astar_Xp, "astar_x", "[x] artificial star position", false);
			createValue (astar_Yp, "astar_y", "[y] artificial star position", false);

			createValue (astarX, "astar_sx", "[pixels] artificial star width along X axis", false, RTS2_VALUE_WRITABLE);
			astarX->setValueDouble (5);

			createValue (astarY, "astar_sy", "[pixels] artificial star width along Y axis", false, RTS2_VALUE_WRITABLE);
			astarY->setValueDouble (5);

			createValue (aamp, "astar_amp", "[adu] artificial star amplitude", false, RTS2_VALUE_WRITABLE);
			aamp->setValueInteger (200000);

			createValue (moffat_beta, "moffat_beta", "artificial star Moffat beta", false, RTS2_VALUE_WRITABLE);
			moffat_beta->setValueInteger (2.5);

			createValue (defocus_scale, "defoc_scale", "scale of defocusing", false, RTS2_VALUE_WRITABLE);
			defocus_scale->setValueInteger (100);

			createValue (mag_slope, "mag_slope", "slope of brightness distribution of stars", false, RTS2_VALUE_WRITABLE);
			mag_slope->setValueInteger (1);

			createValue (noiseBias, "noise_bias", "[ADU] noise base level", false, RTS2_VALUE_WRITABLE);
			noiseBias->setValueDouble (400);

			createValue (noiseRange, "noise_range", "readout noise range", false, RTS2_VALUE_WRITABLE);
			noiseRange->setValueDouble (300);

			createValue (hasError, "has_error", "if true, info will report error", false, RTS2_VALUE_WRITABLE);
			hasError->setValueBool (false);

			createExpType ();

			createShiftStore ();

			width = 200;
			height = 100;
			dataSize = -1;
			written = NULL;

			addOption (OPT_FRAMETRANS, "frame-transfer", 0, "when set, dummy CCD will act as frame transfer device");
			addOption (OPT_INFOSLEEP, "info-sleep", 1, "device will sleep <param> seconds before each info and baseInfo return");
			addOption (OPT_READSLEEP, "read-sleep", 1, "device will sleep <parame> seconds before each readout");
			addOption (OPT_WIDTH, "width", 1, "width of simulated CCD");
			addOption (OPT_HEIGHT, "height", 1, "height of simulated CCD");
			addOption (OPT_DATA_SIZE, "datasize", 1, "size of data block transmitted over TCP/IP");
			addOption (OPT_CHANNELS, "channels", 1, "number of data channels");
			addOption (OPT_REMOVE_TEMP, "no-temp", 0, "do not show temperature related fields");
			addOption (OPT_GENTYPE, "gentype", 1, "data generation algorithm");
		}

		virtual ~Dummy (void)
		{
			readoutSleep = NULL;
			delete[] written;
		}

		virtual int processOption (int in_opt)
		{
			switch (in_opt)
			{
				case OPT_FRAMETRANS:
					supportFrameT = true;
					break;
				case OPT_INFOSLEEP:
					infoSleep = atoi (optarg);
					break;
				case OPT_READSLEEP:
					readoutSleep->setValueDouble (atoi (optarg));
					break;
				case OPT_WIDTH:
					width = atoi (optarg);
					break;
				case OPT_HEIGHT:
					height = atoi (optarg);
					break;
				case OPT_DATA_SIZE:
					dataSize = atoi (optarg);
					break;
				case OPT_CHANNELS:
					createDataChannels ();
					setNumChannels (atoi (optarg));
					break;
				case OPT_REMOVE_TEMP:
					showTemp = false;
					break;
				case OPT_GENTYPE:
					genType->setValueInteger (atoi (optarg));
					break;
				default:
					return Camera::processOption (in_opt);
			}
			return 0;
		}
		virtual int initHardware ()
		{
			if (showTemp)
			{
				createTempAir ();
				createTempSet ();

				createValue (tempMin, "temp_min", "[C] minimal set temperature", false, RTS2_VALUE_WRITABLE);
				createValue (tempMax, "temp_max", "[C] maximal set temperature", false, RTS2_VALUE_WRITABLE);

				tempMin->setValueDouble (-257);
				tempMax->setValueDouble (50);

				tempSet->setMin (-257);
				tempSet->setMax (50);
			}

			if (channels)
			{
				written = new ssize_t[channels->size ()];
				for (unsigned int i = 1; i < channels->size (); i++)
					written[i] = -1;
			}
			else
			{
				written = new ssize_t[1];
			}
			written[0] = -1;

			setExposureMinMax (0,3600);
			expMin->setValueDouble (0);
			expMax->setValueDouble (3600);

			usleep (infoSleep);
			ccdRealType->setValueCharArr ("Dummy");
			serialNumber->setValueCharArr ("1");

			srandom (time (NULL));

			return initChips ();
		}
		virtual int initChips ()
		{
			initCameraChip (width, height, 0, 0);
			return Camera::initChips ();
		}
		virtual int info ()
		{
			if (hasError->getValueBool ())
			{
				raiseHWError ();
				return -1;
			}
			clearHWError ();
			usleep (infoSleep);
			float t = tempSet->getValueFloat ();
			if (std::isnan (t))
				t = 30;
			tempCCD->setValueFloat (t + random_num() * 3);
			if (tempAir)
			{
				tempAir->setValueFloat (t + random_num () * 3);
			}
			return Camera::info ();
		}
		virtual int startExposure ()
		{
			if (fitsTransfer->getValueBool ())
				setFitsTransfer ();
			written[0] = -1;
			if (channels)
			{
				for (unsigned int i = 1; i < channels->size (); i++)
					written[i] = -1;
			}
			return callReadout->getValueBool () ? 0 : 1;
		}

		virtual long isExposing ()
		{
			long ret = Camera::isExposing ();
			if (ret == -2)
				return (callReadout->getValueBool () == true) ? -2 : -4;
			return ret;
		}

		virtual size_t suggestBufferSize ()
		{
			if (dataSize < 0)
				return Camera::suggestBufferSize ();
			return dataSize;
		}
		virtual int doReadout ();

		virtual bool supportFrameTransfer () { return supportFrameT; }
	protected:
		virtual void initBinnings ()
		{
			Camera::initBinnings ();

			addBinning2D (2, 2);
			addBinning2D (3, 3);
			addBinning2D (4, 4);
			addBinning2D (1, 2);
			addBinning2D (2, 1);
			addBinning2D (4, 1);
		}

		virtual void initDataTypes ()
		{
			Camera::initDataTypes ();
			addDataType (RTS2_DATA_BYTE);
			addDataType (RTS2_DATA_SHORT);
			addDataType (RTS2_DATA_LONG);
			addDataType (RTS2_DATA_LONGLONG);
			addDataType (RTS2_DATA_FLOAT);
			addDataType (RTS2_DATA_DOUBLE);
			addDataType (RTS2_DATA_SBYTE);
			addDataType (RTS2_DATA_ULONG);

			setDataTypeWritable ();
		}

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

		virtual int shiftStoreStart (rts2core::Connection *conn, float exptime);

		virtual int shiftStoreShift (rts2core::Connection *conn, int shift, float exptime);

		virtual int shiftStoreEnd (rts2core::Connection *conn, int shift, float exptime);

	private:
		bool supportFrameT;
		int infoSleep;
		rts2core::ValueBool *callReadout;
		rts2core::ValueDouble *readoutSleep;
		rts2core::ValueLong *callReadoutSize;
		rts2core::ValueSelection *genType;
		rts2core::ValueDouble *noiseRange;
		rts2core::ValueDouble *noiseBias;
		rts2core::ValueBool *hasError;

		rts2core::ValueDouble *expMin;
		rts2core::ValueDouble *expMax;
		rts2core::ValueDouble *tempMin;
		rts2core::ValueDouble *tempMax;

		rts2core::ValueInteger *astar_num;

		rts2core::ValueRectangle *astarLimits;

		rts2core::DoubleArray *astar_Xp;
		rts2core::DoubleArray *astar_Yp;

		rts2core::ValueDouble *astarX;
		rts2core::ValueDouble *astarY;

		rts2core::ValueDouble *aamp;

		rts2core::ValueDouble *moffat_beta;
		rts2core::ValueDouble *defocus_scale;
		rts2core::ValueDouble *mag_slope;

		rts2core::ValueBool *fitsTransfer;

		int width;
		int height;

		long dataSize;

		bool showTemp;

		void generateImage (size_t pixelsize, int chan);

		template <typename dt> void generateData (dt *data, size_t pixelsize);

		// data written during readout
		ssize_t *written;
};

};

using namespace rts2camd;

int Dummy::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == expMin)
	{
		setExposureMinMax (new_value->getValueDouble (), expMax->getValueDouble ());
		return 0;
	}
	else if (old_value == expMax)
	{
		setExposureMinMax (expMin->getValueDouble (), new_value->getValueDouble ());
		return 0;
	}
	else if (old_value == tempMin)
	{
		tempSet->setMin (new_value->getValueDouble ());
		updateMetaInformations (tempSet);
		return 0;
	}
	else if (old_value == tempMax)
	{
		tempSet->setMax (new_value->getValueDouble ());
		updateMetaInformations (tempSet);
		return 0;
	}
	return Camera::setValue (old_value, new_value);
}

int Dummy::shiftStoreStart (rts2core::Connection *conn, float exptime)
{
	return Camera::shiftStoreStart (conn, exptime);
}

int Dummy::shiftStoreShift (rts2core::Connection *conn, int shift, float exptime)
{
	return Camera::shiftStoreShift (conn, shift, exptime);
}

int Dummy::shiftStoreEnd (rts2core::Connection *conn, int shift, float exptime)
{
	return Camera::shiftStoreEnd (conn, shift, exptime);
}

int Dummy::doReadout ()
{
	int ret;
	ssize_t usedSize = chipByteSize ();
	size_t pixelSize = chipUsedSize ();
	int nch = 0;
	rts2image::Image *image = NULL;
	if (fitsTransfer->getValueBool ())
	{
		struct timeval expStart;
		gettimeofday (&expStart, NULL);
		image = new rts2image::Image ("/tmp/fits_data.fits", &expStart, true);
	}
	if (channels)
	{
		// generate image
		if (written[0] == -1)
		{
			for (unsigned int ch = 0; ch < channels->size (); ch++)
			{
				if (channels && (*channels)[ch] == false)
					continue;
				generateImage (pixelSize, ch);
				written[ch] = 0;
			}
		}
		// send data from channel..
		usleep ((int) (readoutSleep->getValueDouble () * USEC_SEC));
		if (image == NULL)
		{
			for (unsigned int ch = 0; ch < channels->size (); ch++)
			{
				size_t s = usedSize - written[ch] < callReadoutSize->getValueLong () ? usedSize - written[ch] : callReadoutSize->getValueLong ();
				ret = sendReadoutData (getDataTop (ch), s, nch);

				if (ret < 0)
					return ret;
				written[ch] += s;
				nch++;
			}
		}
		else
		{
			for (unsigned int ch = 0; ch < channels->size (); ch++)
			{
				long sizes[2];
				sizes[0] = getUsedWidthBinned ();
				sizes[1] = getUsedHeightBinned ();

				int fits_status = 0;

				fits_resize_img (image->getFitsFile (), getDataType (), 2, sizes, &fits_status);

				switch (getDataType ())
				{
					case RTS2_DATA_BYTE:
						fits_write_img_byt (image->getFitsFile (), 0, 1, pixelSize, (unsigned char *) getDataBuffer (ch), &fits_status);
						break;
					case RTS2_DATA_SHORT:
						fits_write_img_sht (image->getFitsFile (), 0, 1, pixelSize, (int16_t *) getDataBuffer (ch), &fits_status);
						break;
					case RTS2_DATA_LONG:
						fits_write_img_int (image->getFitsFile (), 0, 1, pixelSize, (int *) getDataBuffer (ch), &fits_status);
						break;
					case RTS2_DATA_LONGLONG:
						fits_write_img_lnglng (image->getFitsFile (), 0, 1, pixelSize, (LONGLONG *) getDataBuffer (ch), &fits_status);
						break;
					case RTS2_DATA_FLOAT:
						fits_write_img_flt (image->getFitsFile (), 0, 1, pixelSize, (float *) getDataBuffer (ch), &fits_status);
						break;
					case RTS2_DATA_DOUBLE:
						fits_write_img_dbl (image->getFitsFile (), 0, 1, pixelSize, (double *) getDataBuffer (ch), &fits_status);
						break;
					case RTS2_DATA_SBYTE:
						fits_write_img_sbyt (image->getFitsFile (), 0, 1, pixelSize, (signed char *) getDataBuffer (ch), &fits_status);
						break;
					case RTS2_DATA_USHORT:
						fits_write_img_usht (image->getFitsFile (), 0, 1, pixelSize, (short unsigned int *) getDataBuffer (ch), &fits_status);
						break;
					case RTS2_DATA_ULONG:
						fits_write_img_uint (image->getFitsFile (), 0, 1, pixelSize, (unsigned int *) getDataBuffer (ch), &fits_status);
						break;
					default:
						logStream (MESSAGE_ERROR) << "Unknow dataType " << getDataType () << sendLog;
						return -1;
				}
				if (fits_status)
				{
					logStream (MESSAGE_ERROR) << "cannot write channel " << ch << sendLog;
				}
			}
			image->closeFile ();
			delete image;
			fitsDataTransfer ("/tmp/fits_data.fits");
			return -2;
		}
	}
	else
	{
		if (written[0] == -1)
		{
			generateImage (usedSize / 2, 0);
			written[0] = 0;
		}
		usleep ((int) (readoutSleep->getValueDouble () * USEC_SEC));
		if (image == NULL)
		{
			if (written[0] < (ssize_t) chipByteSize ())
			{
				size_t s = (ssize_t) chipByteSize () - written[0] < callReadoutSize->getValueLong () ? chipByteSize () - written[0] : callReadoutSize->getValueLong ();
				ret = sendReadoutData (getDataTop (0), s, 0);
				std::cout << "ret " << ret << " " << s << std::endl;

				if (ret < 0)
					return ret;
				written[0] += s;
			}
		}
		else
		{
			long sizes[2];
			sizes[0] = getUsedWidthBinned ();
			sizes[1] = getUsedHeightBinned ();

			int fits_status = 0;

			fits_resize_img (image->getFitsFile (), RTS2_DATA_USHORT, 2, sizes, &fits_status);
			fits_write_img_usht (image->getFitsFile (), 0, 1, usedSize / 2, (uint16_t *) getDataTop (0), &fits_status);
			if (fits_status)
			{
				logStream (MESSAGE_ERROR) << "cannot write FITS file" << sendLog;
			}
			image->closeFile ();
			delete image;
			fitsDataTransfer ("/tmp/fits_data.fits");
			return -2;
		}
	}

	if (getWriteBinaryDataSize () == 0)
	{
		if (getDataType () == RTS2_DATA_USHORT)
			findSepStars ((uint16_t *) getDataBuffer (0));
		return -2;				 // no more data..
	}
	return 0;					 // imediately send new data
}

void Dummy::generateImage (size_t pixelsize, int chan)
{
	// artifical star center
	astar_Xp->clear ();
	astar_Yp->clear ();

	if (genType->getValueInteger () == 6)
	{
		// Sensible number of stars for a stellar field
		astar_num->setValueInteger (std::min(getUsedWidthBinned () * getUsedHeightBinned () / 300 / astarX->getValueDouble () / astarY->getValueDouble (), 100.0));

		// Fix random seed so the field is the same every run
		srandom (0);
	}

	for (int i = 0; i < astar_num->getValueInteger (); i++)
	{
		astar_Xp->addValue (random_num () * getUsedWidthBinned ());
		astar_Yp->addValue (random_num () * getUsedHeightBinned ());
	}

	sendValueAll (astar_Xp);
	sendValueAll (astar_Yp);

	if (genType->getValueInteger () == 6)
	{
		// Make it again random
		srandom (0 + time (NULL) + getExposureNumber ());
	}

	switch (getDataType ())
	{
		case RTS2_DATA_BYTE:
			generateData ((unsigned char *) getDataBuffer (chan), pixelsize);
			break;
		case RTS2_DATA_SHORT:
			generateData ((uint16_t *) getDataBuffer (chan), pixelsize);
			break;
		case RTS2_DATA_LONG:
			generateData ((int32_t *) getDataBuffer (chan), pixelsize);
			break;
		case RTS2_DATA_LONGLONG:
			generateData ((LONGLONG *) getDataBuffer (chan), pixelsize);
			break;
		case RTS2_DATA_FLOAT:
			generateData ((float *) getDataBuffer (chan), pixelsize);
			break;
		case RTS2_DATA_DOUBLE:
			generateData ((double *) getDataBuffer (chan), pixelsize);
			break;
		case RTS2_DATA_SBYTE:
			generateData ((uint8_t *) getDataBuffer (chan), pixelsize);
			break;
		case RTS2_DATA_USHORT:
			generateData ((uint16_t *) getDataBuffer (chan), pixelsize);
			break;
		case RTS2_DATA_ULONG:
			generateData ((uint32_t *) getDataBuffer (chan), pixelsize);
			break;
	}
}

inline double moffat (double r, double r0, double sigma2, double beta)
{
	return pow (1 + (r - r0)*(r - r0) / sigma2, -beta);
}

template <typename dt> void Dummy::generateData (dt *data, size_t pixelSize)
{
	double sx = astarX->getValueDouble () / binningHorizontal ();
	int xmax = ceil (sx * 10);
	sx *= sx;

	double sy = astarY->getValueDouble () / binningVertical ();
	int ymax = ceil (sy * 10);
	sy *= sy;

	int focPos = getFocPos ();
	double beta = moffat_beta->getValueDouble (); // Moffat profile beta
	double r0; // Moffat off-center parameter for defocusing
	double deFocScale = defocus_scale->getValueDouble (); // Defocusing scale, focuser shift causing large effect
	double magSlope = mag_slope->getValueDouble (); // Slope of brightness distribution in stellar field

	if (focPos != -1 && genType->getValueInteger () == 6)
	{
		// Simulate de-focusing for stellar field.
		// Optimal focus is at -1, to match absence of focuser
		sx *= (1.0 + fabs (focPos + 1) / deFocScale);
		sy *= (1.0 + fabs (focPos + 1) / deFocScale);

		xmax *= fmin (3.0, (1.0 + fabs (focPos + 1) / deFocScale));
		ymax *= fmin (3.0, (1.0 + fabs (focPos + 1) / deFocScale));

		beta = fmax (1.02, 2.5 - 0.03 * fabs (focPos + 1) / deFocScale);

		r0 = 0.1*xmax*fmax (0.3, 0.3*fabs (focPos + 1) / deFocScale);
	}

	// Normalization for (ring-shaped) Moffat can't be done analytically, so ...
	double moffat_norm = 0;

	for(int x = -xmax; x < xmax; x++)
		for(int y = -ymax; y < ymax; y++)
			moffat_norm += moffat (hypot (x, y), r0, sx, beta);

	for (size_t i = 0; i < pixelSize; i++, data++)
	{
		double n;
		if (genType->getValueInteger () != 1 && genType->getValueInteger () != 2)
			n = noiseRange->getValueDouble ();
		// generate data
		switch (genType->getValueInteger ())
		{
			case 0:  // random
			case 5:  // artificial star
			case 6:  // artificial stellar field
				*data = noiseBias->getValueDouble () + n * random_num () - n / 2;
				break;
			case 1:  // linear
				*data = i;
				break;
			case 2:
				// linear shifted
				*data = i + (int) (getExposureNumber () * 10);
				break;
			case 3:
				// flats dusk
				*data = n * random_num ();
				if (getScriptExposureNumber () < 55)
					*data += (56 - getScriptExposureNumber ()) * 1000;
				*data -= n / 2;
				break;
			case 4:
				// flats dawn
				*data = n * random_num ();
				if (getScriptExposureNumber () < 55)
					*data += getScriptExposureNumber () * 1000;
				else
				  	*data += 55000;
				*data -= n / 2;
				break;
		}

		if (getExpType () != 0)
			// Shutter is closed
			continue;

		// genarate artificial star - using cos
		if (genType->getValueInteger () == 5)
		{
			int x = i % getUsedWidthBinned ();
			int y = i / getUsedWidthBinned ();

			for (int j = 0; j < astar_num->getValueInteger (); j++)
			{
				double aax = x - (*astar_Xp)[j];
				double aay = y - (*astar_Yp)[j];

				if (fabs (aax) < xmax && fabs (aay) < ymax)
				{
					aax *= aax;
					aay *= aay;

					*data += aamp->getValueDouble () * exp (-(aax / (2 * sx) + aay / (2 * sy))) / 2 / M_PI / sqrt (sx*sy);
				}
			}
		}
		// genarate artificial stellar field using Moffat profile
		if (genType->getValueInteger () == 6)
		{
			int x = i % getUsedWidthBinned ();
			int y = i / getUsedWidthBinned ();

			for (int j = 0; j < astar_num->getValueInteger (); j++)
			{
				double aax = x - (*astar_Xp)[j];
				double aay = y - (*astar_Yp)[j];

				double r = hypot (aax, aay);

				if (r < xmax)
				{
					double flux = pow (1.0 + j, -magSlope); // Distribution of relative fluxes
					*data += 3.0 * flux * aamp->getValueDouble () * moffat (r, r0, sx, beta) / moffat_norm;
				}
			}
		}
	}
}

int main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}
