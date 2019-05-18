/* 
 * ngDome cupola driver.
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

#include "cupola.h"
#include "connection/tcsng.h"

using namespace rts2dome;

namespace rts2dome
{

/**
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ngDome:public Cupola
{
	private:
		//rts2core::ValueInteger * mcount;
		//rts2core::ValueInteger *moveCountTop;
		rts2core::ConnTCSNG * domeconn;
		rts2core::ValueBool *opened;
		rts2core::ValueBool *closed;
		rts2core::ValueString *domeStatus;
		HostString *host;
		const char *obsid;
		const char *cfgFile;
		const char *devname;

		
	protected:
		virtual int init()
		{
			host = new HostString ("10.30.3.68", "5750");
			//Hard code obsid and devname bdecase not all options are processed before this is called
			domeconn = new rts2core::ConnTCSNG (this, host->getHostname(), host->getPort(), "BIG61", "UDOME");
			opened->setValueBool(true);

			domeconn->setDebug(false);

			if (domeconn->getDebug())
			{
				logStream(MESSAGE_INFO) << host->getHostname()<< " : " << host->getPort() << " ," << obsid << " ," << devname << sendLog;
			}

			return Dome::init();
		}



		virtual int processOption (int in_opt)
		{
			switch (in_opt)
			{
				case OPT_CONFIG:
					cfgFile = optarg;
					break;
				case 't':
					host = new HostString (optarg, "5750");
					break;
				case 'n':
					devname = optarg;

					break;
				case 'o':
					obsid = optarg;
					break;

				default:
					return Dome::processOption (in_opt);
			}
			return 0;
		}


		virtual int setValue(rts2core::Value *oldValue, rts2core::Value *newValue)
		{
			//logStream(MESSAGE_ERROR) << "changing value!!!!!" << sendLog;
			if(oldValue == domeStatus)
			{
				domeStatus->setValueString("DOME FUCKING STATUS");
				sendValueAll(domeStatus);
			}
			return Cupola::setValue(oldValue, newValue);
		}
		virtual int moveStart ()
		{
			//mcount->setValueInteger (0);
			//sendValueAll (mcount);
			return Cupola::moveStart ();
		}
		virtual int moveEnd ()
		{
			//struct ln_hrz_posn hrz;
			//getTargetAltAz (&hrz);
			//setCurrentAz (hrz.az);
			return Cupola::moveEnd ();
		}
		virtual long isMoving ()
		{
			//mcount->inc ();
			//sendValueAll (mcount);
			
			return USEC_SEC;
		}

		virtual int startOpen ()
		{
			//if ((getState () & DOME_DOME_MASK) == DOME_OPENING)
				//return 0;
			domeconn->command("OPEN");

			//domeconn->command("OPEN");
			//mcount->setValueInteger (0);
			//sendValueAll (mcount);
			
			return 0;
		}

		virtual long isOpened ()
		{
			std::string resp;
			try
			{
				resp = domeconn->request("STATE");
				resp = strip(resp);
				logStream(MESSAGE_ERROR) << "resp is " << resp << sendLog;
				domeStatus->setValueString( resp );
			}
			catch(rts2core::ConnTimeoutError)
			{
				logStream(MESSAGE_ERROR) << "Timeout error Could not communicate with Upper Dome" << sendLog;
				domeStatus->setValueString("Timeout ERROR");
				resp = "Timeout ERROR";
				throw;
			}
			catch(rts2core::Error)
			{

				logStream(MESSAGE_ERROR) << "Error Could not communicate with Upper Dome" << sendLog;
				domeStatus->setValueString("ERROR");
				resp = "ERROR";
				throw;
			}

			sendValueAll(domeStatus);
			if ( domeStatus->getValueString() == "OPENED")
				return -2;
			return USEC_SEC;
		}

		virtual int endOpen ()
		{
			return 0;
		}

		virtual int startClose ()
		{
			
			domeconn->command("CLOSE");
			return 0;
		}

		virtual long isClosed ()
		{
			std::string resp;
			try
			{
				resp = domeconn->request("STATE");
				resp = strip(resp);
			}
			catch(rts2core::ConnTimeoutError)
			{
				 logStream(MESSAGE_ERROR) << "Timeout error Could not communicate with Upper Dome" << sendLog;
				 return -1;
			}


			if (resp == "CLOSED")
				return -2;

			else if(resp == "OPENED")
				return USEC_SEC;
			else if(resp == "MOVING")
				return USEC_SEC;
			else
			{
				logStream(MESSAGE_ERROR) << "Did not understand upper dome resp: "<< resp << sendLog;
				return -1;
			}

		}

		virtual int endClose ()
		{
			return 0;
		}

	public:
		ngDome (int argc, char **argv):Cupola (argc, argv)
		{
			//createValue (mcount, "mcount", "moving count", false);
			//createValue (moveCountTop, "moveCountTop", "move count top", false, RTS2_VALUE_WRITABLE);
			//moveCountTop->setValueInteger (100);
			domeconn = NULL;
			host = NULL;
			obsid = NULL;
			addOption ('t', NULL, 1, "TCS NG hostname[:port]");
			addOption ('n', NULL, 1, "TCS NG telescope device name");
			addOption ('o', NULL, 1, "TCS NG observatory id");
			createValue (domeStatus, "slit_status", "State of the dome slit", true, RTS2_VALUE_WRITABLE);
			createValue (opened, "isOpen", "Is the slit open?", false);
			createValue (closed, "isClosed", "Is the slit closed?", false);

			
		}
		virtual int initValues ()
		{
			setCurrentAz (0);
			return Cupola::initValues();

		}

		virtual double getSlitWidth (double alt)
		{
			return 1;
		}

		std::string strip(std::string str)
		{
			while( str[str.length()-1] == '\r' ||  str[str.length()-1] == '\n')
			{
				str=str.erase(str.length()-1, 1);
			}
			return str;
		}

};

}

int main (int argc, char **argv)
{
	ngDome device (argc, argv);
	return device.run ();
}
