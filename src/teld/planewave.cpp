/**
 * Bridge driver for Planewave L-500 mount
 * Copyright (C) 2019 Michael Mommert <mommermiscience@gmail.com>
 * and Giannina Guzman <gguzman@lowell.edu>
 *
 * Note for anyone maintaining this in the future:
 * function information are available in rts2/include/teld.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <curl/curl.h>
#include <unistd.h>

#include "teld.h"
#include "configuration.h"
#include "status.h"


using namespace std;

namespace rts2teld
{

class PlaneWave:public Telescope
{
	public:
		PlaneWave (int in_argc, char **in_argv);
		~PlaneWave(void);

		virtual int init();
		virtual int initValues();

		virtual int info();		

	  	virtual int startResync();
		virtual int isMoving();
		virtual int stopMove();

		virtual int startPark();
		virtual int isParking();
		virtual int endPark();

		virtual int setTracking(int track, bool addTrackingTimer=false, bool send=true);
		virtual void startTracking();
		virtual void stopTracking();

	protected:
		virtual int processOption(int in_opt);

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
		azimuth, altitude, field_angle_local, field_angle_target, field_anglerate, 
		path_angle_target, path_anglerate, axis0_rms_error_arcsec, 
		axis0_dist_to_target_arcsec, axis0_err, axis0_position, 
		axis1_rms_error_arcsec, axis1_dist_to_target_arcsec, axis1_err, 
		axis1_position, model_numpoints_total, model_numpoints_enabled,
		model_rms_error;

		const char *url;
};

};

using namespace rts2teld;

PlaneWave::PlaneWave(int argc, char **argv):Telescope(argc, argv)
{
	//addOption ('e', NULL, 1, "IP and port (separated by :)");

	url = NULL;
	axis0_is_enabled = -1;
	axis1_is_enabled = -1;
	is_connected = -1;
	is_tracking = -1;
	is_slewing = -1;

	// define parking position in alt/az
	parkPos.alt = 10;
	parkPos.az = 270; // E; (N:180, W:90, S:0, E:270)	
}

PlaneWave::~PlaneWave(void)
{
	send_command("disconnect");
}

size_t PlaneWave::curl_parser(void *contents, size_t size, size_t nmemb, std::string *s)
{
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

int  PlaneWave::mountstatus_parser(std::string s)
{
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

	sscanf(sCleaned.c_str(), "mount.is_connected=%d\nmount.geometry=%f\n"
	        "mount.ra_apparent_hours=%f\nmount.dec_apparent_degs=%f\n"
		"mount.ra_j2000_hours=%f\nmount.dec_j2000_degs=%f\n"
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
		&dec_j2000_d, &azimuth, &altitude, &is_slewing, &is_tracking, &field_angle_local, 
		&field_angle_target, &field_anglerate, &path_angle_target, &path_anglerate, 
		&axis0_is_enabled, &axis0_rms_error_arcsec, &axis0_dist_to_target_arcsec, 
		&axis0_err, &axis0_position, &axis1_is_enabled, &axis1_rms_error_arcsec, 
		&axis1_dist_to_target_arcsec, &axis1_err, &axis1_position, pointmodel_used, 
		&model_numpoints_total, &model_numpoints_enabled, &model_rms_error);

	// convert ra from hours to degs
	ra_j2000_d = floor(ra_j2000_h)*15+(ra_j2000_h-floor(ra_j2000_h))*15;
	
	return 0;

}

int PlaneWave::processOption(int in_opt)
{
	// switch (in_opt)
	// {
	// 	case 'e':
	// 		host = new HostString (optarg, "1000");
	// 		break;
	//     // case 's':
	// 	// 	filterSleep = atoi (optarg);
	// 	// 	break;
    //     //         default:
    //     //                 return Filterd::processOption (in_opt);
	// }

	url = "http://localhost:8220/";
	sprintf(pointmodel_filename, "timo.pxp");
	
	return 0;
}

int PlaneWave::send_command(const char *command)
{
	CURL *curl;
	CURLcode res;

	std::ostringstream os;
	os << url << command;
	std::string s;

	logStream (MESSAGE_DEBUG) << "sending " << os.str().c_str() << sendLog;
	
	curl = curl_easy_init();
	
	curl_easy_setopt(curl, CURLOPT_URL, os.str().c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, PlaneWave::curl_parser);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
	
	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
	{
		logStream (MESSAGE_ERROR) << "curl failed " << url <<
			curl_easy_strerror(res) << sendLog;	
		return -1;
	}

	curl_easy_cleanup(curl);
	
	mountstatus_parser(s);

	return 0;
}


int PlaneWave::init()
{
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
	logStream (MESSAGE_INFO) << "homing telescope" << sendLog;	

	// load pointing model
	char cmdbuffer[120];
	sprintf(cmdbuffer, "mount/model/load?filename=%s", pointmodel_filename);
	ret = send_command(cmdbuffer);
	logStream (MESSAGE_INFO) << "loading pointing model" << sendLog;	

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

int PlaneWave::initValues()
{
	rts2core::Configuration *config;
	config = rts2core::Configuration::instance();
	config->loadFile();
	setTelLongLat(config->getObserver()->lng, config->getObserver()->lat);
	setTelAltitude(config->getObservatoryAltitude());
	return Telescope::initValues();
}


int PlaneWave::info()
{
	// this function is called when you use <info> in rts2-mon; should also be used
	// to update telescope information
	
	if (send_command("status") != 0)
		return -1;

	// set rts2-internal telescope coordinates
	setTelRa(ra_j2000_d);
	setTelDec(dec_j2000_d);

	return Telescope::info();	
}


int PlaneWave::startResync()
{
	// this function is called when you use <move ra dec> in rts2-mon



	
    // check if target is above/below horizon
	struct ln_equ_posn tar;	
	getTarget (&tar);
	
	// send telescope to target position
	char cmd[100];
	sprintf(cmd, "mount/goto_ra_dec_j2000?ra_hours=%f&dec_degs=%f",
			tar.ra/15, tar.dec);
	if (send_command(cmd) < 0)
		return -1;

	struct ln_hrz_posn hrz;
	double JD = ln_get_julian_from_sys ();

	//ln_get_hrz_from_equ (&localPosition, rts2core::Configuration::instance ()->getObserver (), ln_get_julian_from_timet (&localDate), &hrz);

	getTargetAltAz(&hrz, JD);
	
	return 0;
}

int PlaneWave::isMoving()
{
	// this function is called during slewing to figure out when the telescope is done

	// update status
	if (send_command("status") != 0)
		return -1;

	if (is_slewing)
	{
		// if telescope is still slewing, call this function again in 500 msec
		//logStream (MESSAGE_INFO) << "I am done moving" << sendLog;
		return 5000;
	}
	else
	{
		// if telescope is done slewing, assume it arrived at its target
		// might want to replace this with a position check in the future
		//logStream (MESSAGE_INFO) << "I am done moving" << sendLog;
		return -2;
	}
}

int PlaneWave::stopMove()
{
	// this function will be called when stop is used in rts2-mon
	
	if (send_command("mount/stop") != 0)
		return -1;

    //logStream (MESSAGE_INFO) << "stopping movement" << sendLog;	
	
	return 0;
}


int PlaneWave::startPark()
{
	// this function is called when you use <park> in rts2-mon

	rts2core::Configuration *config = rts2core::Configuration::instance ();
	int ret = config->loadFile ();
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

int PlaneWave::isParking()
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

	// return -2 if parking is complete
	return -2;
}

int PlaneWave::endPark()
{
	// this function is called once the parking is done

	int ret;
	
	// check if telescope is actually close to parkPos
	if (info() != 0)
		return -1;
	if ((abs(altitude-parkPos.alt) > 10) || (abs(azimuth-parkPos.az) > 20))
	{
		//logStream (MESSAGE_ERROR) << "parking failed!" << sendLog;		
		return -1;
	}

	//logStream (MESSAGE_ERROR) << "parking succeeded" << sendLog;		
	
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
}

int PlaneWave::setTracking(int track, bool addTrackingTimer, bool send)
{
	return 0;
}

void PlaneWave::startTracking()
{
	// // activate tracking
	// ret = send_command("mount/tracking_on");
    // usleep(USEC_SEC * 1);
	// info();
	// if (is_tracking != 1)
	// {
	// 	logStream (MESSAGE_ERROR) << "could not activate tracking" <<
	// 		sendLog;	
	// 	return -1;
	// }
	// logStream (MESSAGE_INFO) << "tracking is activated" << sendLog;	


}

void PlaneWave::stopTracking()
{
	// // deactivate tracking
	// ret = send_command("mount/tracking_off");
    // usleep(USEC_SEC * 1);
	// info();
	// if (is_tracking != 0)
	// {
	// 	logStream (MESSAGE_ERROR) << "could not deactivate tracking" <<
	// 		sendLog;	
	// 	return -1;
	// }
	// logStream (MESSAGE_INFO) << "tracking is deactivated" << sendLog;	
}




int main(int argc, char ** argv)
{
	PlaneWave device(argc, argv);
	return device.run();
}
