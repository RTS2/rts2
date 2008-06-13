/* 
 * Driver for IR (OpenTPL) dome.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "dome.h"
#include "irconn.h"
#include "../utils/rts2config.h"

#include <libnova/libnova.h>

/**
 * Driver for Bootes IR dome.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevDomeIR:public Rts2DevDome
{
	private:
		std::string ir_ip;
		int ir_port;

		IrConn *irConn;

		Rts2ValueFloat *domeUp;
		Rts2ValueFloat *domeDown;

		Rts2ValueDouble *domeCurrAz;
		Rts2ValueDouble *domeTargetAz;
		Rts2ValueBool *domePower;
		Rts2ValueDouble *domeTarDist;

		Rts2ValueBool *domeAutotrack;

		enum { D_OPENED, D_OPENING, D_CLOSING, D_CLOSED } dome_state;

		int getSNOW (float *temp, float *humi, float *wind);

		/**
		 * Set dome autotrack.
		 *
		 * @param new_auto New auto track value. True or false.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int setDomeTrack (bool new_auto);
		int initIrDevice ();

	protected:
		virtual int processOption (int in_opt);

		virtual int setValue (Rts2Value *old_value, Rts2Value *new_value);

	public:
		Rts2DevDomeIR (int argc, char **argv);
		virtual ~ Rts2DevDomeIR (void);
		virtual int init ();
		virtual int idle ();

		virtual int ready ();
		virtual int off ();
		virtual int standby ();
		virtual int observing ();
		virtual int baseInfo ();
		virtual int info ();

		virtual int openDome ();
		virtual int closeDome ();
};


int
Rts2DevDomeIR::setValue (Rts2Value *old_value, Rts2Value *new_value)
{	
	int status = TPL_OK;
	if (old_value == domeAutotrack)
	{
		status = setDomeTrack (((Rts2ValueBool *) new_value)->getValueBool ());
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == domeUp)
	{
		status = irConn->tpl_set ("DOME[1].TARGETPOS", new_value->getValueFloat (), &status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == domeDown)
	{
		status = irConn->tpl_set ("DOME[2].TARGETPOS", new_value->getValueFloat (), &status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == domeTargetAz)
	{
		status = irConn->tpl_set ("DOME[0].TARGETPOS",
			ln_range_degrees (new_value->getValueDouble () - 180.0),
			&status);
		if (status != TPL_OK)
			return -2;
		return 0;

	}
	if (old_value == domePower)
	{
		status =  irConn->tpl_set ("DOME[0].POWER",
			((Rts2ValueBool *) new_value)->getValueBool ()? 1 : 0,
			&status);
		if (status != TPL_OK)
			return -2;
		return 0;

	}
	return Rts2DevDome::setValue (old_value, new_value);
}


int
Rts2DevDomeIR::openDome ()
{
	int status = TPL_OK;
	dome_state = D_OPENING;
	status = irConn->tpl_set ("DOME[1].TARGETPOS", 1, &status);
	status = irConn->tpl_set ("DOME[2].TARGETPOS", 1, &status);
	logStream (MESSAGE_INFO) << "opening dome, status " << status << sendLog;
	if (status != TPL_OK)
		return -1;
	return Rts2DevDome::openDome ();
}


int
Rts2DevDomeIR::closeDome ()
{
	int status = TPL_OK;
	dome_state = D_CLOSING;
	status = irConn->tpl_set ("DOME[1].TARGETPOS", 0, &status);
	status = irConn->tpl_set ("DOME[2].TARGETPOS", 0, &status);
	logStream (MESSAGE_INFO) << "closing dome, status " << status << sendLog;
	if (status != TPL_OK)
		return -1;
	return Rts2DevDome::closeDome ();
}


int
Rts2DevDomeIR::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'I':
			ir_ip = std::string (optarg);
			break;
		case 'N':
			ir_port = atoi (optarg);
			break;
		default:
			return Rts2DevDome::processOption (in_opt);
	}
	return 0;
}


Rts2DevDomeIR::Rts2DevDomeIR (int argc, char **argv)
:Rts2DevDome (argc, argv)
{
	domeModel = "IR";

	createValue (domeAutotrack, "dome_auto_track", "dome auto tracking", false);
	domeAutotrack->setValueBool (true);

	createValue (domeUp, "dome_up", "upper dome cover", false);
	createValue (domeDown, "dome_down", "dome down cover", false);

	createValue (domeCurrAz, "dome_curr_az", "dome current azimunt", false, RTS2_DT_DEGREES);
	createValue (domeTargetAz, "dome_target_az", "dome targer azimut", false, RTS2_DT_DEGREES);
	createValue (domePower, "dome_power", "if dome have power", false);
	createValue (domeTarDist, "dome_tar_dist", "dome target distance", false, RTS2_DT_DEG_DIST);

	addOption ('I', "ir_ip", 1, "IR TCP/IP address");
	addOption ('N', "ir_port", 1, "IR TCP/IP port number");

	dome_state = D_OPENING;
}


Rts2DevDomeIR::~Rts2DevDomeIR (void)
{

}


int
Rts2DevDomeIR::getSNOW (float *temp, float *humi, float *wind)
{
	int sockfd, bytes_read, ret, i, j = 0;
	struct sockaddr_in dest;
	char buffer[300], buff[300];
	fd_set rfds;
	struct timeval tv;
	double wind1, wind2;
	char *token;

	if ((sockfd = socket (PF_INET, SOCK_STREAM, 0)) < 0)
	{
		fprintf (stderr, "unable to create socket");
		return 1;
	}

	bzero (&dest, sizeof (dest));
	dest.sin_family = PF_INET;
	dest.sin_addr.s_addr = inet_addr ("193.146.151.70");
	dest.sin_port = htons (6341);
	ret = connect (sockfd, (struct sockaddr *) &dest, sizeof (dest));
	if (ret)
	{
		fprintf (stderr, "unable to connect to SNOW");
		return 1;
	}

	/* send command to SNOW */
	sprintf (buffer, "%s", "RCD");
	ret = write (sockfd, buffer, strlen (buffer));
	if (!ret)
	{
		fprintf (stderr, "unable to send command");
		return 1;
	}

	FD_ZERO (&rfds);
	FD_SET (sockfd, &rfds);

	tv.tv_sec = 10;
	tv.tv_usec = 0;

	ret = select (FD_SETSIZE, &rfds, NULL, NULL, &tv);

	if (ret == 1)
	{
		bytes_read = read (sockfd, buffer, sizeof (buffer));
		if (bytes_read > 0)
		{
			for (i = 5; i < bytes_read; i++)
			{
				if (buffer[i] == ',')
					buff[j++] = '.';
				else
					buff[j++] = buffer[i];
			}
		}
	}
	else
	{
		fprintf (stderr, ":-( no data");
		return 1;
	}

	close (sockfd);

	token = strtok ((char *) buff, "|");
	for (j = 1; j < 40; j++)
	{
		token = (char *) strtok (NULL, "|#");
		if (j == 3)
			wind1 = atof (token);
		if (j == 6)
			wind2 = atof (token);
		if (j == 18)
			*temp = atof (token);
		if (j == 24)
			*humi = atof (token);
	}

	if (wind1 > wind2)
		*wind = wind1;
	else
		*wind = wind2;

	return 0;
}


int
Rts2DevDomeIR::setDomeTrack (bool new_auto)
{
	int status = TPL_OK;
	int old_track;
	status = irConn->tpl_get ("POINTING.TRACK", old_track, &status);
	if (status != TPL_OK)
		return -1;
	old_track &= ~128;
	status = irConn->tpl_set ("POINTING.TRACK", old_track | (new_auto ? 128 : 0), &status);
	if (status != TPL_OK)
	{
		logStream (MESSAGE_ERROR) << "Cannot setDomeTrack" << sendLog;
	}
	return status;
}


int
Rts2DevDomeIR::initIrDevice ()
{
	Rts2Config *config = Rts2Config::instance ();
	config->loadFile (NULL);
	// try to get default from config file
	if (ir_ip.length () == 0)
	{
		config->getString ("ir", "ip", ir_ip);
	}
	if (!ir_port)
	{
		config->getInteger ("ir", "port", ir_port);
	}
	if (ir_ip.length () == 0 || !ir_port)
	{
		std::cerr << "Invalid port or IP address of mount controller PC"
			<< std::endl;
		return -1;
	}

	irConn = new IrConn (ir_ip, ir_port);

	// are we connected ?
	if (!irConn->isOK ())
	{
		std::cerr << "Connection to server failed" << std::endl;
		return -1;
	}

	return 0;
}


int
Rts2DevDomeIR::info ()
{
	double dome_curr_az, dome_target_az, dome_tar_dist, dome_power;
	int status = TPL_OK;
	status = irConn->tpl_get ("DOME[0].CURRPOS", dome_curr_az, &status);
	status = irConn->tpl_get ("DOME[0].TARGETPOS", dome_target_az, &status);
	status = irConn->tpl_get ("DOME[0].TARGETDISTANCE", dome_tar_dist, &status);
	status = irConn->tpl_get ("DOME[0].POWER", dome_power, &status);
	if (status == TPL_OK)
	{
		domeCurrAz->setValueDouble (ln_range_degrees (dome_curr_az + 180));
		domeTargetAz->setValueDouble (ln_range_degrees (dome_target_az + 180));
		domeTarDist->setValueDouble (dome_tar_dist);
		domePower->setValueBool (dome_power == 1);
	}

	if (dome_state == D_CLOSING && dome_state == D_OPENING)
	{
		status = TPL_OK;
		double dome_up, dome_down;
		status = irConn->tpl_get ("DOME[1].CURRPOS", dome_up, &status);
		status = irConn->tpl_get ("DOME[2].CURRPOS", dome_down, &status);
		if (status != TPL_OK)
		{
			logStream (MESSAGE_ERROR) << "unknow dome state" << sendLog;
			return -1;
		}
		domeUp->setValueFloat (dome_up);
		domeDown->setValueFloat (dome_down);
		if (dome_up == 1.0 && dome_down == 1.0)
			dome_state = D_OPENED;
		if (dome_up == 0.0 && dome_down == 0.0)
			dome_state = D_CLOSED;
	}

	//rain = weatherConn->getRain ();
	float t_temp;
	float t_hum;
	float t_wind;

	getSNOW (&t_temp, &t_hum, &t_wind);
	setTemperature (t_temp);
	setHumidity (t_hum);
	setWindSpeed (t_wind);
	return Rts2DevDome::info ();
}


int
Rts2DevDomeIR::baseInfo ()
{
	return 0;
}


int
Rts2DevDomeIR::off ()
{
	closeDome ();
	return 0;
}


int
Rts2DevDomeIR::standby ()
{
	closeDome ();
	return 0;
}


int
Rts2DevDomeIR::ready ()
{
	return 0;
}


int
Rts2DevDomeIR::idle ()
{
	return Rts2DevDome::idle ();
}


int
Rts2DevDomeIR::init ()
{
	int ret = Rts2DevDome::init ();
	if (ret)
		return ret;
	return initIrDevice ();
}


int
Rts2DevDomeIR::observing ()
{
	if ((getState () & DOME_DOME_MASK) == DOME_CLOSED)
		return openDome ();
	return 0;
}


int
main (int argc, char **argv)
{
	Rts2DevDomeIR device = Rts2DevDomeIR (argc, argv);
	return device.run ();
}
