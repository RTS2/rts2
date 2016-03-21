/* 
 * Galil filter driver.
 * Copyright (C) 2016 Scott Swindell
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

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>


#include "filterd.h"
#include "connection/tcp.h"
namespace rts2filterd
{

class GalilConn:public rts2core::ConnTCP
{
	public:
		GalilConn( rts2core::Block *_master, const char *_hostname, int _port):ConnTCP( _master, _hostname, _port )
		{
		}
		~GalilConn()
		{
			sendData ("CLIENTDONE\r\n", false);
		}
		std::string writeRead( const char *cmd )
		{
			char rspBuf[500];
			std::string rspStr;
			rts2core::ConnTCP::writeRead( cmd, strlen(cmd), rspBuf, sizeof(rspBuf), '\n', 5, false);
			
			//check for 'OK:' in begingin of str
			if( strncmp(rspBuf, "OK:", 3) != 0 )
				return "";
			
			rspStr = std::string( &rspBuf[4] );
			
			
			return rspStr.substr(0,rspStr.length()-1);
		}
		
		
		int isBusy()
		{
			return atoi( writeRead("SHOW FWBUSY").c_str() );
		
		}
		
		int loadFilter( const char * nextFilter)
		{
			char cmdBuf[30];
			printf("loading fitler %s \n", nextFilter);
			sprintf( cmdBuf, "LOADFILTER %s\r\n", nextFilter );
			return 0;
		}
		
		virtual int init()
		{
			int rtn = rts2core::ConnTCP::init();
		
			//pass connection problem along
			if( rtn != 0)
				return rtn;
			
			//Get the galil server greeting.
			writeRead("");
			//std::istringstream *_is;
			//receiveData (&_is, 1, '\n');
		
			return rtn;
		}

	
		
	
};

class GalilFilter:public Filterd
{
	public:
		GalilFilter (int argc, char **argv);
		virtual ~GalilFilter (void);
		virtual int processOption (int in_opt);
		virtual int info();
	 
	protected:
		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);
		virtual int getFilterNum (void);
		virtual int setFilterNum (int new_filter);
		virtual int initHardware();

	private:
		int readAllFilters();
    	
		int filterNum;
		int nFilters;
		int filterSleep;
		char rspBuff[100];
		rts2core::ValueString *filterNames;
		rts2core::ValueString *loadedFilter;
		GalilConn *galilConn;
		HostString *host;
		
};

GalilFilter::GalilFilter (int argc, char **argv):Filterd (argc, argv)
{
	host = NULL;
	galilConn = NULL;
	nFilters = -1;
	filterNum = 0;
	filterSleep = 3;
	addOption ('n', "nfilters", 1, "Number of Filters");
	addOption ('g', "hostname", 1, "name or ip address of galilserver");
	createValue (filterNames, "filter_names", "filter names (will be parsed)", false, RTS2_VALUE_WRITABLE);
	createValue (loadedFilter, "loadedFilter", "THis Filter", false);
}

GalilFilter::~GalilFilter ()
{
	delete galilConn ;
	delete host;
}

int GalilFilter::processOption (int in_opt)
{
	switch (in_opt)
	{
		 
		 case 'g':
			host = new HostString( optarg, "9875" );
			break;
		 case 'n':
	 		nFilters = atoi(optarg);
	 		break;
		 default:
			  return Filterd::processOption (in_opt);
	}
	return 0;
}




int GalilFilter::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == filterNames)
	{
		if (setFilters ((char *) (new_value->getValue ())))
			return -2;
		updateMetaInformations (old_value);
		return 0;
	}

	return Filterd::setValue (old_value, new_value);
}

int GalilFilter::getFilterNum (void)
{
	return atoi( galilConn->writeRead( "SHOW FWPOS\r\n" ).c_str() ) - 1;
}

int GalilFilter::info()
{
	loadedFilter->setValueString (galilConn->writeRead ("SHOWLOADEDFILTER\r\n"));
	return Filterd::info();
}


int GalilFilter::setFilterNum( int num )
{
	const char *nextFilter = getFilterNameFromNum( num );
	
	logStream (MESSAGE_INFO) << "filter name is " << nextFilter<< sendLog;
	char cmdBuf[30];
	
	sprintf( cmdBuf, "LOADFILTER %s\r\n", nextFilter );
	
	galilConn->writeRead( (const char *)cmdBuf );
	sleep(1);
	while( galilConn->isBusy() )
	{
		logStream (MESSAGE_INFO) << "FW is moving!" << sendLog;
		sleep(1);
	}

	return 0;
}

int GalilFilter::initHardware()
{

	std::string filterStr;
	std::string word;
	
	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "you need to specify galil server host with -g argument" << sendLog;
		return -1;
	}
	if (nFilters <= 0)
	{
		logStream (MESSAGE_ERROR) << "You Need to specify a positive number of filters with the -n argument" << sendLog;
		return -1;
	}
	
	galilConn = new GalilConn( this, host->getHostname(), host->getPort()  );
	galilConn->setDebug (getDebug ());
	if (galilConn->init() == -1 )
	{
		logStream (MESSAGE_ERROR) << "Galil Server Connection Error" << sendLog;
		return -1;
	}
	
	readAllFilters();
	
	return Filterd::initHardware();
}

int GalilFilter::readAllFilters( )
{
	std::string filter1;
	int pos; 

	//const char *allFilters[6] = {"Bessell-U", "Harris-R", "Harris-B", "Harris-V", "Arizona-I", "Schott-8612"};
	std::string allFilters[nFilters];
	int ii =0;

	while( ii < nFilters )
	{
		
		if( galilConn->isBusy() )
		{
			logStream (MESSAGE_INFO) << "FW is busy" << sendLog;
			sleep(1);
		}
		else
		{
			ii++;
			pos = atoi( galilConn->writeRead("SHOW FWPOS\r\n").c_str() )-1;
			allFilters[pos] = galilConn->writeRead("SHOWLOADEDFILTER\r\n");
			logStream (MESSAGE_INFO) << "Filter number " << pos << " is " << allFilters[pos] << sendLog;
			galilConn->writeRead("MOVE1\r\n");
			sleep(1);
		}
	}


	for( ii=0; ii< nFilters; ii++)
	{
		logStream (MESSAGE_INFO) << "adding filter " << ii << " " << allFilters[ii] << sendLog;
		addFilter (allFilters[ii].c_str ());
	
	}
	sendFilterNames ( );
	return 0;
}

};

using namespace rts2filterd;

int main (int argc, char **argv)
{
	GalilFilter device = GalilFilter (argc, argv);
	return device.run ();
}
