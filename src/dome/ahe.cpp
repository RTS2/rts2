/* 
 * Dome driver for 7 Foot Astrohaven Enterprise Domes
 * Should work for 12', 16', and 20' domes (untested)
 * Copyright (C) 2011 Lee Hicks 
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

#include "dome.h"
#include "ahe.h"

using namespace rts2dome;

int AHE::openALeaf()
{
    cmdSent=0;
    response = '\x00';

	while(response != POLL_A_OPENED && cmdSent < MAX_COMMANDS)
	{
		sconn->writePort(CMD_A_OPEN);
		usleep(SERIAL_SLEEP);
		sconn->readPort(response);
		cmdSent++;

	}

    if(cmdSent < MAX_COMMANDS)
    {
       logStream(MESSAGE_DEBUG) << "Leaf closed in " << cmdSent << " less than " << MAX_COMMANDS << sendLog;
        return 1;
    }
    else
    {
		logStream(MESSAGE_ERROR) << "Opening of leaf A exceeded max commands." << sendLog;
        return -1;
    }
}

int AHE::openBLeaf()
{
    cmdSent=0;
    response = '\x00';

    while(response != POLL_B_OPENED && cmdSent < MAX_COMMANDS)
    {
        sconn->writePort(CMD_B_OPEN);
		usleep(SERIAL_SLEEP);
        sconn->readPort(response);
        cmdSent++;
    }

    if(cmdSent < MAX_COMMANDS)
    {
       logStream(MESSAGE_DEBUG) << "Leaf closed in " << cmdSent << " less than " << MAX_COMMANDS << sendLog;
        return 1;
    }
    else
    {
		logStream(MESSAGE_ERROR) << "Opening of leaf B exceeded max commands." << sendLog;
        return -1;
    }
}

int AHE::closeALeaf()
{
   cmdSent=0; 
   response = '\x00';

   while(response != POLL_A_CLOSED && cmdSent < MAX_COMMANDS)
   {
       sconn->writePort(CMD_A_CLOSE);
       usleep(SERIAL_SLEEP);
       sconn->readPort(response);
       cmdSent++;
   }


   if(cmdSent < MAX_COMMANDS)
   {
       logStream(MESSAGE_DEBUG) << "Leaf closed in " << cmdSent << " less than " << MAX_COMMANDS << sendLog;
       return 1;
   }
   else
   {
	   logStream(MESSAGE_ERROR) << "Closing of leaf A exceeded max commands." << sendLog;
       return -1;
   }
}

int AHE::closeBLeaf()
{
    
   cmdSent=0;
   response = '\x00';

   while(response != POLL_B_CLOSED && cmdSent < MAX_COMMANDS)
   {
       sconn->writePort(CMD_B_CLOSE);
       usleep(SERIAL_SLEEP);
       sconn->readPort(response);
       cmdSent++;
   }


   if(cmdSent < MAX_COMMANDS)
   {
       logStream(MESSAGE_DEBUG) << "Leaf closed in " << cmdSent << " less than " << MAX_COMMANDS << sendLog;
       return 1;
   }
   else
   {
	   logStream(MESSAGE_ERROR) << "Closing of leaf B exceeded max commands." << sendLog;
       return -1;
   }
}

int AHE::processOption(int opt)
{
    switch (opt)
    {
        default:
            return Dome::processOption (opt);
    }
    return 0;
}

int AHE::setValue(rts2core::Value *old_value, rts2core::Value *new_value)
{
    if(old_value == closeDome)
    {
        if(((rts2core::ValueBool*) new_value)->getValueBool() == true)
        {
            logStream(MESSAGE_DEBUG) << "Told to open dome" << sendLog;
            startOpen();
        }
        else
        {
            logStream(MESSAGE_DEBUG) << "Told to close dome" << sendLog;
            startClose();
        }
    }
    else if(old_value == leafA)
    {
        if(((rts2core::ValueBool*) new_value)->getValueBool() == true)
        { 
            logStream(MESSAGE_DEBUG) << "Told leaf A to open" << sendLog;
            openALeaf();
        }
        else
        {
            logStream(MESSAGE_DEBUG) << "Told leaf A to close" << sendLog;
            closeALeaf();
        }
    }
    else if(old_value == leafB)
    {
        if(((rts2core::ValueBool*) new_value)->getValueBool() == true)
        { 
            logStream(MESSAGE_DEBUG) << "Told leaf B to open" << sendLog;
            openBLeaf();
        }
        else
        {
            logStream(MESSAGE_DEBUG) << "Told leaf B to close" << sendLog;
            closeBLeaf();
        }
    }

    return Dome::setValue(old_value, new_value);
}

int AHE::init()
{

    logStream (MESSAGE_DEBUG) << "AHE Dome initing..." << sendLog;
    
    sconn = new rts2core::ConnSerial(dev, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 10);

    int ret = Dome::init();
    if(ret)
    {
        return ret;
    }

    return 0;
}

int AHE::info()
{
    return Dome::info();
}


int AHE::startOpen()
{
    status = OPENING;
    domeStatus->setValueString("Opening....");
    if(openALeaf() && openBLeaf())
    {
        status = OPENED;
        domeStatus->setValueString("Opened");
        return 0;
    }
    else
    {
        status = ERROR;
        domeStatus->setValueString("Error opening");
        return 1;
    }

    return 1;
}

long AHE::isOpened()
{
	if(status == ERROR)
	{
		return -1;
	}
    else if(status != OPENED)
	{
		return 5000;
	}    
	else
	{
		return -2;
	}
}

int AHE::endOpen()
{
   //do nothing
   return 0;
}

int AHE::startClose()
{
    status = CLOSING;
    domeStatus->setValueString("Closing....");
    logStream(MESSAGE_DEBUG) << "Closing leafs..." << sendLog;
    if(closeALeaf() && closeBLeaf())
    {
        status = CLOSED;
        domeStatus->setValueString("Closed");
        return 0;
    }
    else
    {
        status = ERROR;
        domeStatus->setValueString("Error closing");
        return 1;
    }

}

long AHE::isClosed()
{

	if(status == ERROR)
	{
		return -1;
	}
    else if(status != CLOSED)
	{
		return 5000;
	}    
	else
	{
		return -2;
	}
    
}

int AHE::endClose()
{
   //do nothing
   return 0;
}

AHE::AHE(int argc, char **argv):Dome(argc, argv)
{
    logStream (MESSAGE_DEBUG) << "AHE Dome constructor called" << sendLog;
    dev = "/dev/ttyS0";
    response = '\x00';

    createValue(domeStatus, "DOME_STAT", "dome status", true);
    createValue(closeDome, "DOME_OPEN", "Dome open?", false, RTS2_VALUE_WRITABLE);
    createValue(leafA, "Leaf_A_Opn", "Leaf A open?", false, RTS2_VALUE_WRITABLE);
    createValue(leafB, "Leaf_B_Opn", "Leaf B open?", false, RTS2_VALUE_WRITABLE);

    closeDome->setValueBool(true);
    leafA->setValueBool(false);
    leafB->setValueBool(false);
}

AHE::~AHE()
{
}

int main(int argc, char **argv)
{
    AHE device(argc, argv);
    return device.run();
}
