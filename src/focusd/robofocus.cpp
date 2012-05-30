/**
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
#define  OPT_STEPSIZE      OPT_LOCAL + 102

#define  OPT_MINTICKS   OPT_LOCAL + 103
#define  OPT_MAXTICKS   OPT_LOCAL + 104
#define  OPT_GOTOMODE   OPT_LOCAL + 105
#define  OPT_MAXTRAVEL  OPT_LOCAL + 106
#define  OPT_REGPOS     OPT_LOCAL + 107
#define  OPT_REGBACK    OPT_LOCAL + 108
#define  OPT_ADDTO      OPT_LOCAL + 109

#define CMD_FIRMWARE          "FV"
#define CMD_FOCUS_MOVE_IN     "FI"
#define CMD_FOCUS_MOVE_OUT    "FO"
#define CMD_FOCUS_GET         "FG"
#define CMD_TEMP_GET          "FT"
#define CMD_FOCUS_GOTO        "FG"
#define CMD_SETAS_CURRENT_POS "FS"
#define CMD_BACKLASH          "FB"


#define SEL_ABSOLUTE 0x00
#define SEL_RELATIVE 0x01
#define SEL_ADDtoIN  0x00
#define SEL_ADDtoOUT 0x01

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
		char checksum;
		int step_num;

		rts2core::ValueBool *switches[4];

		// high-level I/O functions
		int moveRelativeInOut (const char *cmd, int steps);
		void compute_checksum (char *cmd);

		int getPos ();
		int getTemp ();
		int getSwitchState ();
		int setSwitch (int switch_num, bool new_state);
                rts2core::ValueInteger *duty;
                rts2core::ValueInteger *microStepPause;
                rts2core::ValueInteger *stepSize;
                int motorSettings();
                rts2core::ValueInteger *minTicks;
                rts2core::ValueInteger *maxTicks;
                rts2core::ValueSelection *gotoMode; 
                rts2core::ValueInteger *maxTravel;
                rts2core::ValueInteger *registerPosition;
                rts2core::ValueInteger *registerBacklash;
                rts2core::ValueString  *firmware;
                int getFirmware();
                void toHex(char*buf, int len);
                rts2core::ValueInteger *foc_tar_rel;
                rts2core::ValueInteger *setAsCurrentPosition;
                int setasCurrentPosition();
                int setMaxTravel();
                rts2core::ValueSelection *backlashMode; 
                rts2core::ValueInteger *backlashAmount;
                int setBacklash();

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

	//ToDo decide wht goes to FITS file
	createValue (foc_tar_rel, "FOC_TAR_REL", "target position in relative mode", false, RTS2_VALUE_WRITABLE);
	createValue (setAsCurrentPosition, "setAsCurrentPosition", "set value as current position (sync)", false, RTS2_VALUE_WRITABLE);



	createValue (duty, "duty", "duty cycle", false, RTS2_VALUE_WRITABLE );
	createValue (microStepPause, "microStepPause", "step microStepPause", false, RTS2_VALUE_WRITABLE);
	createValue (stepSize, "stepSize", "steps per tick", false, RTS2_VALUE_WRITABLE);
	duty->setValueInteger(60);
	microStepPause->setValueInteger(5);
	stepSize->setValueInteger(1);

	createValue (minTicks, "minTicks", "lower limit [tick]", false, RTS2_VALUE_WRITABLE);
	createValue (maxTicks, "maxTicks", "upper limit [tick]", false, RTS2_VALUE_WRITABLE);
	minTicks->setValueInteger(100);
	maxTicks->setValueInteger(55000);

	createValue (gotoMode, "gotoMode", "goto mode either absolute or relative", false, RTS2_VALUE_WRITABLE);
	gotoMode->addSelVal ("ABSOLUTE"); 
	gotoMode->addSelVal ("RELATIVE");

	createValue (maxTravel, "maxTravel", "maximum travel", false, RTS2_VALUE_WRITABLE);
	//ToDo: what are those lines good for
	createValue (registerPosition, "registerPosition", "register position setting", false, RTS2_VALUE_WRITABLE);
	createValue (registerBacklash, "registerBacklash", "register backlash setting", false, RTS2_VALUE_WRITABLE);
	//ToDo: better help texts
	createValue (backlashMode, "backlashMode", "backlashMode", false, RTS2_VALUE_WRITABLE);
	backlashMode->addSelVal ("ADDtoIN");
	backlashMode->addSelVal ("ADDtoOUT");
	createValue (backlashAmount, "backlashAmount", "backlashAmount", false, RTS2_VALUE_WRITABLE);

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

	addOption ('f', NULL, 1, "device file (usualy /dev/ttySx");
	addOption (OPT_DUTY, "duty_cycle",  1, "duty cycle, value [0,100]");
	addOption (OPT_MICROSTEPPAUSE, "microStepPause",  1, "step microStepPause [1,20]");
	addOption (OPT_STEPSIZE, "stepsSize",  1, "StepSize, microsteps/step [1,255])");
	addOption (OPT_MINTICKS, "minTicks",  1, "lower limit [tick]");
	addOption (OPT_MAXTICKS, "maxTicks",  1, "upper limit [tick]");
	addOption (OPT_GOTOMODE, "gotoMode",  1, "false: relative, true absolute");
	addOption (OPT_MAXTRAVEL, "maxTravel",  1, "maximum travel, see RoboFocus manual chapter Manual Full Travel Calibration");
	addOption (OPT_REGPOS, "registerPosition",  1, "register position setting");
	addOption (OPT_REGBACK, "registerBacklash",  1, "register backlash setting");
	addOption (OPT_ADDTO, "addto",  1, "add backlash to IN: 0, to OUT: 1");

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
	char command[10], rbuf[10];
	char command_buffer[9];

	// Put together command with neccessary checksum
	sprintf (command_buffer, "FG000000");
	compute_checksum (command_buffer);

	sprintf (command, "%s%c", command_buffer, checksum);

	if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
		return -1;
	position->setValueInteger (atoi (rbuf + 2));
	return 0;
}

int Robofocus::getTemp ()
{
	char command[10], rbuf[10];
	char command_buffer[9];

	// Put together command with neccessary checksum
	sprintf (command_buffer, "FT000000");
	compute_checksum (command_buffer);

	sprintf (command, "%s%c", command_buffer, checksum);

	if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
		return -1;
								 // return temp in Celsius
	temperature->setValueFloat ((atof (rbuf + 2) / 2) - 273.15);
	return 0;
}

int Robofocus::getSwitchState ()
{
	char command[10], rbuf[10];
	char command_buffer[9];
	int ret;

	sprintf (command_buffer, "FP000000");
	compute_checksum (command_buffer);

	sprintf (command, "%s%c", command_buffer, checksum);
	if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
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
	char command[9], command_buf[10];
	sprintf (command, "FG%06i", (int) num);
	compute_checksum (command);
	sprintf (command_buf, "%s%c", command, checksum);
	if (robofocConn->writePort (command_buf, 9))
		return -1;
	return 0;
}

int Robofocus::setSwitch (int switch_num, bool new_state)
{
	char command[10], rbuf[10];
	char command_buffer[9] = "FP001111";
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
			command_buffer[i + 4] = '2';
	}
	command_buffer[switch_num + 4] = (new_state ? '2' : '1');
	compute_checksum (command_buffer);
	sprintf (command, "%s%c", command_buffer, checksum);

	if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
		return -1;

	infoAll ();
	return 0;
}

int Robofocus::moveRelativeInOut (const char *cmd, int steps)
{
  int ret ;
	char command[10];
	char command_buffer[9];
	char rbuf[9];

	// Number of steps moved must account for backlash compensation
	//  if (strcmp (cmd, CMD_FOCUS_MOVE_OUT) == 0)
	//    num_steps = steps + 40;
	//  else
	//    num_steps = steps;

	// Pad out command with leading zeros
	sprintf (command_buffer, "%s%06i", cmd, steps);

	//Get checksum character
	compute_checksum (command_buffer);

	sprintf (command, "%s%c", command_buffer, checksum);

	// Send command to focuser

	if (robofocConn->writePort (command, 9))
		return -1;

	//ToDo was step_num = steps;

	// if we get F, read out end command
	while(true){
	  ret = robofocConn->readPort (rbuf, 1);
	  if (ret == -1)
	    return ret;
	  if (*rbuf == 'F')
	    {
	      ret = robofocConn->readPort (rbuf + 1, 8);
	      usleep (USEC_SEC/10);
	      if (ret != 8)
		return -1;
	      return 0;
	    }
	}

	return 0;
}

int Robofocus::isFocusing ()
{
	char rbuf[10];
	int ret;
	ret = robofocConn->readPort (rbuf, 1);
	if (ret == -1)
		return ret;
	// if we get F, read out end command
	if (*rbuf == 'F')
	{
		ret = robofocConn->readPort (rbuf + 1, 8);
		usleep (USEC_SEC/10);
		if (ret != 8)
			return -1;
		return -2;
	}
	return 0;
}

int Robofocus::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
        int ret ;
	for (int i = 0; i < 4; i++)
	{
		if (switches[i] == oldValue)
		{
			return setSwitch (i, ((rts2core::ValueBool *) newValue)->getValueBool ()) == 0 ? 0 : -2;
		}
	}
	if (oldValue ==  gotoMode)
	{
	  gotoMode->setValueInteger (newValue->getValueInteger ());

	} else if(oldValue == foc_tar_rel) {
	  // relative goto mode: user interaction only
	  // negative: inward, else outward
	  if((gotoMode->getValueInteger ())== SEL_RELATIVE){

	    foc_tar_rel->setValueInteger (newValue->getValueInteger ());
	    if( foc_tar_rel->getValueInteger () <0){
	      ret= moveRelativeInOut ( CMD_FOCUS_MOVE_IN, -foc_tar_rel->getValueInteger ());
	    } else {
	      ret= moveRelativeInOut ( CMD_FOCUS_MOVE_OUT, foc_tar_rel->getValueInteger ());
	    }
	    if(ret)
	      return -1;
	  } else{
	    logStream (MESSAGE_WARNING) << "Robofocus::setValue not in relative goto mode"<< sendLog ;
	  }
	  return 0;
	} else if(oldValue == target) {

	  if((gotoMode->getValueInteger ())== SEL_RELATIVE){

	    logStream (MESSAGE_WARNING) << "Robofocus::setValue not in absolute goto mode"<< sendLog ;
	    return -1 ;
	  }
	} else if(oldValue == setAsCurrentPosition) {
	  setAsCurrentPosition->setValueInteger (newValue->getValueInteger ());
	 
	  if( setasCurrentPosition ())
	    return -1;
	} else if(oldValue == maxTravel) {
	  maxTravel->setValueInteger (newValue->getValueInteger ());
	  //ToDo
	  setFocusExtend(0.,(double)maxTravel->getValueInteger ());
	  
	  if( setMaxTravel ())
	    return -1;
	} else if(oldValue == backlashMode) {
	  backlashMode->setValueInteger (newValue->getValueInteger ());
	  
	} else if(oldValue == backlashAmount) {
	  backlashAmount->setValueInteger (newValue->getValueInteger ());

	}
	return Focusd::setValue (oldValue, newValue);
}

// Calculate checksum (according to RoboFocus spec.)
void Robofocus::compute_checksum (char *cmd)
{
	int bytesum = 0;
	unsigned int size, i;

	size = strlen (cmd);

	for (i = 0; i < size; i++)
		bytesum = bytesum + (int) (cmd[i]);

	checksum = toascii ((bytesum % 340));
}

void Robofocus::toHex( char *buf, int len){
  fprintf(stderr, "buffer %s\n", buf +'\0');

  for( int i= 0 ; i < len; i++){
    fprintf(stderr, "%02d: 0x%02x\n", i, (uint8_t)buf[i]);
  }
  fprintf(stderr, "\n");
}

int Robofocus::getFirmware()
{

  char command[32] ;
  char rbuf[10];

  strcpy(command, "FV000000" ) ;

  if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
    return -1;
  toHex(rbuf, 9);

  rbuf[8]='\0' ;
  firmware->setValueString( &rbuf[4]);

  return 0;
}


int Robofocus::motorSettings()
{

  char command[32] ;
  char rbuf[10];
  if(( duty->getValueInteger()== 0 ) && (microStepPause->getValueInteger()== 0) && (stepSize->getValueInteger()== 0) ){

    strcpy(command, "FC000000" ) ;

  } else {

    command[0]=  'F' ;
    command[1]=  'C' ;
    command[2]= (char) duty->getValueInteger() ;
    command[3]= (char) microStepPause->getValueInteger() ;
    command[4]= (char) stepSize->getValueInteger() ;
    command[5]= '0' ;
    command[6]= '0' ;
    command[7]= '0' ;
    command[8]=  0 ;
  }

  if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
    return -1;

  duty->setValueInteger((double) command[2]) ;
  microStepPause->setValueInteger( (float) command[3]) ;
  stepSize->setValueInteger( (float) command[4]) ;

  return 0;
}
int Robofocus::setasCurrentPosition ()
{
        char rbuf[10];
	char command[9], command_buf[10];
	sprintf (command, "FS%06i", (int) setAsCurrentPosition->getValueInteger());
	compute_checksum (command);
	sprintf (command_buf, "%s%c", command, checksum);
	if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
		return -1;

	return 0;
}
int Robofocus::setMaxTravel ()
{
        char rbuf[10];
	char command[9], command_buf[10];
	sprintf (command, "FL%06i", (int) maxTravel->getValueInteger());
	compute_checksum (command);
	sprintf (command_buf, "%s%c", command, checksum);
	if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
		return -1;

	return 0;
}
int Robofocus::setBacklash ()
{
        char rbuf[10];
	char command[9], command_buf[10];
	

	if(( backlashMode->getValueInteger ())==SEL_ADDtoIN){

	  sprintf (command, "FB2%05i",(int) maxTravel->getValueInteger());
	} else {
	  sprintf (command, "FB3%05i",(int) maxTravel->getValueInteger());
	}

	compute_checksum (command);
	sprintf (command_buf, "%s%c", command, checksum);
	if (robofocConn->writeRead (command, 9, rbuf, 9) != 9)
		return -1;

	return 0;
}

int main (int argc, char **argv)
{
	Robofocus device (argc, argv);
	return device.run ();
}
