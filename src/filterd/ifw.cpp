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
#include "connection/serial.h"

namespace rts2filterd
{

/**
 * Class for OPTEC filter wheel.
 *
 * http://www.optecinc.com/astronomy/catalog/ifw/images/17350_manual.pdf
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author John French
 */
class Ifw:public Filterd
{
	public:
		Ifw (int in_argc, char **in_argv);
		virtual ~ Ifw (void);
		virtual int processOption (int in_opt);
		virtual int init (void);
		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);
		virtual int getFilterNum (void);
		virtual int setFilterNum (int new_filter);

		virtual int homeFilter ();
	private:
		char *dev_file;
		rts2core::ConnSerial *ifwConn;

		char filter_buff[6];
		int readPort (size_t len);
		void shutdown ();

		int homeCount;
};

};

using namespace rts2filterd;

int Ifw::homeFilter ()
{
	int ret;
	ifwConn->flushPortIO ();

	ret = ifwConn->writeRead ("WHOME\r", 6, filter_buff, 6, '\r');
	if (ret == -1)
		return ret;
	if (strstr (filter_buff, "ER"))
	{
		return -1;
	}
	return 0;
}

void Ifw::shutdown (void)
{
	if (ifwConn == NULL)
		return;

	/* shutdown filter wheel communications */
	ifwConn->writeRead ("WEXITS", 6, filter_buff, 6, '\r');

	/* Check for correct response from filter wheel */
	if (strcmp (filter_buff, "END"))
	{
		logStream (MESSAGE_ERROR) << "filter ifw shutdown FILTER WHEEL ERROR" << sendLog;
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "filter ifw shutdown Filter wheel shutdown: " << filter_buff << sendLog;
	}
}

Ifw::Ifw (int in_argc, char **in_argv):Filterd (in_argc, in_argv)
{
	ifwConn = NULL;
	homeCount = 0;

	addOption ('f', "device_name", 1, "device name (/dev..)");
}

Ifw::~Ifw (void)
{
	shutdown ();
	delete ifwConn;
}

int Ifw::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dev_file = optarg;
			break;
		default:
			return Filterd::processOption (in_opt);
	}
	return 0;
}

int Ifw::init (void)
{
	int ret;

	ret = Filterd::init ();
	if (ret)
		return ret;

	ifwConn = new rts2core::ConnSerial (dev_file, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, 200);
	ret = ifwConn->init ();
	if (ret)
	  	return ret;

	ifwConn->setDebug ();

	ifwConn->flushPortIO ();

	/* initialise filter wheel */
	ret = ifwConn->writeRead ("WSMODE", 6, filter_buff, 6, '\r');
	if (ret == -1)
		return ret;
	
	/* Check for correct response from filter wheel */
	if (filter_buff[0] != '!')
	{
		logStream (MESSAGE_DEBUG) << "filter ifw init FILTER WHEEL ERROR" << sendLog;
		return -1;
	}
	logStream (MESSAGE_DEBUG) << "filter ifw init Filter wheel initialised" << sendLog;

	return 0;
}

void Ifw::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
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
	Filterd::changeMasterState (old_state, new_state);
}

int Ifw::getFilterNum (void)
{
	int filter_number;
	int n;

	n = ifwConn->writeRead ("WFILTR", 6, filter_buff, 6, '\r');
	if (n == -1)
	{
	  	ifwConn->flushPortIO ();
		return -1;
	}

	if (strstr (filter_buff, "ER"))
	{
		logStream (MESSAGE_DEBUG) <<
			"filter ifw getFilterNum FILTER WHEEL ERROR" << sendLog;
		ifwConn->flushPortIO ();
		filter_number = -1;
	}
	else
	{
		filter_number = atoi (filter_buff) - 1;
	}
	return filter_number;
}

int Ifw::setFilterNum (int new_filter)
{
	char set_filter[] = "WGOTOx";
	int ret;

	if (new_filter > 7 || new_filter < 0)
	{
		logStream (MESSAGE_ERROR) << "filter ifw setFilterNum bad filter number: " << new_filter << sendLog;
		return -1;
	}

	set_filter[5] = (char) new_filter + '1';

	ret = ifwConn->writeRead (set_filter, 6, filter_buff, 6, '\r');
	if (ret == -1)
	{
		ifwConn->flushPortIO ();
		return -1;
	}

	if (filter_buff[0] != '*')
	{
		logStream (MESSAGE_ERROR) << "filter ifw setFilterNum FILTER WHEEL ERROR" << sendLog;
		ifwConn->flushPortIO ();
		// make sure we will home filter, but home only once if there is still error
		if (homeCount == 0)
		{
			homeFilter ();
			homeCount++;
			// try to set filter number again after homing
			setFilterNum (new_filter);
		}
		return -1;
	}
	homeCount = 0;
	return Filterd::setFilterNum (new_filter);
}

int main (int argc, char **argv)
{
	Ifw device = Ifw (argc, argv);
	return device.run ();
}
