/* 
 * Sensor for Fine offset weather stations (WH1080, WH1081, W-8681, WH3080, WH3081 etc.)
 * Copyright (C) 2016 Petr Kubanek, Insitute of Physics <kubanek@fzu.cz>
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

#include <libusb-1.0/libusb.h>

#define OPT_WIND_BAD        OPT_LOCAL + 370
#define OPT_PEEKWIND_BAD    OPT_LOCAL + 371
#define OPT_HUM_BAD         OPT_LOCAL + 372

#define FOWS_VENDOR         0x1941
#define FOWS_PRODUCT        0x8021

// Weather Station buffer parameters
#define WS_BUFFER_SIZE		0x10000	// Size of total buffer
#define WS_BUFFER_START		0x100	// First address of up to 4080 buffer records
#define WS_BUFFER_END		0xFFF0	// Start of last buffer record
#define WS_BUFFER_RECORD	0x10	// Size of one buffer record
#define WS_BUFFER_CHUNK		0x20	// Size of chunk received over USB
#define WS_FIXED_BLOCK_START	0x0000	// First address of fixed block
#define WS_FIXED_BLOCK_SIZE	0x0100	// Size of fixed block

// Weather Station buffer memory positions
#define WS_DELAY		 0	// Position of delay parameter
#define WS_HUMIDITY_IN		 1	// Position of inside humidity parameter
#define WS_TEMPERATURE_IN	 2	// Position of inside temperature parameter
#define WS_HUMIDITY_OUT		 4	// Position of outside humidity parameter
#define WS_TEMPERATURE_OUT	 5	// Position of outside temperature parameter
#define WS_ABS_PRESSURE		 7	// Position of absolute pressure parameter
#define WS_WIND_AVE		 9	// Position of wind direction parameter
#define WS_WIND_GUST		10	// Position of wind direction parameter
#define WS_WIND_DIR		12	// Position of wind direction parameter
#define WS_RAIN			13	// Position of rain parameter
#define WS_STATUS		15	// Position of status parameter
#define WS_DATA_COUNT		27	// Position of data_count parameter
#define WS_CURRENT_POS		30	// Position of current_pos parameter
#define WS_CURR_REL_PRESSURE	32	// Position of current relative pressure parameter
#define WS_CURR_ABS_PRESSURE	34	// Position of current absolute pressure parameter

// Calculated rain parameters
// NOTE: These positions are NOT stored in the Weather Station
#define WS_RAIN_HOUR		0x08	// Position of hourly calculated rain
#define WS_RAIN_DAY		0x0A	// Position of daily calculated rain
#define WS_RAIN_WEEK		0x0C	// Position of weekly calculated rain
#define WS_RAIN_MONTH		0x0E	// Position of monthly calculated rain


// The following setting parameters are for reference only
// A future user interface could interpret these parameters
// Unit settings
#define WS_UNIT_SETTING_IN_T_C_F	0x01
#define WS_UNIT_SETTING_OUT_T_C_F	0x02
#define WS_UNIT_SETTING_RAIN_FALL_CM_IN	0x04
#define WS_UNIT_SETTING_PRESSURE_HPA	0x20
#define WS_UNIT_SETTING_PRESSURE_INHG	0x40
#define WS_UNIT_SETTING_PRESSURE_MMHG	0x80
// Unit wind speed settings
#define WS_UNIT_SETTING_WIND_SPEED_MS	0x01
#define WS_UNIT_SETTING_WIND_SPEED_KMH	0x02
#define WS_UNIT_SETTING_WIND_SPEED_KNOT	0x04
#define WS_UNIT_SETTING_WIND_SPEED_MH	0x08
#define WS_UNIT_SETTING_WIND_SPEED_BFT	0x10
// Display format 0
#define WS_DISPLAY_FORMAT_P_ABS_REL	0x01
#define WS_DISPLAY_FORMAT_WSP_AVG_GUST	0x02
#define WS_DISPLAY_FORMAT_H_24_12	0x04
#define WS_DISPLAY_FORMAT_DDMMYY_MMDDYY	0x08
#define WS_DISPLAY_FORMAT_TS_H_12_24	0x10
#define WS_DISPLAY_FORMAT_DATE_COMPLETE	0x20
#define WS_DISPLAY_FORMAT_DATE_AND_WK	0x40
#define WS_DISPLAY_FORMAT_ALARM_TIME	0x80
// Display format 1
#define WS_DISPLAY_FORMAT_OUT_T		0x01
#define WS_DISPLAY_FORMAT_OUT_WINDCHILL	0x02
#define WS_DISPLAY_FORMAT_OUT_DEW_POINT	0x04
#define WS_DISPLAY_FORMAT_RAIN_FALL_1H	0x08
#define WS_DISPLAY_FORMAT_RAIN_FALL_24H	0x10
#define WS_DISPLAY_FORMAT_RAIN_FALL_WK	0x20
#define WS_DISPLAY_FORMAT_RAIN_FALL_MO	0x40
#define WS_DISPLAY_FORMAT_RAIN_FALL_TOT	0x80
// Alarm enable 0
#define WS_ALARM_ENABLE_TIME		0x02
#define WS_ALARM_ENABLE_WIND_DIR	0x04
#define WS_ALARM_ENABLE_IN_RH_LO	0x10
#define WS_ALARM_ENABLE_IN_RH_HI	0x20
#define WS_ALARM_ENABLE_OUT_RH_LO	0x40
#define WS_ALARM_ENABLE_OUT_RH_HI	0x80
// Alarm enable 1
#define WS_ALARM_ENABLE_WSP_AVG		0x01
#define WS_ALARM_ENABLE_WSP_GUST	0x02
#define WS_ALARM_ENABLE_RAIN_FALL_1H	0x04
#define WS_ALARM_ENABLE_RAIN_FALL_24H	0x08
#define WS_ALARM_ENABLE_ABS_P_LO	0x10
#define WS_ALARM_ENABLE_ABS_P_HI	0x20
#define WS_ALARM_ENABLE_REL_P_LO	0x40
#define WS_ALARM_ENABLE_REL_P_HI	0x80
// Alarm enable 2
#define WS_ALARM_ENABLE_IN_T_LO		0x01
#define WS_ALARM_ENABLE_IN_T_HI		0x02
#define WS_ALARM_ENABLE_OUT_T_LO	0x04
#define WS_ALARM_ENABLE_OUT_T_HI	0x08
#define WS_ALARM_ENABLE_WINDCHILL_LO	0x10
#define WS_ALARM_ENABLE_WINDCHILL_HI	0x20
#define WS_ALARM_ENABLE_DEWPOINT_LO	0x40
#define WS_ALARM_ENABLE_DEWPOINT_HI	0x80

namespace rts2sensord
{

// Table for decoding raw weather station data.
// Each key specifies a (pos, type, scale) tuple that is understood by CWS_decode().
// See http://www.jim-easterbrook.me.uk/weather/mm/ for description of data

enum ws_types {ub,sb,us,ss,dt,tt,pb,wa,wg,wd/*wind dir*/};

/**
 * Class for FOWS USB connected weather station.
 *
 * Based on the following C code:
 * https://github.com/ajauberg/fowsr
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class FOWS:public SensorWeather
{
	public:
		FOWS (int argc, char **argv);
		virtual ~FOWS ();

		virtual int info ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual bool isGoodWeather ();
	private:
		struct libusb_device_handle *devh;

		// values with FOWS data
		rts2core::ValueFloat *barometer;
		rts2core::ValueFloat *insideTemp;
		rts2core::ValueFloat *insideHumidity;
		rts2core::ValueFloat *outsideTemp;
		rts2core::ValueFloat *outsideHumidity;
		rts2core::ValueFloat *windSpeed;
		rts2core::ValueFloat *windGust;
		rts2core::ValueInteger *windDirection;
		rts2core::ValueFloat *wind10min;
		rts2core::ValueFloat *wind2min;
		rts2core::ValueFloat *wind10mingust;
		rts2core::ValueInteger *wind10mingustDirection;
		rts2core::ValueFloat *dewPoint;
		rts2core::ValueFloat *rainRate;
		rts2core::ValueInteger *uvIndex;
		rts2core::ValueInteger *stormRain;

		rts2core::ValueInteger *maxWindSpeed;
                rts2core::ValueInteger *maxPeekWindSpeed;
                rts2core::ValueInteger *maxHumidity;
		
		/**
		 * Check good/bad weather alarms
		 */
		void checkTriggers ();

		unsigned char m_buf[WS_BUFFER_SIZE];	// Raw WS data
		time_t m_timestamp;

		short USBReadBlock (unsigned short ptr, unsigned char* buf);
		void USBClose ();

		short dataHasChanged (unsigned char OldBuf[], unsigned char NewBuf[], size_t size);
		short readFixedBlock ();

		double decode (unsigned char* raw, ws_types ws_type, float scale);

		int CWS_Read ();
};

}


using namespace rts2sensord;

FOWS::FOWS (int argc, char **argv):SensorWeather (argc, argv)
{
	memset (m_buf, 0xFF, sizeof (m_buf));

	maxHumidity = NULL;
	maxWindSpeed = NULL;
	maxPeekWindSpeed = NULL;

	createValue (barometer, "PRESSURE", "[hPa] barometer reading", false);
	createValue (insideTemp, "TEMP_IN", "[C] inside temperature", false);
	createValue (insideHumidity, "HUM_IN", "[%] inside humidity", false);
	createValue (outsideTemp, "TEMP_OUT", "[C] outside temperature", false);
	createValue (outsideHumidity, "HUM_OUT", "[%] outside humidity", false);
	createValue (windSpeed, "WIND", "[m/s] wind average speed", false);
	createValue (windGust, "WIND_GUST", "[m/s] wind gust", false);
	createValue (windDirection, "WIND_DIR", "wind direction", false, RTS2_DT_DEGREES);
	createValue (rainRate, "RAINRT", "[mm/hour] rain rate", false);
	createValue (uvIndex, "UVINDEX", "UV index", false);
	createValue (stormRain, "RAINST", "[mm] storm rain", false);

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSBn)");
	addOption (OPT_WIND_BAD, "wind-bad", 1, "wind speed bad trigger point");
        addOption (OPT_PEEKWIND_BAD, "peekwind-bad", 1, "peek wind speed bad trigger point");
        addOption (OPT_HUM_BAD, "hum-bad", 1, "humidity bad trigger point");

}

FOWS::~FOWS ()
{
	USBClose ();
}

unsigned short CWS_unsigned_short (unsigned char* raw)
{
 	return ((unsigned short)raw[1] << 8) | raw[0];
}

signed short CWS_signed_short (unsigned char* raw)
{
 	unsigned short us = ((((unsigned short)raw[1])&0x7F) << 8) | raw[0];
	
	if(raw[1]&0x80)	// Test for sign bit
		return -us;	// Negative value
	else
		return us;	// Positive value
}

int FOWS::info ()
{
	CWS_Read ();

	//time_t timestamp = m_timestamp - m_timestamp % 60; // Set to current minute
	unsigned short current_pos = CWS_unsigned_short (&m_buf[WS_CURRENT_POS]);

	insideHumidity->setValueFloat (decode (m_buf + current_pos + WS_HUMIDITY_IN, ub, 1.0));
	insideTemp->setValueFloat (decode (m_buf + current_pos + WS_TEMPERATURE_IN, ss, 0.1));
	outsideHumidity->setValueFloat (decode (m_buf + current_pos + WS_HUMIDITY_OUT, ub, 1.0));
	outsideTemp->setValueFloat (decode (m_buf + current_pos + WS_TEMPERATURE_OUT, ss, 0.1));
	barometer->setValueFloat (decode (m_buf + current_pos + WS_ABS_PRESSURE, us, 0.1));
	windSpeed->setValueFloat (decode (m_buf + current_pos + WS_WIND_AVE, wa, 0.1));
	windGust->setValueFloat (decode (m_buf + current_pos + WS_WIND_GUST, wg, 0.1));
	windDirection->setValueInteger (decode (m_buf + current_pos + WS_WIND_DIR, wd, 22.5));

        // do not send infotime..
	return 0;
}

int FOWS::processOption (int opt)
{
	switch (opt)
	{
		case OPT_WIND_BAD:
			createValue (maxWindSpeed, "MAX_WIND_TRIGGER", "[m/s] maximum wind speed", false);
			maxWindSpeed->setValueInteger (atoi(optarg));
			logStream (MESSAGE_INFO) << "DAVIS max wind speed:" << maxWindSpeed << sendLog;
                        break;
                case OPT_PEEKWIND_BAD:
			createValue (maxPeekWindSpeed, "MAX_PEEKWIND_TRIGGER", "[m/s] maximum peek wind speed", false);
			maxPeekWindSpeed->setValueInteger (atoi(optarg));
			logStream (MESSAGE_INFO) << "DAVIS max peek wind speed:" << maxPeekWindSpeed << sendLog;
                        break;
                case OPT_HUM_BAD:
			createValue (maxHumidity, "MAX_HUM_TRIGGER", "[%] maximum humidity", false);
                        maxHumidity->setValueInteger (atoi(optarg));
			logStream (MESSAGE_INFO) << "DAVIS max humidity:" << maxHumidity << sendLog;
                        break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}

int FOWS::initHardware ()
{
	libusb_init (NULL);
	if (getDebug ())
		libusb_set_debug (NULL, LIBUSB_LOG_LEVEL_DEBUG);

	devh = libusb_open_device_with_vid_pid (NULL, FOWS_VENDOR, FOWS_PRODUCT);
	if (devh == NULL)
	{
		logStream (MESSAGE_ERROR) << "cannot find any device with VID 0x" << std::hex << FOWS_VENDOR << " and PID 0x" << std::hex << FOWS_PRODUCT << sendLog;
		return -1;
	}

	int ret = libusb_set_auto_detach_kernel_driver (devh, 1);
	if (ret < 0)
	{
		logStream (MESSAGE_WARNING) << "cannot set auto detach kernel driver " << ret << sendLog;
	}

	ret = libusb_claim_interface (devh, 0);
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "claim failed with error " << ret << sendLog;
		return -1;
	}

	unsigned char buf[1000];
	memset (buf, '\0', sizeof(buf));

	ret = libusb_get_descriptor(devh, LIBUSB_DT_DEVICE, 0, buf, 0x09);
	ret = libusb_get_descriptor(devh, LIBUSB_DT_CONFIG, 0, buf, 0x09);
	ret = libusb_get_descriptor(devh, LIBUSB_DT_CONFIG, 0, buf, 0x22);
	ret = libusb_release_interface(devh, 0);
	if (ret)
		logStream (MESSAGE_WARNING) << "failed to release interface before set_configuration: " << ret << sendLog;

	ret = libusb_set_configuration(devh, 1);
	ret = libusb_claim_interface(devh, 0);
	if (ret)
		logStream (MESSAGE_WARNING) << "claim after set_configuration failed with error " << ret << sendLog;
	//ret = libusb_set_altinterface(devh, 0);
	ret = libusb_control_transfer(devh, LIBUSB_REQUEST_TYPE_CLASS + LIBUSB_RECIPIENT_INTERFACE, 0xa, 0, 0, buf, 0, 1000);
	ret = libusb_get_descriptor(devh, LIBUSB_DT_REPORT, 0, buf, 0x74);

	return 0;
}

bool FOWS::isGoodWeather ()
{
	return SensorWeather::isGoodWeather ();
}

void FOWS::checkTriggers ()
{
        if (maxHumidity && outsideHumidity->getValueFloat () >= maxHumidity->getValueInteger ())
        {
         	setWeatherTimeout (600, "humidity has crossed alarm limits");
                valueError (outsideHumidity);
        }
        else
	{
        	valueGood (outsideHumidity);
	}
	
	if (maxWindSpeed && wind2min->getValueFloat () >= maxWindSpeed->getValueInteger ())
        {
		setWeatherTimeout (300, "high wind");
		valueError (windSpeed);
	}
	else
	{
		valueGood (windSpeed);
        }

	if (maxPeekWindSpeed && wind10mingust->getValueFloat () >= maxPeekWindSpeed->getValueInteger ())
        {
		setWeatherTimeout (300, "high peek wind");
		valueError (wind10mingust);
	}
	else
	{
		valueGood (wind10mingust);
        }
	
	if (rainRate->getValueFloat () > 0)
	{
        	setWeatherTimeout (1200, "raining (rainrate)");
                valueError (rainRate);
	}
        else
	{
        	valueGood (rainRate);
	}

}

short FOWS::USBReadBlock (unsigned short ptr, unsigned char* buf)
{
/*
Read 32 bytes data command	

After sending the read command, the device will send back 32 bytes data wihtin 100ms. 
If not, then it means the command has not been received correctly.
*/
	char buf_1 = (char)(ptr / 256);
	char buf_2 = (char)(ptr & 0xFF);
	unsigned char tbuf[8];
	tbuf[0] = 0xA1;		// READ COMMAND
	tbuf[1] = buf_1;	// READ ADDRESS HIGH
	tbuf[2] = buf_2;	// READ ADDRESS LOW
	tbuf[3] = 0x20;		// END MARK
	tbuf[4] = 0xA1;		// READ COMMAND
	tbuf[5] = 0;		// READ ADDRESS HIGH
	tbuf[6] = 0;		// READ ADDRESS LOW
	tbuf[7] = 0x20;		// END MARK

	// Prepare read of 32-byte chunk from position ptr
	int ret = libusb_control_transfer (devh, LIBUSB_REQUEST_TYPE_CLASS + LIBUSB_RECIPIENT_INTERFACE, 9, 0x200, 0, tbuf, 8, 1000);
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "usb_control_msg failed (" << ret << ") whithin readBblock: " << std::hex << ptr << sendLog;
	}
	else
	{
		int transferred = 0;
		// Read 32-byte chunk and place in buffer buf
		ret = libusb_interrupt_transfer (devh, 0x81, buf, 0x20, &transferred, 1000);
		if (ret < 0)
			logStream (MESSAGE_ERROR) << "usb_interrupt_read failed (" << ret << ") whithin readBlock: " << std::hex << ptr << sendLog;
	}
	return ret;
}

void FOWS::USBClose ()
{
	int ret = libusb_release_interface (devh, 0);
	logStream (MESSAGE_DEBUG) << "usb_release_interface ret:" << ret << sendLog;
	libusb_close (devh);
}

short FOWS::dataHasChanged (unsigned char OldBuf[], unsigned char NewBuf[], size_t size)
{	// copies size bytes from NewBuf to OldBuf, if changed
	// returns 0 if nothing changed, otherwise 1
	short NewDataFlg = 0;

	for (size_t i = 0;i < size;i++)
	{
		if (OldBuf[i] != NewBuf[i])
		{
			NewDataFlg = 1;
			OldBuf[i] = NewBuf[i];
		}
	}
	return NewDataFlg;
}

short FOWS::readFixedBlock ()
{
	// Read fixed block in 32 byte chunks
	unsigned short i;
	unsigned char fb_buf[WS_FIXED_BLOCK_SIZE];
	char NewDataFlg = 0;

	for (i = WS_FIXED_BLOCK_START;i < WS_FIXED_BLOCK_SIZE;i += WS_BUFFER_CHUNK)
		if (USBReadBlock (i, &fb_buf[i]) < 0)
			return 0; //failure while reading data
	// Check for new data
	memcpy (&m_buf[WS_FIXED_BLOCK_START], fb_buf, 0x10); //disables change detection on the rain val positions 
	NewDataFlg = dataHasChanged (&m_buf[WS_FIXED_BLOCK_START], fb_buf, sizeof(fb_buf));
	// Check for valid data
	if (((m_buf[0]==0x55) && (m_buf[1]==0xAA)) || ((m_buf[0]==0xFF) && (m_buf[1]==0xFF)))
		return NewDataFlg;
	
	logStream (MESSAGE_ERROR) << "fixed block is not valid" << sendLog;
	return -1;
}

unsigned char CWS_bcd_decode (unsigned char byte)
{
        unsigned char lo = byte & 0x0F;
        unsigned char hi = byte / 16;
        return (lo + (hi * 10));
}

double FOWS::decode (unsigned char* raw, ws_types ws_type, float scale)
{
	double res;
	int m = 0;

	switch (ws_type)
	{
		case ub:
			m = 1;
			res = raw[0] * scale;
			break;
		case sb:
			m = 1;
			res = raw[0] & 0x7F;
			if (raw[0] & 0x80)	// Test for sign bit
				res -= res;	//negative value
			res *= scale;
			break;
		case us:
			m = 2;
			res = CWS_unsigned_short(raw) * scale;
			break;
		case ss:
			m = 2;
			res = CWS_signed_short(raw) * scale;
			break;
		case dt:
			{
/*				unsigned char year, month, day, hour, minute;
				year   = CWS_bcd_decode(raw[0]);
				month  = CWS_bcd_decode(raw[1]);
				day    = CWS_bcd_decode(raw[2]);
				hour   = CWS_bcd_decode(raw[3]);
				minute = CWS_bcd_decode(raw[4]); */
				m = 5;
				//n = sprintf(result,"%4d-%02d-%02d %02d:%02d", year + 2000, month, day, hour, minute);
			}
			break;
		case tt:
			m = 2;
			//n = sprintf(result,"%02d:%02d", CWS_bcd_decode(raw[0]), CWS_bcd_decode(raw[1]));
			break;
		case pb:
			m = 1;
			//n = sprintf(result,"%02x", raw[0]);
			break;
		case wa:
			m = 3;
			// wind average - 12 bits split across a byte and a nibble
			res = raw[0] + ((raw[2] & 0x0F) * 256);
			res *= scale;
			break;
		case wg:
			m = 3;
			// wind gust - 12 bits split across a byte and a nibble
			res = raw[0] + ((raw[1] & 0xF0) * 16);
			res *= scale;
			break;
		case wd:
			m = raw[0] & 0xF0 ? 0 : -1;
			res = raw[0] * scale;
			break;
		default:
			logStream (MESSAGE_ERROR) << "decode: Unknown type " << ws_type << sendLog;
			return NAN;
	}

	if (m < 0)
		return res;

	for (int i=0;i < m;i++)
	{
		if (raw[i] != 0xFF)
			return res;
	}

	return NAN;
}

unsigned short CWS_dec_ptr(unsigned short ptr)
{
	// Step backwards through buffer.
	ptr -= WS_BUFFER_RECORD;             
	if (ptr < WS_BUFFER_START)
		// Start is reached, step to end of buffer.
		ptr = WS_BUFFER_END;
	return ptr;
}

unsigned short CWS_inc_ptr(unsigned short ptr)
{
	// Step forward through buffer.
	ptr += WS_BUFFER_RECORD;             
	if ((ptr > WS_BUFFER_END) || (ptr < WS_BUFFER_START))
		// End is reached, step to start of buffer.
		ptr = WS_BUFFER_START;
	return ptr;
}

int FOWS::CWS_Read ()
{
// Read fixed block
// - Get current_pos
// - Get data_count
// Read records backwards from current_pos untill previous current_pos reached
// Step 0x10 in the range 0x10000 to 0x100, wrap at 0x100
// USB is read in 0x20 byte chunks, so read at even positions only
// return 1 if new data, otherwise 0
	m_timestamp = time(NULL);	// Set to current time
	int old_pos = CWS_unsigned_short(&m_buf[WS_CURRENT_POS]);

	int n, NewDataFlg = readFixedBlock ();
	unsigned char DataBuf[WS_BUFFER_CHUNK];

	unsigned short data_count = CWS_unsigned_short(&m_buf[WS_DATA_COUNT]);
	unsigned short current_pos = CWS_unsigned_short(&m_buf[WS_CURRENT_POS]);
	unsigned short i;

	if ((current_pos % WS_BUFFER_RECORD) || (current_pos < WS_BUFFER_START))
	{
		logStream (MESSAGE_ERROR) << "CWS_Read: wrong current_pos=0x" << std::hex << current_pos << sendLog;
		return -1;
	}

	for (i=0; i<data_count; )
	{
		if (!(current_pos & WS_BUFFER_RECORD))
		{
			// Read 2 records on even position
			n = USBReadBlock (current_pos, DataBuf);
			if(n<32)
				exit(1);
			i += 2;
			NewDataFlg |= dataHasChanged (&m_buf[current_pos], DataBuf, sizeof(DataBuf));
		}
		if (current_pos == (old_pos & (~WS_BUFFER_RECORD)))
			break;	//break only on even position
		current_pos = CWS_dec_ptr (current_pos);
	}
	if ((old_pos==0) || (old_pos==0xFFFF))	//cachefile empty or empty eeprom was read
		old_pos = CWS_inc_ptr (current_pos);

	return NewDataFlg;
}

int main (int argc, char **argv)
{
	FOWS device (argc, argv);
	return device.run ();
}
