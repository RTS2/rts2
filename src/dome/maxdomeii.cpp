/*
    Max Dome II Driver
    Copyright (C) 2009 Ferran Casarramona (ferran.casarramona@gmail.com)
    Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>

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

#define MAXDOME_TIMEOUT	5		/* FD timeout in seconds */

// Start byte
#define START       0x01
#define MAX_BUFFER  15

// Message Destination
#define TO_MAXDOME  0x00
#define TO_COMPUTER 0x80

// Commands available
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



// Direction fo azimuth movement                                                                                               
#define MAXDOMEII_EW_DIR 0x01
#define MAXDOMEII_WE_DIR 0x02

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

	private:
		const char *devFile;
        	rts2core::ConnSerial *sconn;

		rts2core::ValueInteger *state;

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
                double TicksToAzimuth(int nTicks);
                int AzimuthToTicks(double nAzimuth);
                rts2core::ValueDouble *homeAzSetPoint;
                rts2core::ValueDouble *homeAzOffset;
                rts2core::ValueSelection *azCmd;
                int Home_Azimuth_MaxDomeII();


};

MaxDomeII::MaxDomeII (int argc, char **argv):Cupola (argc, argv)
{
	devFile = "/dev/ttyS0";
	sconn = NULL;

	createValue (state, "shutter", "shutter state", false);

	addOption ('f', NULL, 1, "serial port (defaults to /dev/ttyS0)");

	createValue (shutterCmd, "MANUAL_SHUTTER", "manual door operation", false, RTS2_VALUE_WRITABLE);
	shutterCmd->addSelVal ("ABORT_SHUTTER", (rts2core::Rts2SelData *) ABORT_SHUTTER);
	shutterCmd->addSelVal ("CLOSE_SHUTTER", (rts2core::Rts2SelData *) CLOSE_SHUTTER);
	shutterCmd->addSelVal ("OPEN_SHUTTER", (rts2core::Rts2SelData *) OPEN_SHUTTER);
	shutterCmd->addSelVal ("OPEN_UPPER_ONLY_SHUTTER", (rts2core::Rts2SelData *) OPEN_UPPER_ONLY_SHUTTER);
	shutterCmd->addSelVal ("EXIT_SHUTTER", (rts2core::Rts2SelData *) EXIT_SHUTTER);

	createValue (block_shutter, "BLOCK_SHUTTER", "true inhibits rts2-centrald initiated shutter movements", false, RTS2_VALUE_WRITABLE);
	block_shutter->setValueBool (true); 

	createValue (ticksPerRevolution, "ticks/revolution", "ticks perrevolution, default 228", false, RTS2_VALUE_WRITABLE);
	ticksPerRevolution->setValueInteger(228);

	createValue (manualAz, "AZsetpoint", "command the dome to azimuth", false, RTS2_VALUE_WRITABLE);
	manualAz->setValueDouble(0.);

	createValue (homeAzSetPoint, "homeAZsetpoint", "home azimuth", false, RTS2_VALUE_WRITABLE);
	homeAzSetPoint->setValueDouble(0.);

	createValue (homeAzOffset, "homeAZOffset", "home azimuth offset", false);
	homeAzOffset->setValueDouble(0.);

	createValue (azCmd, "MANUAL_AZ", "manual dome azimuth operation", false, RTS2_VALUE_WRITABLE);
	azCmd->addSelVal ("HOME", (rts2core::Rts2SelData *) HOME_CMD);
//ToDo:
#define NOTHING 255
	//ToDo:
	azCmd->addSelVal ("NOTHING", (rts2core::Rts2SelData *) NOTHING);
}

MaxDomeII::~MaxDomeII ()
{
	delete sconn;
}

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
		default:
			return Dome::processOption (opt);
	}
	return 0;
}

int MaxDomeII::initHardware ()
{
        char buffer[MAX_BUFFER];

	sconn = new rts2core::ConnSerial (devFile, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, MAXDOME_TIMEOUT);
	sconn->setDebug (getDebug ());
	sconn->init ();

 	buffer[0] = (uint8_t)((uint16_t)ticksPerRevolution->getValueInteger ())>>8;
	buffer[1] = (uint8_t)((uint8_t)ticksPerRevolution->getValueInteger ());
	exchangeMessage (TICKS_CMD, buffer, 2, buffer);

	return 0;
}

int MaxDomeII::info ()
{
	char buffer[MAX_BUFFER];
	exchangeMessage (STATUS_CMD, NULL, 0, buffer);

	setCurrentAz (((uint16_t) buffer[4]) << 8 | buffer[5]); 
	state->setValueInteger (buffer[2]);
        int  val;
	val= (((uint16_t) buffer[6]) << 8 | buffer[7]);
	fprintf(stderr, "----------val %d\n", val);
	homeAzOffset->setValueDouble((double)val);

	return Cupola::info ();
}

int MaxDomeII::startOpen ()
{
        if( block_shutter->getValueBool()) {
	  logStream (MESSAGE_ERROR) << "MaxDomeII::startOpen blocked shutter opening" << sendLog ;
	  return -1;
	}  

	char buffer[MAX_BUFFER];
	buffer[0] = OPEN_SHUTTER;

	exchangeMessage (SHUTTER_CMD, buffer, 1, buffer);
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

	return state->getValueInteger () == 1 ? -2 : USEC_SEC;
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

	char buffer[MAX_BUFFER];
	buffer[0] = CLOSE_SHUTTER;

	exchangeMessage (SHUTTER_CMD, buffer, 1, buffer);
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

	return state->getValueInteger () == 0 ? -2 : USEC_SEC;
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

    fprintf(stderr, ">sendMessagebuffer write len %ld ---0x", len+4);
    for ( int j=0; j < (int) len+4; j++){
      fprintf(stderr, "%d: %02x\n", j, (uint8_t)wbuf[j] );
    }
    fprintf(stderr, " end buffer\n");

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
  char buffer[MAX_BUFFER];
  if (oldValue ==  shutterCmd)
  {
    shutterCmd->setValueInteger (newValue->getValueInteger ());
    buffer[0] = (uint8_t)shutterCmd->getValueInteger ();

    //    exchangeMessage (SHUTTER_CMD, buffer, 1, buffer);
  } else if(oldValue ==  ticksPerRevolution) {

        ticksPerRevolution->setValueInteger (newValue->getValueInteger ());
 	buffer[0] = ((uint16_t)ticksPerRevolution->getValueInteger ())>>8;
	buffer[1] = ((uint8_t)ticksPerRevolution->getValueInteger ());
	//fprintf(stderr, "buffer %02x, %02x\n", buffer[0], buffer[1]);
	exchangeMessage (TICKS_CMD, buffer, 2, buffer);

  } else if(oldValue ==  manualAz) {
        manualAz->setValueDouble (newValue->getValueDouble ());
	GotoAzimuth(manualAz->getValueDouble ());

  } else if(oldValue ==  azCmd) {

    azCmd->setValueInteger (newValue->getValueInteger ());

    if( (size_t) azCmd->getData ()== HOME_CMD){
      fprintf( stderr, "HOME\n");
	//Home_Azimuth_MaxDomeII();
    }else{
      fprintf( stderr, "NOTHING\n");
    }
 

  }

  return 0 ;
}
int MaxDomeII::GotoAzimuth(double newAZ)
{
  double currAZ = 0;
  int newTicks = 0, nDir = 0xff;
  char buffer[MAX_BUFFER];

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

  buffer[0] = (char)nDir;
  buffer[1] = ((uint16_t)newTicks)>>8 ;
  buffer[2] = (uint8_t)(newTicks);

  exchangeMessage (GOTO_CMD, buffer, 3, buffer);


  //ToDO  nTargetAzimuth = newPos;
  //ToDo nTimeSinceAzimuthStart = 0;     // Init movement timer                                                                 

  return 0; //ToDo
}

double MaxDomeII::TicksToAzimuth(int nTicks)
{
  double nAz=0.;
  nAz = homeAzSetPoint->getValueDouble() + nTicks * 360.0 / ticksPerRevolution->getValueInteger();
  while (nAz < 0) nAz += 360;
  while (nAz >= 360) nAz -= 360;

  return nAz;
}
int MaxDomeII::AzimuthToTicks(double nAzimuth)
{
  int nTicks=0.;

  nTicks = floor(0.5 + (nAzimuth - homeAzSetPoint->getValueDouble()) * ticksPerRevolution->getValueInteger() / 360.0);
  while (nTicks > ticksPerRevolution->getValueInteger()) nTicks -= ticksPerRevolution->getValueInteger();
  while (nTicks < 0) nTicks += ticksPerRevolution->getValueInteger();

  return nTicks;
}

int MaxDomeII::Home_Azimuth_MaxDomeII()
{
  char buffer[MAX_BUFFER];
  exchangeMessage (HOME_CMD, NULL, 0, buffer);
  //ToDo
  return 0 ;
}


int main (int argc, char **argv)
{
	MaxDomeII device (argc, argv);
	return device.run ();
}
