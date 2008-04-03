/* 
 * Driver for IFW from Optec.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include "filterd.h"
#include "../utils/rts2connserial.h"

/**
 * Class for OPTEC filter wheel.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author John French
 */
class Rts2DevFilterdIfw:public Rts2DevFilterd
{
	private:
		char *dev_file;
		Rts2ConnSerial *ifwConn;

		char filter_buff[5];
		int readPort (size_t len);
		void shutdown ();

		int homeCount;
	public:
		Rts2DevFilterdIfw (int in_argc, char **in_argv);
		virtual ~ Rts2DevFilterdIfw (void);
		virtual int processOption (int in_opt);
		virtual int init (void);
		virtual int changeMasterState (int new_state);
		virtual int getFilterNum (void);
		virtual int setFilterNum (int new_filter);

		virtual int homeFilter ();
};


int
Rts2DevFilterdIfw::homeFilter ()
{
	int ret;
	ret = ifwConn->writeRead ("WHOME\r", 6, filter_buff, 4, '\r');
	if (ret == -1)
		return ret;
	if (strstr (filter_buff, "ER"))
	{
		logStream (MESSAGE_ERROR) << "filter ifw init error while homing " << filter_buff << sendLog;
		return -1;
	}
	return 0;
}


void
Rts2DevFilterdIfw::shutdown (void)
{
	int n;

	if (ifwConn == NULL)
		return;

	/* shutdown filter wheel communications */
	n = ifwConn->writeRead ("WEXITS\r", 7, filter_buff, 4, '\r');

	/* Check for correct response from filter wheel */
	if (strcmp (filter_buff, "END"))
	{
		logStream (MESSAGE_ERROR) << "filter ifw shutdown FILTER WHEEL ERROR: "
			<< filter_buff << sendLog;
	}
	else
	{
		logStream (MESSAGE_DEBUG) <<
			"filter ifw shutdown Filter wheel shutdown: " << filter_buff <<
			sendLog;
	}
}


Rts2DevFilterdIfw::Rts2DevFilterdIfw (int in_argc, char **in_argv):Rts2DevFilterd (in_argc,
in_argv)
{
	ifwConn = NULL;
	homeCount = 0;

	filterType = "IFW";
	serialNumber = "001";

	addOption ('f', "device_name", 1, "device name (/dev..)");
}


Rts2DevFilterdIfw::~Rts2DevFilterdIfw (void)
{
	shutdown ();
	delete ifwConn;
}


int
Rts2DevFilterdIfw::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dev_file = optarg;
			break;
		default:
			return Rts2DevFilterd::processOption (in_opt);
	}
	return 0;
}


int
Rts2DevFilterdIfw::init (void)
{
	int ret;

	ret = Rts2DevFilterd::init ();
	if (ret)
		return ret;

	ifwConn = new Rts2ConnSerial (dev_file, this, BS19200, C8, NONE, 200);
	ret = ifwConn->init ();
	if (ret)
	  	return ret;

	ifwConn->setDebug ();

	ifwConn->flushPortIO ();

	/* initialise filter wheel */
	ret = ifwConn->writeRead ("WSMODE\r", 7, filter_buff, 4, '\r');
	if (ret == -1)
		return ret;
	
	/* Check for correct response from filter wheel */
	if (filter_buff[0] != '!')
	{
		logStream (MESSAGE_DEBUG) << "filter ifw init FILTER WHEEL ERROR: " << filter_buff << sendLog;
		return -1;
	}
	logStream (MESSAGE_DEBUG) << "filter ifw init Filter wheel initialised: " << filter_buff << sendLog;

	return 0;
}


int
Rts2DevFilterdIfw::changeMasterState (int new_state)
{
	switch (new_state & SERVERD_STATUS_MASK)
	{
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			break;
		default:
			// home FW when we will not need it
			homeFilter ();
			homeCount = 0;
			break;
	}
	return Rts2Device::changeMasterState (new_state);
}


int
Rts2DevFilterdIfw::getFilterNum (void)
{
	int filter_number;
	int n;

	n = ifwConn->writeRead ("WFILTR\r", 7, filter_buff, 4, '\r');
	if (n == -1)
		return -1;

	if (strstr (filter_buff, "ER"))
	{
		logStream (MESSAGE_DEBUG) <<
			"filter ifw getFilterNum FILTER WHEEL ERROR: " << filter_buff <<
			sendLog;
		filter_number = -1;
	}
	else
	{
		filter_number = atoi (filter_buff) - 1;
	}
	return filter_number;
}


int
Rts2DevFilterdIfw::setFilterNum (int new_filter)
{
	char set_filter[] = "WGOTOx\r";
	int ret;

	if (new_filter > 4 || new_filter < 0)
	{
		logStream (MESSAGE_ERROR) <<
			"filter ifw setFilterNum bad filter number: " << new_filter <<
			sendLog;
		return -1;
	}

	set_filter[5] = (char) new_filter + '1';

	ret = ifwConn->writeRead (set_filter, 7, filter_buff, 4, '\r');
	if (ret == -1)
		return -1;

	if (filter_buff[0] != '*')
	{
		logStream (MESSAGE_ERROR) <<
			"filter ifw setFilterNum FILTER WHEEL ERROR: " << filter_buff <<
			sendLog;
		// make sure we will home filter, but home only once if there is still error
		if (homeCount == 0)
		{
			homeFilter ();
			homeCount++;
		}
		ret = -1;
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "filter ifw setFilterNum Set filter: " <<
			filter_buff << sendLog;
		homeCount = 0;
		ret = 0;
	}

	return ret;
}


int
main (int argc, char **argv)
{
	Rts2DevFilterdIfw device = Rts2DevFilterdIfw (argc, argv);
	return device.run ();
}
