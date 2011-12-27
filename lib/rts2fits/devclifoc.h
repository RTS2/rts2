#ifndef __RTS2_DEVCLIFOC__
#define __RTS2_DEVCLIFOC__

#include "devcliimg.h"
#include "connfork.h"

#include <fstream>
#include <vector>

#define EVENT_START_FOCUSING  RTS2_LOCAL_EVENT + 500
#define EVENT_FOCUSING_END  RTS2_LOCAL_EVENT + 501
#define EVENT_CHANGE_FOCUS  RTS2_LOCAL_EVENT + 502

namespace rts2image
{

class ConnFocus;

class DevClientCameraFoc:public DevClientCameraImage
{
	private:
		int isFocusing;

	protected:
		char *exe;

		ConnFocus *focConn;
	public:
		DevClientCameraFoc (Rts2Conn * in_connection, const char *in_exe);
		virtual ~ DevClientCameraFoc (void);
		virtual void postEvent (Rts2Event * event);
		virtual imageProceRes processImage (Image * image);
		// will cause camera to change focus by given steps BEFORE exposition
		// when change == INT_MAX, focusing don't converge
		virtual void focusChange (Rts2Conn * focus);
};

class DevClientFocusFoc:public DevClientFocusImage
{
	protected:
		virtual void focusingEnd ();
	public:
		DevClientFocusFoc (Rts2Conn * in_connection);
		virtual void postEvent (Rts2Event * event);
};

class ConnFocus:public rts2core::ConnFork
{
	private:
		char *img_path;
		Image *image;
		int change;
		int endEvent;
	protected:
		virtual void initFailed ();
		virtual void beforeFork ();
	public:
		ConnFocus (rts2core::Block * in_master, Image * in_image, const char *in_exe, int in_endEvent);
		virtual ~ ConnFocus (void);
		virtual int newProcess ();
		virtual void processLine ();
		int getChange () { return change; }
		void setChange (int new_change) { change = new_change; }
		const char *getCameraName () { return image->getCameraName (); }
		Image *getImage () { return image; }
		void nullCamera () { image = NULL; }
};

class DevClientPhotFoc:public rts2core::Rts2DevClientPhot
{
	private:
		std::ofstream os;
		char *photometerFile;
		float photometerTime;
		int photometerFilterChange;
		int countCount;
		std::vector < int >skipFilters;
		int newFilter;
	protected:
		virtual void addCount (int count, float exp, bool is_ov);
	public:
		DevClientPhotFoc (Rts2Conn * in_conn, char *in_photometerFile,
			float in_photometerTime,
			int in_photometerFilterChange,
			std::vector < int >in_skipFilters);
		virtual ~ DevClientPhotFoc (void);
};

}
#endif							 /* !CLIFOC__ */
