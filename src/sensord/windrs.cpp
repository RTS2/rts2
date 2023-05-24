/*
 * Sensor for Papouch WindRS wind sensor, derived from Davis driver
 * Copyright (C) 2019 Ronan Cunnife <ronan@cunniffe.net>
 * Copyright (C) 2015 Petr Kubanek (petr@kubanek.net>
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
#include "connection/serial.h"

#define LUDICROUS_SPEED 		20		// default value (m/s) for speed limits
#define WINDRS_WEATHER_TIMEOUT	300		// seconds a "high wind" warning will be kept alive

#define EVENT_LOOP          RTS2_LOCAL_EVENT + 1701
#define OPT_LIMIT_AVG       OPT_LOCAL + 370		// FIXME values
#define OPT_LIMIT_PEAK		OPT_LOCAL + 371


// WindRS Spinel command codes
#define WINDRS_GET_AVG		0x51
#define WINDRS_SET_AVG_SETTINGS		0x52
#define WINDRS_GET_AVG_SETTINGS 0x53

// Spinel ACK codes
#define SPINEL_ACK_OK		0x00
#define SPINEL_ACK_ERROR	0x01	// Unspecified error
#define SPINEL_ACK_BAD_INST	0x02	// Unknown instruction code
#define SPINEL_ACK_BAD_DATA	0x03	// Length or contents of data are wrong
#define SPINEL_ACK_DENIED	0x04	// Write access denied (see manual)
#define SPINEL_ACK_DEV_FAIL	0x05	// Yep, internal device failure.
#define SPINEL_ACK_NO_DATA	0x06	// No data available


/**********
 * Spinel protocol stuff
 */

#define SPINEL_MAX_DATA_BYTES	32
struct spinel_packet
{
		unsigned char header[7];
		short data_bytes;
		unsigned char data[SPINEL_MAX_DATA_BYTES];
		unsigned char footer[2];
};


unsigned char spinel_checksum(struct spinel_packet *p)
{
		unsigned char sum = 0xff;
		for (int i=0; i<7; i++) {
				sum -= p->header[i];
		}
		for (int i=0; i<p->data_bytes; i++) {
				sum -= p->data[i];
		}
		return sum;
}


void spinel_make_packet(struct spinel_packet *p, unsigned char addr, unsigned char slot, unsigned char cmd,		unsigned char *data, unsigned char data_bytes)
{
	if (data_bytes>SPINEL_MAX_DATA_BYTES)
	{
			logStream (MESSAGE_ERROR) << "truncating overlong message (" << data_bytes << " bytes) to " << SPINEL_MAX_DATA_BYTES << " bytes." << sendLog;
			p->data_bytes = SPINEL_MAX_DATA_BYTES;
	} else {
			p->data_bytes = data_bytes;
	}
	memcpy(&p->data, data, p->data_bytes);
	p->data_bytes=data_bytes;
	// *FIXME* error

	p->header[0] = '*';
	p->header[1] = 0x61;
	p->header[2] = (data_bytes + 5) >> 8;
	p->header[3] = (data_bytes + 5) % 0xff;
	p->header[4] = addr;
	p->header[5] = slot;
	p->header[6] = cmd;
	p->footer[0] = spinel_checksum(p);
	p->footer[1] = 0x0d;
}


namespace rts2sensord
{

/**
 * Class for Papouch WindRS wind sensor
 *
 * @author Ronan Cunniffe <ronan@cunniffe.net>
 */

class WindRS:public SensorWeather
{
	public:
		WindRS (int argc, char **argv);
		virtual ~WindRS ();

		virtual void getData ();

		virtual int info ();

		virtual void postEvent (rts2core::Event *event);

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual bool isGoodWeather ();

	private:
		int openConnection();
		void closeConnection();
		char *devicename;
		rts2core::ConnSerial *WindRSConn;

		rts2core::ValueFloat *windAvg;
		rts2core::ValueFloat *windDir;

		rts2core::ValueFloat *windLimitAvg;
		rts2core::ValueFloat *windTimeout;
};

}


using namespace rts2sensord;

WindRS::WindRS (int argc, char **argv):SensorWeather (argc, argv)
{
	devicename = NULL;
	WindRSConn = NULL;

	windLimitAvg = NULL;

	createValue (windAvg, "WIND_AVG", "[m/s] average wind speed over last N minutes", false);
	createValue (windDir, "WIND_DIR", "wind direction (deg)", false, RTS2_DT_DEGREES);
	createValue (windLimitAvg, "AVG_LIMIT", "[m/s]maximum safe average windspeed", false, RTS2_VALUE_WRITABLE);
	windLimitAvg->setValueFloat (LUDICROUS_SPEED);

	createValue (windTimeout, "timeout", "timeout for bad weather", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	windTimeout->setValueFloat (300);

	addOption ('f', NULL, 1, "serial port (usually /dev/ttyUSBx)");
	addOption (OPT_LIMIT_AVG, "avg-limit", 1, "limit (m/s) for average wind.");
//	addOption (OPT_AVG_SECS, "avg-secs", 1, "duration (minutes) for averaging.");

}

WindRS::~WindRS ()
{
	delete WindRSConn;
}


int WindRS::info ()
{
    // do not send infotime..
	return 0;
}

void WindRS::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_LOOP:
			getData();
			addTimer(10, event);
			return;
	}
	SensorWeather::postEvent (event);
}


void WindRS::getData()
{
	// *NOTE* on comms failure, this code leaves the previous stale values intact.
	struct spinel_packet tx, rx;

	unsigned char d = 0x00;

	spinel_make_packet(&tx, 0xfe, 0x00, WINDRS_GET_AVG, &d, 1);

	if (WindRSConn == NULL) {
		if (openConnection()!=0)
		{
			return;
		}
	}
	WindRSConn->flushPortIO();
	if ((WindRSConn->writePort((char *)tx.header, 7)!=0) ||
			(WindRSConn->writePort((char *)tx.data, tx.data_bytes)!=0) ||
			(WindRSConn->writePort((char *)tx.footer, 2)!=0))
	{
		closeConnection();
		return;
	}

	usleep(100000);

	if (WindRSConn->readPort((char *)rx.header, 7)!=7)
	{
		closeConnection();
		return;
	}
	rx.data_bytes = (rx.header[2]<<8) + rx.header[3] - 5;

	if ((WindRSConn->readPort((char *)rx.data, rx.data_bytes)!=rx.data_bytes) ||
			(WindRSConn->readPort((char *)rx.footer, 2)!=2))
	{
		closeConnection();
		return;
	}

	if (rx.footer[0] != spinel_checksum(&rx)) {
		logStream (MESSAGE_ERROR) << "Bad checksum in reply." << sendLog;
		return;
	}

	if (rx.data_bytes<8) {
		logStream (MESSAGE_ERROR) << "Short reply, expected 8 bytes, got " << rx.data_bytes << sendLog;
		return;
	}

	if ((rx.data[0] != 0x01) || (rx.data[2] != 0) || (rx.data[4] != 0x02)) {
		logStream (MESSAGE_ERROR) << "Corrupted reply.\n" << sendLog;
		return;
	}

	if (rx.data[1] != 0x80) {
		logStream (MESSAGE_WARNING) << "Invalid wind direction reading." << sendLog;
		windDir->setValueFloat(NAN);
	} else {
		windDir->setValueFloat(rx.data[3] * 22.5);
	}

	// last two bytes are 0-512, indicating 0-51.2 m/s
	if (rx.data[5] != 0x80) {
		logStream (MESSAGE_WARNING) << "Invalid wind speed measurement." << sendLog;
		windAvg->setValueFloat(NAN);
	} else {
		windAvg->setValueFloat(0.1 * ((rx.data[6] << 8) + rx.data[7]));

		// if (windAvg->getValueFloat () > windLimitAvg->getValueFloat ())
		// 	setWeatherState (false, "Wind speed above limit");
		// else
		// 	setWeatherState (true, "Wind speed below limit");
	}
	sendValueAll(windDir);
	sendValueAll(windAvg);
}

int WindRS::processOption (int opt)
{
	logStream (MESSAGE_INFO) << "************* WindRS::processOption ************" <<  sendLog;
	switch (opt)
	{
		case 'f':
			devicename = optarg;
			break;
		case OPT_LIMIT_AVG:
			windLimitAvg->setValueFloat (atof(optarg));
			logStream (MESSAGE_INFO) << "WindRS average speed limit:" << windLimitAvg << sendLog;
            break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}


int WindRS::openConnection()
{
	WindRSConn = new rts2core::ConnSerial (devicename, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 30);
	int ret = WindRSConn->init ();
	if (ret) {
		logStream (MESSAGE_ERROR) << "Failed to open " << devicename << sendLog;
		WindRSConn = NULL;
		return ret;
	}
	WindRSConn->setDebug (getDebug ());
	return 0;
}

void WindRS::closeConnection()
{
	delete WindRSConn;
	WindRSConn = NULL;
}

int WindRS::initHardware ()
{
	if (devicename == NULL)
	{
		logStream (MESSAGE_ERROR) << "No device specified.  Expected '-f /dev/tty<something>'. Quitting." << sendLog;
		return -1;
	}
	int ret = openConnection();
	if (ret)
		return ret;
	addTimer(1, new rts2core::Event (EVENT_LOOP, this));
	return 0;
}

bool WindRS::isGoodWeather ()
{
	if (windAvg->getValueFloat () > windLimitAvg->getValueFloat ())
	{
		setWeatherTimeout (windTimeout->getValueDouble (), "Wind speed above limit");
		return false;
	}
	return SensorWeather::isGoodWeather ();
}

int main (int argc, char **argv)
{
	WindRS device (argc, argv);
	return device.run ();
}
