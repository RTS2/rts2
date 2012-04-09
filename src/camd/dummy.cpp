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

#define OPT_WIDTH        OPT_LOCAL + 1
#define OPT_HEIGHT       OPT_LOCAL + 2
#define OPT_DATA_SIZE    OPT_LOCAL + 3
#define OPT_CHANNELS     OPT_LOCAL + 4
#define OPT_REMOVE_TEMP  OPT_LOCAL + 5

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
			genType->setValueInteger (0);

			createValue (astar_Xp, "astar_x", "[x] artificial star position", false);
			createValue (astar_Yp, "astar_y", "[y] artificial star position", false);

			createValue (astarX, "astar_sx", "[pixels] artificial star width along X axis", false, RTS2_VALUE_WRITABLE);
			astarX->setValueDouble (5);

			createValue (astarY, "astar_sy", "[pixels] artificial star width along Y axis", false, RTS2_VALUE_WRITABLE);
			astarY->setValueDouble (5);

			createValue (aamp, "astar_amp", "[adu] artificial star amplitude", false, RTS2_VALUE_WRITABLE);
			aamp->setValueInteger (20000);

			createValue (noiseBias, "noise_bias", "[ADU] noise base level", false, RTS2_VALUE_WRITABLE);
			noiseBias->setValueDouble (400);

			createValue (noiseRange, "noise_range", "readout noise range", false, RTS2_VALUE_WRITABLE);
			noiseRange->setValueDouble (300);

			createValue (hasError, "has_error", "if true, info will report error", false, RTS2_VALUE_WRITABLE);
			hasError->setValueBool (false);

			createExpType ();

			width = 200;
			height = 100;
			dataSize = -1;

			written = -1;

			addOption ('f', NULL, 0, "when set, dummy CCD will act as frame transfer device");
			addOption ('i', NULL, 1, "device will sleep <param> seconds before each info and baseInfo return");
			addOption ('r', NULL, 1, "device will sleep <parame> seconds before each readout");
			addOption (OPT_WIDTH, "width", 1, "width of simulated CCD");
			addOption (OPT_HEIGHT, "height", 1, "height of simulated CCD");
			addOption (OPT_DATA_SIZE, "datasize", 1, "size of data block transmitted over TCP/IP");
			addOption (OPT_CHANNELS, "channels", 1, "number of data channels");
			addOption (OPT_REMOVE_TEMP, "no-temp", 0, "do not show temperature related fields");
		}

		virtual ~Dummy (void)
		{
			readoutSleep = NULL;
		}

		virtual int processOption (int in_opt)
		{
			switch (in_opt)
			{
				case 'f':
					supportFrameT = true;
					break;
				case 'I':
					infoSleep = atoi (optarg);
					break;
				case 'r':
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

			setExposureMinMax (0,3600);
			expMin->setValueDouble (0);
			expMax->setValueDouble (3600);

			usleep (infoSleep);
			strcpy (ccdType, "Dummy");
			strcpy (serialNumber, "1");

			srand (time (NULL));

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
			if (isnan (t))
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
			written = -1;
			return 0;
		}
		virtual long suggestBufferSize ()
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

	private:
		bool supportFrameT;
		int infoSleep;
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

		rts2core::ValueRectangle *astarLimits;

		rts2core::ValueDouble *astar_Xp;
		rts2core::ValueDouble *astar_Yp;

		rts2core::ValueDouble *astarX;
		rts2core::ValueDouble *astarY;

		rts2core::ValueDouble *aamp;

		int width;
		int height;

		long dataSize;

		bool showTemp;

		void generateImage (long usedSize);

		// data written during readout
		long written;
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

int Dummy::doReadout ()
{
	int ret;
	long usedSize = dataBufferSize;
	if (usedSize > getWriteBinaryDataSize ())
		usedSize = getWriteBinaryDataSize ();
	int nch = 0;
	if (channels)
	{
		for (size_t ch = 0; ch < channels->size (); ch++)
		{
			if (channels && (*channels)[ch] == false)
				continue;
			generateImage (usedSize / 2);
			for (written = 0; written < usedSize;)
			{
				usleep ((int) (readoutSleep->getValueDouble () * USEC_SEC));
				size_t s = usedSize - written < callReadoutSize->getValueLong () ? usedSize - written : callReadoutSize->getValueLong ();
				ret = sendReadoutData (dataBuffer + written, s, nch);

				if (ret < 0)
					return ret;
				written += s;
			}
			nch++;
		}
	}
	else
	{
		if (written == -1)
		{
			generateImage (usedSize / 2);
			written = 0;
		}
		usleep ((int) (readoutSleep->getValueDouble () * USEC_SEC));
		if (written < chipByteSize ())
		{
			size_t s = chipByteSize () - written < callReadoutSize->getValueLong () ? chipByteSize () - written : callReadoutSize->getValueLong ();
			ret = sendReadoutData (dataBuffer + written, s, 0);

			if (ret < 0)
				return ret;
			written += s;
		}
	}

	if (getWriteBinaryDataSize () == 0)
		return -2;				 // no more data..
	return 0;					 // imediately send new data
}

void Dummy::generateImage (long usedSize)
{
	// artifical star center
	astar_Xp->setValueDouble (random_num () * getUsedWidthBinned ());
	astar_Yp->setValueDouble (random_num () * getUsedHeightBinned ());

	sendValueAll (astar_Xp);
	sendValueAll (astar_Yp);

	double sx = astarX->getValueDouble ();
	int xmax = ceil (sx * 10);
	sx *= sx;

	double sy = astarY->getValueDouble ();
	int ymax = ceil (sy * 10);
	sy *= sy;

	uint16_t *d = (uint16_t *) dataBuffer;

	for (int i = 0; i < usedSize; i++, d++)
	{
		double n;
		if (genType->getValueInteger () != 1 && genType->getValueInteger () != 2)
			n = noiseRange->getValueDouble ();
		// generate data
		switch (genType->getValueInteger ())
		{
			case 0:  // random
			case 5:  // artificial star
				*d = noiseBias->getValueDouble () + n * random_num () - n / 2;
				break;
			case 1:  // linear
				*d = i;
				break;
			case 2:
				// linear shifted
				*d = i + (int) (getExposureNumber () * 10);
				break;
			case 3:
				// flats dusk
				*d = n * random_num ();
				if (getScriptExposureNumber () < 55)
					*d += (56 - getScriptExposureNumber ()) * 1000;
				*d -= n / 2;
				break;
			case 4:
				// flats dawn
				*d = n * random_num ();
				if (getScriptExposureNumber () < 55)
					*d += getScriptExposureNumber () * 1000;
				else
				  	*d += 55000;
				*d -= n / 2;
				break;
		}
		// genarate artificial star - using cos
		if (genType->getValueInteger () == 5)
		{
			int x = i % getUsedWidthBinned ();
			int y = i / getUsedWidthBinned ();

			double aax = x - astar_Xp->getValueDouble ();
			double aay = y - astar_Yp->getValueDouble ();

			if (fabs (aax) < xmax && fabs (aay) < ymax)
			{
				aax *= aax;
				aay *= aay;

				*d += aamp->getValueDouble () * exp (-(aax / (2 * sx) + aay / (2 * sy)));
			}
		}
	}
}

int main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}
