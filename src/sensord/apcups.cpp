/* 
 * APCUPS deamon sensor.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "sensord.h"

#include "connection/tcp.h"

#include <map>
#include <string.h>

#define OPT_MINB_TIME      OPT_LOCAL + 122

namespace rts2sensord
{

    /**
     * Class for communication with APC UPS.
     *
     * @author Petr Kubanek <petr@kubanek.net>
     */
    class ConnApcUps: public rts2core::ConnTCP
    {
	public:
	    /**
	     * Create new connection to APC UPS daemon.
	     *
	     * @param _master   Reference to master holding this connection.
	     *
	     * @param _hostname APC UPSD IP address or hostname.
	     * @param _port     Portnumber of APC UPSD daemon (default to 3551).
	     */
	    ConnApcUps (rts2core::Block *_master, const char *_hostname, int _port);

	    /**
	     * Call command, get reply.
	     *
	     * @param cmd       Command.
	     *
	     * @throw ConnError
	     */
	    int command (const char *cmd);

	    const char *getString (const char *val);
	    float getPercents (const char *val);
	    float getTemp (const char *val);
	    int getTime (const char *val);
	    time_t getDate (const char *val);

	private:
	    std::map <std::string, std::string> values;
    };

    /**
     * Sensor for accessing APC UPS informations.
     *
     * @author Petr Kubanek <petr@kubanek.net>
     */
    class ApcUps:public SensorWeather
    {
	public:
	    ApcUps (int argc, char **argv);
	    virtual ~ApcUps (void);

	    virtual int info ();

	protected:
	    virtual int processOption (int opt);

	    virtual int init ();

	private:
	    HostString *host;

	    rts2core::ValueString *model;

	    rts2core::ValueFloat *linev;
	    rts2core::ValueFloat *outputv;
	    rts2core::ValueFloat *battv;
	    rts2core::ValueFloat *loadpct;
	    rts2core::ValueFloat *bcharge;
	    rts2core::ValueInteger *timeleft;
	    rts2core::ValueFloat *itemp;
	    rts2core::ValueInteger *tonbatt;
	    rts2core::ValueString *status;

	    rts2core::ValueInteger *battimeout;

	    rts2core::ValueFloat *minbcharge;
	    rts2core::ValueInteger *mintimeleft;

	    rts2core::ValueTime *xOnBatt;
	    rts2core::ValueTime *xOffBatt;
    };

}

using namespace rts2sensord;

ConnApcUps::ConnApcUps (rts2core::Block *_master, const char *_hostname, int _port):rts2core::ConnTCP (_master, _hostname, _port)
{
}

int ConnApcUps::command (const char *cmd)
{
    uint16_t len = htons (strlen (cmd));
    while (true)
    {
	int rsize;
	char reply_data[502];

	sendData (&len, 2, true);
	sendData (cmd);

	receiveData (reply_data, 2, 5);

	rsize = ntohs (*((uint16_t *) reply_data));
	if (rsize == 0)
	    return 0;
	receiveData (reply_data, rsize, 2);

	reply_data[rsize] = '\0';
	// try to parse reply
	if (reply_data[9] != ':')
	{
	    logStream (MESSAGE_ERROR) << "Invalid reply data" << reply_data << sendLog;
	    return -1;
	}
	reply_data[9] = '\0';
	if (strcmp (reply_data, "END APC  ") == 0)
	{
	    return 0;
	}
	else
	{
	    // eat any spaces..
	    char *pchr = reply_data + 8;
	    while (isspace (*pchr) && pchr > reply_data)
		pchr--;
	    pchr[1] = '\0';
	    char *dat = reply_data + rsize - 1;
	    while (isspace (*dat))
		dat--;
	    dat[1] = '\0';
	    dat = reply_data + 10;
	    while (isspace (*dat))
		dat++;
	    values[std::string (reply_data)] = std::string (dat);
	}
    }
}

const char* ConnApcUps::getString (const char *val)
{
    std::map <std::string, std::string>::iterator iter = values.find (val);
    if (values.find (val) == values.end ())
	throw rts2core::ConnError (this, (std::string ("Value ") + (*iter).second + std::string ("not found")).c_str ());
    return (*iter).second.c_str ();
}

float ConnApcUps::getPercents (const char *val)
{
    std::map <std::string, std::string>::iterator iter = values.find (val);
    if (values.find (val) == values.end ())
	throw rts2core::ConnError (this, "Cannot get percents");
    return atof ((*iter).second.c_str());
}

float ConnApcUps::getTemp (const char *val)
{
    const char *v = getString (val);
    if (strchr (v, 'C') == NULL)
	throw rts2core::ConnError (this, "Value is not in deg C");
    return atof (v);
}

int ConnApcUps::getTime (const char *val)
{
    const char *v = getString (val);
    if (strcasestr (v, "hours") != NULL)
	return (int) (atof (v) * 3600);
    if (strcasestr (v, "minutes") != NULL)
	return (int) (atof (v) * 60);
    if (strcasestr (v, "seconds") != NULL)
	return atoi (v);
    throw rts2core::ConnError (this, "Cannot convert time");
}

time_t ConnApcUps::getDate (const char *val)
{
    const char *v = getString (val);
    struct tm _tm;
    char *te = strptime (v, "%Y-%m-%d %X %Z", &_tm);
    if (te == NULL || *te != '\0')
    {
	{
	    throw rts2core::ConnError (this, "Cannot convert date");
	}
    }
    return mktime (&_tm);
}

int ApcUps::processOption (int opt)
{
    switch (opt)
    {
	case 'a':
	    host = new HostString (optarg, "3551");
	    break;
	case OPT_MINB_TIME:
	    mintimeleft->setValueCharArr (optarg);
	    break;
	default:
	    return SensorWeather::processOption (opt);
    }
    return 0;
}

int ApcUps::init ()
{
    int ret;
    ret = SensorWeather::init ();
    if (ret)
	return ret;

    ret = info ();
    if (ret)
	return ret;
    setIdleInfoInterval (10);
    return 0;
}

int ApcUps::info ()
{
    int ret;
    ConnApcUps *connApc = new ConnApcUps (this, host->getHostname (), host->getPort ());
    try
    {
	connApc->init (false);
	ret = connApc->command ("status");
	if (ret)
	{
	    logStream (MESSAGE_WARNING) << "cannot retrieve data from apcups, putting UPS to bad weather state" << sendLog;
	    setWeatherTimeout (120, "cannot retrieve data from UPS");
	    return ret;
	}
	model->setValueString (connApc->getString ("MODEL"));
	linev->setValueFloat (connApc->getPercents( "LINEV"));
	outputv->setValueFloat (connApc->getPercents( "OUTPUTV"));
	battv->setValueFloat (connApc->getPercents( "BATTV"));
	loadpct->setValueFloat (connApc->getPercents ("LOADPCT"));
	bcharge->setValueFloat (connApc->getPercents ("BCHARGE"));
	timeleft->setValueInteger (connApc->getTime ("TIMELEFT"));
	itemp->setValueFloat (connApc->getTemp ("ITEMP"));
	tonbatt->setValueInteger (connApc->getTime ("TONBATT"));
	status->setValueString (connApc->getString ("STATUS"));
	xOnBatt->setValueTime (connApc->getDate ("XONBATT"));
	xOffBatt->setValueTime (connApc->getDate ("XOFFBATT"));
	setInfoTime (connApc->getDate ("DATE"));
    }
    catch (rts2core::ConnError er)
    {
	logStream (MESSAGE_ERROR) << er << sendLog;
	setWeatherTimeout (120, er.what ());
	return -1;
    }



    if (tonbatt->getValueInteger () > battimeout->getValueInteger ())
    {
	logStream (MESSAGE_WARNING) <<  "running for too long on batteries: " << tonbatt->getValueInteger () << sendLog;
	setWeatherTimeout (battimeout->getValueInteger () + 60, "running for too long on batteries");
    }

    if (bcharge->getValueFloat () < minbcharge->getValueFloat ())
    {
	logStream (MESSAGE_WARNING) << "battery charge too low: " << bcharge->getValueFloat () << " < " << minbcharge->getValueFloat () << sendLog;
	setWeatherTimeout (1200, "low battery charge");
    }

    if (timeleft->getValueInteger () < mintimeleft->getValueInteger ())
    {
	logStream (MESSAGE_WARNING) << "minimal battery time too low: " << timeleft->getValueInteger () << " < " << mintimeleft->getValueInteger () << sendLog;
	setWeatherTimeout (1200, "low minimal baterry time");

    }

    // if there is any UPS error, set big timeout..
    if (strcmp(status->getValue(), "BOOST ONLINE") && strcmp (status->getValue(), "ONLINE") && strcmp (status->getValue(), "ONBATT"))
    {
	logStream (MESSAGE_WARNING) <<  "unknown status " << status->getValue () << sendLog;
	setWeatherTimeout (1200, "unknown status");
    }

    delete connApc;

    return 0;
}

ApcUps::ApcUps (int argc, char **argv):SensorWeather (argc, argv)
{
    createValue (model, "model", "UPS mode", false);
    createValue (linev, "linev", "Line voltage", false);
    createValue (outputv, "outputv", "Output voltage", false);
    createValue (battv, "battv", "Battery voltage", false);
    createValue (loadpct, "load", "UPS load", false);
    createValue (bcharge, "bcharge", "battery charge", false);
    createValue (timeleft, "timeleft", "time left for on-UPS operations", false);
    createValue (itemp, "temperature", "internal UPS temperature", false);
    createValue (tonbatt, "tonbatt", "time on battery", false);
    createValue (status, "status", "UPS status", false);

    createValue (xOnBatt, "xonbatt", "time of last on battery event", false);
    createValue (xOffBatt, "xoffbatt", "time of last off battery event", false);

    createValue (battimeout, "battery_timeout", "shorter then those onbatt interruptions will be ignored", false, RTS2_VALUE_WRITABLE);
    battimeout->setValueInteger (60);

    createValue (minbcharge, "min_bcharge", "minimal battery charge for opening", false, RTS2_VALUE_WRITABLE);
    minbcharge->setValueFloat (50);
    createValue (mintimeleft, "min_tleft", "minimal time left for UPS operation", false, RTS2_VALUE_WRITABLE);
    mintimeleft->setValueInteger (1200);

    addOption ('a', NULL, 1, "hostname[:port] of apcupds");
    addOption (OPT_MINB_TIME, "min-btime", 1, "minimal battery run time");
}

ApcUps::~ApcUps (void)
{
}

int main (int argc, char **argv)
{
    ApcUps device = ApcUps (argc, argv);
    return device.run ();
}
