#ifndef __RTS2_GENFOC__
#define __RTS2_GENFOC__
#include "../utils/rts2client.h"
#include "../writers/rts2devclifoc.h"

#include <vector>

#define PP_NEG

#define GAMMA 0.995

#define PP_MED 0.60
#define PP_HIG 0.995
#define PP_LOW (1 - PP_HIG)

#define HISTOGRAM_LIMIT   65536

// events types
#define EVENT_START_EXPOSURE  RTS2_LOCAL_EVENT + 300
#define EVENT_STOP_EXPOSURE RTS2_LOCAL_EVENT + 301

#define EVENT_INTEGRATE_START   RTS2_LOCAL_EVENT + 302
#define EVENT_INTEGRATE_STOP    RTS2_LOCAL_EVENT + 303

#define EVENT_XWIN_SOCK   RTS2_LOCAL_EVENT + 304

class Rts2GenFocCamera;

class Rts2GenFocClient:public Rts2Client
{
	private:
		float defExposure;
		int defCenter;
		int defBin;

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
		virtual Rts2GenFocCamera *initFocCamera (Rts2GenFocCamera * cam);
	public:
		Rts2GenFocClient (int argc, char **argv);
		virtual ~ Rts2GenFocClient (void);

		virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
			int other_device_type);
		virtual int init ();

		virtual void centraldConnRunning ();

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
	public:
		Rts2GenFocCamera (Rts2Conn * in_connection, Rts2GenFocClient * in_master);
		virtual ~ Rts2GenFocCamera (void);

		virtual void postEvent (Rts2Event * event);

		virtual void stateChanged (Rts2ServerState * state);
		virtual Rts2Image *createImage (const struct timeval *expStart);
		virtual imageProceRes processImage (Rts2Image * image);
		virtual void focusChange (Rts2Conn * focus);
		void center (int centerWidth, int centerHeight);
};
#endif							 /* !__RTS2_GENFOC__ */
