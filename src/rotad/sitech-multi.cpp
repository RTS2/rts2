/* 
 * Master for Sitech rotators.
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#include "sitech-multi.h"

#define OPT_DEF_DER1    OPT_LOCAL + 2451
#define OPT_DEF_DER2    OPT_LOCAL + 2452
#define OPT_PA_TRACK    OPT_LOCAL + 2453

using namespace rts2rotad;

SitechMulti::SitechMulti (int argc, char **argv):rts2core::MultiBase (argc, argv, "SR")
{
	der_tty = NULL;
	derConn = NULL;

	paTrack = 3;

	memset (derdefaults, 0, sizeof (derdefaults));
	memset (rotators, 0, sizeof (rotators));

	addOption ('f', NULL, 1, "derotator serial port (ussually /dev/ttyUSBxx)");
	addOption (OPT_DEF_DER1, "defaults-der1", 1, "defaults values for derotator 1");
	addOption (OPT_DEF_DER2, "defaults-der2", 1, "defaults values for derotator 2");

	addOption (OPT_PA_TRACK, "pa-track", 1, "switch PA tracking on only for given derotator (accept only 0, 1 or 2)");
}

SitechMulti::~SitechMulti ()
{
	delete derConn;
	derConn = NULL;
}

int SitechMulti::run ()
{
	int ret = init ();
	if (ret)
		return ret;
	ret = initHardware ();
	if (ret)
		return ret;
	md.initMultidev (getDebug ());
	while (true)
	{
		md.runLoop (0.005);
		callInfo ();
	}
}

int SitechMulti::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			der_tty = optarg;
			break;

		case OPT_DEF_DER1:
			derdefaults[0] = optarg;
			break;

		case OPT_DEF_DER2:
			derdefaults[1] = optarg;
			break;

		case OPT_PA_TRACK:
			if (!strcmp (optarg, "0"))
				paTrack = 0;
			else if (!strcmp (optarg, "1"))
				paTrack = 1;
			else if (!strcmp (optarg, "2"))
				paTrack = 2;
			else
				return -1;
			break;

		default:
			return MultiBase::processOption (opt);
	}
	return 0;
}

int SitechMulti::initHardware ()
{
	derConn = new rts2core::ConnSitech (der_tty, this);
	derConn->setDebug (getDebug ());

	int ret = derConn->init ();
	if (ret)
		return -1;
	derConn->flushPortIO ();

	rotators[0] = new rts2rotad::SitechRotator ('X', "DER1", derConn, this, derdefaults[0], false);
	rotators[1] = new rts2rotad::SitechRotator ('Y', "DER2", derConn, this, derdefaults[1], true);

	if (!(paTrack & 0x01))
		rotators[0]->unsetPaTracking ();
	if (!(paTrack & 0x02))
		rotators[1]->unsetPaTracking ();

	ybits = derConn->getSiTechValue ('Y', "B");
	xbits = derConn->getSiTechValue ('X', "B");

	for (int i = 0; i < NUM_ROTATORS; i++)
	{
		rotators[i]->getConfiguration ();
		rotators[i]->getPIDs ();
	}

	md.push_back (rotators[0]);
	md.push_back (rotators[1]);

	return 0;
}

int SitechMulti::callInfo ()
{
	try
	{
		derConn->getAxisStatus ('X');

		if (rotators[0] != NULL)
			rotators[0]->processAxisStatus (&(derConn->last_status));
		if (rotators[1] != NULL)
			rotators[1]->processAxisStatus (&(derConn->last_status));

		if ((rotators[0] != NULL && rotators[0]->updated) || (rotators[1] != NULL && rotators[1]->updated))
			derSetTarget ();
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << "exception during callInfo: " << er << sendLog;
		delete derConn;
		delete rotators[0];
		delete rotators[1];

		memset (rotators, 0, sizeof (rotators));
		md.clear ();

		// try to reinit..

		int ret = initHardware ();
		md.initMultidev (getDebug ());
		return ret;
	}
	return 0;
}

void SitechMulti::callUpdate ()
{
	derSetTarget ();
}

void SitechMulti::derSetTarget ()
{
	der_Xrequest.y_dest = 0;
	der_Xrequest.x_dest = 0;

	if (rotators[0] != NULL)
	{
		der_Xrequest.x_dest = rotators[0]->t_pos->getValueLong ();
		der_Xrequest.x_speed = derConn->degsPerSec2MotorSpeed (rotators[0]->speed->getValueDouble (), rotators[0]->ticks->getValueLong (), 360) * SPEED_MULTI;
	}
	if (rotators[1] != NULL)
	{
		der_Xrequest.y_dest = rotators[1]->t_pos->getValueLong ();
		der_Xrequest.y_speed = derConn->degsPerSec2MotorSpeed (rotators[1]->speed->getValueDouble (), rotators[1]->ticks->getValueLong (), 360) * SPEED_MULTI;
	}

	//xbits &= ~(0x01 << 4);
	//ybits &= ~(0x01 << 4);

	der_Xrequest.x_bits = xbits;
	der_Xrequest.y_bits = ybits;

	try
	{
		derConn->sendXAxisRequest (der_Xrequest);

		if (rotators[0] != NULL)
		{
			rotators[0]->updated = false;
			rotators[0]->checkRotators ();
			rotators[0]->updateTrackingFrequency ();
		}
		if (rotators[1] != NULL)
		{
			rotators[1]->updated = false;
			rotators[1]->checkRotators ();
			rotators[1]->updateTrackingFrequency ();
		}
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << "error in derSetTarget: " << er << sendLog;
	}

}

int main (int argc, char **argv)
{
	SitechMulti mb (argc, argv);
	return mb.run ();
}
