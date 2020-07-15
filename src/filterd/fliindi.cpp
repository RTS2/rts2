/*
 * INDI bridge driver for FLI filter wheel.
 *
 * Copyright (C) 2019 Michael Mommert, Lowell Observatory
 * based on ../teld/indibridge.cpp by Markus Wildi and fli.cpp by Petr 
 * Kubanek
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * ===========
 * 
 * For this driver to work with rts2, an indiserver has to run in the 
 * background, e.g., using:
 *
 *   > indiserver -vvvl /tmp -f /tmp/indififo  indi_fli_wheel
 *
 * (create indififo with: mkfifo /tmp/indififo) 
 * 
 * for some reason, this driver also requires an independent client to 
 * be actively connected to the indiserver:
 *   > python fliindi_client.py
 *
 * known bugs:
 * - filter slot seems to jump to slot 2 (start counting at 0) randomly
 *   (is it homing randomly?)
 */

#include "filterd.h"
#include "rts2toindi.h"

#define OPT_INDI_SERVER		OPT_LOCAL + 53
#define OPT_INDI_PORT		OPT_LOCAL + 54
#define OPT_INDI_DEVICE		OPT_LOCAL + 55
#define OPT_FLI_DEFAULT_FILTER             OPT_LOCAL + 270

namespace rts2filterd
{

class FLIINDI: public Filterd
{
	public:
		FLIINDI(int argc, char ** argv);
		virtual ~FLIINDI(void);
	protected:
		virtual int processOption(int in_opt);
		virtual int initHardware();
		virtual int initValues();
		
		virtual void changeMasterState(rts2_status_t old_state,
									   rts2_status_t new_state);
		virtual int getFilterNum(void);
		virtual int setFilterNum(int new_filter);
		virtual int homeFilter();

	private:
		char indiserver[256];
		int indiport;
		char indidevice[256];

		long filter_count;
		rts2core::ValueInteger *defaultFilterPosition;
};

}

using namespace rts2filterd;


FLIINDI::FLIINDI(int in_argc, char **in_argv):Filterd (in_argc, in_argv)
{
	// INDI setup
	strcpy(indiserver, "127.0.0.1");
	indiport = 7624;
	strcpy(indidevice, "FLI CFW");

	addOption(OPT_INDI_SERVER, "indiserver", 1,
			   "host name or IP address of the INDI server, default: 127.0.0.1");
	addOption(OPT_INDI_PORT, "indiport", 1,
			   "port at which the INDI server listens, default here 7624");
	addOption(OPT_INDI_DEVICE, "indidevice", 1, "name of the INDI device");

	// Filter Wheel setup
	defaultFilterPosition = NULL;
	addOption (OPT_FLI_DEFAULT_FILTER, "default-filter", 1,
			   "default filter position (number), is set after init and during day");
}


FLIINDI::~FLIINDI (void)
{
	rts2closeINDIServer();
}


int FLIINDI::processOption(int in_opt)
{
	switch (in_opt)
	{
		case OPT_INDI_SERVER:
		        strcpy( indiserver, optarg);
			break;
		case OPT_INDI_PORT:
		        indiport = atoi(optarg);
			break;
		case OPT_INDI_DEVICE:
			strcpy(indidevice, optarg);
			break;
		case OPT_FLI_DEFAULT_FILTER:
			createValue (defaultFilterPosition, "default_filter",
						 "default filter position after init and during day",
						 false);
			defaultFilterPosition->setValueCharArr(optarg);
			break;
		default:
			return Filterd::processOption(in_opt);
	}

	return 0;
}


int FLIINDI::initHardware()
{
	rts2openINDIServer(indiserver, indiport);
	
	logStream (MESSAGE_DEBUG) << "INDI connection established to " <<
		indiserver <<" on port " << indiport << sendLog;

	return 0;
}


int FLIINDI::initValues()
{
	pthread_t thread_0;

	/* adopted from indibridge.cpp, not sure if necessary
    // This first call is necessary to receive further updates from the INDI server
	SearchDef *getINDI=NULL;
	int getINDIn=0;
	float slot;

	logStream (MESSAGE_DEBUG) << "here1" << sendLog;

    rts2getINDI(indidevice, "defNumberVector", "FILTER_SLOT",
				"FILTER_SLOT_VALUE",
				&getINDIn, &getINDI) ;

	logStream (MESSAGE_DEBUG) << "here2" << sendLog;
	
	sscanf(getINDI[0].gev[0].gv, "%f", &slot);

	rts2free_getINDIproperty(getINDI, getINDIn);

	logStream (MESSAGE_DEBUG) << "here3" << sendLog;	
	*/

	// --------------------------------- new stuff
	// SearchDef *getINDI=NULL;
	// int getINDIn=0;
	// float slot;

	// logStream (MESSAGE_DEBUG) << "here1" << sendLog;

    // rts2getINDI(indidevice, "defNumberVector", "FILTER_SLOT",
	// 			"FILTER_SLOT_VALUE",
	// 			&getINDIn, &getINDI);
	
	// logStream (MESSAGE_DEBUG) << "here2" << sendLog;
	
	// rts2free_getINDIproperty(getINDI, getINDIn);

	// logStream (MESSAGE_DEBUG) << "here3" << sendLog;	
	// -----------------------------------------------
	
	pthread_create(&thread_0, NULL, rts2listenINDIthread, indidevice);
	return Filterd::initValues();
}


void FLIINDI::changeMasterState(rts2_status_t old_state,
								rts2_status_t new_state)
{
	if ((new_state & SERVERD_STATUS_MASK) == SERVERD_DAY)
		homeFilter();
	Filterd::changeMasterState (old_state, new_state);
}


int FLIINDI::getFilterNum (void)
{
	logStream (MESSAGE_DEBUG) << "retrieving current filter wheel slot " <<
		sendLog;	
	
	SearchDef *getINDI=NULL;
	int getINDIn=0 ;
	float slot;

	/* property information available from:
       https://indilib.org/develop/developer-manual/101-standard-properties.html#h6-filter-wheels

	   python script to explore properties available here:
       https://sourceforge.net/p/pyindi-client/code/HEAD/tree/trunk/pip/pyindi-client/examples/testindiclient.py
	*/
    rts2getINDI(indidevice, "defNumberVector", "FILTER_SLOT",
				"FILTER_SLOT_VALUE",
				&getINDIn, &getINDI);

	// extract slot value as float
	sscanf(getINDI[0].gev[0].gv, "%f", &slot);

	return (int) slot+1;
}


int FLIINDI::setFilterNum(int new_filter)
{
	logStream (MESSAGE_DEBUG) << "setting filter wheel to slot " <<
		new_filter << sendLog;	

	char cvalues[1024];
	
	// convert new slot value to string, as required by rts2setINDI
	sprintf(cvalues, "%2.1f", (float)(new_filter+1));

	/* property information available from:
       https://indilib.org/develop/developer-manual/101-standard-properties.html#h6-filter-wheels

	   python script to explore properties available here:
       https://sourceforge.net/p/pyindi-client/code/HEAD/tree/trunk/pip/pyindi-client/examples/testindiclient.py
	*/
	rts2setINDI(indidevice, "defNumberVector", "FILTER_SLOT",
				"FILTER_SLOT_VALUE", cvalues);

	return Filterd::setFilterNum(new_filter+1);
}


int FLIINDI::homeFilter()
{
	logStream (MESSAGE_WARNING) << "sending filter wheel to home" << sendLog;	

	// if defaultFilterPosition not defined, use slot 1
	return setFilterNum (defaultFilterPosition != NULL? defaultFilterPosition->getValueInteger() : 1);
}


int main(int argc, char **argv)
{
	FLIINDI device = FLIINDI(argc, argv);

	return device.run();
}
