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
#include "dome-target-az.h"

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

// Shutter commands
#define OPEN_SHUTTER            0x01
#define OPEN_UPPER_ONLY_SHUTTER 0x02
#define CLOSE_SHUTTER           0x03
#define EXIT_SHUTTER            0x04  // Command send to shutter on program exit
#define ABORT_SHUTTER           0x07

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

// Shutter operation
#define OPERATE_AT_ANY_AZIMUTH 0x00
#define PARK_FIRST 0x01

// Synchronization
//ToDo create options

#define OPT_MAXDOMEII_HOMEAZ       OPT_LOCAL + 54
#define OPT_MAXDOMEII_TICKSPERREV  OPT_LOCAL + 55
#define OPT_MAXDOMEII_PARKAZ       OPT_LOCAL + 56
#define OPT_MAXDOMEII_SHUTTER_OP   OPT_LOCAL + 57

#define OPT_MAXDOMEII_XD     OPT_LOCAL + 60
#define OPT_MAXDOMEII_ZD     OPT_LOCAL + 61
#define OPT_MAXDOMEII_RDEC   OPT_LOCAL + 62
#define OPT_MAXDOMEII_RDOME  OPT_LOCAL + 63

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

		virtual double getSplitWidth (double alt);
	
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

		rts2core::ValueInteger *shutterState;

		/**
		 * Calculate message checksum. Works on full buffer, assuming
		 * the starting 0x01 is not included in it.
		 */
		signed char checkSum (char *msg, size_t len);

		void sendMessage (uint8_t cmd, char *args, size_t len);

		/**
		 * @param buffer minimal size must be at least MAX_BUFFER
		 */
		void readReply (char *buffer);

		/**
		 *
		 */
		void exchangeMessage (uint8_t cmd, char *args, size_t len, char *buffer);

                rts2core::ValueSelection *shutterCmd;
                rts2core::ValueBool *block_shutter;
                rts2core::ValueInteger *ticksPerRevolution;
                rts2core::ValueDouble *manualAz;
                int GotoAzimuth(double newAZ);
                double TicksToAzimuth(int Ticks);
                int AzimuthToTicks(double Azimuth);
                rts2core::ValueDouble *homeAz;
                rts2core::ValueSelection *azCmd;
                void AzimuthCmd(uint8_t cmd);
		rts2core::ValueInteger *azState;
		rts2core::ValueInteger *homeTicks;
                int ParkPositionShutterOperation(int Ticks);
                rts2core::ValueDouble *parkAz;
                rts2core::ValueBool *shutterOperationAtAnyAZ;
                bool needMoveStart();
                double domeTargetAz( ln_equ_posn tel_equ);
                struct geometry domeMountGeometry ;
                rts2core::ValueDouble *xd ;
                rts2core::ValueDouble *zd ;
                rts2core::ValueDouble *rdec ;
                rts2core::ValueDouble *rdome ;

};

MaxDomeII::MaxDomeII (int argc, char **argv):Cupola (argc, argv)
{
	devFile = "/dev/ttyS0";
	sconn = NULL;

	createValue (shutterState, "shutter state", "shutter state", false);
	createValue (azState, "az state", "dome azimuth state", false);

	addOption ('f', NULL, 1, "serial port (defaults to /dev/ttyS0)");


	createValue (block_shutter, "BLOCK_SHUTTER", "true inhibits rts2-centrald initiated shutter commands", false, RTS2_VALUE_WRITABLE);
	block_shutter->setValueBool (true); 

	createValue (shutterCmd, "MANUAL_SHUTTER", "manual shutter operation", false, RTS2_VALUE_WRITABLE);
	shutterCmd->addSelVal ("ABORT_SHUTTER", (rts2core::Rts2SelData *) ABORT_SHUTTER);
	shutterCmd->addSelVal ("CLOSE_SHUTTER", (rts2core::Rts2SelData *) CLOSE_SHUTTER);
	shutterCmd->addSelVal ("OPEN_SHUTTER", (rts2core::Rts2SelData *) OPEN_SHUTTER);
	shutterCmd->addSelVal ("OPEN_UPPER_ONLY_SHUTTER", (rts2core::Rts2SelData *) OPEN_UPPER_ONLY_SHUTTER);
	shutterCmd->addSelVal ("EXIT_SHUTTER", (rts2core::Rts2SelData *) EXIT_SHUTTER);

	createValue (azCmd, "MANUAL_AZ", "manual dome azimuth operation", false, RTS2_VALUE_WRITABLE);
	azCmd->addSelVal ("HOME", (rts2core::Rts2SelData *) HOME_CMD);
	azCmd->addSelVal ("ABORT", (rts2core::Rts2SelData *) ABORT_CMD);
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

	createValue (shutterOperationAtAnyAZ, "shutterOperationAtAnyAZ", "park on shutter close", false, RTS2_VALUE_WRITABLE);
	shutterOperationAtAnyAZ->setValueBool (false); 


	addOption (OPT_MAXDOMEII_HOMEAZ, "homeaz", 1, "home azimuth position");
	addOption (OPT_MAXDOMEII_TICKSPERREV, "ticksperrevolution", 1, "ticks per revolution");
	addOption (OPT_MAXDOMEII_PARKAZ, "parkaz", 1, "park azimuth position");
	addOption (OPT_MAXDOMEII_SHUTTER_OP, "shutteroperation", 1, "true: operate the shutter at any azimuth angle");
	addOption (OPT_MAXDOMEII_XD, "xd", 1, "xd, see Toshimi Taki's paper");
	addOption (OPT_MAXDOMEII_ZD, "zd", 1, "zd");
	addOption (OPT_MAXDOMEII_RDEC, "rdec", 1, "distance mount pivot optical axis");
	addOption (OPT_MAXDOMEII_RDOME, "rdome", 1, "dome radius");


}

MaxDomeII::~MaxDomeII ()
{
	delete sconn;
}
// ToDo, do I need it?
double MaxDomeII::getSplitWidth (double alt)
{
	return 10;
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
                	shutterOperationAtAnyAZ->setValueBool (false); 
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

		default:
			return Dome::processOption (opt);
	}
	return 0;
}

int MaxDomeII::initHardware ()
{
        char args[MAX_BUFFER];

	sconn = new rts2core::ConnSerial (devFile, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, MAXDOME_TIMEOUT);
	sconn->setDebug (getDebug ());
	sconn->init ();

	// talk to the controller
	exchangeMessage (ACK_CMD, NULL, 0, args);
	// set ticks per revolution
 	args[0] = (uint8_t)((uint16_t)ticksPerRevolution->getValueInteger ())>>8;
	args[1] = (uint8_t)((uint8_t)ticksPerRevolution->getValueInteger ());
	exchangeMessage (TICKS_CMD, args, 2, args);
	// set park position and shutter operation mode
	int parkTicks= AzimuthToTicks((int)parkAz->getValueDouble()); //ToDo cast 
	ParkPositionShutterOperation (parkTicks);
	//ToDo n0t the place
	domeMountGeometry.xd= xd->getValueDouble();
	domeMountGeometry.zd= zd->getValueDouble();
	domeMountGeometry.rdec= rdec->getValueDouble();
	domeMountGeometry.rdome= rdome->getValueDouble();


	return 0;
}

int MaxDomeII::info ()
{
	char args[MAX_BUFFER];
	exchangeMessage (STATUS_CMD, NULL, 0, args);

	shutterState->setValueInteger (args[2]);
        azState->setValueInteger (args[3]);

	double az= TicksToAzimuth( (((uint8_t)args[4])*256 + (uint8_t)args[5])) ;
	setCurrentAz (az); 

	homeTicks->setValueInteger( ((uint8_t)args[6]) *256 + (uint8_t)args[7]);// ToDo: what use case

	// fprintf(stderr, "----------ticks %d, off %d\n",  ((uint8_t)args[4])*256 + (uint8_t)args[5], homeTicks->getValueInteger());
	// fprintf(stderr, "----------az %lf, azoff: %lf\n", az, homeAz->getValueDouble() );
	// fprintf(stderr, ">info buffer read ---0x");

	// for ( int j=0; j < 8; j++){
	//   fprintf(stderr, "%02x", (uint8_t)args[j] );
	// }
	// fprintf(stderr, " end buffer\n");
	// fprintf(stderr, "section target\n");
	
	fprintf(stderr, "tarRa %f, tarDec %f\n", getTargetRa(), getTargetDec());


	return Cupola::info ();
}
int MaxDomeII::idle ()
{
  //no needSlitChange here, so emmulate it
  long ret;
  if ((getState () & DOME_CUP_MASK_MOVE) == DOME_CUP_MOVE)
  {
      ret = isMoving ();
      if (ret >= 0)
	setTimeout (ret);
      else
      {
	  infoAll ();
	  if (ret == -2)
	    moveEnd ();
	  else
	    moveStop ();
      }
  }
  else if (needMoveStart () > 0)
  {
      moveStart ();
      setTimeout (USEC_SEC);
  }
  else
  {
      setTimeout (10 * USEC_SEC);
  }
  return Dome::idle ();

}

int MaxDomeII::startOpen ()
{
        if( block_shutter->getValueBool()) {
	  logStream (MESSAGE_ERROR) << "MaxDomeII::startOpen blocked shutter opening" << sendLog ;
	  return -1;
	}  

	char args[MAX_BUFFER];
	args[0] = OPEN_SHUTTER;

	exchangeMessage (SHUTTER_CMD, args, 1, args);
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
        if( block_shutter->getValueBool()) {
	  logStream (MESSAGE_ERROR) << "MaxDomeII::startClosed blocked shutter closing" << sendLog ;
	  return -1;
	}  

	char args[MAX_BUFFER];
	args[0] = CLOSE_SHUTTER;

	exchangeMessage (SHUTTER_CMD, args, 1, args);
	return 0;
}

long MaxDomeII::isClosed ()
{
	if (info ())
		return -1;
	if( block_shutter->getValueBool()) {
	  //logStream (MESSAGE_DEBUG) << "MaxDomeII::isClosed blocked shutter closing, returning -2 (is closed)" << sendLog ;
	  return -2;
	}  

	return shutterState->getValueInteger () == 0 ? -2 : USEC_SEC;
}

int MaxDomeII::endClose ()
{
	return 0;
}
//ToDo: was size_t i = 0
signed char MaxDomeII::checkSum (char *msg, size_t len)
{
	char nChecksum = 0;
    // fprintf(stderr, ">checksum buffer write ---0x");

    // for ( int j=0; j < (int) len; j++){
    //    fprintf(stderr, "%02x", (uint8_t)msg[j] );
    //  }
     // fprintf(stderr, " end buffer\n");

	for (size_t i = 0; i < len; i++){
		nChecksum -= msg[i];
	}
	return nChecksum;
}
//ToDo: was
void MaxDomeII::sendMessage (uint8_t cmd, char *args, size_t len)
{
	// sconn->writePort (START);
	// sconn->writePort ((uint8_t)(len + 2));
	// sconn->writePort (cmd);
	// logStream (MESSAGE_ERROR) << "MaxDomeII::send :"<< args  << sendLog ;

	// if (args != NULL)
	// {
	// 	sconn->writePort (args, len);
	// 	sconn->writePort (checkSum (args, len) - cmd - (len + 2));
	// }
	// else
	// {
	// 	// checksum calculation in-place
	// 	sconn->writePort (0 - cmd - (len + 2));
	// }
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

    sconn->writePort ((const char *)wbuf, len+4); //ToDo: cast
}

void MaxDomeII::readReply (char *buffer)
{

	char c;
	while (true)
	{
	  //if (sconn->readPort (c) < 0)
	  //  throw rts2core::Error ("cannot read start character from serial port");
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
	//ToDo was
	// if (sconn->readPort (buffer + 1, c))
	// 	throw rts2core::Error ("cannot read full packet");
	int nb= (int) buffer[0] ;
	int ret ;

	if (( ret=sconn->readPort (buffer + 1, nb)) !=  nb)
		throw rts2core::Error ("cannot read full packet");
       
	//ToDo was
	// if (checkSum (buffer, c + 1) != 0)
	// 	throw rts2core::Error ("invalid checksum");
	if (checkSum (buffer, nb+1) != 0)
	  	throw rts2core::Error ("invalid checksum");
}

void MaxDomeII::exchangeMessage (uint8_t cmd, char *args, size_t len, char *buffer)
{
	sendMessage (cmd, args, len);
	sleep(1);
	readReply (buffer);
	//buffer is char*
	if (((uint8_t)buffer[1]) != (cmd | TO_COMPUTER))
		throw rts2core::Error ("invalid reply - invalid command byte");
}


int MaxDomeII::setValue (rts2core::Value * oldValue, rts2core::Value *newValue)
{
  char args[MAX_BUFFER];
  if (oldValue ==  shutterCmd)
  {
    shutterCmd->setValueInteger (newValue->getValueInteger ());
    args[0] = (uint8_t)shutterCmd->getValueInteger ();

    //    exchangeMessage (SHUTTER_CMD, args, 1, args);
  } else if(oldValue ==  ticksPerRevolution) {

    ticksPerRevolution->setValueInteger (newValue->getValueInteger ());
    args[0] = ((uint16_t)ticksPerRevolution->getValueInteger ())>>8;
    args[1] = ((uint8_t)ticksPerRevolution->getValueInteger ());
    //fprintf(stderr, "args %02x, %02x\n", args[0], args[1]);
    exchangeMessage (TICKS_CMD, args, 2, args);

  } else if(oldValue ==  manualAz) {

    manualAz->setValueDouble (newValue->getValueDouble ());
    GotoAzimuth(manualAz->getValueDouble ());

  } else if(oldValue ==  azCmd) {

    azCmd->setValueInteger (newValue->getValueInteger ());

    if( (size_t) azCmd->getValueInteger ()== SEL_HOME){
      fprintf( stderr, "HOME\n");
      AzimuthCmd( HOME_CMD);
    } else if( (size_t) azCmd->getValueInteger ()== SEL_ABORT){
      fprintf( stderr, "ABORT\n");
      AzimuthCmd( ABORT_CMD);
    } else{
      // ToDo 
      fprintf( stderr, "NOTHING\n");
    }
 

  }

  return 0 ;
}
int MaxDomeII::GotoAzimuth(double newAZ)
{
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
  fprintf(stderr, " Azimut %f, %d", newAZ, newTicks);

  args[0] = (char)nDir;
  args[1] = ((uint16_t)newTicks)>>8 ;
  args[2] = (uint8_t)(newTicks);

  exchangeMessage (GOTO_CMD, args, 3, args);

  return 0; //ToDo
}

double MaxDomeII::TicksToAzimuth(int Ticks)
{
  double nAz=0.;
  //ToDo nAz = homeAzSetPoint->getValueDouble() + Ticks * 360.0 / ticksPerRevolution->getValueInteger();
  nAz = homeAz->getValueDouble() + Ticks * 360.0 / ticksPerRevolution->getValueInteger();
  //ToD why not modulo
  while (nAz < 0) nAz += 360;
  while (nAz >= 360) nAz -= 360;

  return nAz;
}
int MaxDomeII::AzimuthToTicks(double Azimuth)
{
  int Ticks=0.;

  //ToDo Ticks = floor(0.5 + (Azimuth - homeAzSetPoint->getValueDouble()) * ticksPerRevolution->getValueInteger() / 360.0);
  Ticks = floor(0.5 + (Azimuth - homeAz->getValueDouble()) * ticksPerRevolution->getValueInteger() / 360.0);
  //ToD why not modulo
  while (Ticks > ticksPerRevolution->getValueInteger()) Ticks -= ticksPerRevolution->getValueInteger();
  while (Ticks < 0) Ticks += ticksPerRevolution->getValueInteger();

  return Ticks;
}

void MaxDomeII::AzimuthCmd(uint8_t cmd)
{
  char args[MAX_BUFFER];
  exchangeMessage (cmd, NULL, 0, args);
}

int MaxDomeII::ParkPositionShutterOperation (int Ticks)
{
  char args[MAX_BUFFER];

  if( shutterOperationAtAnyAZ->getValueBool()){
    args[0]=  OPERATE_AT_ANY_AZIMUTH; 
  } else {
    args[0]=  PARK_FIRST; 
  }
  args[1] = (char)(Ticks / 256);
  args[2] = (char)(Ticks % 256);

  exchangeMessage (SETPARK_CMD, args, 3, args);

  //ToDo
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
  if (( azState->getValueInteger()== SYNCED) || (azState->getValueInteger()== SYNCED_CROSSED)) { //ToDo see SYNCED_CROSSED
    logStream (MESSAGE_ERROR) << "MaxDomeII::isMoving SYNCED"<< sendLog ;
    return -2;
  } else {
    logStream (MESSAGE_ERROR) << "MaxDomeII::isMoving not yet there"<< sendLog ;
  }
  return USEC_SEC;
}

// needSlitChange () checks TargetAltAz without dome geometry so emmulate it
// doing my best to sperate code, the current stems from the fact that originally
// needSlitChange is used to decide if moveStart should be called. That means
// using needSlitChange twice.
bool MaxDomeII::needMoveStart()
{
        struct ln_equ_posn tel_equ ;
	double targetDifference ;

        tel_equ.ra= getTargetRa() ;
        tel_equ.dec= getTargetDec() ;

        if (isnan (tel_equ.ra) || isnan (tel_equ.dec))
           return false;

	double target_az= domeTargetAz( tel_equ) ;

	targetDifference = getCurrentAz () - target_az;
	fprintf( stderr, "difference %f, abs %f, %f\n", targetDifference, getCurrentAz (), target_az);
        if (targetDifference > 180)
	  targetDifference = (targetDifference - 360);
        else if (targetDifference < -180)
	  targetDifference = (targetDifference + 360);


        if (fabs (targetDifference) < AZ_MINIMAL_STEP)
	{
	  if ((getState () & DOME_CUP_MASK_SYNC) == DOME_CUP_NOT_SYNC){
	    infoAll ();
	    maskState (DOME_CUP_MASK_SYNC | BOP_EXPOSURE, DOME_CUP_SYNC);
	  }
	  return false;
        }

	return true;
}
double MaxDomeII::domeTargetAz( ln_equ_posn tel_equ)
{
  struct ln_lnlat_posn obs_location ;

  struct ln_lnlat_posn  *obs_loc_tmp= Cupola::getObserver() ;
  obs_location.lat= obs_loc_tmp->lat;
  obs_location.lng= obs_loc_tmp->lng;


  // az setpoint as function of dome geometry 
  return dome_target_az( tel_equ, obs_location, domeMountGeometry) ; //ToDo replace astrofire
}

int MaxDomeII::moveStart ()
{
	int ret;

	if(needMoveStart()){

           struct ln_equ_posn tel_equ ;
           tel_equ.ra= getTargetRa() ;
           tel_equ.dec= getTargetDec() ;
           if (isnan (tel_equ.ra) || isnan (tel_equ.dec))
	     return 1; // true

           double target_az= domeTargetAz( tel_equ) ;
	   //ToDo check the az state
	   if((ret= GotoAzimuth(target_az))==0)
	   {
	       return 0;
	   }
	   fprintf(stderr, "---------------------------START: %f %f %f", tel_equ.ra, tel_equ.dec, target_az);
        } else {

	  return 1 ; // true
	}
        return Cupola::moveStart ();
}

int main (int argc, char **argv)
{
	MaxDomeII device (argc, argv);
	return device.run ();
}
