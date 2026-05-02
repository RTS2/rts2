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
#include "client.h"
#include "rts2fits/devclifoc.h"
#include <vector>

// events types
#define EVENT_INTEGRATE_START      RTS2_LOCAL_EVENT + 302
#define EVENT_INTEGRATE_STOP       RTS2_LOCAL_EVENT + 303

#define EVENT_XWIN_SOCK            RTS2_LOCAL_EVENT + 304

#define EVENT_EXP_CHECK            RTS2_LOCAL_EVENT + 305

class FocusCameraClient;

class FocusClient:public rts2core::Client
{
	public:
		FocusClient (int argc, char **argv);
		virtual ~ FocusClient (void);

		virtual rts2core::DevClient *createOtherType (rts2core::Connection * conn, int other_device_type);
		virtual int init ();

		virtual void postEvent (rts2core::Event *event);

		float defaultExpousure () { return defExposure; }
		const char *getExePath () { return focExe; }
		int getAutoSave () { return autoSave; }
		int getFocusingQuery () { return query; }
		int getAutoDark () { return autoDark; }

		bool printChanges () { return printStateChanges; };

	protected:
		int autoSave;

		virtual int processOption (int in_opt);
		std::vector < char *>cameraNames;

		char *focExe;

		virtual FocusCameraClient *createFocCamera (rts2core::Connection * conn);
		FocusCameraClient *initFocCamera (FocusCameraClient * cam);

	private:
		float defExposure;
		int defCenter;
		int defBin;

		// to take darks images, set that to true
		bool darks;

		int xOffset;
		int yOffset;

		int imageHeight;
		int imageWidth;

		int autoDark;
		int query;
		double tarRa;
		double tarDec;

		char *photometerFile;
		float photometerTime;
		int photometerFilterChange;

		std::vector < int >skipFilters;

		char *configFile;
		int bop;
		bool printStateChanges;
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

class FocusCameraClient:public rts2image::DevClientCameraFoc
{
	public:
		FocusCameraClient (rts2core::Connection * in_connection, FocusClient * in_master);
		virtual ~ FocusCameraClient (void);

		virtual void postEvent (rts2core::Event *event);

		virtual void stateChanged (rts2core::ServerState * state);
		virtual rts2image::Image *createImage (const struct timeval *expStart);
		void center (int centerWidth, int centerHeight);

		virtual rts2image::imageProceRes processImage (rts2image::Image * image);

		/**
		 * Set condition mask describing when command cannot be send.
		 *
		 * @param bop Something from BOP_EXPOSURE,.. values
		 */
		void setBop (int _bop) { bop = _bop; }

		int singleSave;
	protected:
		int autoSave;

		virtual void exposureStarted (bool expectImage);

	private:
		FocusClient * master;
		int bop;
};
#endif							 /* !__RTS2_GENFOC__ */
