#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "timestamp.h"

#include "rts2fits/devclifoc.h"

#include <errno.h>
#include <algorithm>
#include <unistd.h>

using namespace rts2image;

DevClientCameraFoc::DevClientCameraFoc (rts2core::Rts2Connection * in_connection, const char *in_exe):DevClientCameraImage (in_connection)
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

DevClientCameraFoc::~DevClientCameraFoc (void)
{
	delete[]exe;
	if (focConn)
		focConn->nullCamera ();
}

void DevClientCameraFoc::postEvent (rts2core::Event * event)
{
	rts2core::Rts2Connection *focus;
	DevClientFocusFoc *focuser;
	ConnFocus *eventConn;
	const char *focName;
	const char *cameraFoc;
	switch (event->getType ())
	{
		case EVENT_CHANGE_FOCUS:
			eventConn = (ConnFocus *) event->getArg ();
			focus = connection->getMaster ()->getOpenConnection (getConnection ()->getValueChar ("focuser"));
			if (eventConn && eventConn == focConn)
			{
				focusChange (focus);
				focConn = NULL;
			}
			break;
		case EVENT_FOCUSING_END:
			if (!exe)			 // don't care about messages from focuser when we don't have focusing script
				break;
			focuser = (DevClientFocusFoc *) event->getArg ();
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
	DevClientCameraImage::postEvent (event);
}

imageProceRes DevClientCameraFoc::processImage (Image * image)
{
	// create focus connection
	int ret;

	imageProceRes res = DevClientCameraImage::processImage (image);

	//else if (darkImage)
	//	image->substractDark (darkImage);
	if ((image->getShutter () == SHUT_OPENED) && exe)
	{
		focConn = new ConnFocus (getMaster (), image, exe, EVENT_CHANGE_FOCUS);
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

void DevClientCameraFoc::focusChange (rts2core::Rts2Connection * focus)
{
	int change = focConn->getChange ();
	if (change == INT_MAX || !focus)
	{
		return;
	}
	focus->postEvent (new rts2core::Event (EVENT_START_FOCUSING, (void *) &change));
	isFocusing = 1;
}

DevClientFocusFoc::DevClientFocusFoc (rts2core::Rts2Connection * in_connection):DevClientFocusImage (in_connection)
{
}

void DevClientFocusFoc::postEvent (rts2core::Event * event)
{
	switch (event->getType ())
	{
		case EVENT_START_FOCUSING:
			connection->queCommand (new rts2core::CommandChangeFocus (this, *((int *) event->getArg ())));
			break;
	}
	DevClientFocusImage::postEvent (event);
}

void DevClientFocusFoc::focusingEnd ()
{
	rts2core::DevClientFocus::focusingEnd ();
	connection->getMaster ()->postEvent (new rts2core::Event (EVENT_FOCUSING_END, (void *) this));
}

ConnFocus::ConnFocus (rts2core::Block * in_master, Image * in_image, const char *in_exe, int in_endEvent):rts2core::ConnFork (in_master, in_exe, false, false)
{
	change = INT_MAX;
	img_path = new char[strlen (in_image->getFileName ()) + 1];
	strcpy (img_path, in_image->getFileName ());
	image = in_image;
	endEvent = in_endEvent;
}

ConnFocus::~ConnFocus (void)
{
	if (change == INT_MAX)		 // we don't get focus change, let's try next image..
		getMaster ()->postEvent (new rts2core::Event (endEvent, (void *) this));
	delete[]img_path;
	delete image;
}

void ConnFocus::initFailed ()
{
	image = NULL;
}

void ConnFocus::beforeFork ()
{
	if (exePath)
	{
		// don't care about DB stuff - it will care about it
		// when we will delete image
		image->closeFile ();
	}
}

int ConnFocus::newProcess ()
{
	if (exePath)
	{
		execl (exePath, exePath, img_path, (char *) NULL);
		// when execl fails..
		logStream (MESSAGE_ERROR) << "ConnFocus::newProcess: " <<
			strerror (errno) << " exePath: '" << exePath << "'" << sendLog;
	}
	return -2;
}

void ConnFocus::processLine ()
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
		getMaster ()->postEvent (new rts2core::Event (endEvent, (void *) this));
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

DevClientPhotFoc::DevClientPhotFoc (rts2core::Rts2Connection * in_conn, char *in_photometerFile, float in_photometerTime, int in_photometerFilterChange, std::vector < int >in_skipFilters):rts2core::DevClientPhot (in_conn)
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

DevClientPhotFoc::~DevClientPhotFoc (void)
{
	os.close ();
}

void DevClientPhotFoc::addCount (int count, float exp, bool is_ov)
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
		while (std::find (skipFilters.begin (), skipFilters.end (), newFilter) != skipFilters.end ()); 

		connection-> queCommand (new rts2core::CommandIntegrate (this, newFilter, photometerTime, photometerFilterChange));
		countCount = 0;
	}
	else if (exp != photometerTime || newFilter != currFilter)
	{
		// we take care of filter change..
		if (photometerFilterChange > 0)
		{
			connection->queCommand (new rts2core::CommandIntegrate (this, newFilter, photometerTime, photometerFilterChange));
		}
		else
		{
			// update only counter integration time..
			connection->queCommand (new rts2core::CommandIntegrate (this, photometerTime, 1));
			// we don't care about filter..
			newFilter = currFilter;
		}
	}
}
