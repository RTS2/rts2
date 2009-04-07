#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2devclifoc.h"
#include "../utils/timestamp.h"

#include <errno.h>
#include <algorithm>
#include <unistd.h>

Rts2DevClientCameraFoc::Rts2DevClientCameraFoc (Rts2Conn * in_connection,
const char *in_exe):
Rts2DevClientCameraImage (in_connection)
{
	if (in_exe)
	{
		exe = new char[strlen (in_exe) + 1];
		strcpy (exe, in_exe);
	}
	else
	{
		exe = NULL;
	}
	isFocusing = 0;
	focConn = NULL;
}


Rts2DevClientCameraFoc::~Rts2DevClientCameraFoc (void)
{
	delete[]exe;
	if (focConn)
		focConn->nullCamera ();
}


void
Rts2DevClientCameraFoc::postEvent (Rts2Event * event)
{
	Rts2Conn *focus;
	Rts2DevClientFocusFoc *focuser;
	Rts2ConnFocus *eventConn;
	const char *focName;
	const char *cameraFoc;
	switch (event->getType ())
	{
		case EVENT_CHANGE_FOCUS:
			eventConn = (Rts2ConnFocus *) event->getArg ();
			focus =
				connection->getMaster ()->
				getOpenConnection (getConnection ()->getValueChar ("focuser"));
			if (eventConn && eventConn == focConn)
			{
				focusChange (focus);
				focConn = NULL;
			}
			break;
		case EVENT_FOCUSING_END:
			if (!exe)			 // don't care about messages from focuser when we don't have focusing script
				break;
			focuser = (Rts2DevClientFocusFoc *) event->getArg ();
			focName = focuser->getName ();
			cameraFoc = getConnection ()->getValueChar ("focuser");
			if (focName && cameraFoc
				&& !strcmp (focuser->getName (),
				getConnection ()->getValueChar ("focuser")))
			{
				isFocusing = 0;
			}
			break;
	}
	Rts2DevClientCameraImage::postEvent (event);
}


imageProceRes Rts2DevClientCameraFoc::processImage (Rts2Image * image)
{
	// create focus connection
	int ret;

	imageProceRes res = Rts2DevClientCameraImage::processImage (image);

	//else if (darkImage)
	//	image->substractDark (darkImage);
	if ((image->getShutter () == SHUT_OPENED)
		&& exe)
	{
		focConn =
			new Rts2ConnFocus (getMaster (), image, exe, EVENT_CHANGE_FOCUS);
		ret = focConn->init ();
		if (ret)
		{
			delete focConn;
			return IMAGE_DO_BASIC_PROCESSING;
		}
		// after we finish, we will call focus routines..
		connection->getMaster ()->addConnection (focConn);
		return IMAGE_KEEP_COPY;
	}
	return res;
}


void
Rts2DevClientCameraFoc::focusChange (Rts2Conn * focus)
{
	int change = focConn->getChange ();
	if (change == INT_MAX || !focus)
	{
		return;
	}
	focus->postEvent (new Rts2Event (EVENT_START_FOCUSING, (void *) &change));
	isFocusing = 1;
}


Rts2DevClientFocusFoc::Rts2DevClientFocusFoc (Rts2Conn * in_connection):Rts2DevClientFocusImage
(in_connection)
{
}


void
Rts2DevClientFocusFoc::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_START_FOCUSING:
			connection->
				queCommand (new
				Rts2CommandChangeFocus (this,
				*((int *) event->getArg ())));
			break;
	}
	Rts2DevClientFocusImage::postEvent (event);
}


void
Rts2DevClientFocusFoc::focusingEnd ()
{
	Rts2DevClientFocus::focusingEnd ();
	connection->getMaster ()->
		postEvent (new Rts2Event (EVENT_FOCUSING_END, (void *) this));
}


Rts2ConnFocus::Rts2ConnFocus (Rts2Block * in_master, Rts2Image * in_image,
const char *in_exe, int in_endEvent):
Rts2ConnFork (in_master, in_exe)
{
	change = INT_MAX;
	img_path = new char[strlen (in_image->getImageName ()) + 1];
	strcpy (img_path, in_image->getImageName ());
	image = in_image;
	endEvent = in_endEvent;
}


Rts2ConnFocus::~Rts2ConnFocus (void)
{
	if (change == INT_MAX)		 // we don't get focus change, let's try next image..
		getMaster ()->postEvent (new Rts2Event (endEvent, (void *) this));
	delete[]img_path;
	delete image;
}


void
Rts2ConnFocus::initFailed ()
{
	image = NULL;
}


void
Rts2ConnFocus::beforeFork ()
{
	if (exePath)
	{
		// don't care about DB stuff - it will care about it
		// when we will delete image
		image->closeFile ();
	}
}


int
Rts2ConnFocus::newProcess ()
{
	if (exePath)
	{
		execl (exePath, exePath, img_path, (char *) NULL);
		// when execl fails..
		logStream (MESSAGE_ERROR) << "Rts2ConnFocus::newProcess: " <<
			strerror (errno) << " exePath: '" << exePath << "'" << sendLog;
	}
	return -2;
}


void
Rts2ConnFocus::processLine ()
{
	int ret;
	int id;
	ret = sscanf (getCommand (), "change %i %i", &id, &change);
	if (ret == 2)
	{
		logStream (MESSAGE_DEBUG) << "Get change: " << id << " " << change <<
			sendLog;
		if (change == INT_MAX)
			return;			 // that's not expected .. ignore it
		getMaster ()->postEvent (new Rts2Event (endEvent, (void *) this));
		// post it to focuser
	}
	else
	{
		struct stardata sr;
		ret =
			sscanf (getCommand (), "%lf %lf %lf %lf %lf %i", &sr.X, &sr.Y, &sr.F,
			&sr.Fe, &sr.fwhm, &sr.flags);
		if (ret != 6)
		{
			logStream (MESSAGE_ERROR) << "Get line: " << getCommand () <<
				sendLog;
		}
		else
		{
			if (image)
				image->addStarData (&sr);
			logStream (MESSAGE_DEBUG) << "Sex added (" << sr.X << ", " << sr.
				Y << ", " << sr.F << ", " << sr.Fe << ", " << sr.
				fwhm << ", " << sr.flags << ")" << sendLog;
		}
	}
	return;
}


Rts2DevClientPhotFoc::Rts2DevClientPhotFoc (Rts2Conn * in_conn, char *in_photometerFile, float in_photometerTime, int in_photometerFilterChange, std::vector < int >in_skipFilters):Rts2DevClientPhot
(in_conn)
{
	photometerFile = in_photometerFile;
	photometerTime = in_photometerTime;
	photometerFilterChange = in_photometerFilterChange;
	countCount = 0;
	if (photometerFile)
	{
		os.open (photometerFile, std::ofstream::out | std::ofstream::app);
		if (!os.is_open ())
		{
			connection->getMaster ()->
				logStream (MESSAGE_ERROR) << "Cannot write to " << photometerFile
				<< ", exiting." << sendLog;
			exit (1);
		}
	}
	skipFilters = in_skipFilters;
	newFilter = 0;
}


Rts2DevClientPhotFoc::~Rts2DevClientPhotFoc (void)
{
	os.close ();
}


void
Rts2DevClientPhotFoc::addCount (int count, float exp, bool is_ov)
{
	int currFilter = getConnection ()->getValueInteger ("filter");
	connection->getMaster ()->
		logStream (MESSAGE_DEBUG) << "Count on " << connection->
		getName () << ": filter: " << currFilter << " count " << count << " exp: "
		<< exp << " is_ov: " << is_ov << sendLog;
	if (newFilter == currFilter)
		countCount++;
	if (photometerFile)
	{
		os << Timestamp (time (NULL))
			<< " " << connection->getName ()
			<< " " << getConnection ()->getValueInteger ("filter")
			<< " " << count << " " << exp << " " << is_ov << std::endl;
		os.flush ();
	}
	if (photometerFilterChange > 0 && countCount >= photometerFilterChange)
	{
		// try to find filter in skipped one..
		do
		{
			newFilter = (newFilter + 1) % 10;
		}
		while (std::
			find (skipFilters.begin (), skipFilters.end (),
			newFilter) != skipFilters.end ());
		connection->
			queCommand (new
			Rts2CommandIntegrate (this, newFilter, photometerTime,
			photometerFilterChange));
		countCount = 0;
	}
	else if (exp != photometerTime || newFilter != currFilter)
	{
		// we take care of filter change..
		if (photometerFilterChange > 0)
		{
			connection->
				queCommand (new
				Rts2CommandIntegrate (this, newFilter, photometerTime,
				photometerFilterChange));
		}
		else
		{
			// update only counter integration time..
			connection->
				queCommand (new Rts2CommandIntegrate (this, photometerTime, 1));
			// we don't care about filter..
			newFilter = currFilter;
		}
	}
}
