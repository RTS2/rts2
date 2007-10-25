#include "cameraimage.h"
#include "rts2devcliimg.h"
#include "../utils/timestamp.h"

CameraImage::~CameraImage (void)
{
	for (std::vector < ImageDeviceWait * >::iterator iter =
		deviceWaits.begin (); iter != deviceWaits.end (); iter++)
	{
		delete *iter;
	}
	deviceWaits.clear ();
	delete image;
}


void
CameraImage::waitForDevice (Rts2DevClient * devClient, double after)
{
	deviceWaits.push_back (new ImageDeviceWait (devClient, after));
}


bool
CameraImage::waitingFor (Rts2DevClient * devClient)
{
	bool ret = false;
	for (std::vector < ImageDeviceWait * >::iterator iter =
		deviceWaits.begin (); iter != deviceWaits.end ();)
	{
		ImageDeviceWait *idw = *iter;
		if (idw->getClient () == devClient
			&& (isnan (devClient->getConnection ()->getInfoTime ())
			|| idw->getAfter () <
			devClient->getConnection ()->getInfoTime ()))
		{
			delete idw;
			iter = deviceWaits.erase (iter);
			ret = true;
		}
		else
		{
			logStream (MESSAGE_DEBUG) << "waitingFor " << (idw->getClient () ==
				devClient) <<
				Timestamp (devClient->getConnection ()->
				getInfoTime ()) << " " << Timestamp (idw->
				getAfter ()) <<
				sendLog;
			iter++;
		}
	}
	return ret;
}


bool
CameraImage::canDelete ()
{
	if (isnan (exEnd) || !dataWriten)
		return false;
	return deviceWaits.empty ();
}


CameraImages::~CameraImages (void)
{
	for (CameraImages::iterator iter = begin (); iter != end (); iter++)
	{
		delete (*iter);
	}
	clear ();
}


void
CameraImages::deleteOld ()
{
	for (CameraImages::iterator iter = begin (); iter != end ();)
	{
		CameraImage *ci = *iter;
		if (ci->canDelete ())
		{
			delete ci;
			iter = erase (iter);
		}
		else
		{
			iter++;
		}
	}
}


void
CameraImages::infoOK (Rts2DevClientCameraImage * master,
Rts2DevClient * client)
{
	for (CameraImages::iterator iter = begin (); iter != end ();)
	{
		CameraImage *ci = *iter;
		if (ci->waitingFor (client))
		{
			ci->image->writeConn (client->getConnection (), EXPOSURE_END);
			if (ci->canDelete ())
			{
				iter = master->processCameraImage (iter);
			}
		}
		else
		{
			iter++;
		}
	}
}


void
CameraImages::infoFailed (Rts2DevClientCameraImage * master,
Rts2DevClient * client)
{
	for (CameraImages::iterator iter = begin (); iter != end ();)
	{
		CameraImage *ci = *iter;
		if (ci->waitingFor (client))
		{
			ci->image->writeConn (client->getConnection (), EXPOSURE_END);
			iter = master->processCameraImage (iter);
		}
		else
		{
			iter++;
		}
	}
}
