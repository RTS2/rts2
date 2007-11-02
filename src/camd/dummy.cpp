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

class Rts2DevCameraDummy:public Rts2DevCamera
{
	private:
		char *data;

		bool supportFrameT;
		int infoSleep;
		Rts2ValueDouble *readoutSleep;
		int width;
		int height;

	protected:
		virtual void initBinnings ()
		{
			Rts2DevCamera::initBinnings ();

			addBinning2D (2, 2);
			addBinning2D (3, 3);
			addBinning2D (4, 4);
			addBinning2D (1, 2);
			addBinning2D (2, 1);
			addBinning2D (4, 1);
		}

		virtual void initDataTypes ()
		{
			Rts2DevCamera::initDataTypes ();
			addDataType (RTS2_DATA_BYTE);
			addDataType (RTS2_DATA_SHORT);
			addDataType (RTS2_DATA_LONG);
			addDataType (RTS2_DATA_LONGLONG);
			addDataType (RTS2_DATA_FLOAT);
			addDataType (RTS2_DATA_DOUBLE);
			addDataType (RTS2_DATA_SBYTE);
			addDataType (RTS2_DATA_ULONG);
		}

		virtual int setGain (double in_gain)
		{
			return 0;
		}

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value)
		{
			if (old_value == readoutSleep)
			{
				return 0;
			}
			return Rts2DevCamera::setValue (old_value, new_value);
		}
	public:
		Rts2DevCameraDummy (int in_argc, char **in_argv):Rts2DevCamera (in_argc,
			in_argv)
		{
			createTempCCD ();

			supportFrameT = false;
			infoSleep = 0;
			createValue (readoutSleep, "readout", "readout sleep in sec", true, 0,
				CAM_EXPOSING | CAM_READING | CAM_DATA, true);
			readoutSleep->setValueDouble (0);

			width = 200;
			height = 100;
			addOption ('f', "frame_transfer", 0,
				"when set, dummy CCD will act as frame transfer device");
			addOption ('I', "info_sleep", 1,
				"device will sleep i nanosecunds before each info and baseInfo return");
			addOption ('r', "readout_sleep", 1,
				"device will sleep i nanosecunds before each readout");
			addOption ('w', "width", 1, "width of simulated CCD");
			addOption ('g', "height", 1, "height of simulated CCD");

			data = NULL;
		}
		virtual ~ Rts2DevCameraDummy (void)
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
				case 'w':
					width = atoi (optarg);
					break;
				case 'g':
					height = atoi (optarg);
					break;
				default:
					return Rts2DevCamera::processOption (in_opt);
			}
			return 0;
		}
		virtual int init ()
		{
			int ret;
			ret = Rts2DevCamera::init ();
			if (ret)
				return ret;

			usleep (infoSleep);
			strcpy (ccdType, "Dummy");
			strcpy (serialNumber, "1");

			return initChips ();
		}
		virtual int initChips ()
		{
			initCameraChip (width, height, 0, 0);
			return Rts2DevCamera::initChips ();
		}
		virtual int ready ()
		{
			return 0;
		}
		virtual int info ()
		{
			usleep (infoSleep);
			tempCCD->setValueDouble (100);
			return Rts2DevCamera::info ();
		}
		virtual int camChipInfo ()
		{
			return 0;
		}

		virtual int startExposure (int light, float exptime)
		{
			return 0;
		}
		virtual int readoutOneLine ();

		virtual int readoutEnd ()
		{
			if (data)
			{
				delete data;
				data = NULL;
			}
			return Rts2DevCamera::endReadout ();
		}

		virtual bool supportFrameTransfer ()
		{
			return supportFrameT;
		}
};

int
Rts2DevCameraDummy::readoutOneLine ()
{
	int ret;
	int lineSize = lineByteSize ();
	if (sendLine == 0)
	{
		ret = sendFirstLine ();
		if (ret)
			return ret;
		data = new char[lineSize];
	}
	if (readoutLine == 0 && readoutSleep->getValueDouble () > 0)
		usleep ((int) (readoutSleep->getValueDouble () * USEC_SEC));
	for (int i = 0; i < lineSize; i++)
	{
		data[i] = i + readoutLine;
	}
	readoutLine++;
	sendLine++;
	ret = sendReadoutData (data, lineSize);
	if (ret < 0)
		return ret;
	if (readoutLine <
		(chipUsedReadout->getYInt () + chipUsedReadout->getHeightInt ()) / binningVertical ())
		return 0;				 // imediately send new data
	return -2;					 // no more data..
}


int
main (int argc, char **argv)
{
	Rts2DevCameraDummy device = Rts2DevCameraDummy (argc, argv);
	return device.run ();
}
