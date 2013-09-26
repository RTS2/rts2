/**
 * Copyright (C) 2012 Markus Wildi <wildi.markus@bluewin.ch>
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2005-2006 John French
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

#define BAUDRATE B9600
#define FOCUSER_PORT "/dev/ttyS1"

#define  OPT_DUTY           OPT_LOCAL + 100
#define  OPT_MICROSTEPPAUSE OPT_LOCAL + 101
#define  OPT_STEPSIZE       OPT_LOCAL + 102

#define  OPT_MINTICKS   OPT_LOCAL + 103
#define  OPT_MAXTICKS   OPT_LOCAL + 104
#define  OPT_GOTOMODE   OPT_LOCAL + 105
#define  OPT_MAXTRAVEL  OPT_LOCAL + 106
#define  OPT_REGPOS     OPT_LOCAL + 107
#define  OPT_REGBACK    OPT_LOCAL + 108
#define  OPT_BACKLASHMODE    OPT_LOCAL + 109
#define  OPT_BACKLASHAMOUNT  OPT_LOCAL + 110
//F: denotes focuser command
//X: is a alpha character denoting which command
//?: is an arbitrary character as a placeholder
//NNNNNN: are six decimal digits with leading zeros as necessary
//Z: is a checksum character of value 0-255. Its value is the result of adding all the previous characters
//together and setting Z equal to the least significant byte.
//#define CMD_FIRMWARE          "FV" //FVXXXXXXZ, FVXXXXXXZ
//#define CMD_FOCUS_MOVE_IN     "FI" //FI?XXXXXZ, FI?XXXXXZ 
//#define CMD_FOCUS_MOVE_OUT    "FO" //FO?XXXXXZ, FO?XXXXXZ
//#define CMD_TEMP_GET          "FT" //FTXXXXXXZ, FTXXNNNNZ
//#define CMD_FOCUS_GOTO        "FG" //FG?XXXXXZ, FD?XXXXXZ, if moving: O, I foreach step
//#define CMD_SETAS_CURRENT_POS "FS" //FS?XXXXXZ, new current position (format?)
//#define CMD_BACKLASH          "FB" //FBNXXXXXZ, FBNXXXXXZ
//#define CND_Motor_SETTINGS    "FC" //FCABCDEFZ, FCABCDEFZ
//#define CMD_MAX_TRAVEL        "FL" //FL?XXXXXZ, FL?XXXXXZ
//#define CMD_IO                "FP" //FP??XXXXZ, FP??XXXXZ


#define SEL_ABSOLUTE 0x00
#define SEL_RELATIVE 0x01
#define SEL_ADDtoIN  0x00
#define SEL_ADDtoOUT 0x01

#define SEL_START    0x00 //dummy 
#define SEL_READ_MAX_TRAVEL  0x01

// pre 3.20 versions behave differently, see robofocus manual
// those devices are not yet supported
#define FIRMWARE_32 "3.20"

#include "focusd.h"
#include "connection/serial.h"

namespace rts2focusd
{

class Robofocus:public Focusd
{
	private:
		const char *device_file;
		rts2core::ConnSerial *robofocConn;

		rts2core::ValueBool *switches[4];

		// high-level I/O functions
		int moveRelativeInOut ();
		void compute_checksum (char *cmd);

		int getPos ();
		int getTemp ();
		int getSwitchState ();
		int setSwitch (int switch_num, bool new_state);
                rts2core::ValueInteger *duty;
                rts2core::ValueInteger *microStepPause;
                rts2core::ValueInteger *microStepPerTick;
                int motorSettings();
                rts2core::ValueInteger *minTicks;
                rts2core::ValueInteger *maxTicks;
                rts2core::ValueSelection *gotoMode; 
                rts2core::ValueInteger *maxTravel;
                rts2core::ValueString  *firmware;
                int getFirmware();
                void toHex(char*buf);
                rts2core::ValueInteger *foc_tar_rel;
                rts2core::ValueInteger *setAsCurrentPosition;
                int setasCurrentPosition();
                int setMaxTravel();
                int getMaxTravel();
                rts2core::ValueSelection *backlashMode; 
                rts2core::ValueInteger *backlashAmount;
                int setBacklash();
                int sendReceive( char *cmd, char *rbuf);
                int checkReadBuffer (char *sbuf, char *rbuf);
                rts2core::ValueSelection *calibration; 
                int discardOI( char *rbuf) ;
	protected:
		virtual int processOption (int in_opt);
		virtual int isFocusing ();

		virtual bool isAtStartPosition ()
		{
			return false;
		}

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);
	public:
		Robofocus (int argc, char **argv);
		~Robofocus (void);

		virtual int init ();
		virtual int initValues ();
		virtual int info ();
		virtual int setTo (double num);
		virtual double tcOffset () {return 0.;};
};

}

using namespace rts2focusd;

Robofocus::Robofocus (int argc, char **argv):Focusd (argc, argv)
{
	device_file = FOCUSER_PORT;

	createValue (foc_tar_rel, "FOC_TAR_REL", "target position in relative mode", false, RTS2_VALUE_WRITABLE);
	createValue (setAsCurrentPosition, "setAsCurrentPosition", "set value as current position (sync) [minTicks,maxTicks]", false, RTS2_VALUE_WRITABLE);

	createValue (duty, "duty", "duty cycle [0,250]", false, RTS2_VALUE_WRITABLE );
	createValue (microStepPause, "microStepPause", "speed, pause between micro steps [msec]", false, RTS2_VALUE_WRITABLE);
	createValue (microStepPerTick, "microStepPerTick", "fineness, steps per tick [1,255]", false, RTS2_VALUE_WRITABLE);
	duty->setValueInteger(10);
	microStepPause->setValueInteger(5);
	microStepPerTick->setValueInteger(1);

	createValue (minTicks, "minTicks", "lower limit [0,64000],[tick]", false, RTS2_VALUE_WRITABLE);
	createValue (maxTicks, "maxTicks", "upper limit [0,64000],[tick]", false, RTS2_VALUE_WRITABLE);
	minTicks->setValueInteger(2);
	maxTicks->setValueInteger(65000);

	// maxTravel not present: min and max travel are set via minTicks, maxTicks
        // 
        // If the calibration procedure defined in the robofocus manual was applied 
	// use value of option maxTravel.
        // If option maxTravel is present, then settings are as follows:
        // minTicks= 2 
        // maxTicks= maxTravel
        // maxTravel is set and transmitted to robofocus
	createValue (maxTravel, "maxTravel", "maximum travel either read back or set as option (see calibration procedure in robofocus manual)", false);
	maxTravel->setValueInteger(-1);
	createValue (gotoMode, "gotoMode", "goto mode either absolute or relative", false, RTS2_VALUE_WRITABLE);
	gotoMode->addSelVal ("ABSOLUTE"); 
	gotoMode->addSelVal ("RELATIVE");
	gotoMode->setValueInteger(SEL_ABSOLUTE);
   
	createValue (backlashMode, "backlashMode", "backlash mode add either ro In or OUT movement", false, RTS2_VALUE_WRITABLE);
	backlashMode->addSelVal ("ADDtoIN");
	backlashMode->addSelVal ("ADDtoOUT");
	createValue (backlashAmount, "backlashAmount", "backlash amount", false, RTS2_VALUE_WRITABLE);
	backlashAmount->setValueInteger(0);

	for (int i = 0; i < 4; i++)
	{
		std::ostringstream _name;
		std::ostringstream _desc;
		_name << "switch_" << i+1;
		_desc << "plug number " << i+1;
		createValue (switches[i], _name.str().c_str(), _desc.str().c_str(), false, RTS2_VALUE_WRITABLE);
	}

	createTemperature ();
	createValue (firmware, "firmware", "firmware", false);
	createValue (calibration, "calibration", "calibrate maximum travel, see robofocus manual", false, RTS2_VALUE_WRITABLE);
	calibration->addSelVal ("START"); 
	calibration->addSelVal ("READ maxTravel");

	addOption ('f', NULL, 1, "device file (usualy /dev/ttySx");
	addOption (OPT_DUTY, "dutyCycle",  1, "duty cycle, value [0,250]");
	addOption (OPT_MICROSTEPPAUSE, "microStepPause",  1, "speed, pause between micro steps [msec]");
	addOption (OPT_STEPSIZE, "stepsize",  1, "fineness, micro steps per tick  [1,255])");
	addOption (OPT_MINTICKS, "minTicks",  1, "lower limit [tick]");
	addOption (OPT_MAXTICKS, "maxTicks",  1, "upper limit [tick]");
	addOption (OPT_MAXTRAVEL,"maxTravel", 1, "it is the upper limit [tick], if robofocus has been calibrated");
	addOption (OPT_BACKLASHMODE, "backlashMode",  1, "add backlash to IN: 0, to OUT: 1");
	addOption (OPT_BACKLASHAMOUNT, "backlashAmount",  1, "backlash compensation [tick]");

}

Robofocus::~Robofocus ()
{
  	delete robofocConn;
}

int Robofocus::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
         	case OPT_DUTY:
	                 duty->setValueCharArr(optarg);
	                 break;
	        case OPT_MICROSTEPPAUSE:
	                 microStepPause->setValueCharArr(optarg);
	                 break;
         	case OPT_STEPSIZE:
	                 microStepPerTick->setValueCharArr(optarg);
	                 break;
         	case OPT_MINTICKS:
	                 minTicks->setValueCharArr(optarg);
	                 break;
         	case OPT_MAXTICKS:
	                 maxTicks->setValueCharArr(optarg);
	                 break;
        	case OPT_MAXTRAVEL:
	                 maxTravel->setValueCharArr(optarg);
	                 break;
       	        case OPT_BACKLASHMODE:
	                 backlashMode->setValueCharArr(optarg);
	                 break;
       	        case OPT_BACKLASHAMOUNT:
	                 backlashAmount->setValueCharArr(optarg);
	                 break;
		default:
			return Focusd::processOption (in_opt);
	}
	return 0;
}

/*!
 * Init focuser, connect on given port port, set manual regime
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int Robofocus::init ()
{
	int ret;

	ret = Focusd::init ();

	if (ret)
		return ret;
	  robofocConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 40);
	  ret = robofocConn->init ();

	if (ret)
		return ret;

	robofocConn->flushPortIO ();
	// turn paramount on
	ret = setSwitch (1, true);
	if (ret)
		return -1;

	ret = setSwitch (2, false);
	if (ret)
	  	return -1;

	ret = getFirmware();	
	if (ret)
	  	return -1;

	if(strcmp(firmware->getValueString().c_str(), FIRMWARE_32)){
	    logStream (MESSAGE_WARNING) << "Robofocus::int driver is tested with firmware revision 3.20, current: "<< firmware->getValueString()<< sendLog ;

	}
	// 2012-06-28, wildi, temporarily disabled, due to FOC_TOFF, FOC_FOFF problem
	// if( minTicks->getValueInteger() < 2)
	//   minTicks->setValueInteger(2); 
	if( minTicks->getValueInteger() < 0)
	  minTicks->setValueInteger(0); 
	if( minTicks->getValueInteger() > 65000)
	  minTicks->setValueInteger(65000); 

	// if( maxTicks->getValueInteger() < 2)
	//   maxTicks->setValueInteger(2); 
	if( maxTicks->getValueInteger() < 0)
	  maxTicks->setValueInteger(0); 
	if( maxTicks->getValueInteger() > 65000)
	  maxTicks->setValueInteger(65000); 

	if( maxTravel->getValueInteger() > 65000)
	  maxTravel->setValueInteger(65000); 

	// if robofocus has been calibrated
	if(maxTravel->getValueInteger()>1){

	  // setFocusExtent( 2.,(double)maxTravel->getValueInteger());
	  // minTicks->setValueInteger (2);
	  setFocusExtent( 0.,(double)maxTravel->getValueInteger());
	  minTicks->setValueInteger (0);
	  maxTicks->setValueInteger( maxTravel->getValueInteger());
	} else {

	  setFocusExtent( (double)minTicks->getValueInteger(),(double)maxTicks->getValueInteger());
	  maxTravel->setValueInteger( maxTicks->getValueInteger());
	}
	if( setMaxTravel ())
	  return -1 ;

	if( backlashAmount->getValueInteger() > 64000)
	   backlashAmount->setValueInteger(64000); 

	if( setBacklash ())
	  return -1;

	if( duty->getValueInteger() < 0)
	  duty->setValueInteger(0); 
	if( duty->getValueInteger() > 250)
	  duty->setValueInteger(250); 

	if( microStepPause->getValueInteger() < 1)
	  microStepPause->setValueInteger(1); 
	if( microStepPause->getValueInteger() > 64) 
	  microStepPause->setValueInteger(64); 

	if( microStepPerTick->getValueInteger() < 1)
	  microStepPerTick->setValueInteger(1); 
	if( microStepPerTick->getValueInteger() > 64)
	  microStepPerTick->setValueInteger(64); 

	if( motorSettings ())
	  return -1;
       
	return 0;
}

int Robofocus::initValues ()
{
	focType = std::string ("ROBOFOCUS");
	return Focusd::initValues ();
}

int Robofocus::info ()
{
	getPos ();
	getTemp ();
	// querry for switch state
	int swstate = getSwitchState ();
	if (swstate >= 0)
	{
		for (int i = 0; i < 4; i++)
		{
			switches[i]->setValueBool (swstate & (1 << i));
		}
	}

	return Focusd::info ();
}

int Robofocus::getPos ()
{
        char sbuf[9]="FG000000";
        char rbuf[9];

	if( sendReceive(sbuf, rbuf))
	  return -1;

	position->setValueInteger (atoi (rbuf + 2));
	return 0;
}

int Robofocus::getTemp ()
{
        char sbuf[9]="FT000000";
        char rbuf[9];
	if( sendReceive(sbuf, rbuf))
	  return -1;

	// return temp in Celsius
	temperature->setValueFloat ((atof (rbuf + 2) / 2) - 273.15);
	return 0;
}

int Robofocus::getSwitchState ()
{
  int ret;
	char sbuf[9]="FP000000";
        char rbuf[9];

	if( sendReceive(sbuf, rbuf))
	  return -1;

	ret = 0;
	for (int i = 0; i < 4; i++)
	{
		if (rbuf[i + 4] == '2')
			ret |= (1 << i);
	}
	return ret;
}

int Robofocus::setTo (double num)
{
	char sbuf[9];
	sprintf (sbuf, "FG%06i", (int) num);
	compute_checksum (sbuf);
	if (robofocConn->writePort (sbuf, 9))
	  return -1;
	return 0;
}

int Robofocus::setSwitch (int switch_num, bool new_state)
{
	char sbuf[9] = "FP001111";
        char rbuf[9];
	if (switch_num >= 4)
	{
		return -1;
	}
	int swstate = getSwitchState ();
	if (swstate < 0)
		return -1;
	for (int i = 0; i < 4; i++)
	{
		if (swstate & (1 << i))
			sbuf[i + 4] = '2';
	}
	sbuf[switch_num + 4] = (new_state ? '2' : '1');

	if( sendReceive(sbuf, rbuf))
	  return -1;

	infoAll ();
	return 0;
}

int Robofocus::moveRelativeInOut ()
{
	char sbuf[9];
	char rbuf[9];

	if( foc_tar_rel->getValueInteger () <0){
	  sprintf (sbuf, "FI%06i", -foc_tar_rel->getValueInteger ());
	} else {
	  sprintf (sbuf, "FO%06i", foc_tar_rel->getValueInteger ());
	}
	compute_checksum (sbuf);

	if (robofocConn->writePort (sbuf, 9)){
	  return -1;
	}
	if(discardOI( rbuf))
	  return -1;

	return 0;
}

int Robofocus::isFocusing ()
{
	int ret;
	char rbuf[9];
	ret = robofocConn->readPort (rbuf, 1);
	if (ret == -1)
	  return ret;

	if(discardOI( rbuf))
	  return -1;

	return 0;
}

int Robofocus::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	for (int i = 0; i < 4; i++)
	{
		if (switches[i] == oldValue)
		{
			return setSwitch (i, ((rts2core::ValueBool *) newValue)->getValueBool ()) == 0 ? 0 : -2;
		}
	}
	if (oldValue ==  gotoMode) {

	  gotoMode->setValueInteger (newValue->getValueInteger ());
	  return 0;

	} else if (oldValue == foc_tar_rel) {

	  if((gotoMode->getValueInteger ())== SEL_ABSOLUTE){
	    logStream (MESSAGE_WARNING) << "Robofocus::setValue not in relative goto mode"<< sendLog ;
	    return -1 ;
	  }
	  foc_tar_rel->setValueInteger (newValue->getValueInteger ());
	  return 0;

	  if(moveRelativeInOut ())
	    return -1;

	} else if (oldValue == target) {

	  if((gotoMode->getValueInteger ())== SEL_RELATIVE){
	    logStream (MESSAGE_WARNING) << "Robofocus::setValue not in absolute goto mode"<< sendLog ;
	    return -1 ;
	  }
	  //NO return 0;
          // set is done in base class
	} else if (oldValue == setAsCurrentPosition) {

	  setAsCurrentPosition->setValueInteger (newValue->getValueInteger ());
	  if( setAsCurrentPosition->getValueInteger () < minTicks->getValueInteger() )
	    logStream (MESSAGE_WARNING) << "Robofocus::setValue setAsCurrentPosition < minTicks"<< sendLog ;
	 
	  if( setAsCurrentPosition->getValueInteger () > maxTicks->getValueInteger())
	    logStream (MESSAGE_WARNING) << "Robofocus::setValue setAsCurrentPosition > maxTicks"<< sendLog ;
	  
	  if( setasCurrentPosition ())
	    return -1;
	  return 0;

	} else if (oldValue == backlashMode) {

	  backlashMode->setValueInteger (newValue->getValueInteger ());
	  return 0;

	} else if (oldValue == backlashAmount) {

	  backlashAmount->setValueInteger (newValue->getValueInteger ());
	  if( setBacklash ())
	    return -1;
	  return 0;

	} else if (oldValue == minTicks) {

	  minTicks->setValueInteger (newValue->getValueInteger ());

	  if( minTicks->getValueInteger() < 2)
	    minTicks->setValueInteger(2); 
	  if( minTicks->getValueInteger() > 65000)
	    minTicks->setValueInteger(65000); 

	  setFocusExtent((double)minTicks->getValueInteger (),(double)maxTicks->getValueInteger ());
	  // set maxTravel knowing limits are checked in the base class
	  if( setMaxTravel ())
	    return -1;
	  return 0;

	} else if (oldValue == maxTicks) {

	  maxTicks->setValueInteger (newValue->getValueInteger ());

	  if( maxTicks->getValueInteger() < 2)
	    maxTicks->setValueInteger(2); 
	  if( maxTicks->getValueInteger() > 65000)
	    maxTicks->setValueInteger(65000); 

	  setFocusExtent((double)minTicks->getValueInteger (),(double)maxTicks->getValueInteger ());
	  if( setMaxTravel ())
	    return -1;
	  return 0;

	} else if (oldValue == calibration) {

	  calibration->setValueInteger (newValue->getValueInteger ());
	  if((calibration->getValueInteger ())== SEL_READ_MAX_TRAVEL){
	    if( getMaxTravel ()){
	      return -1 ;
	    }
	    // minTicks is always 2 (see rebofocus manual)
	    // maxTravel is the result of calibration
	    setFocusExtent( 2.,(double)maxTravel->getValueInteger());
	    minTicks->setValueInteger (2);
            maxTicks->setValueInteger( maxTravel->getValueInteger());
	  }
	  return 0;

	} else if (oldValue == duty) {

	  duty->setValueInteger (newValue->getValueInteger ());
	  if( duty->getValueInteger() < 0)
	    duty->setValueInteger(0); 
	  if( duty->getValueInteger() > 250)
	    duty->setValueInteger(250); 
	  if( motorSettings ())
	    return -1;
	  sendValueAll (duty);
	  return 0;

	} else if (oldValue == microStepPause) {

	  microStepPause->setValueInteger (newValue->getValueInteger ());
	  if( microStepPause->getValueInteger() < 1)
	    microStepPause->setValueInteger(1); 
	  if( microStepPause->getValueInteger() > 64) 
	    microStepPause->setValueInteger(64); 
	  if( motorSettings ())
	    return -1;
	  sendValueAll (microStepPause);
	  return 0;

	} else if (oldValue == microStepPerTick) {

	  microStepPerTick->setValueInteger (newValue->getValueInteger ());
	  if( microStepPerTick->getValueInteger() < 1)
	    microStepPerTick->setValueInteger(1); 

	  if( microStepPerTick->getValueInteger() > 64){
	    microStepPerTick->setValueInteger(64); 	
	  }
	    microStepPerTick->setValueInteger(100); 	
	  
	  if( motorSettings ())
	    return -1;
	  sendValueAll (microStepPerTick);
	  return 0;
	}

	return Focusd::setValue (oldValue, newValue);
}
// Calculate checksum (according to RoboFocus spec.)
// always 8 bytes + 1 checksum
void  Robofocus::compute_checksum (char *cmd)
{
	int bytesum = 0;
	int i;

	for (i = 0; i < 8; i++)
		bytesum = bytesum + (int) (cmd[i]);

	cmd[8]= bytesum % 256;
}
int  Robofocus::checkReadBuffer (char *sbuf, char *rbuf) 
{
        int i;
	char cbuf[9];

        // exception of the scheme FG -> FD
	if(((sbuf[0]==rbuf[0]) && (sbuf[1]==rbuf[1])) || ( sbuf[1]=='G' && rbuf[1]=='D')){
	    for (i = 0; i < 8; i++)
	      cbuf[i]=rbuf[i];

	    compute_checksum ( cbuf) ;

	    if( cbuf[8]== rbuf[8]){
		return 0;
	    } else {
	    }
	}
	return -1;
}
int Robofocus::getFirmware()
{
        char sbuf[9]="FV000000" ;
	char rbuf[9];

	if( sendReceive(sbuf, rbuf))
	  return -1;

	rbuf[8]='\0' ;
	firmware->setValueString( &rbuf[4]);

	return 0;
}
// Attention:
//FCABCDEFZ. Commands RoboFocus to set configuration. If ABCDE=0 then RoboFocus responds with
//  current configuration settings. If non-zero, configurations be set as follows:
//A-spare
//B-spare
//C-spare
//D-Duty Cycle. This character has an ASCI value of 0-250 which corresponds to a duty cycle for the
//...
//the frist three bytes are not spare bytes 
int Robofocus::motorSettings()
{
        char sbuf[9]= "FC000000";
	char rbuf[9];
	sprintf (sbuf, "FC%1c%1c%1c000", (uint8_t)duty->getValueInteger(), (uint8_t) microStepPause->getValueInteger(), (uint8_t)microStepPerTick->getValueInteger());

	if( sendReceive(sbuf, rbuf)){
	  return -1;
	}
	//ToDo if values are out of limits (e.g. > 64) the value is not updated as seen
	// with rts2-mon, values sent by robofocus are correct!
	duty->setValueInteger((int)rbuf[2]) ;
	microStepPause->setValueInteger( (int) rbuf[3]) ;
	microStepPerTick->setValueInteger( (int) rbuf[4]) ;

	return 0;
}
int Robofocus::setasCurrentPosition ()
{
	char sbuf[9];
        char rbuf[9];

	if(! (( setAsCurrentPosition->getValueInteger()>= getFocusMin()) && (setAsCurrentPosition->getValueInteger()<= getFocusMax ()))){
	  return -1;
	}
	sprintf (sbuf, "FS%06i", (int) setAsCurrentPosition->getValueInteger());
        if( sendReceive(sbuf, rbuf))
            return -1;

	rbuf[8]= '\0';
	int cr= atoi( rbuf + 3);
	position->setValueInteger(cr);

	return 0;
}
int Robofocus::setMaxTravel ()
{
        char sbuf[9];
        char rbuf[9];
	if(maxTravel->getValueInteger()>0){
	  sprintf (sbuf, "FL%06i", (int) maxTravel->getValueInteger());
	}
        if( sendReceive(sbuf, rbuf))
            return -1;
	return 0;
}
int Robofocus::getMaxTravel ()
{
        char sbuf[9]= "FL000000"; // read the result from the calibration procedure, see robofocus manual
        char rbuf[9];
	robofocConn->flushPortIO (); // many OOO or III ar sent

        if( sendReceive(sbuf, rbuf))
            return -1;
	rbuf[8]= '\0';
	int mx= atoi( rbuf + 3);
	maxTravel->setValueInteger(mx);
	return 0;
}
//Attention: 
// N=2 is compensation added to IN motion, N=3 is compensation added to OUT motion.
// IN: towards 0 (home)
// robofocus manual:
// Direction. 
// To check the RoboFocus direction, push the OUT button. 
// ...
// If the direction is wrong, turn off the RoboFocus. After about ten seconds, turn on the 
// RoboFocus while pressing the OUT button. As the RoboFocus powers up, it will sense the 
// OUT button, 
// ...
// The RoboFocus will remember this direction information for future sessions.
//
int Robofocus::setBacklash ()
{
	char sbuf[9];
        char rbuf[9];

	if(( backlashMode->getValueInteger ())==SEL_ADDtoIN){
	  sprintf (sbuf, "FB2%05i",(int) backlashAmount->getValueInteger());
	} else {
	  sprintf (sbuf, "FB3%05i",(int) backlashAmount->getValueInteger());
	}
        if( sendReceive(sbuf, rbuf))
            return -1;
	return 0;
}
int Robofocus::discardOI( char *rbuf)
{
        int ret;
	while(true){
	  ret = robofocConn->readPort (rbuf, 1);
	  if (ret == -1){
	    return ret;
	  }
	  if (*rbuf == 'F') {
	      ret = robofocConn->readPort (rbuf + 1, 8);
	      usleep (USEC_SEC/10);
	      if (ret != 8)
		return -1;

	      return 0;
	  }
	}
	return -1;
}
int Robofocus::sendReceive( char *sbuf, char *rbuf)
{
	compute_checksum (sbuf);

	if (robofocConn->writePort ( sbuf, 9))
	  return -1;

	// discard OOO..., III...
	if ( discardOI(rbuf))
	  return -1;

	if (checkReadBuffer(sbuf, rbuf)){
	  logStream (MESSAGE_ERROR) << "Robofocus::sendReceive bad answer: "<< sendLog ;
	  return -1;
	}

	return 0;
}

void Robofocus::toHex( char *buf){
       fprintf(stderr, "buffer %s\n", buf +'\0');

       for( int i= 0 ; i < 9; i++){
	 fprintf(stderr, "%02d: 0x%02x %c\n", i, (uint8_t)buf[i], toascii((uint8_t)buf[i]));
       }
       fprintf(stderr, "\n");
}

int main (int argc, char **argv)
{
	Robofocus device (argc, argv);
	return device.run ();
}
