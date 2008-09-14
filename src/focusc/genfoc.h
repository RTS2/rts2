/* 
 * Generic class for focusing.
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

#ifndef __RTS2_GENFOC__
#define __RTS2_GENFOC__
#include "../utils/rts2client.h"
#include "../writers/rts2devclifoc.h"

#include <vector>

#define PP_NEG

#define GAMMA                      0.995

#define PP_MED                     0.60
#define PP_HIG                     0.995
#define PP_LOW                     (1 - PP_HIG)

#define HISTOGRAM_LIMIT            65536

// events types
#define EVENT_INTEGRATE_START      RTS2_LOCAL_EVENT + 302
#define EVENT_INTEGRATE_STOP       RTS2_LOCAL_EVENT + 303

#define EVENT_XWIN_SOCK            RTS2_LOCAL_EVENT + 304

class Rts2GenFocCamera;

class Rts2GenFocClient:public Rts2Client
{
	private:
		float defExposure;
		int defCenter;
		int defBin;

		// to take darks images, set that to true
		bool darks;

		int centerHeight;
		int centerWidth;

		int autoDark;
		int query;
		double tarRa;
		double tarDec;

		char *photometerFile;
		float photometerTime;
		int photometerFilterChange;

		std::vector < int >skipFilters;

		char *configFile;

	protected:
		int autoSave;

		virtual int processOption (int in_opt);
		std::vector < char *>cameraNames;

		char *focExe;

		virtual Rts2GenFocCamera *createFocCamera (Rts2Conn * conn);
		Rts2GenFocCamera *initFocCamera (Rts2GenFocCamera * cam);
	public:
		Rts2GenFocClient (int argc, char **argv);
		virtual ~ Rts2GenFocClient (void);

		virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
			int other_device_type);
		virtual int init ();

		virtual void centraldConnRunning (Rts2Conn *conn);

		float defaultExpousure ()
		{
			return defExposure;
		}
		const char *getExePath ()
		{
			return focExe;
		}
		int getAutoSave ()
		{
			return autoSave;
		}
		int getFocusingQuery ()
		{
			return query;
		}
		int getAutoDark ()
		{
			return autoDark;
		}
};

class fwhmData
{
	public:
		int num;
		int focPos;
		double fwhm;
		fwhmData (int in_num, int in_focPos, double in_fwhm)
		{
			num = in_num;
			focPos = in_focPos;
			fwhm = in_fwhm;
		}
};

class Rts2GenFocCamera:public Rts2DevClientCameraFoc
{
	private:
		Rts2GenFocClient * master;
	protected:
		unsigned short low, med, hig, max, min;
		double average;
		double stdev;
		double bg_stdev;
		int autoSave;
		int histogram[HISTOGRAM_LIMIT];

		std::list < fwhmData * >fwhmDatas;
		virtual void printFWHMTable ();

		virtual void getPriority ();
		virtual void lostPriority ();

		virtual void exposureStarted ();
	public:
		Rts2GenFocCamera (Rts2Conn * in_connection, Rts2GenFocClient * in_master);
		virtual ~ Rts2GenFocCamera (void);

		virtual void stateChanged (Rts2ServerState * state);
		virtual Rts2Image *createImage (const struct timeval *expStart);
		virtual imageProceRes processImage (Rts2Image * image);
		virtual void focusChange (Rts2Conn * focus);
		void center (int centerWidth, int centerHeight);
};
#endif							 /* !__RTS2_GENFOC__ */
