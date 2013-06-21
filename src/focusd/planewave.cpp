/**
 * Hedrick Focuser from Planewave Instruments.
 *
 * Copyright (C) 2012 Shashikiran Ganesh <shashi@prl.res.in>
 * based on work by John Kielkopf (kielkopf@louisville.edu) (C) 2010
 * This file derived from planewave.cpp (part of rts2 : focusd)
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

#include "focusd.h"
#include "connection/serial.h"

/* Use these for CDK125 */

#define FOCUSSCALE       11.513442 
#define FOCUSDIR            1  

/* Use these for CDK20N */

/* #define FOCUSSCALE       7447. */ 
/* #define FOCUSDIR            1  */ 

/* Use these for CDK20S */

/* #define FOCUSSCALE       1179. */  
/* #define FOCUSDIR           -1  */ 

#define FOCUSERPORT "/dev/ttyS1" 

namespace rts2focusd
{

/**
 * Class for planewave focuser. 
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Shashikiran Ganesh <shashi@prl.res.in>
 *
 */
class Planewave:public Focusd
{
	public:
		Planewave (int argc, char **argv);
		~Planewave (void);
		virtual int init ();
		virtual int setTo (double num);
		virtual double tcOffset () {return 0.;};
		virtual int isFocusing ();
		virtual int info ();

	protected:
		virtual int processOption (int opt);
		virtual int initValues ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

		virtual bool isAtStartPosition () { return false; }

	private:
		void getValue(const char *name, rts2core::Value *val);
		void openRem ();
		int findOptima ();
		
		int getPos ();
		int getTemp ();
		int setFan (int fancmod);
		int GetTemperature(double *teltemperature, int tempsensor);
		int focusdir;  /*  use -1 if focuser moves in opposite direcion */
		
		int SetHedrickFocuser (int focuscmd, int focusspd);
		
		char buf[15];
		const char *device_file;
		rts2core::ValueFloat *primMirrorTemp;
		rts2core::ValueFloat *ambientTemp;
		rts2core::ValueBool *fanMode;
		
		rts2core::ConnSerial *planewaveConn; // communication port with Hedrick focuser unit
};

}

using namespace rts2focusd;

Planewave::Planewave (int argc, char **argv):Focusd (argc, argv)
{
	focType = std::string ("Planewave");
	createTemperature ();

	createValue (primMirrorTemp, "Primary_temp", "[C] Primary mirror temperature", true);
	createValue (ambientTemp, "Ambient_temp", "[C] Ambient temperature in scope", true);

	createValue (fanMode, "FANMODE", "Fan ON? : TRUE/FALSE", false, RTS2_VALUE_WRITABLE);
		
	setFocusExtend (-1000, 30000);
	
	addOption ('f', NULL, 1, "device file (usually /dev/ttySx)");
	
}

Planewave::~Planewave ()
{
	delete planewaveConn;
}

int Planewave::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Focusd::processOption (opt);
	}
	return 0;
}

int Planewave::initValues ()
{
	focusdir = 1; /* use -1 if focuser moves in opposite direction */

	return Focusd::initValues ();
}

int Planewave::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == fanMode)
	{
		setFan (new_value->getValueInteger());
		return 0;
	}

  	return Focusd::setValue (old_value, new_value);
}

int Planewave::setFan(int fancmd)
{
	/* Default fan string for off */
	
	char outputstr[] = { 0x50, 0x02, 0x13, 0x27, 0x00, 0x00, 0x00, 0x00 };
	char returnstr[] = {0x00, 0x00, 0x00, 0x00 };
	
	
	/* Request the fans on through the EFA on */
	
	if ( fancmd > 0 )
	{
		outputstr[4] = 0x01;
	} 

	logStream (MESSAGE_DEBUG) << " fan cmd is " << fancmd << sendLog;
	
	/* Send the command */
	planewaveConn->writeRead(outputstr, 8, returnstr, 4);
	if (returnstr[0] != '#') 
	    logStream (MESSAGE_ERROR) << " No acknowledgement from thermal control" << sendLog;
	    
	return 0;
}    

int Planewave::GetTemperature(double *teltemperature, int tempsensor)
{
	char outputstr[] = { 0x50, 0x02, 0x12, 0x26, 0x00, 0x00, 0x00, 0x02 };
	char returnstr[] = { 0x00, 0x00, 0x00, 0x00 };	
	int b0,b1;
	int count;
	double value;
	double tempsign;
		
	/* Select sensor primary = 0; ambient = 1; secondary = 2 */
	/* outputstr[4] = 0x0; */
	/* outputstr[4] = 0x1; */
		
	if (tempsensor == 0)
	{
		outputstr[4] = 0x0;
	}	
	else if (tempsensor == 1)
	{
		outputstr[4] = 0x1;
	}
	else if (tempsensor == 2)
	{
		outputstr[4] = 0x2;
	}
	else
	{
		outputstr[4] = 0x0;
	}
			
	value = 0.;
		
	/* Send the command */
	
//	writen(thermalportfd,outputstr,8);		
	
	/* Read a response */
//	readn(thermalportfd,returnstr,3,1);
	planewaveConn->writeRead(outputstr, 8, returnstr, 3);

	b0 = (unsigned char) returnstr[0];
	b1 = (unsigned char) returnstr[1];
	
	/* In our CDK125 b0 is the least signficant 8 bits of a 12 bit word.	*/
	/* High bit of b1 determines sign. */
	/* Untested for temperatures that would use other bits of b1. */
		
	count = b0 + 256*b1;
	
	/* Use a scale which goes negative below zero and sets the sign bit */
	tempsign = 1.;
	if (count > 32767)
	{
		/* Set the sign flag */
		tempsign = -1.;
		
		/* Invert the count under 2^16 = 65536 such that 0xFFFFFFFE = 65535 => -1 */
		count = 65536 - count;
	}

	/* Apply the calibration for the Maxim DS18B20 sensor in the EFA */
	/* Right shift 4 bits to match the specification calibration */
	
	value = count;
	value = 0.0625 * value * tempsign;
	
	/* Test for out of range as an indicator of sensor not present */
	/* Set an out of range value that is not annoying in a display */
	/* if ((value < -50 ) || (value > 50) )
	{
		value = 0.;
	}*/
		
	*teltemperature = value;
	return 0;
}
 
int Planewave::SetHedrickFocuser(int focuscmd, int focusspd)
{
	char sendstr[] = { 0x50, 0x02, 0x12, 0x24, 0x01, 0x00, 0x00, 0x00 };
	char returnstr[2048];
	
	/* Set the speed */

	sendstr[4] = 0x08;

	if (focusspd >= 4)
	{
		sendstr[4] = 0x08;
	}
	
	if (focusspd == 3)
	{
		sendstr[4] = 0x06;
	}
	
	if (focusspd == 2)
	{
		sendstr[4] = 0x04;
	}
	
	if (focusspd == 1)
	{
		sendstr[4] = 0x02;
	}
	  
	if ( focuscmd == -1 )
	{
		/* Run the focus motor in */
		/* Set the direction based on focusdir */
		
		if (focusdir > 0)
		{
			sendstr[3] = 0x25;
		}
		else
		{
			sendstr[3] = 0x24;
		}
			      
		/* Send the command */
		/* Wait up to a second for an acknowledgement */
		
		for (;;) 
		{
			if ( planewaveConn->writeRead(sendstr,8,returnstr,1) ) 
			{
				if (returnstr[0] == '#')
					break;
			}
			else 
			{ 
				fprintf(stderr,"No acknowledgement from focuser\n");
			}
		}	    
	}
	
	if ( focuscmd == 1 )
	{  
		/* Run the focus motor out */
		/* Set the direction based on focusdir */
		if (focusdir > 0)
		{
			sendstr[3] = 0x24;
		}
		else
		{
			sendstr[3] = 0x25;
		}
	      
		/* Send the command */
		/* Wait up to a second for an acknowledgement */
	  
		for (;;) 
		{
			if (planewaveConn->writeRead(sendstr,8,returnstr,1))
			{
				if (returnstr[0] == '#')
					break;
			}
			else 
			{ 
				fprintf(stderr,"No acknowledgement from focus control\n");
			}
		}      
	}
	 
	if (focuscmd == 0)
	{
		/* Set the speed to zero to stop the motion */
	  
		sendstr[4] = 0x00;
	  
		/* Send the command */
		/* Wait up to a second for an acknowledgement */
	  
		for (;;) 
		{
			if (planewaveConn->writeRead(sendstr,8,returnstr,1))
			{
				if (returnstr[0] == '#')
					break;
			}
			else 
			{ 
				fprintf(stderr,"No acknowledgement from focuser\n");
			}
		}            
	}  
	return 0;
}


// send focus to given position
//
int Planewave::setTo (double num)
{
	//logStream (MESSAGE_DEBUG) << " in setTo " << sendLog ;  

	target->setValueDouble(num);

	return 0;
}

int Planewave::isFocusing ()
{
	float pos_diff =position->getValueDouble() - target->getValueDouble();
	float abspos_diff = fabs(pos_diff);
	int focspeed = 4;
	int focdir = 0;

	/* Focus speed values   */
	/*                      */
	/* Fast     4           */
	/* Medium   3           */
	/* Slow     2           */
	/* Precise  1           */
  
	/* Focus command values */
	/*                      */
	/* Out     +1           */
	/* In      -1           */
	/* Off      0           */

	pos_diff = position->getValueDouble() - target->getValueDouble();
	abspos_diff = fabs(pos_diff);

	if (abspos_diff <= 1)
	{
		SetHedrickFocuser(0,0);
		return -2;
	}

	//  focuser is not at target - so move the focuser here
	focspeed = 4;   // assume it is at large deviation and needs highest speed... 

	if (pos_diff > 0)
		focdir = -1;
	if (pos_diff < 0)
		focdir = 1;

	if (abspos_diff < 1000)
		focspeed = 3;
		
	if (abspos_diff < 50)
		focspeed = 2;
		
	if (abspos_diff < 10)
		focspeed = 1;
		
	if (abspos_diff <= 1)
	{
		focspeed = 0;
		focdir = 0;
		abspos_diff = 0;
	}
				
	//	logStream (MESSAGE_DEBUG) << "changing position from " << position->getValueDouble () << " to " << target->getValueDouble() << sendLog;
	//	logStream (MESSAGE_DEBUG) << "focus speed " << focspeed << " and focus direction " << focdir << sendLog;
	//	logStream (MESSAGE_DEBUG) << "abspos_diff " << abspos_diff << " pos_diff " << pos_diff << sendLog;
		
	SetHedrickFocuser (focdir, focspeed);  // for a fixed duration 
	getPos ();   // get new position 

	if (fabs(target->getValueInteger () - position->getValueInteger ()) <= 1)
	{
		SetHedrickFocuser(0,0);
		return -2;
	}
	return 0;
}

/* 
    this is based on the logic in atc2.cpp - i.e. we ensure that the focuser is connected and switched on 
    - code suitably modified for hedrick focuser commands 
    - basically ask for version number and ensure number of returned bytes and last char is \#...
    
    */
void Planewave::openRem ()
{
	unsigned char sendStr[] = { 0x50, 0x02, 0x12, 0xfe, 0x00, 0x00, 0x00, 0x02 };

	int ret = planewaveConn->writeRead ((char *) sendStr, 8, buf, 3);
	if (ret != 3)
		throw rts2core::Error ("focuser open command didn't respond with 3 chars ");
	if (buf[2] != 0x23 )
	        throw rts2core::Error (std::string ("invalid reply from planewave focuser getVersion command :")+ buf);
	logStream (MESSAGE_DEBUG) << "focuser responded with " << buf <<  " EOL " << sendLog ;

/*      test diagnostic code to start motor moving - test only in free running mode - not on the scope with hardware 
        since it does not include a stop command */

/*	sendStr[3] = 0x24; 
	sendStr[4] = 0x08;
	sendStr[7] = 0x00;
	ret=planewaveConn->writeRead (sendStr, 8, buf, 3);
	logStream (MESSAGE_DEBUG) << "focuser replied with " << buf << " for move command "<< sendLog ;  */
	
}

int Planewave::getTemp ()
{

	double teltemperature; 
	
	GetTemperature(&teltemperature, 0);
	// logStream (MESSAGE_DEBUG) << "Primary Temperature " << teltemperature << sendLog;
	
  	primMirrorTemp->setValueFloat(teltemperature);
	temperature->setValueFloat(teltemperature);  // FOC_TEMP is Primary Mirror Temperature ... 


	GetTemperature(&teltemperature, 1);
	// logStream (MESSAGE_DEBUG) << "Ambient temperature " << teltemperature << sendLog;
	ambientTemp->setValueFloat(teltemperature);
	return 0;

}

int Planewave::info ()
{
	getPos();
	getTemp (); 

	return Focusd::info ();
	
}

int Planewave::getPos ()
{
	char sendStr[] = { 0x50, 0x01, 0x12, 0x01, 0x00, 0x00, 0x00, 0x03 };
	char returnstr[] = { 0x00, 0x00, 0x00, 0x00 };  
 	int b0,b1,b2;
  	int count;
  	double focus, focusscale;
    
  	/* Send the command */
	int ret=planewaveConn->writeRead ( sendStr, 8, returnstr, 4 );
	if (ret != 4) 	
		throw rts2core::Error ("focuser open command didn't respond with 4 chars ");

	b0 = (unsigned char) returnstr[0];
  	b1 = (unsigned char) returnstr[1];
  	b2 = (unsigned char) returnstr[2];
  	count = 256*256*b0 + 256*b1 + b2;
	  
	focus = count;

	/* Apply a conversion so that the focus scale comes out in decimal microns. */
	/* The constant FOCUSSCALE is defined in header file.                       */
  
  	focusscale = FOCUSSCALE; 
  	focus = focus/focusscale;     
	
	position->setValueInteger (focus);
	sendValueAll (position);
	return 0;
	
}

int Planewave::init ()
{
	int ret;
	ret = Focusd::init ();
	
	if (ret) 
		return ret;
		
	planewaveConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 100);

	planewaveConn->setDebug (getDebug ());
	
	ret = planewaveConn->init ();
	if (ret)
		return ret;
		
	planewaveConn->flushPortIO ();
	openRem ();
	setIdleInfoInterval (10);
	
	return 0;
	
}

int main (int argc, char **argv)
{
	Planewave device (argc, argv);
	return device.run ();
}
