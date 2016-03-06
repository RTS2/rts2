/*
 * Driver for Spectral Instruments SI8821 PCI board CCD
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
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

#include "camd.h"
#include "utilsfunc.h"

#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h> // APMONTERO: new

#include "si8821/si8821.h"
#include "si8821/si_app.h"

namespace rts2camd
{
/**
 * Driver for SI 8821 CCD.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SI8821:public Camera
{
	public:
		SI8821 (int argc, char **argv);
		virtual ~SI8821 ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual void initBinnings ()
		{
			Camera::initBinnings ();

			addBinning2D (2, 2);
			addBinning2D (3, 3);
			addBinning2D (4, 4);
			addBinning2D (8, 8);
		}

		virtual int info ();
		
		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int startExposure ();
		virtual int endExposure (int ret);

		virtual int closeShutter ();

		virtual int stopExposure ();

		virtual int doReadout ();

		virtual int setCoolTemp (float new_temp);
		virtual int switchCooling (bool newval);

	private:
		const char *devicePath;
		const char *cameraCfg;
		const char *cameraSet;

		struct SI_CAMERA camera;

		int verbose;

		int init_com (int baud, int parity, int bits, int stopbits, int buffersize);

		int load_cfg (struct CFG_ENTRY **e, const char *fname, const char *var);
		int load_camera_cfg (struct SI_CAMERA *c, const char *fname);
		int parse_cfg_string (struct CFG_ENTRY *entry );
		int getReadoutFromFile (bool bReadFormat, int iMode = 0);
		int setfile_readout (const char *file, bool bReadFormat, int iMode = 0);
		int setfile_config (const char *file );  
		int receive_n_ints (int n, int *data);
		int send_n_ints (int n, int *data); 
		struct CFG_ENTRY *find_readout (char *name);
		struct CFG_ENTRY *find_config (char *name);
		void send_readout ();
		void send_config ();

		void write_dma_data (unsigned char *ptr);
		void dma_unmap ();

		int clear_buffer ();

		int send_command (int cmd);

		/**
		 * @return -1 error, 0 no, 1 yes
		 */
		int send_command_yn (int cmd);

		int send_char (int data);
		int receive_char ();
	
		int expect_yn ();
		void expect_y ();

		int swapl (int *d);

		void setReadout (int idx, int v, const char *name);
		void setConfig (int idx, int v, const char *name);
		
		float getGain( int iMode );

		void print_readout ();
		void print_config ();
		void print_cfg (struct CFG_ENTRY *cfg, int val);

		rts2core::ValueFloat *backplateTemperature;
		rts2core::ValueInteger *ccdPressure;

		rts2core::ValueSelection *shutterStatus;
		
		rts2core::ValueSelection *readOutModes; // Read Out Modes from cam set file - Mode 1, Mode 2, ...
		rts2core::ValueFloat *gain; // Gain in electrons / DN

		rts2core::ValueBool *testImage;
		int total;   // DMA size

		// buffer for reconstructing image
		uint16_t *chanBuffer;
};

}

using namespace rts2camd;

SI8821::SI8821 (int argc, char **argv):Camera (argc, argv, FLOOR)
{
	devicePath = "/dev/SIPCI_0";
	cameraCfg = NULL;
	cameraSet = NULL;

	chanBuffer = NULL;

	createTempCCD ();
	createTempSet ();
	createExpType ();

	createDataChannels ();
	setNumChannels (2);

	createValue (backplateTemperature, "backplate_temp", "[C] backplate temperature", false);
	createValue (ccdPressure, "pressure", "[mTor] pressure inside CCD chamber", false);

	createValue (shutterStatus, "shutter_status", "shutter status (opened/closed)", false);
	shutterStatus->addSelVal ("closed");
	shutterStatus->addSelVal ("opened");

	bzero (&camera, sizeof(struct SI_CAMERA));

	addOption ('f', NULL, 1, "SI device path; default to /dev/SIPCI_0");
	addOption ('c', NULL, 1, "SI camera configuration file");
	addOption ('s', NULL, 1, "SI camera configuration set (values) file");

	createValue (testImage, "test_image", "if true, make a test image", true, RTS2_VALUE_WRITABLE);
	testImage->setValueBool (false);

	createValue (readOutModes, "READOUT_MODES", "ReadOut Camera Selectable Modes", false, RTS2_VALUE_WRITABLE);
	createValue (gain, "GAIN", "Gain in electrons / DN", false, RTS2_VALUE_WRITABLE);

}

SI8821::~SI8821 ()
{
        if (camera.ptr)
                dma_unmap ();

	if (camera.fd)
		close (camera.fd);
	delete []chanBuffer;
}

int SI8821::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			devicePath = optarg;
			break;
		case 'c':
			cameraCfg = optarg;
			break;
		case 's':
			cameraSet = optarg;
			break;
		default:
			return Camera::processOption (opt);
	}
	return 0;
}

int SI8821::initHardware ()
{
	if (cameraCfg == NULL)
	{
		logStream (MESSAGE_ERROR) << "missing -c parameter for SI configuration" << sendLog;
		return -1;
	}

	if (cameraSet == NULL)
	{
		logStream (MESSAGE_ERROR) << "missing -s parameter for SI configuration set" << sendLog;
		return -1;
	}

	camera.fd = open (devicePath, O_RDWR, 0);
	if (camera.fd < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot open " << devicePath << ":" << strerror (errno) << sendLog;
		return -1;
	}

	verbose = 1;
	if (ioctl (camera.fd, SI_IOCTL_VERBOSE, &verbose) < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot set camera to verbose mode" << sendLog;
		return -1;
	}

	if (init_com (250000, 0, 8, 1, 9000)) /* setup uart for camera */
	{
		logStream (MESSAGE_ERROR) << "cannot init uart for camera: " << strerror << sendLog;
		return -1;
	}

	if (load_camera_cfg (&camera, cameraCfg))
	{
		logStream (MESSAGE_ERROR) << "cannot load camera configuration file " << cameraCfg << sendLog;
		return -1;
	}
	logStream (MESSAGE_INFO) << "loaded configuration file " << cameraCfg << sendLog;


	// Loads [Readout & Format] parameters from file	
	if (getReadoutFromFile (true))
	{
		logStream (MESSAGE_ERROR) << "cannot load Readout & Format from file "  << cameraCfg << sendLog;
		return -1;
	}
	// Loads [Readout Mode X] parameters from file
	//getReadoutFromFile (false, 0);
        readOutModes->addSelVal ("Mode 0");
	//getReadoutFromFile (false, 1);
        readOutModes->addSelVal ("Mode 1");
	//getReadoutFromFile (false, 2);
        readOutModes->addSelVal ("Mode 2");
	//getReadoutFromFile (false, 3);
        readOutModes->addSelVal ("Mode 3");
	//getReadoutFromFile (false, 4);
        readOutModes->addSelVal ("Mode 4");
	//getReadoutFromFile (false, 5);
        readOutModes->addSelVal ("Mode 5");
	//getReadoutFromFile (false, 6);
        readOutModes->addSelVal ("Mode 6");

	// Load config parameters from file 
	setfile_config (cameraSet);
	send_config ();
	send_command ('L'); // J    - load config
	//print_config (); // APMONTERO: FOR TESTING
	receive_n_ints (32, (int *) &camera.config );
	if( expect_yn () != 1 )
	{
	   	logStream (MESSAGE_ERROR) << "cannot read configuration parameters from camera" << sendLog;
		return -1;
	}
	else
	{
		logStream (MESSAGE_INFO) << "configuration parameters have been read succesfully from camera" << sendLog;
		//print_config (); // APMONTERO: FOR TESTING
	}

	chanBuffer = new uint16_t[(camera.readout[READOUT_SERIAL_ORIGIN_IX] + camera.readout[READOUT_SERIAL_LENGTH_IX]) * (camera.readout[READOUT_PARALLEL_ORIGIN_IX] + camera.readout[READOUT_PARALLEL_LENGTH_IX])];

	setSize (camera.readout[READOUT_SERIAL_LENGTH_IX], camera.readout[READOUT_PARALLEL_LENGTH_IX], camera.readout[READOUT_SERIAL_ORIGIN_IX], camera.readout[READOUT_PARALLEL_ORIGIN_IX]);

	//printf("set_temp: %f\n",kelvinToCelsius(camera.config[CONFIG_SET_TEMP_IX]/10.0));
	//printf("temp: %u\n",(unsigned)camera.config[CONFIG_SET_TEMP_IX]);
	setCoolTemp( kelvinToCelsius (camera.config[CONFIG_SET_TEMP_IX] / 10.0));
	//switchCooling (true);
	//instrumentSerialNumber->setValueString(std::to_string(camera.config[CONFIG_INSTRUMENT_SN_IX]));  
	serialNumber->setValueInteger(camera.config[CONFIG_INSTRUMENT_SN_IX]);  

	camera.dma_config.maxever = 16 * getWidth () * getHeight ();

	camera.ptr = (unsigned short *) mmap (0, camera.dma_config.maxever, PROT_READ, MAP_SHARED, camera.fd, 0);
	if(!camera.ptr)
	{
        	logStream (MESSAGE_ERROR) << "mmap " << errno << " " << strerror (errno) << sendLog;
	        return -1;
	}

	return 0;
}

int SI8821::info ()
{
	send_command ('I');
	receive_n_ints (16, camera.status);
	if (expect_yn () != 1)
	{
		logStream (MESSAGE_WARNING) << "cannot read camera status" << sendLog;
		return -1;
	}

	tempCCD->setValueFloat (kelvinToCelsius (camera.status[STATUS_CCD_TEMP_IX] / 10.0));
	backplateTemperature->setValueFloat (kelvinToCelsius (camera.status[STATUS_BACKPLATE_TEMP_IX] / 10.0));
	ccdPressure->setValueInteger (camera.status[STATUS_PRESSURE_IX]);

	shutterStatus->setValueInteger (camera.status[STATUS_SHUTTER_IX]);

	coolingOnOff->setValueBool (camera.status[STATUS_COOLER_IX] == 1);

	send_command ('L'); // L     - load config
	receive_n_ints (32, (int *) &camera.config);
	if (expect_yn () != 1)
	{
		logStream (MESSAGE_WARNING) << "cannot read camera config" << sendLog;
		return -1;
	}

	tempSet->setValueDouble (kelvinToCelsius (camera.config[CONFIG_SET_TEMP_IX] / 10.0));
	gain->setValueFloat(camera.readout[READOUT_GAIN_IX] * 0.01); // APMONTERO: TESTING
	return Camera::info ();
}

int SI8821::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == readOutModes)
        {
		int iMode = ((rts2core::ValueInteger *) newValue)->getValueInteger ();
		if (getReadoutFromFile (false, iMode) != 0) 
		{
			logStream (MESSAGE_WARNING) << "cannot read camera Readout Mode " << iMode << sendLog;
		}
		else
		{
			logStream (MESSAGE_INFO) << "[Readout Mode " << iMode << "] "
			<< " DSI Sample Time : " << camera.readout[READOUT_DSI_SAMPLE_IX]
			<< " | Analog Attenuation : " << camera.readout[READOUT_AATTENUATION_IX]
			<< " | Port 1 Offset : " << (camera.readout[READOUT_PORT1_OFFSET_IX] - pow(2,24))
			<< " | Port 2 Offset : "  << (camera.readout[READOUT_PORT2_OFFSET_IX] - pow(2,24)) << sendLog;
			gain->setValueFloat(camera.readout[READOUT_GAIN_IX] * 0.01); // APMONTERO: TESTING
			sendValueAll(gain);
		}
        }
	return Camera::setValue (oldValue, newValue);
}

int SI8821::startExposure ()
{
	try
	{
		// exposure time in ms
		int exptime_ms = getExposure () * 1000;

		if (camera.readout[READOUT_EXPOSURE_IX] != exptime_ms)
		{
			// set exposure time
			setReadout (READOUT_EXPOSURE_IX, exptime_ms, "exposure time");
		}

		// setup window - ROI
		if (camera.readout[READOUT_SERIAL_ORIGIN_IX] != chipTopX ())
		{
			setReadout (READOUT_SERIAL_ORIGIN_IX, chipTopX (), "serial origin");
		}

		if (camera.readout[READOUT_SERIAL_LENGTH_IX] != getUsedWidthBinned ())
		{
			setReadout (READOUT_SERIAL_LENGTH_IX, getUsedWidth (), "serial length");
		}

		if (camera.readout[READOUT_SERIAL_BINNING_IX] != binningHorizontal ())
		{
			setReadout (READOUT_SERIAL_BINNING_IX, binningHorizontal (), "serial binning");
		}

		if (camera.readout[READOUT_PARALLEL_ORIGIN_IX] != chipTopY ())
		{
			setReadout (READOUT_PARALLEL_ORIGIN_IX, chipTopY (), "parallel origin");
		}

		if (camera.readout[READOUT_PARALLEL_LENGTH_IX] != getUsedHeightBinned ())
		{
			setReadout (READOUT_PARALLEL_LENGTH_IX, getUsedHeight (), "parallel length");
		}

		if (camera.readout[READOUT_PARALLEL_BINNING_IX] != binningVertical ())
		{
			setReadout (READOUT_PARALLEL_BINNING_IX, binningVertical (), "parallel binning");
		}
	}
	catch (rts2core::Error &e)
	{
		logStream (MESSAGE_ERROR) << e.what () << sendLog;
		return -1;
	}

	total = getUsedWidthBinned () * getUsedHeightBinned () * 2 * 2; /* 2 bytes per short, 2 readout?? */

	camera.dma_config.total = total;
	camera.dma_config.buflen = 1024*1024; /* power of 2 makes it easy to mmap */
	camera.dma_config.timeout = 50000;
	camera.dma_config.config = SI_DMA_CONFIG_WAKEUP_ONEND;

	if (ioctl (camera.fd, SI_IOCTL_DMA_INIT, &camera.dma_config) < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot init DMA " << errno << " " << strerror (errno) << sendLog;
    		return -1;
	}

	int nbufs = camera.dma_config.total / camera.dma_config.buflen ;
	if (camera.dma_config.total % camera.dma_config.buflen)
		nbufs += 1;

	camera.dma_active = 1;

	if (ioctl (camera.fd, SI_IOCTL_DMA_START, &camera.dma_status ) < 0)
	{
		logStream (MESSAGE_ERROR) << "dma start " << errno << " " << strerror (errno) << sendLog;
		return -1;
	}

	bzero (&camera.dma_status, sizeof(struct SI_DMA_STATUS));

	char cmd = 'C';
	if (testImage->getValueBool () == false)
		cmd = getExpType () == 0 ? 'D' : 'E';

	send_command_yn (cmd);

	return 0;
}

int SI8821::endExposure (int ret)
{
	return Camera::endExposure (ret);
}

int SI8821::closeShutter ()
{
	// close shutter
	if (send_command_yn ('B') != 1)
		return -1;
	return 0;
}

int SI8821::stopExposure ()
{
	send_command_yn ('O');
	return Camera::stopExposure ();
}

int SI8821::doReadout ()
{
	logStream (MESSAGE_DEBUG) << "do readout" << sendLog;

	// split to half..1st and 2nd channel
	int ret = ioctl (camera.fd, SI_IOCTL_DMA_NEXT, &camera.dma_status);

	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "ioctl dma_next " << errno << " " << strerror (errno) << sendLog;
	}

	if (camera.dma_status.transferred != camera.dma_config.total)
	{
		logStream (MESSAGE_WARNING) << "NEXT wakeup xfer " << camera.dma_status.transferred << " %d bytes" << sendLog;
		return 0;
	}

	if (!(camera.dma_status.status & SI_DMA_STATUS_DONE))
	{
		logStream (MESSAGE_ERROR) << "timeout occured" << sendLog;
		return -1;
	}

	logStream (MESSAGE_DEBUG) << "got data" << sendLog;

	short unsigned int *p = chanBuffer;
	short unsigned int *d = camera.ptr;

	// get first channel
	for (int r = 0; r < getUsedHeightBinned (); r++)
	{
		for (int c = 0; c < getUsedWidthBinned (); c++)
		{
			*p = *d;
			p++;
			d += 2;
		}
	}

	ret = sendReadoutData ((char*) chanBuffer, getWriteBinaryDataSize (0), 0);

	if (ret < 0)
	{
		return -1;
	}

	p = chanBuffer;
	d = camera.ptr + 1;

	// get first channel
	for (int r = 0; r < getUsedHeightBinned (); r++)
	{
		for (int c = 0; c < getUsedWidthBinned (); c++)
		{
			*p = *d;
			p++;
			d += 2;
		}
	}

	ret = sendReadoutData ((char*) chanBuffer, getWriteBinaryDataSize (1), 1);

	if (ret < 0)
		return -1;

	if (getWriteBinaryDataSize () == 0)
		return -2;

	return 0;
}

int SI8821::setCoolTemp (float new_temp)
{
	try
	{
//		setConfig (CONFIG_SET_TEMP_IX, celsiusToKelvin (new_temp) * 10, "ccd setpoint");
	}
	catch (rts2core::Error &e)
	{
		logStream (MESSAGE_ERROR) << e.what () << sendLog;
		return -1;
	}
	return Camera::setCoolTemp (new_temp);
}

int SI8821::switchCooling (bool newval)
{
	if (newval)
	{
		send_command ('S');
	}	
	else
	{
		send_command ('T');
	}
	logStream (MESSAGE_WARNING) << "switchCooling " << newval << sendLog;
	return Camera::switchCooling (newval);
}

float SI8821::getGain (int iMode)
{
	return (float)iMode;
}

void SI8821::dma_unmap ()
{
	int nbufs;

	if (!camera.dma_active)
		return;

	nbufs = camera.dma_config.total / camera.dma_config.buflen;
	if (camera.dma_config.total % camera.dma_config.buflen)
		nbufs += 1;

	if (munmap (camera.ptr, camera.dma_config.buflen*nbufs))
		logStream (MESSAGE_ERROR) << "failed to unmap:" << errno << " " << strerror(errno) << sendLog;

	camera.dma_active = 0;
}

void SI8821::write_dma_data (unsigned char *ptr)
{
	FILE *fd;
	//int tmm;
	long int tmm; // APMONTERO: Changed to solve compiling error
	char buf[256];

	time(&tmm);
	sprintf( buf, "dma_%d.cam", (int)tmm ); // APMONTERO: Changed to solve compiling error

	printf("WRITING DATA %s\n",buf);
	if( !(fd = fopen( buf, "w+" ))) {
		printf("cant write dma.cam\n");
		return;
	}
	fwrite(ptr, total, 1, fd ); 
	fclose(fd);
	printf("wrote %d bytes to %s\n", total, buf );
}

void SI8821::expect_y ()
{
	if (expect_yn() != 1)
		logStream (MESSAGE_ERROR) << "ERROR expected yes got no from uart\n" << sendLog;
}

/* read n bigendian integers and return it */
int SI8821::receive_n_ints (int n, int *data)
{
	int len, i;
	int ret;

	len = n * sizeof(int);

	if (camera.fd < 0)
	{
		bzero( data, len );
		return 0;
	}

	if ((ret = read (camera.fd, data, len )) != len )
		return -1;

	for (i=0; i<n; i++)
		swapl(&data[i]);

	return 0;
}

int SI8821::send_n_ints (int n, int *data)
{
	int len, i;
	int *d;

	len = n * sizeof(int);
	d = (int *)malloc( len );
	memcpy( d, data, len );
	for (i=0; i<n; i++)
		swapl(&d[i]);

	if ((i = write (camera.fd, d, len )) != len)
		return -1;

//  memset( d, 0, len );
//  if( receive_n_ints( fd, len, d ) < 0 )
//    return -1;

//  if( memcmp( d, data, len ) != 0 )
//    return -1;
	free(d);

	return len;
}

struct CFG_ENTRY *SI8821::find_readout (char *name )
{
	struct CFG_ENTRY *cfg;
	int i;

	for (i=0; i<SI_READOUT_MAX; i++)
	{
		cfg = camera.e_readout[i];

		if (cfg && cfg->name)
		{
			//printf ("%s|%s", cfg->name, name);
			if (strncasecmp (cfg->name, name, strlen(cfg->name)) == 0)
			{
				// printf ("%s|%s", cfg->name, name);
				return cfg;
			}
		}
	}
	//logStream (MESSAGE_ERROR) << "parameter " << name << " not found in readout parameters" << sendLog;
	return NULL;
}

void SI8821::send_readout ()
{
	send_command ('F'); // F    - Send Readout Parameters
	send_n_ints (32, (int *)&camera.readout );
	expect_yn ();
}

int SI8821::setfile_readout (const char *file, bool bReadFormat, int iMode)
{
	FILE *fd;
	const char *delim = "=\n";
	char *s;
	char buf[256];
	struct CFG_ENTRY *cfg;

	if (!(fd = fopen( file, "r" )))
	{
		return -1;
	}

	bool bSectionFound = false;
	char sectionName[17];
	if( bReadFormat )
	{
        	sprintf( sectionName, "[Readout & Format]" );
	}
	else
	{
        	sprintf( sectionName, "[Readout Mode %d]", iMode );
	}

	
	while (fgets (buf, 256, fd))
	{
		if( !bSectionFound )
		{	
	       		if (strncasecmp (sectionName, buf, strlen(sectionName)) == 0)
                        {
		 		bSectionFound = true;
		 	}	
		 	continue;
		}
		else if ( buf[0] == '[' ) 
		{
			break;
		}
		else
		{
			//printf ("BUF: %s",buf);
			if (!(cfg = find_readout (buf)))
				continue;
	
			strtok (buf, delim);
			// if( s = strtok( NULL, delim ))
			s = strtok (NULL, delim);
			if (s)
			{
			//	printf ("PARAM... %s\n",s);
				camera.readout [cfg->index] = atoi(s);
			}
		}
	}
	fclose (fd);
	return 0;
}

int SI8821::getReadoutFromFile (bool bReadFormat, int iMode)
{
	setfile_readout (cameraSet, bReadFormat, iMode);
	send_readout ();
	send_command ('H');     // H    - load readout
	//print_readout (); // APMONTERO: FOR TESTING
	receive_n_ints (32, (int *) &camera.readout);
	if (expect_yn () != 1)
	{
		logStream (MESSAGE_ERROR) << "cannot read readout parameters from camera" << sendLog; 
		return -1; 
	}
	else
	{
		logStream (MESSAGE_INFO) << "readout parameters have been read successfully from camera" << sendLog;
		print_readout(); // APMONTERO: FOR TESTING
	} 
	return 0;
}

/*int SI8821::setfile_readout (const char *file )
{
	FILE *fd;
	const char *delim = "=\n";
	char *s;
	char buf[256];
	struct CFG_ENTRY *cfg;

	if (!(fd = fopen( file, "r" )))
	{
		return -1;
	}

	while (fgets (buf, 256, fd))
	{
		//printf ("BUF: %s",buf);
		if (!(cfg = find_readout (buf)))
			continue;
	
		strtok (buf, delim);
		// if( s = strtok( NULL, delim ))
		s = strtok (NULL, delim);
		if (s)
		{
		//	printf ("PARAM... %s\n",s);
			camera.readout [cfg->index] = atoi(s);
		}
	}
	fclose (fd);
	return 0;
}*/

struct CFG_ENTRY *SI8821::find_config (char *name )
{
	struct CFG_ENTRY *cfg;
	int i;

	for (i=0; i<SI_CONFIG_MAX; i++)
	{
		cfg = camera.e_config[i];

		if (cfg && cfg->name)
		{
			//printf ("%s|%s", cfg->name, name);
			if (strncasecmp (cfg->name, name, strlen(cfg->name)) == 0)
			{
				return cfg;
			}
		}
	}
	//logStream (MESSAGE_ERROR) << "parameter " << name << " not found in configuration parameters" << sendLog;
	return NULL;
}

void SI8821::send_config ()
{
	send_command ('J'); // J    - Send Configuration Parameters
	send_n_ints (32, (int *) &camera.config); 
	expect_yn ();
}

int SI8821::setfile_config (const char *file )
{
	FILE *fd;
	const char *delim = "=\n";
	char *s;
	char buf[256];
	struct CFG_ENTRY *cfg;

	if (!(fd = fopen( file, "r" )))
	{
		return -1;
	}

	while (fgets (buf, 256, fd))
	{
		//printf ("BUF: %s",buf);
		if (!(cfg = find_config (buf)))
			continue;
	
		strtok (buf, delim);
		// if( s = strtok( NULL, delim ))
		s = strtok (NULL, delim);
		if (s)
		{
			//printf ("PARAM... %s\n",s);
			camera.config [cfg->index] = atoi(s);
		}
	}
	fclose (fd);
	return 0;
}

int SI8821::load_camera_cfg( struct SI_CAMERA *c, const char *fname)
{
	load_cfg (c->e_status, fname, "SP");
//	load_cfg (c->e_readout, fname, "ESP");
	load_cfg (c->e_readout, fname, "RFP");
	load_cfg (c->e_config, fname, "CP");

	return 0;
}

int SI8821::load_cfg( struct CFG_ENTRY **e, const char *fname, const char *var )
{
	int len, varlen, index, pindex;
	FILE *fd;
	char buf[256];
	struct CFG_ENTRY *entry;

	logStream (MESSAGE_ERROR) << " ---> load_cfg"  << sendLog;
	varlen = strlen(var);

	if( !(fd = fopen(fname,"r")) ) {
		printf("unable to open %s\n", fname );
		return -1;
	}

	pindex = 0;
	while( fgets( buf, 256, fd )) {
		if( strncmp( buf, var, varlen ) == 0 ) {
			index = atoi( &buf[varlen] );
			len = strlen(buf);
			//len -=1;
			buf[len] = '\0';

			entry = (struct CFG_ENTRY *)malloc( sizeof(struct CFG_ENTRY ));

			bzero( entry, sizeof(struct CFG_ENTRY ));
			entry->index = index;


			entry->cfg_string = (char *)malloc( len + 1 );
			strcpy( entry->cfg_string, buf );

			parse_cfg_string( entry );
			if( index!=pindex || index > 32 ) {
				printf("CFG error\n");
				index = pindex;
			}
	      
			e[pindex] = entry; /* warning here no bounds check */
			pindex++;
			if( pindex >= 32 )
				break;
		}
  	}

	fclose(fd);
	return -1; // APMONTERO: new
}


int SI8821::parse_cfg_string (struct CFG_ENTRY *entry)
{
	const char *delim = "=\",\n\r";
	char *s;
	char buf[512];
	int i, tot;//, min, max; APMONTERO unused variables
	unsigned int mask;

	strcpy( buf, entry->cfg_string );
	if( !(s = strtok( buf, delim ))) 
		return -1;

	if( !(s = strtok( NULL, delim ))) 
		return -1;

	int oritype = atoi(s);

	if (oritype == 1 || oritype == 2 || oritype == 3 || oritype == 4 || oritype == 5
	|| oritype == 6 || oritype == 7 || oritype == 11 || oritype == 12 
	|| oritype == 13 || oritype == 14)
		oritype = 1;
	else if (oritype == 8)
		oritype = 8;
	else if (oritype == 9)
		oritype = 9;
	else if (oritype == 10)
		oritype = 3;
	else if (oritype == 0)
		oritype = 0;
	else 
	{
		printf("CFG type error\n");
		exit(1);
	}
  
	entry->type = oritype;

	if( entry->type == CFG_TYPE_NOTUSED )
		return 0;

	/* AI:20152021 Security val does not exist any longer
	if( !(s = strtok( NULL, delim ))) 
		return -1;
	entry->security = atoi(s);
	*/

	if( !(s = strtok( NULL, delim ))) 
		return -1;

	entry->name = (char*)malloc(strlen(s)+1);

	strcpy( entry->name, s );
	printf("READING entry->NAME %s\n",entry->name);
  
	printf("READING ENTRY %i\n",entry->type);

	switch( entry->type ) {
		case CFG_TYPE_INPUTD:
			if( !(s = strtok( NULL, delim ))) 
				return -1;
			entry->u.iobox.min = atoi(s);
			if( !(s = strtok( NULL, delim ))) 
				return -1;
			entry->u.iobox.max = atoi(s);
			entry->u.iobox.mult = 1.0; // APMONTERO: line added to solve parsing errors with oritype 2,3,7,11
			entry->u.iobox.offset = 0; // APMONTERO: line added to solve parsing errors with oritype 2,3,7,11

			if( !(s = strtok( NULL, delim ))) 
				return -1;
			entry->u.iobox.units = (char*)malloc(strlen(s)+1); // APMONTERO: (char*) added to compile
			strcpy( entry->u.iobox.units, s );

			if( !(s = strtok( NULL, delim )))  {
				entry->u.iobox.mult = 1.0;
				return -1;
			}
			entry->u.iobox.mult = atof(s);

			if( !(s = strtok( NULL, delim ))) 
				return -1;
			entry->u.iobox.offset = atof(s);

			if( !(s = strtok( NULL, delim ))) 
				return -1;
			entry->u.iobox.status = atoi(s);
		break;
		case CFG_TYPE_DROPDI:
			if( !(s = strtok( NULL, delim ))) 
				return -1;
			tot = atoi(s);
			entry->u.dropi.min = 0;

			entry->u.dropi.max = tot-1;
			if( tot <= 0 )
				return -1;
			entry->u.dropi.list = (char **)malloc( sizeof(char *)*tot);
			bzero( entry->u.dropi.list, sizeof(char *)*tot);
			for( i=0; i<tot; i++ ){
				if( !(s = strtok( NULL, delim ))) 
					return -1;
				printf("DROP %s\n",s);
				entry->u.dropi.list[i] = (char *)malloc( strlen(s)+1);
				strcpy( entry->u.dropi.list[i], s );
			}
		break;
		case CFG_TYPE_DROPDP:
			if( !(s = strtok( NULL, delim ))) 
				return -1;
			tot = atoi(s);

			entry->u.dropp.list = (char **)malloc( sizeof(char *)*tot);
			bzero( entry->u.dropp.list, sizeof(char *)*tot);
			for( i=0; i<tot; i++ ){
				if( !(s = strtok( NULL, delim ))) 
					return -1;
				entry->u.dropp.val = atoi(s); //TODO APMONTERO: overwrite values?
				printf("DROPP %s\n",s);
				if( !(s = strtok( NULL, delim ))) 
					return -1;
				entry->u.dropp.list[i] = (char *)malloc( strlen(s)+1);
				printf("DROPP STRING %s\n",s);
				strcpy( entry->u.dropp.list[i], s );
			}

		break;
		case CFG_TYPE_BITF:
			if( !(s = strtok( NULL, delim ))) 
				return -1;
			mask = entry->u.bitf.mask = atoi(s);
			tot = 0;
			while( mask &1 ) {
				tot++;
				mask = (mask>>1);
			}
			entry->u.bitf.list = (char **)malloc( sizeof(char *)*tot);
			bzero( entry->u.bitf.list, sizeof(char *)*tot);
			for( i=0; i<tot; i++ ){
				if( !(s = strtok( NULL, delim )))
					return -1;
				entry->u.bitf.list[i] = (char *)malloc( strlen(s)+1);
				strcpy( entry->u.bitf.list[i], s );
			}
		break;
	}
	return -1;
}

int SI8821::init_com (int baud, int parity, int bits, int stopbits, int buffersize)
{
	struct SI_SERIAL_PARAM serial;

	bzero( &serial, sizeof(struct SI_SERIAL_PARAM));

	serial.baud = baud;
	serial.buffersize = buffersize;
	serial.fifotrigger = 8;
	serial.bits = bits;
	serial.stopbits = stopbits;
	serial.parity = parity;

	serial.flags = SI_SERIAL_FLAGS_BLOCK;
	serial.timeout = 1000;
  
	return ioctl (camera.fd, SI_IOCTL_SET_SERIAL, &serial);
}

int SI8821::clear_buffer ()
{
	if (ioctl (camera.fd, SI_IOCTL_SERIAL_CLEAR, 0))
		return -1;
	else
		return 0;
}

int SI8821::send_command (int cmd)
{
  	clear_buffer ();    
	return send_char (cmd);
}

int SI8821::send_command_yn (int cmd)
{
	send_command (cmd);

	return expect_yn ();
}

int SI8821::send_char (int data)
{
	unsigned char wbyte, rbyte;
	int n;

	wbyte = (unsigned char) data;
	if (write (camera.fd,  &wbyte, 1) != 1)
	{
		logStream (MESSAGE_ERROR) << "cannot send char " << data << ":" << strerror << sendLog;
		return -1;
	}

	if ((n = read (camera.fd,  &rbyte, 1)) != 1)
	{
		logStream (MESSAGE_ERROR) << "cannot read reply to command " << data << ":" << strerror << sendLog;
		return -1;
	}

	return 0;
}

int SI8821::receive_char ()
{
	unsigned char rbyte;
	int n;

	if ((n = read (camera.fd,  &rbyte, 1)) != 1)
	{
		if (n<0)
		      logStream (MESSAGE_ERROR) << "receive_char read:" << strerror << sendLog;
		return -1;
	}
	else
	{
		return (int)rbyte;
	}
}

int SI8821::expect_yn ()
{
	int got;
	if ((got = receive_char()) < 0)
		return -1;

	if (got != 'Y' && got != 'N')
		return -1;

	return (got == 'Y');
}
//------------------------------------------------------------------------

int SI8821::swapl( int *d )
{
	union {
		int i;
		unsigned char c[4];
	} u;
	unsigned char tmp;

	u.i = *d;
	tmp = u.c[0];
	u.c[0] = u.c[3];
	u.c[3] = tmp;

	tmp = u.c[1];
	u.c[1] = u.c[2];
	u.c[2] = tmp;

	*d = u.i;
	return 0;
}

void SI8821::setReadout (int idx, int v, const char *name)
{
	send_command ('G');
 
	int data[2];
	data[0] = idx;
	data[1] = v;
	send_n_ints (2, data);
	if (expect_yn () != 1)
	{
		throw rts2core::Error (std::string ("cannot set ") + name);
	}
	camera.readout[idx] = v;
}

void SI8821::setConfig (int idx, int v, const char *name)
{
	send_command ('K');
 
	int data[2];
	data[0] = idx;
	data[1] = v;
	send_n_ints (2, data);
	if (expect_yn () != 1)
	{
		throw rts2core::Error (std::string ("cannot set ") + name);
	}
	camera.config[idx] = v;
}

void SI8821::print_readout () 
{
	int i;
  	struct CFG_ENTRY *cfg;

	printf("**********************************************\n");
	printf("Readout Parameters\n");
	printf("------------------\n");
	for (i=0; i< SI_READOUT_MAX; i++)
	{
		cfg = camera.e_readout[i];
		if( cfg && cfg->name )
		{
			print_cfg (cfg, camera.readout[i]);
		}
	}
	printf("**********************************************\n");
}


void SI8821::print_config ()
{
	int i;
	struct CFG_ENTRY *cfg;
	printf("**********************************************\n");
	printf("Configuration Parameters\n");
	printf("------------------------\n");
	for( i=0; i< SI_CONFIG_MAX; i++ )
	{
		cfg = camera.e_config[i];
		if( cfg && cfg->name )
		{
			print_cfg (cfg, camera.config[i]);
		}
  	}
	printf("**********************************************\n");
}

void SI8821::print_cfg (struct CFG_ENTRY *cfg, int val)
{
	int ix;
	unsigned int mask;
	double value;
	const char *units;

	switch( cfg->type )
	{
		case CFG_TYPE_NOTUSED:
			break;
		case CFG_TYPE_INPUTD:
        		if (cfg->u.iobox.units)
			{
				units = cfg->u.iobox.units;
			}
        		else
			{
          			units = "";
			}
		        value = cfg->u.iobox.mult * val + cfg->u.iobox.offset;
		        //printf("%s %f %s val %d index %d\n", cfg->name, value, units, val, cfg->index );
		        printf("%s %f %s val %d index %d offset %f\n", cfg->name, value, units, val, cfg->index, cfg->u.iobox.offset );
			break;
		case CFG_TYPE_DROPDI:
			ix = val-cfg->u.dropi.min;
			if (ix < cfg->u.dropi.min || ix > cfg->u.dropi.max)
			{	
				printf("%s val %d index %d\n", cfg->name, val, cfg->index);
			}
			else
			{
				printf("%s %s val %d index %d\n", cfg->name, cfg->u.dropi.list[ix], val, cfg->index);
			}
			break;
	      case CFG_TYPE_DROPDP:
			ix = val-cfg->u.dropp.val;
			if (ix < cfg->u.dropp.val || ix > cfg->u.dropp.val)
			{
				printf ("%s val %d index %d\n", cfg->name, val, cfg->index);
			}
		        else
			{
		        	printf ("%s %s val %d index %d\n", cfg->name, cfg->u.dropp.list[ix], val, cfg->index);
			}
			break;
		case CFG_TYPE_BITF:
			mask = cfg->u.bitf.mask&val;
			ix = 0;
			while (!(mask & 1))
			{
				ix++;
				mask = (mask>>1);
			}
			printf ("%s %s val %d index %d\n", cfg->name, cfg->u.bitf.list[ix], val, cfg->index);
			break;
	}
}

int main (int argc, char **argv)
{
	SI8821 device (argc, argv);
	return device.run ();
}
