/* 
 * Dome driver for 7 Foot Astrohaven Enterprise Domes w/ Vision 130 controller
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
    if(readSerial(CMD_A_OPEN, response, POLL_A_OPENED) > MAX_COMMANDS)
    {
        return -1;
    }

    return 1;
}

int AHE::openBLeaf()
{
    if(readSerial(CMD_B_OPEN, response, POLL_B_OPENED) > MAX_COMMANDS) 
    {
        return -1;
    }

    return 1;
}

int AHE::closeALeaf()
{
    if(readSerial(CMD_A_CLOSE, response, POLL_A_CLOSED) > MAX_COMMANDS) 
    {
        return -1;
    }

    return 1;
}

int AHE::closeBLeaf()
{
    if(readSerial(CMD_B_CLOSE, response, POLL_B_CLOSED) > MAX_COMMANDS) 
    {
        return -1;
    }

    return 1;
}

int AHE::readSerial(const char send, char reply, const char exitState)
{//since the controller write a heart beat have to iterate through it
 //all to find what we are looking for, reason why whe couldn't cleanly
 //use readPort (char *rbuf, int b_len, char endChar)
    
    cmdSent = 0;
    reply = 'L';

    while(reply != exitState && cmdSent < MAX_COMMANDS)
    {
        sconn->writePort(send);
        usleep(SERIAL_SLEEP); 
        sconn->readPort(reply);
        logStream(MESSAGE_DEBUG) << "Just read port" << sendLog;
        cmdSent ++;
    }

    return cmdSent;
}


int AHE::processOption(int opt)
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

int AHE::setValue(rts2core::Value *old_value, rts2core::Value *new_value)
{
    if(old_value == closeDome)
    {
        if(new_value->getValueInteger() == 1)
        {
            startOpen();
        }
        else if(new_value->getValueInteger() == 0)
        {
            startClose();
        }
    }
    else if(old_value == ignoreComm)
    {
        if(new_value->getValueInteger() == 0)
        {//false
            ignoreCommands = false;
        }
        else if(new_value->getValueInteger() == 1)
        {//true
            ignoreCommands = true;
        }
    }

    return Dome::setValue(old_value, new_value);
}

int AHE::init()
{

    sconn = new rts2core::ConnSerial(devFile, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 50);
    sconn->init();

    int ret = Dome::init();
    if(ret)
    {
        return ret;
    }

    return 0;
}

char AHE::getHeartBeat()
{
    response = '\x00';
    while(sconn->readPort(response) == -1)
    {
    //    usleep(SERIAL_SLEEP);
    }

    return response;

}

int AHE::info()
{
    response = getHeartBeat();

    switch(response){
        case STATUS_AB_CLOSED:
            domeStatus->setValueString("Closed");
            status = CLOSED;
            break;
        case STATUS_A_OPEN:
            domeStatus->setValueString("A Open");
            status = OPENED;
            break;
        case STATUS_B_OPEN:
            domeStatus->setValueString("B Open");
            status = OPENED;
            break;
        case STATUS_AB_OPEN:
            domeStatus->setValueString("Open");
            status = OPENED;
    }

    return Dome::info();
}


int AHE::startOpen()
{
    if(ignoreCommands)
    {//dead man switch turned on
        return 1;
    }

    status = OPENING;
    domeStatus->setValueString("Opening....");
    sendValueAll(domeStatus);
    if(openALeaf() && openBLeaf())
    {
        status = OPENED;
        domeStatus->setValueString("Opened");
        closeDome->setValueString("Open");
        
        return 0;
    }
    else
    {
        status = ERROR;
        domeStatus->setValueString("Error opening");
    }

    return 1;
}

int AHE::startClose()
{
    if(ignoreCommands)
    {//dead man switch turned on
        return 1;
    }

    status = CLOSING;
    domeStatus->setValueString("Closing....");
    sendValueAll(domeStatus);
    if(closeALeaf() && closeBLeaf())
    {
        status = CLOSED;
        domeStatus->setValueString("Closed");
        closeDome->setValueString("Close");
        return 0;
    }
    else
    {
        status = ERROR;
        domeStatus->setValueString("Error closing");
    }

    return 1;

}

long AHE::isOpened()
{
    if (status == OPENED)
    {
        return -2;
    }

    return 2000;
}

long AHE::isClosed()
{
    if (status == CLOSED)
    {
        return -2;
    }

    return 2000;
}

int AHE::endOpen()
{
   //do nothing
   return 0;
}



int AHE::endClose()
{
   //do nothing
   domeStatus->setValueString("Closed");
   return 0;
}


AHE::AHE(int argc, char **argv):Dome(argc, argv)
{
    logStream (MESSAGE_DEBUG) << "AHE Dome constructor called" << sendLog;
    devFile = "/dev/ttyS0";
    response = '\x00';
    ignoreCommands = false;


    createValue(domeStatus, "status", "current dome status", true);
    createValue(closeDome, "state", "switch state of dome", false, RTS2_VALUE_WRITABLE);
    createValue(ignoreComm,"ignore_com","driver will ignore commands (not open/close dome)", false, RTS2_VALUE_WRITABLE);

    addOption('f',NULL,1, "path to device, default is /dev/ttyUSB0");

    closeDome->addSelVal("Close");
    closeDome->addSelVal("Open");

    ignoreComm->addSelVal("False");
    ignoreComm->addSelVal("True");



    setIdleInfoInterval(5);
}

AHE::~AHE()
{
}

int main(int argc, char **argv)
{
    AHE device(argc, argv);
    return device.run();
}
