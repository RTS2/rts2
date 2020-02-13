/*
 * Bridge driver for Planewave L-500 mount
 * Copyright (C) 2019 Michael Mommert <mommermiscience@gmail.com>
 * and Giannina Guzman <gguzman@lowell.edu>
 *
 * This driver requires an instance of PWI4 running on the same machine.
 * up-to-date with PWI4-version 4.0.5 beta16
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

#include <curl/curl.h>
#include <unistd.h>
#include "teld.h"
#include "configuration.h"

namespace rts2teld
{

class Planewave:public Telescope
{
	public:
        Planewave (int argc, char **argv);
		~Planewave (void);
		
	protected:
      	virtual int processOption (int in_opt);
		virtual int init();
		virtual int initValues ();

		virtual int info();

		virtual int startResync();
		virtual int isMoving();
		virtual int stopMove();
		virtual int endMove();
		
		virtual int startPark();
		virtual int isParking();
		virtual int endPark();

		virtual int setTracking(int, bool, bool);
	   
	private:
		virtual int mountstatus_parser(std::string s);
		virtual int send_command(const char *command);
		static size_t curl_parser(void *contents, size_t size,
								  size_t nmemb, std::string *s);
	
		int is_connected;
		int is_slewing; 
		int is_tracking;
	    int axis0_is_enabled;
		int axis1_is_enabled;

		struct ln_hrz_posn parkPos;

		char pointmodel_used[100];
		char pointmodel_filename[100];
		
		float geometry, ra_apparent_h, dec_apparent_d, ra_j2000_d, dec_j2000_d, 
		azimuth, altitude, field_angle_local, field_angle_target,
		field_anglerate, 
		path_angle_target, path_anglerate, axis0_rms_error_arcsec, 
		axis0_dist_to_target_arcsec, axis0_err, axis0_position, 
		axis1_rms_error_arcsec, axis1_dist_to_target_arcsec, axis1_err, 
		axis1_position, model_numpoints_total, model_numpoints_enabled,
		model_rms_error,
		target_ra_apparent_hours, target_dec_apparent_degs;

		const char *url;
};

}

using namespace rts2teld;

Planewave::Planewave (int argc, char **argv):Telescope (argc, argv, true, true)
{
	// initialize some variables
	url = NULL;
	axis0_is_enabled = -1;
	axis1_is_enabled = -1;
	is_connected = -1;
	is_tracking = -1;
	is_slewing = -1;

	// url for communications with pwi4
	url = "http://localhost:8220/";

	// pointing model filename
	sprintf(pointmodel_filename, "timo.pxp");

	// define parking position in alt/az
	parkPos.alt = 10;
	parkPos.az = 270; // E; (N:180, W:90, S:0, E:270)	
}

Planewave::~Planewave(void)
{
	startPark();
	endPark();
}


int Planewave::initValues()
{
	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();
	setTelLongLat (config->getObserver ()->lng, config->getObserver ()->lat);
	setTelAltitude (config->getObservatoryAltitude ());
	trackingInterval->setValueFloat (0.5);
	return Telescope::initValues ();
}

int Planewave::init()
{
	// initialize and setup telescope for operations

	int ret, status;

	status = Telescope::init();
	if (status)
		return status;

	// connect to the mount
	ret = send_command("mount/connect");
	usleep(USEC_SEC * 1);
	info();
	if (is_connected != 1)
	{
		logStream (MESSAGE_ERROR) << "could not connect to mount" <<
			sendLog;	
		return -1;
	}
	logStream (MESSAGE_INFO) << "mount connected" << sendLog;	

	// enable az axis
	ret = send_command("mount/enable?axis=0");
    usleep(USEC_SEC * 1);
	info();
	if (axis0_is_enabled != 1)
	{
		logStream (MESSAGE_ERROR) << "could not enable axis 0" <<
			sendLog;	
		return -1;
	}
	logStream (MESSAGE_INFO) << "az axis enabled" << sendLog;	

	// enable el axis
	ret = send_command("mount/enable?axis=1");
    usleep(USEC_SEC * 1);
	info();
	if (axis1_is_enabled != 1)
	{
		logStream (MESSAGE_ERROR) << "could not enable axis 1" <<
			sendLog;	
		return -1;
	}
	logStream (MESSAGE_INFO) << "el axis enabled" << sendLog;	

	// home telescope
	ret = send_command("mount/find_home");
    usleep(USEC_SEC * 20);
	logStream (MESSAGE_INFO) << "telescope @ home" << sendLog;	

	// load pointing model
	char cmdbuffer[120];
	sprintf(cmdbuffer, "mount/model/load?filename=%s", pointmodel_filename);
	ret = send_command(cmdbuffer);
	logStream (MESSAGE_INFO) << "pointing model loaded" << sendLog;	

	// send telescope to park position
	startPark();
	while (1)
	{
		if (isParking() == -2)
			break;
		usleep(USEC_SEC);
	}
	logStream (MESSAGE_INFO) << "mount is in park position" << sendLog;
	
	// now, the telescope is ready for operations

	return 0;
}


size_t Planewave::curl_parser(void *contents, size_t size, size_t nmemb, std::string *s)
{
	// curl parser for parsing status report from PWI4

	size_t newlength = size*nmemb;

	try 
	{
		s->append((char*)contents, newlength);
	}
	catch(std::bad_alloc &e)
	{
		return 0;
	}

	return newlength;

}

int  Planewave::mountstatus_parser(std::string s)
{
	// parse all mount-related information from status report
	
	float ra_j2000_h;
	
	// strip off lines unrelated to mount
	unsigned first = s.find("mount");
	unsigned last = s.find("focuser");
	std::string sCleaned = s.substr(first, last-first);

	// replace strings with integers where applicable
	size_t pos = 0;
    while ((pos = sCleaned.find("true", pos)) != std::string::npos)
	{
         sCleaned.replace(pos, 4, "1");
         pos += 1;
	}
	pos = 0;
    while ((pos = sCleaned.find("false", pos)) != std::string::npos)
	{
         sCleaned.replace(pos, 5, "0");
         pos += 1;
	}

	// check whether status was properly retrieved
	field_angle_target = -999;
	
	sscanf(sCleaned.c_str(),
		"mount.is_connected=%d\nmount.geometry=%f\n"
	    "mount.ra_apparent_hours=%f\nmount.dec_apparent_degs=%f\n"
		"mount.ra_j2000_hours=%f\nmount.dec_j2000_degs=%f\n"
		"mount.target_ra_apparent_hours=%f\nmount.target_dec_apparent_degs=%f\n"
		"mount.azimuth_degs=%f\nmount.altitude_degs=%f\n"
		"mount.is_slewing=%d\nmount.is_tracking=%d\n"
		"mount.field_angle_here_degs=%f\n"
		"mount.field_angle_at_target_degs=%f\n"
		"mount.field_angle_rate_at_target_degs_per_sec=%f\n"
		"mount.path_angle_at_target_degs=%f\n"
		"mount.path_angle_rate_at_target_degs_per_sec=%f\n"
		"mount.axis0.is_enabled=%d\nmount.axis0.rms_error_arcsec=%f\n"
		"mount.axis0.dist_to_target_arcsec=%f\n"
		"mount.axis0.servo_error_arcsec=%f\nmount.axis0.position_degs=%f\n"
		"mount.axis1.is_enabled=%d\nmount.axis1.rms_error_arcsec=%f\n"
		"mount.axis1.dist_to_target_arcsec=%f\n"
		"mount.axis1.servo_error_arcsec=%f\nmount.axis1.position_degs=%f\n"
		"mount.model.filename=%s\n"
		"mount.model.num_points_total=%f\n"
		"mount.model.num_points_enabled=%f\nmount.model.rms_error_arcsec=%f",
		&is_connected, &geometry, &ra_apparent_h, &dec_apparent_d, &ra_j2000_h, 
		&dec_j2000_d,
        &target_ra_apparent_hours, &target_dec_apparent_degs,
	    &azimuth, &altitude, &is_slewing, &is_tracking, &field_angle_local, 
		&field_angle_target, &field_anglerate, &path_angle_target, &path_anglerate, 
		&axis0_is_enabled, &axis0_rms_error_arcsec, &axis0_dist_to_target_arcsec, 
		&axis0_err, &axis0_position, &axis1_is_enabled, &axis1_rms_error_arcsec, 
		&axis1_dist_to_target_arcsec, &axis1_err, &axis1_position, pointmodel_used, 
		&model_numpoints_total, &model_numpoints_enabled, &model_rms_error);

	// convert ra from hours to degs
	ra_j2000_d = floor(ra_j2000_h)*15+(ra_j2000_h-floor(ra_j2000_h))*15;
	
	// check whether status was properly retrieved
	if (field_angle_target == -999)
		return -1;
	
	return 0;
}


int Planewave::send_command(const char *command)
{
	// send a command to PWI4

	CURL *curl;
	CURLcode res;

	std::ostringstream os;
	os << url << command;
	std::string s;

	logStream (MESSAGE_DEBUG) << "sending " << os.str().c_str() << sendLog;
	
	curl = curl_easy_init();
	
	curl_easy_setopt(curl, CURLOPT_URL, os.str().c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Planewave::curl_parser);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
	
	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
	{
		logStream (MESSAGE_ERROR) << "curl failed " << url <<
			curl_easy_strerror(res) << "; is PWI4 running?" << sendLog;	
		return -1;
	}

	curl_easy_cleanup(curl);

	// parse status report
	mountstatus_parser(s);

	return 0;
}



int Planewave::processOption (int in_opt)
{
	// process options provided in /etc/rts2/devices
	// not much here yet
	// TODO: implement option to change pointing model file name here

	switch (in_opt)
	{
		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}


int Planewave::info()
{
	// parse and adopt status report from PWI4
	// called constantly and whenever "info" is called in rts2-mon/T0
	
	if (send_command("status") != 0)
	{
		logStream (MESSAGE_ERROR) << "could not retrieve status" << sendLog;
		return -1;
	}
	
	// set rts2-internal telescope coordinates
	setTelRa(ra_j2000_d);
	setTelDec(dec_j2000_d);
	
	return Telescope::info ();	
}



int Planewave::startResync ()
{
	// this function is called when the telescope is supposed to move
	// e.g., when "move <ra> <dec>" is called in rts2-mon/T0
	
	// check if target is above/below horizon
	struct ln_equ_posn tar;
	struct ln_hrz_posn hrz;

	double JD = ln_get_julian_from_sys ();

	// retrieve target coordinates
	getTarget (&tar);

	ln_get_hrz_from_equ (&tar, rts2core::Configuration::instance ()->getObserver (), JD, &hrz);

	if (hrz.alt < 0 && !getIgnoreHorizon ())
	{
		logStream (MESSAGE_ERROR) << "cannot move to negative altitude (" << hrz.alt << ")" <<
			sendLog;
		return -1;
	}

	// send telescope to target position
	char cmd[100];
	sprintf(cmd, "mount/goto_ra_dec_j2000?ra_hours=%f&dec_degs=%f",
			tar.ra/15, tar.dec);
  	logStream (MESSAGE_DEBUG) << "moving with: " << cmd << sendLog;	

	if (send_command(cmd) < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot move to position " << sendLog;
		return -1;
	}

	return 0;
}

int Planewave::isMoving ()
{
	// update status
	if (info())
		return -1;

	if (is_slewing)
	{
		// if telescope is still slewing, call this function again in 500 msec
		return 100*USEC_SEC;
	}
	else
	{
		// if telescope is done slewing, assume it arrived at its target
		// might want to replace this with a position check in the future
		return -2;
	}
	
	return USEC_SEC;
}

int Planewave::stopMove()
{
	// this function will be called when "stop" is used in rts2-mon/T0
	if (send_command("mount/stop") != 0)
		return -1;

    logStream (MESSAGE_INFO) << "movement stopped" << sendLog;	
	
	return 0;
}

int Planewave::endMove()
{
	return 0;
}


int Planewave::startPark()
{
	// this function parks the telescope, e.g., when "park" is called in rts2-mon/T0

	logStream (MESSAGE_DEBUG) << "parking telescope" << sendLog;	
	
	rts2core::Configuration *config = rts2core::Configuration::instance();
	int ret = config->loadFile();
	if (ret)
		return -1;

	// derive Alt/Az for parkPos
	struct ln_equ_posn equ;
	ln_get_equ_from_hrz(&parkPos,  config->getObserver(),
						ln_get_julian_from_sys(), &equ);

	// send telescope to park position
	setTarget(equ.ra, equ.dec);

	if (startResync() != 0)
		return -1;
			
    return 0;
}

int Planewave::isParking()
{
	// this function is called repeatedly during parking to figure out when
	// parking is done

	if (info() != 0)
		return -1;
	
	if (is_slewing)
	{
		// return values > 0 means time to wait in millisec until calling this function
		// again
		return 500;
	}

	// disable tracking and return -2 if parking is complete
	stopTracking();
	return -2;
}

int Planewave::endPark()
{
	// this function is called once parking is done
	// (but for some reason not automatically after startPark())

	int ret;

	// check if telescope is actually close to parkPos
	if (info() != 0)
		return -1;
	if ((abs(altitude-parkPos.alt) > 10) || (abs(azimuth-parkPos.az) > 20))
		startPark();

	// stop tracking
	stopTracking();
	
	// deactivate telescope

	// disable az axis
	ret = send_command("mount/disable?axis=0");
    usleep(USEC_SEC * 1);
	info();
	if (axis0_is_enabled != 0)
	{
		logStream (MESSAGE_ERROR) << "could not disable axis 0" <<
			sendLog;	
		return -1;
	}
	logStream (MESSAGE_INFO) << "az axis disabled" << sendLog;	

	// disable el axis
	ret = send_command("mount/disable?axis=1");
    usleep(USEC_SEC * 1);
	info();
	if (axis1_is_enabled != 0)
	{
		logStream (MESSAGE_ERROR) << "could not disable axis 1" <<
			sendLog;	
		return -1;
	}
	logStream (MESSAGE_INFO) << "el axis disabled" << sendLog;	

	// disconnect the mount
	ret = send_command("mount/disconnect");
	usleep(USEC_SEC * 1);
	info();
	if (is_connected != 0)
	{
		logStream (MESSAGE_ERROR) << "could not disconnect mount" <<
			sendLog;	
		return -1;
	}
	logStream (MESSAGE_INFO) << "mount disconnected" << sendLog;	

	logStream (MESSAGE_INFO) << "telescope parked and deactivated" << sendLog;		
}

int Planewave::setTracking(int track, bool addTrackingTimer, bool send)
{
	// this function is called whenever the TRACKING variable changes its value
	// sidereal tracking seems to work; non-sidereal tracking has not yet been tested
	int ret;

	logStream (MESSAGE_INFO) << "tracking? " << isTracking() << sendLog;	
	
	if (track)
		ret = send_command("mount/tracking_on");
	else
		ret = send_command("mount/tracking_off");

	if (ret)
		return ret;
	
	return 0;
}


int main (int argc, char **argv)
{
	Planewave device (argc, argv);
	return device.run ();
}
