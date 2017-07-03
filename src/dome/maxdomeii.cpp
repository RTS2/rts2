/*
    Max Dome II Driver
    Copyright (C) 2009 Ferran Casarramona (ferran.casarramona@gmail.com)
    Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
    Copyright (C) 2012 Markus Wildi <wildi.markus@bluewin.ch>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "cupola.h"
#include "connection/serial.h"
#include "slitazimuth.h"

#define MAXDOME_TIMEOUT	5		/* FD timeout in seconds */

// Start byte
#define START       0x01
#define MAX_BUFFER  15

// Message Destination
#define TO_MAXDOME  0x00
#define TO_COMPUTER 0x80

// Azimuth commands available
#define ABORT_CMD   0x03		// Abort azimuth movement
#define HOME_CMD    0x04		// Move until 'home' position is detected
#define GOTO_CMD    0x05		// Go to azimuth position
#define SHUTTER_CMD 0x06		// Send a command to Shutter
#define STATUS_CMD  0x07		// Retrieve status
#define TICKS_CMD   0x09		// Set the number of tick per revolution of the dome
#define ACK_CMD     0x0A		// ACK (?)
#define SETPARK_CMD 0x0B		// Set park coordinates and if need to park before to operating shutter
#define PARK        0xff                // emmulated command

// Shutter commands
#define OPEN_SHUTTER            0x01
#define OPEN_UPPER_ONLY_SHUTTER 0x02
#define CLOSE_SHUTTER           0x03
#define EXIT_SHUTTER            0x04  // Command send to shutter on program exit
#define ABORT_SHUTTER           0x07

#define SEL_ABORT_SHUTTER 0x00
#define SEL_CLOSE_SHUTTER 0x01
#define SEL_OPEN_SHUTTER  0x02
#define SEL_OPEN_UPPER_ONLY_SHUTTER 0x03
#define SEL_EXIT_SHUTTER  0x04

// Direction for azimuth movement
#define MAXDOMEII_EW_DIR 0x01
#define MAXDOMEII_WE_DIR 0x02

// Azimuth movement status
#define  ERROR_AZ       0x05 // Error. Showed after connect (?) 
#define  SYNCED_CROSSED 0x04 // ToDo: clarify with Ferran: Dome is stationary. Home was crossed in the previous movement.
#define  MOVING_ETOW    0x03 // Dome is moving E to W
#define  MOVING_WTOE    0x02 // Dome is moving W to E
#define  SYNCED         0x01 // Dome is stationary. 
//ToDo make option
#define AZ_MINIMAL_STEP 2 // ToDo unit [deg], minimal azimut step to command the dome to move

// selections for azCmd
#define SEL_HOME      0
#define SEL_ABORT     1
#define SEL_PARK      2

// Shutter operation
#define OPERATE_AT_ANY_AZIMUTH 0x00
#define PARK_FIRST 0x01

// Synchronization
#define TIMEOUT_SYNC_START 3600. // [second]


#define OPT_MAXDOMEII_HOMEAZ       OPT_LOCAL + 54
#define OPT_MAXDOMEII_TICKSPERREV  OPT_LOCAL + 55
#define OPT_MAXDOMEII_PARKAZ       OPT_LOCAL + 56
#define OPT_MAXDOMEII_SHUTTER_OP   OPT_LOCAL + 57

#define OPT_MAXDOMEII_XD     OPT_LOCAL + 60
#define OPT_MAXDOMEII_ZD     OPT_LOCAL + 61
#define OPT_MAXDOMEII_RDEC   OPT_LOCAL + 62
#define OPT_MAXDOMEII_RDOME  OPT_LOCAL + 63

#define OPT_MAXDOMEII_COLD_START          OPT_LOCAL + 70
#define OPT_MAXDOMEII_OPEN_LOWER_SHUTTER  OPT_LOCAL + 71
// we are on batteries, stop if now new target is selected
// e.g. in case teld dies
#define OPT_MAXDOMEII_TIMEOUT_SYNC_START  OPT_LOCAL + 72

/**
 * MaxDome II Sirius driver.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MaxDomeII:public Cupola
{
	public:
		MaxDomeII (int argc, char **argv);
		virtual ~MaxDomeII ();
                virtual double getSlitWidth (double alt); // not used here
                virtual bool needSlitChange ();
                virtual int standby ();
                virtual int off ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual int info ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();
	
		virtual int setValue (rts2core::Value * oldValue, rts2core::Value *newValue);

                virtual int moveStart () ;
                virtual int moveEnd () ;
                virtual long isMoving () ;
                virtual int moveStop () ;

		virtual int idle ();

	private:
		const char *devFile;
        	rts2core::ConnSerial *sconn;

		rts2core::ValueSelection *shutterState;

		/**
		 * Calculate message checksum. Works on full buffer, assuming
		 * the starting 0x01 is not included in it.
		 */
		signed char checkSum (char *msg, size_t len);

		int sendMessage (uint8_t cmd, char *args, size_t len);

		/**
		 * @param buffer minimal size must be at least MAX_BUFFER
		 */
                void readReply (uint8_t cmd, char *buffer);

		/**
		 *
		 */
		int exchangeMessage (uint8_t cmd, char *args, size_t len, char *buffer);

                rts2core::ValueSelection *shutterCmd;
                rts2core::ValueBool *block_shutter;
                rts2core::ValueInteger *ticksPerRevolution;
                rts2core::ValueDouble *manualAz;
                int GotoAzimuth(double newAZ);
                double TicksToAzimuth(int Ticks);
                int AzimuthToTicks(double Azimuth);
                rts2core::ValueDouble *homeAz;
                rts2core::ValueSelection *azCmd;
                int AzimuthCmd(uint8_t cmd);
		rts2core::ValueSelection *azState;
		rts2core::ValueInteger *homeTicks;
                int ParkPositionShutterOperation(int Ticks);
                rts2core::ValueDouble *parkAz;
                rts2core::ValueBool *shutterOperAtAnyAZ;
                double domeTargetAz( ln_equ_posn tel_equ);
                struct tk_geometry domeMountGeometry ;
                // see Toshimi Taki's paper: Matrix Method for Coodinates Transformation
                rts2core::ValueDouble *xd ;
                rts2core::ValueDouble *zd ;
                rts2core::ValueDouble *rdec ;
                rts2core::ValueDouble *rdome ;
                rts2core::ValueDouble *targetAzDifference ;
                int parkCupola ();
                rts2core::ValueBool *coldStart;
                rts2core::ValueBool *openLowerShutter;
  		time_t sync_start_time;
                rts2core::ValueDouble *timeoutSyncStart ;

};

MaxDomeII::MaxDomeII (int argc, char **argv):Cupola (argc, argv)
{
	devFile = "/dev/ttyUSB0";
	sconn = NULL;

	createValue (shutterState, "shutterState", "shutter state", false);
	shutterState->addSelVal ("CLOSED", (rts2core::Rts2SelData *) "CLOSED");
	shutterState->addSelVal ("OPENING", (rts2core::Rts2SelData *) "OPENING");
	shutterState->addSelVal ("OPEN", (rts2core::Rts2SelData *) "OPEN");
	shutterState->addSelVal ("CLOSING", (rts2core::Rts2SelData *) "CLOSING");
	shutterState->addSelVal ("ERROR1", (rts2core::Rts2SelData *) "ERROR1");
	shutterState->addSelVal ("ERROR2", (rts2core::Rts2SelData *) "ERROR2");

	createValue (azState, "azState", "dome azimuth state", false);
	azState->addSelVal ("NONE", (rts2core::Rts2SelData *) "NONE");
	azState->addSelVal ("SYNCED", (rts2core::Rts2SelData *) "SYNCED");
	azState->addSelVal ("MOVING_WTOE", (rts2core::Rts2SelData *) "MOVING_WTOE");
	azState->addSelVal ("MOVING_ETOW", (rts2core::Rts2SelData *) "MOVING_ETOW");
	azState->addSelVal ("SYNCED_CROSSED", (rts2core::Rts2SelData *) "SYNCED_CROSSED");
	azState->addSelVal ("ERROR_AZ", (rts2core::Rts2SelData *) "ERROR_AZ");

	createValue (block_shutter, "BLOCK_SHUTTER", "true inhibits rts2-centrald initiated shutter commands", false, RTS2_VALUE_WRITABLE);
	block_shutter->setValueBool (true); 

	createValue (shutterCmd, "MANUAL_SHUTTER", "manual shutter operation", false, RTS2_VALUE_WRITABLE);
	shutterCmd->addSelVal ("ABORT_SHUTTER");
	shutterCmd->addSelVal ("CLOSE_SHUTTER");
	shutterCmd->addSelVal ("OPEN_SHUTTER");
	shutterCmd->addSelVal ("OPEN_UPPER_ONLY_SHUTTER");
	shutterCmd->addSelVal ("EXIT_SHUTTER");

	createValue (openLowerShutter, "openLowerShutter", "true: lower shutter opened", false, RTS2_VALUE_WRITABLE);
	openLowerShutter->setValueBool (false); 

	createValue (azCmd, "MANUAL_AZ", "manual dome azimuth operation", false, RTS2_VALUE_WRITABLE);
	azCmd->addSelVal ("HOME", (rts2core::Rts2SelData *) HOME_CMD);
	azCmd->addSelVal ("ABORT", (rts2core::Rts2SelData *) ABORT_CMD);
	azCmd->addSelVal ("PARK", (rts2core::Rts2SelData *) PARK); //not a maxdomeii command
//ToDo:
#define NOTHING 255
	//ToDo:
	azCmd->addSelVal ("NOTHING", (rts2core::Rts2SelData *) NOTHING);

	createValue (manualAz, "AZsetpoint", "command the dome to azimuth", false, RTS2_VALUE_WRITABLE);
	manualAz->setValueDouble(0.);

	createValue (homeAz, "homeAZ", "home position azimuth", false, RTS2_VALUE_WRITABLE);
	homeAz->setValueDouble(343.);

	createValue (ticksPerRevolution, "ticks/revolution", "ticks per revolution, default 228", false, RTS2_VALUE_WRITABLE);
	ticksPerRevolution->setValueInteger(228);

	createValue (parkAz, "parkAZ", "azimuth park position ", false, RTS2_VALUE_WRITABLE);
	parkAz->setValueInteger(270.);

	createValue (homeTicks, "homeTicks", "ticks from maxdomeii status message", false);

	createValue (shutterOperAtAnyAZ, "shutterOperAtAnyAZ", "true: operate the shutter at any azimuth angle", false, RTS2_VALUE_WRITABLE);
	shutterOperAtAnyAZ->setValueBool (false); 

	createValue (xd, "xd", "xd", false);
	xd->setValueDouble(-.1016);
	createValue (zd, "zd", "zd", false);
	zd->setValueDouble(0.2032);
	createValue (rdec, "rdec", "distance mount pivot optical axis", false);
	rdec->setValueDouble(0.381);
	createValue (rdome, "rdome", "radius dome", false);
	rdome->setValueDouble(1.1684);

	createValue (targetAzDifference, "targeAzDifference", "maxdomeii azimuth difference", false);

	createValue (coldStart, "coldStart", "true: dome is homed before operation", false);
	coldStart->setValueBool (false); 

	createValue (timeoutSyncStart, "timeoutSync", "after that many seconds synchronization stops", false);
	timeoutSyncStart->setValueDouble(5400.);

	addOption ('f', NULL, 1, "serial port (defaults to /dev/ttyUSB0)");
	addOption (OPT_MAXDOMEII_HOMEAZ, "homeaz", 1, "home azimuth position");
	addOption (OPT_MAXDOMEII_TICKSPERREV, "ticksperrevolution", 1, "ticks per revolution");
	addOption (OPT_MAXDOMEII_PARKAZ, "parkaz", 1, "park azimuth position");
	addOption (OPT_MAXDOMEII_SHUTTER_OP, "shutteroperation", 0, "present: operate the shutter at any azimuth angle");
	addOption (OPT_MAXDOMEII_XD, "xd", 1, "xd, unit [m], see Toshimi Taki's paper");
	addOption (OPT_MAXDOMEII_ZD, "zd", 1, "zd, unit [m]");
	addOption (OPT_MAXDOMEII_RDEC, "rdec", 1, "distance mount pivot optical axis, unit [m]");
	addOption (OPT_MAXDOMEII_RDOME, "rdome", 1, "dome radius, unit [m]");
	addOption (OPT_MAXDOMEII_COLD_START, "cold-start", 0, "present: dome is homed before operation");
	addOption (OPT_MAXDOMEII_OPEN_LOWER_SHUTTER, "open-lower-shutter", 0, "present: lower and upper shutter are opened in unattended mode");
	addOption (OPT_MAXDOMEII_TIMEOUT_SYNC_START, "timeout-sync", 1, "after that many seconds synchronization stops if no new target has been selected");
}

MaxDomeII::~MaxDomeII ()
{
	delete sconn;
}

double MaxDomeII::getSlitWidth (double alt)
{
  return 1;
}
 
int MaxDomeII::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			devFile = optarg;

			break;
	        case OPT_MAXDOMEII_HOMEAZ:
		        homeAz->setValueCharArr(optarg);
	                break;
	        case OPT_MAXDOMEII_TICKSPERREV:
		        ticksPerRevolution->setValueCharArr(optarg);
	                break;
	        case OPT_MAXDOMEII_PARKAZ:
		        parkAz->setValueCharArr(optarg);
	                break;
	        case OPT_MAXDOMEII_SHUTTER_OP:
                	shutterOperAtAnyAZ->setValueBool (true); 
	                break;
	        case OPT_MAXDOMEII_XD:
		        xd->setValueCharArr(optarg);		        
	                break;
	        case OPT_MAXDOMEII_ZD:
		        zd->setValueCharArr(optarg);
	                break;
	        case OPT_MAXDOMEII_RDEC:
		        rdec->setValueCharArr(optarg);
	                break;
	        case OPT_MAXDOMEII_RDOME:
		        rdome->setValueCharArr(optarg);
	                break;
	        case OPT_MAXDOMEII_COLD_START:
                	coldStart->setValueBool (true); 
	                break;
	        case OPT_MAXDOMEII_OPEN_LOWER_SHUTTER:
                	openLowerShutter->setValueBool (true); 
	                break;
		default:
			return Dome::processOption (opt);
	}
	return 0;
}

int MaxDomeII::initHardware ()
{
        int ret;
        char args[MAX_BUFFER];

	sconn = new rts2core::ConnSerial (devFile, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, MAXDOME_TIMEOUT);
	sconn->setDebug (getDebug ());
	sconn->init ();

	// talk to the controller
	ret= exchangeMessage (ACK_CMD, NULL, 0, args);
	if(ret)
	  return ret;
	// set ticks per revolution
 	args[0] = (uint8_t)((uint16_t)ticksPerRevolution->getValueInteger ())>>8;
	args[1] = (uint8_t)((uint8_t)ticksPerRevolution->getValueInteger ());
	ret= exchangeMessage (TICKS_CMD, args, 2, args);
	if(ret)
	  return ret;

	// set park position and shutter operation mode depending on bool shutterOperAtAnyAZ
	int parkTicks= AzimuthToTicks((int)parkAz->getValueDouble()); //ToDo cast 
	ret= ParkPositionShutterOperation (parkTicks);
	if(ret)
	  return ret;

	domeMountGeometry.xd= xd->getValueDouble();
	domeMountGeometry.zd= zd->getValueDouble();
	domeMountGeometry.rdec= rdec->getValueDouble();
	domeMountGeometry.rdome= rdome->getValueDouble();

	if(coldStart->getValueBool()) {

	  ret= AzimuthCmd( HOME_CMD);
	  if(ret)
	    return ret;

	  while(( azState->getValueInteger()== MOVING_WTOE) || (azState->getValueInteger()== MOVING_ETOW)){
	    sleep(1);
	  }
	}
	// set start value for timeout
	time(&sync_start_time);
	return 0;
}

int MaxDomeII::info ()
{
        int ret;
	time_t now;
	char args[MAX_BUFFER];
	ret= exchangeMessage (STATUS_CMD, NULL, 0, args);
	if(ret)
	  return ret;

	shutterState->setValueInteger (args[2]);
        azState->setValueInteger (args[3]);

	double az= TicksToAzimuth( (((uint8_t)args[4])*256 + (uint8_t)args[5])) ;
	setCurrentAz (az); 
	// ToDo: what use case? it is always 227, 228
	homeTicks->setValueInteger( ((uint8_t)args[6]) *256 + (uint8_t)args[7]);

	struct ln_equ_posn tel_equ ;
        tel_equ.ra= getTargetRa() ;
        tel_equ.dec= getTargetDec() ;

        if(! (std::isnan (tel_equ.ra) || std::isnan (tel_equ.dec))){

           double target_az= domeTargetAz( tel_equ) ;
	   double targetDifference = getCurrentAz () - target_az;
	   //fprintf( stderr, "MaxDomeII::info difference AZ %f, abs %f, %f, RA %f %f\n", targetDifference, getCurrentAz (), target_az, tel_equ.ra, tel_equ.dec);
	   targetAzDifference->setValueDouble( targetDifference);
	}
	if(( sync_start_time - time(&now) + TIMEOUT_SYNC_START) < 0.) {
	  return moveStop();
	}
	return Cupola::info ();
}

int MaxDomeII::idle ()
{
        info();
        return Cupola::idle ();
}
int MaxDomeII::startOpen ()
{
        int ret ;
        if( block_shutter->getValueBool()) {
	  logStream (MESSAGE_ERROR) << "MaxDomeII::startOpen blocked shutter opening" << sendLog ;
	  return -1;
	}  

	char args[MAX_BUFFER];
	args[0] = OPEN_SHUTTER;

	if(openLowerShutter->getValueBool()) {
	  args[0] = OPEN_SHUTTER;
	} else {
	  args[0] = OPEN_UPPER_ONLY_SHUTTER;
	}
	ret= exchangeMessage (SHUTTER_CMD, args, 1, args);
	if (ret)
	  return ret;

	return 0;
}

long MaxDomeII::isOpened ()
{
	if (info ())
		return -1;

        if( block_shutter->getValueBool()) {
	  logStream (MESSAGE_ERROR) << "MaxDomeII::isOpened blocked shutter opening" << sendLog ;
	  return -1;
        }  

	return shutterState->getValueInteger () == 1 ? -2 : USEC_SEC;
}

int MaxDomeII::endOpen ()
{
	return 0;
}

int MaxDomeII::startClose ()
{
        int ret ;
        if( block_shutter->getValueBool()) {
	  logStream (MESSAGE_ERROR) << "MaxDomeII::startClosed blocked shutter closing" << sendLog ;
	  return -1;
	}  

	char args[MAX_BUFFER];
	args[0] = CLOSE_SHUTTER;

	ret= exchangeMessage (SHUTTER_CMD, args, 1, args);
	if (ret)
	  return ret;
	return 0;
}

long MaxDomeII::isClosed ()
{
	if (info ())
		return -1;
	if( block_shutter->getValueBool()) {
	  return -2;
	}  

	return shutterState->getValueInteger () == 0 ? -2 : USEC_SEC;
}

int MaxDomeII::endClose ()
{
	return 0;
}

signed char MaxDomeII::checkSum (char *msg, size_t len)
{
	char nChecksum = 0;

	for (size_t i = 0; i < len; i++){
		nChecksum -= msg[i];
	}
	return nChecksum;
}

int MaxDomeII::sendMessage (uint8_t cmd, char *args, size_t len)
{
        int ret ;
	char wbuf[MAX_BUFFER];
	wbuf[0] = 0x01;
	wbuf[1] = (uint8_t)(len + 2);
	wbuf[2] = cmd;    
	for ( int j=0; j < (int) len; j++){
	  wbuf[3 + j]= (uint8_t)args[j];
	}
	wbuf[3+ len] = checkSum(wbuf+1, len+ 2);

	// fprintf(stderr, ">sendMessagebuffer write len %ld ---0x", (long int)len+4); //ToDo ask Petr why changed to ld, goes away anyay
	// for ( int j=0; j < (int) len+4; j++){
	//   fprintf(stderr, "%d: %02x\n", j, (uint8_t)wbuf[j] );
	// }
	// fprintf(stderr, " end buffer\n");

	ret= sconn->writePort ((const char *)wbuf, len+4); //ToDo: cast
	if (ret)
	  return ret;

	return 0;
}

void MaxDomeII::readReply (uint8_t cmd, char *buffer)
{
	char c;
	while (true)
	{
	  sconn->readPort (c);
	  if (c == START)
			break;
	}
	// read length
	if (sconn->readPort (c) < 0)
		throw rts2core::Error ("cannot read length of the message");
	if (c < 0x02 || c > 0x0E)
		throw rts2core::Error ("invalid length of the packet");

	buffer[0] = c;

	int nb= (int) buffer[0] ;
	int ret ;

	if (( ret=sconn->readPort (buffer + 1, nb)) !=  nb)
		throw rts2core::Error ("cannot read full packet");
       
	if (checkSum (buffer, nb+1) != 0)
	  	throw rts2core::Error ("invalid checksum");

	//buffer is char*
	if (((uint8_t)buffer[1]) != (cmd | TO_COMPUTER))
		throw rts2core::Error ("invalid reply - invalid command byte");

}

int MaxDomeII::exchangeMessage (uint8_t cmd, char *args, size_t len, char *buffer)
{
        int ret= 0;

	ret= sendMessage (cmd, args, len);
     	if (ret)
	  return ret;

	try
	{
	  readReply (cmd, buffer);
        }
        catch(rts2core::Error err) 
        {
	  logStream (MESSAGE_ERROR)<<"MaxDomeII::exchangeMessage read error:" << err << sendLog;
	  return -1;
        }
	return 0 ;
}

int MaxDomeII::setValue (rts2core::Value * oldValue, rts2core::Value *newValue)
{
       int ret;
       char args[MAX_BUFFER];
       if (oldValue ==  shutterCmd)
       {
	 shutterCmd->setValueInteger (newValue->getValueInteger ());

	 switch ((size_t)shutterCmd->getValueInteger ())
	 {
		case SEL_ABORT_SHUTTER:
			args[0]= ABORT_SHUTTER;
			break;
		case SEL_CLOSE_SHUTTER:
			args[0]= CLOSE_SHUTTER;
			break;
		case SEL_OPEN_SHUTTER:
			args[0]= OPEN_SHUTTER;
			break;
		case SEL_OPEN_UPPER_ONLY_SHUTTER:
			args[0]= OPEN_UPPER_ONLY_SHUTTER;
			break;
		case SEL_EXIT_SHUTTER:
		        args[0]= EXIT_SHUTTER;
			break;
		default:
			args[0]= CLOSE_SHUTTER;
	 }
	 ret= exchangeMessage (SHUTTER_CMD, args, 1, args);
	 if(ret)
	   return ret;

       } else if(oldValue ==  ticksPerRevolution) {

	 ticksPerRevolution->setValueInteger (newValue->getValueInteger ());
	 args[0] = ((uint16_t)ticksPerRevolution->getValueInteger ())>>8;
	 args[1] = ((uint8_t)ticksPerRevolution->getValueInteger ());

	 ret= exchangeMessage (TICKS_CMD, args, 2, args);
	 if(ret)
	   return ret;

       } else if(oldValue ==  manualAz) {

	 manualAz->setValueDouble (newValue->getValueDouble ());
	 ret= GotoAzimuth(manualAz->getValueDouble ());
	 if(ret)
	   return ret;

       } else if(oldValue ==  azCmd) {

	 azCmd->setValueInteger (newValue->getValueInteger ());

	 if( (size_t) azCmd->getValueInteger ()== SEL_HOME){
	   ret= AzimuthCmd( HOME_CMD);
	   if(ret)
	     return ret;
	 } else if( (size_t) azCmd->getValueInteger ()== SEL_ABORT){
	   ret= AzimuthCmd( ABORT_CMD);
	   if(ret)
	     return ret;

	 } else if( (size_t) azCmd->getValueInteger ()== SEL_PARK){
	   ret= parkCupola ();
	   if(ret)
	     return ret;

	 } else{
	   // ToDo 
	   fprintf( stderr, "NOTHING\n");
	 }
       }
       return 0 ;
}

int MaxDomeII::GotoAzimuth(double newAZ)
{
       int ret ;
       double currAZ = 0;
       int newTicks = 0, nDir = 0xff;
       char args[MAX_BUFFER];

       currAZ = getCurrentAz();

       // Take the shortest path                                                                                              
       if (newAZ > currAZ)
       {
	 if (newAZ - currAZ > 180.0)
	   nDir = MAXDOMEII_WE_DIR;
	 else
	   nDir = MAXDOMEII_EW_DIR;
       }
       else
       {
	 if (currAZ - newAZ > 180.0)
	   nDir = MAXDOMEII_EW_DIR;
	 else
	   nDir = MAXDOMEII_WE_DIR;
       }
       newTicks = AzimuthToTicks(newAZ);

       args[0] = (char)nDir;
       args[1] = ((uint16_t)newTicks)>>8 ;
       args[2] = (uint8_t)(newTicks);

       ret= exchangeMessage (GOTO_CMD, args, 3, args);
       if (ret)
	 return ret;

       return 0;
}

double MaxDomeII::TicksToAzimuth(int Ticks)
{
       double nAz=0.;
       nAz = homeAz->getValueDouble() + Ticks * 360.0 / ticksPerRevolution->getValueInteger();
       //ToD why not modulo
       while (nAz < 0) nAz += 360;
       while (nAz >= 360) nAz -= 360;

       return nAz;
}

int MaxDomeII::AzimuthToTicks(double Azimuth)
{
       int Ticks=0.;

       Ticks = floor(0.5 + (Azimuth - homeAz->getValueDouble()) * ticksPerRevolution->getValueInteger() / 360.0);
       //ToD why not modulo
       while (Ticks > ticksPerRevolution->getValueInteger()) Ticks -= ticksPerRevolution->getValueInteger();
       while (Ticks < 0) Ticks += ticksPerRevolution->getValueInteger();

       return Ticks;
}

int MaxDomeII::AzimuthCmd(uint8_t cmd)
{
       int ret;
       char args[MAX_BUFFER];
       ret= exchangeMessage (cmd, NULL, 0, args);
       if(ret)
	 return ret;
       return 0;
}

int MaxDomeII::ParkPositionShutterOperation (int Ticks)
{
       int ret ;
       char args[MAX_BUFFER];

       if( shutterOperAtAnyAZ->getValueBool()){
	 args[0]=  OPERATE_AT_ANY_AZIMUTH; 
       } else {
	 args[0]=  PARK_FIRST; 
       }
       args[1] = (char)(Ticks / 256);
       args[2] = (char)(Ticks % 256);

       ret= exchangeMessage (SETPARK_CMD, args, 3, args);
       if (ret)
	 return ret;

       return 0;
}
int MaxDomeII::moveEnd ()
{
  return Cupola::moveEnd ();
}
int MaxDomeII::moveStop ()
{
  return Cupola::moveStop ();
}
long MaxDomeII::isMoving ()
{
       //ToDo see SYNCED_CROSSED
       if (( azState->getValueInteger()== SYNCED) || (azState->getValueInteger()== SYNCED_CROSSED)) { 
	 logStream (MESSAGE_ERROR) << "MaxDomeII::isMoving SYNCED"<< sendLog ;
	 return -2;
       } else {
	 logStream (MESSAGE_ERROR) << "MaxDomeII::isMoving not yet there"<< sendLog ;
       }
       return USEC_SEC;
}

bool MaxDomeII::needSlitChange ()
{
       int ret;
       double slitWidth;
       double targetDifference ;

       struct ln_equ_posn tel_equ ;
       tel_equ.ra= getTargetRa() ;
       tel_equ.dec= getTargetDec() ;

       if (std::isnan (tel_equ.ra) || std::isnan (tel_equ.dec)){
	 return false;
       }
       double target_az= domeTargetAz( tel_equ) ;

       slitWidth = getSlitWidth (1.);
       if (slitWidth < 0) // always false
	 return false;
       // get current az                                                                                                      
       ret = info ();
       if (ret)
	 return false;

       // simple check; can be repleaced by some more complicated for more complicated setups                                 
       targetDifference = getCurrentAz () - target_az;
       if (targetDifference > 180)
	 targetDifference = (targetDifference - 360);
       else if (targetDifference < -180)
	 targetDifference = (targetDifference + 360);

       if (fabs (targetDifference) < slitWidth)
       {
	 if ((getState () & DOME_CUP_MASK_SYNC) == DOME_CUP_NOT_SYNC)
	   Cupola::synced ();
	 return false;
       }
       return true;
}

int MaxDomeII::moveStart ()
{
       int ret;
       static double lastRa=-9999., lastDec=-9999. ;

       ret = needSlitChange ();
       if (ret == 0 || ret == -1)
	 return ret; // pretend we change..so other devices can sync on our command        

       // needSlitChnage() ensures not nan values
       struct ln_equ_posn tel_equ ;
       tel_equ.ra= getTargetRa() ;
       tel_equ.dec= getTargetDec() ;

       double target_az= domeTargetAz( tel_equ) ;

       if((ret= GotoAzimuth(target_az))==0)
       {
	 return 0; //Cupola::moveStart () returns 0 
       }
       if(( lastRa != tel_equ.ra)||( lastDec != tel_equ.dec)) {
	 lastRa = tel_equ.ra ;
	 lastDec = tel_equ.dec ;
	 time(&sync_start_time);
       }
       return Cupola::moveStart ();
}

double MaxDomeII::domeTargetAz( ln_equ_posn tel_equ)
{
      struct ln_lnlat_posn obs_location ;
      struct ln_lnlat_posn  *obs_loc_tmp= Cupola::getObserver() ;

      obs_location.lat= obs_loc_tmp->lat;
      obs_location.lng= obs_loc_tmp->lng;

      // dome slit azimuth as function of telescope position and dome/mount geometry 
      return TK_dome_target_az( tel_equ, obs_location, domeMountGeometry) ; 
}

int MaxDomeII::parkCupola ()
{
  int ret;
  // parkCupola is called before cold-start has completed 
  if ( !(( azState->getValueInteger()== MOVING_WTOE) || (azState->getValueInteger()== MOVING_ETOW))){
    ret= GotoAzimuth(parkAz->getValueDouble());
    if(ret)
      return ret;
  }
  return 0;
}

int MaxDomeII::standby ()
{
  parkCupola ();
  return Cupola::standby ();
}


int MaxDomeII::off ()
{
  parkCupola ();
  return Cupola::off ();
}

int main (int argc, char **argv)
{
	MaxDomeII device (argc, argv);
	return device.run ();
}
