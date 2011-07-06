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

    while(response != POLL_A_OPENED || cmdSent < MAX_COMMANDS)
    {
        serial_write_byte(fd, CMD_A_OPEN);
        serial_read_byte(fd, SERIAL_TIMEOUT, &response);
        cmdSend++;
    }

    if(cmdSent < MAX_COMMANDS)
    {
        return 1;
    }
    else
    {
        return -1;
    }
}

int AHE::openBLeaf()
{
    cmdSent=0;

    while(response != POLL_B_OPENED || cmdSent < MAX_COMMANDS)
    {
        serial_write_byte(fd, CMD_B_OPEN);
        serial_read_byte(fd, SERIAL_TIMEOUT, &response);
        cmdSend++;
    }

    if(cmdSent < MAX_COMMANDS)
    {
        return 1;
    }
    else
    {
        return -1;
    }
}

int AHE::closeALeaf()
{
   cmdSent=0; 

   while(response != POLL_A_CLOSED || cmdSent < MAX_COMMANDS)
   {
        serial_write_byte(fd, CMD_A_CLOSE);
        serial_read_byte(fd, SERIAL_TIMEOUT, &response);
        cmdSend++;
   }

   return 1;

   if(cmdSent < MAX_COMMANDS)
   {
       return 1;
   }
   else
   {
       return -1;
   }
}

int AHE::closeBLeaf()
{
    
   cmdSent=0;
   while(response != POLL_B_CLOSED || cmdSent < MAX_COMMANDS)
   {
        serial_write_byte(fd, CMD_B_CLOSE);
        serial_read_byte(fd, SERIAL_TIMEOUT, &response);
        cmdSend++;
   }

   return 1;


   if(cmdSent < MAX_COMMANDS)
   {
       return 1;
   }
   else
   {
       return -1;
   }
}

int AHE::processOption(int opt)
{
   return 0; 
}

int AHE::setValue(rts2core::Value *old_value, rts2core::Value *new_value)
{
    if(old_value == closeDome)
    {
        if(((rts2core::ValueBool*) new_value)->getValueBool() == true)
        {
            logStream(MESSAGE_DEBUG) << "Told to close dome" << sendLog;
            startClose();
        }
        else
        {
            logStream(MESSAGE_DEBUG) << "Told to open dome" << sendLog;
            startOpen();
        }
    }

    return Dome::setValue(old_value, new_value);
}

int AHE::init()
{

    logStream (MESSAGE_DEBUG) << "AHE Dome initing..." << sendLog;
    //TODO add error checking code for fd
    fd = serial_init(dev, 9600, 8, 0, 1);    

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
    }
    else
    {
        status = ERROR;
        domeStatus->setValueString("Error opening");
    }

    return 1;
}

long AHE::isOpened()
{
    return status; 
}

int AHE::endOpen()
{
   //do nothing
   return 1;
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
    }
    else
    {
        status = ERROR;
        domeStatus->setValueString("Error closing");
    }

    return 1;
}

long AHE::isClosed()
{
    
    return status; 
}

int AHE::endClose()
{
   //do nothing
   return 1;
}

AHE::AHE(int argc, char **argv):Dome(argc, argv)
{
    logStream (MESSAGE_DEBUG) << "AHE Dome constructor called" << sendLog;
    dev = "/dev/ttyS0";
    response = '\x00';

    createValue(domeStatus, "DOME_STAT", "dome status", true);
    createValue(closeDome, "DOME_CLOSE", "closing dome", false, RTS2_VALUE_WRITABLE);
    closeDome->setValueBool(true);

}

AHE::~AHE()
{
    serial_shutdown(fd);    
}

int main(int argc, char **argv)
{
    AHE device = AHE(argc, argv);
    return device.run();
}
