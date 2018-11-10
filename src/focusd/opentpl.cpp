/*
 * OpenTpl focuser control.
 * Copyright (C) 2005-2007 Stanislav Vitek
 * Copyright (C) 2005-2009 Petr Kubanek <petr@kubanek.net>
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

#define DEBUG_EXTRA

#include "connection/opentpl.h"
#include "configuration.h"

#include "focusd.h"

namespace rts2focusd
{

class OpenTpl:public Focusd
{
    private:
        HostString *openTPLServer;

        rts2core::OpenTpl *opentplConn;

        rts2core::ValueFloat *realPos;

        int initOpenTplDevice ();

    protected:
        virtual int init ();
        virtual int initValues ();
        virtual int info ();

        virtual int endFocusing ();
        virtual bool isAtStartPosition ();

        virtual int setTo (double num);
        virtual double tcOffset () {return 0.;};
        virtual int isFocusing ();
    public:
        OpenTpl (int argc, char **argv);
        virtual ~ OpenTpl (void);
        virtual int processOption (int in_opt);
};

};

using namespace rts2focusd;

OpenTpl::OpenTpl (int argc, char **argv):Focusd (argc, argv)
{
    openTPLServer = NULL;
    opentplConn = NULL;

    createValue (realPos, "FOC_REAL", "real position of the focuser", true, 0, 0);

    addOption (OPT_OPENTPL_SERVER, "opentpl", 1, "OpenTPL server TCP/IP address and port (separated by :)");
}


OpenTpl::~OpenTpl ()
{
    delete opentplConn;
}


int
OpenTpl::processOption (int in_opt)
{
    switch (in_opt)
    {
        case OPT_OPENTPL_SERVER:
            openTPLServer = new HostString (optarg, "65432");
            break;
        default:
            return Focusd::processOption (in_opt);
    }
    return 0;
}


int
OpenTpl::initOpenTplDevice ()
{
    std::string ir_ip;
    int ir_port = 0;
    if (openTPLServer)
    {
        ir_ip = openTPLServer->getHostname ();
        ir_port = openTPLServer->getPort ();
    }
    else
    {
        rts2core::Configuration *config = rts2core::Configuration::instance ();
        config->loadFile (NULL);
        // try to get default from config file
        config->getString ("ir", "ip", ir_ip);
        config->getInteger ("ir", "port", ir_port);
    }

    if (ir_ip.length () == 0 || !ir_port)
    {
        std::cerr << "Invalid port or IP address of mount controller PC"
            << std::endl;
        return -1;
    }

    try
    {
        opentplConn = new rts2core::OpenTpl (this, ir_ip, ir_port);
        opentplConn->init ();
    }
    catch (rts2core::ConnError &er)
    {
        logStream (MESSAGE_ERROR) << er << sendLog;
        return -1;
    }

    return 0;
}


int
OpenTpl::init ()
{
    int ret;
    ret = Focusd::init ();
    if (ret)
        return ret;

    ret = initOpenTplDevice ();
    return ret;
}


int
OpenTpl::initValues ()
{
    focType = std::string ("FIR");
    return Focusd::initValues ();
}


int
OpenTpl::info ()
{
    int status = 0;

    double f_realPos;
    status = opentplConn->get ("FOCUS.REALPOS", f_realPos, &status);
    if (status)
        return -1;

    realPos->setValueFloat (f_realPos);

    return Focusd::info ();
}

int OpenTpl::setTo (double num)
{
    int status = 0;

    int power = 1;
    int referenced = 0;
    double offset = focusingOffset->getValueDouble () + tempOffset->getValueDouble ();

    status = opentplConn->get ("FOCUS.REFERENCED", referenced, &status);
    if (referenced != 1)
    {
        logStream (MESSAGE_ERROR) << "setTo referenced is: " << referenced << sendLog;
        return -1;
    }
    status = opentplConn->setww ("FOCUS.POWER", power, &status);
    if (status)
    {
        logStream (MESSAGE_ERROR) << "setTo cannot set POWER to 1" << sendLog;
    }
    status = opentplConn->setww ("FOCUS.TARGETPOS", offset, &status);
    if (status)
    {
        logStream (MESSAGE_ERROR) << "setTo cannot set offset!" << sendLog;
        return -1;
    }
    position->setValueDouble (num);
    return 0;
}

int OpenTpl::isFocusing ()
{
    double targetdistance;
    int status = 0;
    status = opentplConn->get ("FOCUS.TARGETDISTANCE", targetdistance, &status);
    if (status)
    {
        logStream (MESSAGE_ERROR) << "focuser IR isFocusing status: " << status
            << sendLog;
        return -1;
    }
    return (fabs (targetdistance) < 0.001) ? -2 : USEC_SEC / 50;
}

int OpenTpl::endFocusing ()
{
    int status = 0;
    int power = 0;
    status = opentplConn->setww ("FOCUS.POWER", power, &status);
    if (status)
    {
        logStream (MESSAGE_ERROR) << "focuser IR endFocusing cannot set POWER to 0" << sendLog;
        return -1;
    }
    return 0;
}

bool OpenTpl::isAtStartPosition ()
{
    int ret;
    ret = info ();
    if (ret)
        return false;
    return (fabs ((float) getPosition () - 15200 ) < 100);
}

int main (int argc, char **argv)
{
    OpenTpl device = OpenTpl (argc, argv);
    return device.run ();
}
