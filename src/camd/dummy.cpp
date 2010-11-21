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
#include "../utils/utilsfunc.h"

#define OPT_WIDTH        OPT_LOCAL + 1
#define OPT_HEIGHT       OPT_LOCAL + 2
#define OPT_DATA_SIZE    OPT_LOCAL + 3

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

			supportFrameT = false;
			infoSleep = 0;
			createValue (readoutSleep, "readout", "readout sleep in sec", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
			readoutSleep->setValueDouble (0);

			createValue (genType, "gen_type", "data generation algorithm", true, RTS2_VALUE_WRITABLE);
			genType->addSelVal ("random");
			genType->addSelVal ("linear");
			genType->addSelVal ("linear shifted");
			genType->addSelVal ("flats dusk");
			genType->addSelVal ("flats dawn");
			genType->setValueInteger (0);

			createValue (noiseRange, "noise_range", "readout noise range", false, RTS2_VALUE_WRITABLE);
			noiseRange->setValueDouble (300);

			createExpType ();

			width = 200;
			height = 100;
			dataSize = -1;

			addOption ('f', NULL, 0, "when set, dummy CCD will act as frame transfer device");
			addOption ('i', NULL, 1, "device will sleep <param> seconds before each info and baseInfo return");
			addOption ('r', NULL, 1, "device will sleep <parame> seconds before each readout");
			addOption (OPT_WIDTH, "width", 1, "width of simulated CCD");
			addOption (OPT_HEIGHT, "height", 1, "height of simulated CCD");
			addOption (OPT_DATA_SIZE, "datasize", 1, "size of data block transmitted over TCP/IP");
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
				default:
					return Camera::processOption (in_opt);
			}
			return 0;
		}
		virtual int init ()
		{
			int ret;
			ret = Camera::init ();
			if (ret)
				return ret;

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
			usleep (infoSleep);
			tempCCD->setValueDouble (100);
			return Camera::info ();
		}
		virtual int startExposure () { return 0; }
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
		}

		virtual int stopExposure ()
		{
			return 0;
		}

	private:
		bool supportFrameT;
		int infoSleep;
		Rts2ValueDouble *readoutSleep;
		Rts2ValueSelection *genType;
		Rts2ValueDouble *noiseRange;
		int width;
		int height;

		long dataSize;
};

};

using namespace rts2camd;

int Dummy::doReadout ()
{
	int ret;
	long usedSize = dataBufferSize;
	if (usedSize > getWriteBinaryDataSize ())
		usedSize = getWriteBinaryDataSize ();
	usleep ((int) (readoutSleep->getValueDouble () * USEC_SEC));
	for (int i = 0; i < usedSize; i += 2)
	{
		uint16_t *d = (uint16_t* ) (dataBuffer + i);
		double n;
		if (genType->getValueInteger () != 1 && genType->getValueInteger () != 2)
			n = noiseRange->getValueDouble ();
		switch (genType->getValueInteger ())
		{
			case 0:  // random
				*d = 20000 + n * random_num () - n / 2;
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
	}
	ret = sendReadoutData (dataBuffer, usedSize);
	if (ret < 0)
		return ret;

	if (getWriteBinaryDataSize () == 0)
		return -2;				 // no more data..
	return 0;					 // imediately send new data
}

int main (int argc, char **argv)
{
	Dummy device = Dummy (argc, argv);
	return device.run ();
}
