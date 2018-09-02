/*
 * Classes for camera image.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

#include "timestamp.h"

#include "rts2fits/cameraimage.h"
#include "rts2fits/devcliimg.h"

using namespace rts2image;

CameraImage::~CameraImage (void)
{
	for (std::vector < ImageDeviceWait * >::iterator iter =	deviceWaits.begin (); iter != deviceWaits.end (); iter++)
	{
		delete *iter;
	}
	deviceWaits.clear ();

	if (image && !dataWriten)
	{
		// Delete the image that did not receive any data - it is just a header
		image->deleteImage();
	}

	delete image;
	image = NULL;
}

void CameraImage::waitForDevice (rts2core::DevClient * devClient, double after)
{
	// check if this device was not prematurely triggered
	if (std::find (prematurelyReceived.begin (), prematurelyReceived.end (), devClient) == prematurelyReceived.end ())
		deviceWaits.push_back (new ImageDeviceWait (devClient, after));
	else
		std::cout << "prematurely received " << devClient->getName () << std::endl;
}

bool CameraImage::waitingFor (rts2core::DevClient * devClient)
{
	bool ret = false;
	for (std::vector < ImageDeviceWait * >::iterator iter = deviceWaits.begin (); iter != deviceWaits.end ();)
	{
		ImageDeviceWait *idw = *iter;
		if (idw->getClient () == devClient && (std::isnan (devClient->getConnection ()->getInfoTime ()) || idw->getAfter () <= devClient->getConnection ()->getInfoTime ()))
		{
			delete idw;
			iter = deviceWaits.erase (iter);
			ret = true;
		}
		else
		{
			logStream (MESSAGE_DEBUG) << "waitingFor "
				<< idw->getClient ()->getName () << " "
				<< (idw->getClient () == devClient) << " "
				<< Timestamp (devClient->getConnection ()->getInfoTime ()) << " "
				<< Timestamp (idw->getAfter ()) <<
				sendLog;
			iter++;
		}
	}
	if (ret)
	{
		image->writeConn (devClient->getConnection (), INFO_CALLED);
	}
	return ret;
}

void CameraImage::waitForTrigger (rts2core::DevClient * devClient)
{
	// check if this device was not prematurely triggered
	if (std::find (prematurelyReceived.begin (), prematurelyReceived.end (), devClient) == prematurelyReceived.end ())
	{
		// do not wait for metadata on images which should be recorded immediately
		if (devClient->getOtherType () == DEVICE_TYPE_CCD && (devClient->getStatus () & CAM_EXPOSING_NOIM))
			return;
		triggerWaits.push_back (devClient);
	}
	else
	{
		std::cout << "received premature trigger " << devClient->getName () << std::endl;
	}
}

bool CameraImage::wasTriggered (rts2core::DevClient * devClient)
{
	for (std::vector < rts2core::DevClient * >::iterator iter = triggerWaits.begin (); iter != triggerWaits.end (); iter++)
	{
		if (*iter == devClient)
		{
			image->writeConn (devClient->getConnection (), TRIGGERED);
			triggerWaits.erase (iter);
			return true;
		}
	}
	return false;
}

bool CameraImage::canDelete ()
{
	if (std::isnan (exEnd) || !dataWriten)
		return false;
	return !waitForMetaData ();
}

bool CameraImage::waitForMetaData ()
{
	return !(deviceWaits.empty () && triggerWaits.empty ());
}

CameraImages::~CameraImages (void)
{
	for (CameraImages::iterator iter = begin (); iter != end (); iter++)
	{
		delete (*iter).second;
	}
	clear ();
}

void CameraImages::deleteOld ()
{
	for (CameraImages::iterator iter = begin (); iter != end ();)
	{
		CameraImage *ci = (*iter).second;
		if (ci->canDelete ())
		{
			delete ci;
			erase (iter++);
		}
		else
		{
			iter++;
		}
	}
}

void CameraImages::infoOK (DevClientCameraImage * master, rts2core::DevClient * client)
{
	for (CameraImages::iterator iter = begin (); iter != end ();)
	{
		CameraImage *ci = (*iter).second;
		if (ci->waitingFor (client))
		{
			if (ci->canDelete ())
			{
				master->processCameraImage (iter++);
			}
			else
			{
				iter++;
			}
		}
		else
		{
			iter++;
		}
	}
}

void CameraImages::infoFailed (DevClientCameraImage * master, rts2core::DevClient * client)
{
	for (CameraImages::iterator iter = begin (); iter != end ();)
	{
		CameraImage *ci = (*iter).second;
		if (ci->waitingFor (client))
		{
			if (ci->canDelete ())
			{
				master->processCameraImage (iter++);
			}
			else
			{
				iter++;
			}
		}
		else
		{
			iter++;
		}
	}
}

bool CameraImages::wasTriggered (DevClientCameraImage * master, rts2core::DevClient * client)
{
	bool ret = false;

	for (CameraImages::iterator iter = begin (); iter != end ();)
	{
		CameraImage *ci = (*iter).second;
		if (ci->wasTriggered (client))
		{
			ret = true;
			if (ci->canDelete ())
				master->processCameraImage (iter++);
			else
				iter++;
		}
		else
		{
			iter++;
		}
	}
	return ret;
}
