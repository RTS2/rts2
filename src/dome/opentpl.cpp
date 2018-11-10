/*
 * Driver for OpenTPL domes.
 * Copyright (C) 2008-2009 Petr Kubanek <petr@kubanek.net>
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
#include "connection/opentpl.h"
#include "configuration.h"

#include <libnova/libnova.h>

namespace rts2dome
{

/**
 * Driver for Bootes IR dome.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class OpenTpl:public Dome
{
    private:
        HostString *openTPLServer;

        rts2core::OpenTpl *opentplConn;

        rts2core::ValueFloat *domeUp;
        rts2core::ValueFloat *domeDown;

        rts2core::ValueDouble *domeCurrAz;
        rts2core::ValueDouble *domeTargetAz;
        rts2core::ValueBool *domePower;
        rts2core::ValueDouble *domeTarDist;

        rts2core::ValueBool *domeAutotrack;

        /**
         * Set dome autotrack.
         *
         * @param new_auto New auto track value. True or false.
         *
         * @return -1 on error, 0 on success.
         */
        int setDomeTrack (bool new_auto);
        int initOpenTplDevice ();

    protected:
        virtual int processOption (int in_opt);

        virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

    public:
        OpenTpl (int argc, char **argv);
        virtual ~ OpenTpl (void);
        virtual int init ();

        virtual int info ();

        virtual int startOpen ();
        virtual long isOpened ();
        virtual int endOpen ();

        virtual int startClose ();
        virtual long isClosed ();
        virtual int endClose ();
};

}

using namespace rts2dome;

int OpenTpl::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
    int status = TPL_OK;
    if (old_value == domeAutotrack)
    {
        status = setDomeTrack (((rts2core::ValueBool *) new_value)->getValueBool ());
        if (status != TPL_OK)
            return -2;
        return 0;
    }
    if (old_value == domeUp)
    {
        status = opentplConn->set ("DOME[1].TARGETPOS", new_value->getValueFloat (), &status);
        if (status != TPL_OK)
            return -2;
        return 0;
    }
    if (old_value == domeDown)
    {
        status = opentplConn->set ("DOME[2].TARGETPOS", new_value->getValueFloat (), &status);
        if (status != TPL_OK)
            return -2;
        return 0;
    }
    if (old_value == domeTargetAz)
    {
        status = opentplConn->set ("DOME[0].TARGETPOS",
            ln_range_degrees (new_value->getValueDouble () - 180.0),
            &status);
        if (status != TPL_OK)
            return -2;
        return 0;

    }
    if (old_value == domePower)
    {
        status =  opentplConn->set ("DOME[0].POWER",
            ((rts2core::ValueBool *) new_value)->getValueBool ()? 1 : 0,
            &status);
        if (status != TPL_OK)
            return -2;
        return 0;

    }
    return Dome::setValue (old_value, new_value);
}

int OpenTpl::startOpen ()
{
    int status = TPL_OK;
    status = opentplConn->set ("DOME[1].TARGETPOS", 1, &status);
    status = opentplConn->set ("DOME[2].TARGETPOS", 1, &status);
    logStream (MESSAGE_INFO) << "opening dome, status " << status << sendLog;
    if (status != TPL_OK)
        return -1;
    return 0;
}

long OpenTpl::isOpened ()
{
    int status = TPL_OK;
    double pos1, pos2;
    status = opentplConn->get ("DOME[1].CURRPOS", pos1, &status);
    status = opentplConn->get ("DOME[2].CURRPOS", pos2, &status);

    if (status != TPL_OK)
        return -1;

    if (pos1 == 1 && pos2 == 1)
        return -2;

    return USEC_SEC;
}

int OpenTpl::endOpen ()
{
    return 0;
}

int OpenTpl::startClose ()
{
    int status = TPL_OK;
    status = opentplConn->set ("DOME[1].TARGETPOS", 0, &status);
    status = opentplConn->set ("DOME[2].TARGETPOS", 0, &status);
    logStream (MESSAGE_INFO) << "closing dome, status " << status << sendLog;
    if (status != TPL_OK)
        return -1;
    return 0;
}

long OpenTpl::isClosed ()
{
    int status = TPL_OK;
    double pos1, pos2;
    status = opentplConn->get ("DOME[1].CURRPOS", pos1, &status);
    status = opentplConn->get ("DOME[2].CURRPOS", pos2, &status);

    if (status != TPL_OK)
        return -1;

    if (pos1 == 0 && pos2 == 0)
        return -2;

    return USEC_SEC;
}

int OpenTpl::endClose ()
{
    return 0;
}

int OpenTpl::processOption (int in_opt)
{
    switch (in_opt)
    {
        case OPT_OPENTPL_SERVER:
            openTPLServer = new HostString (optarg, "65432");
            break;
        default:
            return Dome::processOption (in_opt);
    }
    return 0;
}

OpenTpl::OpenTpl (int argc, char **argv):Dome (argc, argv)
{
    openTPLServer = NULL;
    opentplConn = NULL;

    createValue (domeAutotrack, "dome_auto_track", "dome auto tracking", false, RTS2_VALUE_WRITABLE);
    domeAutotrack->setValueBool (true);

    createValue (domeUp, "dome_up", "upper dome cover", false, RTS2_VALUE_WRITABLE);
    createValue (domeDown, "dome_down", "dome down cover", false, RTS2_VALUE_WRITABLE);

    createValue (domeCurrAz, "dome_curr_az", "dome current azimunt", false, RTS2_DT_DEGREES);
    createValue (domeTargetAz, "dome_target_az", "dome targer azimut", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
    createValue (domePower, "dome_power", "if dome have power", false, RTS2_VALUE_WRITABLE);
    createValue (domeTarDist, "dome_tar_dist", "dome target distance", false, RTS2_DT_DEG_DIST);

    addOption (OPT_OPENTPL_SERVER, "opentpl", 1, "OpenTPL server TCP/IP address and port (separated by :)");
}


OpenTpl::~OpenTpl (void)
{

}

int OpenTpl::setDomeTrack (bool new_auto)
{
    int status = TPL_OK;
    int old_track;
    status = opentplConn->get ("POINTING.TRACK", old_track, &status);
    if (status != TPL_OK)
        return -1;
    old_track &= ~128;
    status = opentplConn->set ("POINTING.TRACK", old_track | (new_auto ? 128 : 0), &status);
    if (status != TPL_OK)
    {
        logStream (MESSAGE_ERROR) << "Cannot setDomeTrack" << sendLog;
    }
    return status;
}

int OpenTpl::initOpenTplDevice ()
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

int OpenTpl::info ()
{
    double dome_curr_az, dome_target_az, dome_tar_dist, dome_power, dome_up, dome_down;
    int status = TPL_OK;
    status = opentplConn->get ("DOME[0].CURRPOS", dome_curr_az, &status);
    status = opentplConn->get ("DOME[0].TARGETPOS", dome_target_az, &status);
    status = opentplConn->get ("DOME[0].TARGETDISTANCE", dome_tar_dist, &status);
    status = opentplConn->get ("DOME[0].POWER", dome_power, &status);
    status = opentplConn->get ("DOME[1].CURRPOS", dome_up, &status);
    status = opentplConn->get ("DOME[2].CURRPOS", dome_down, &status);
    if (status == TPL_OK)
    {
        domeCurrAz->setValueDouble (ln_range_degrees (dome_curr_az + 180));
        domeTargetAz->setValueDouble (ln_range_degrees (dome_target_az + 180));
        domeTarDist->setValueDouble (dome_tar_dist);
        domePower->setValueBool (dome_power == 1);
        domeUp->setValueFloat (dome_up);
        domeDown->setValueFloat (dome_down);
    }

    return Dome::info ();
}

int OpenTpl::init ()
{
    int ret = Dome::init ();
    if (ret)
        return ret;
    return initOpenTplDevice ();
}

int main (int argc, char **argv)
{
    OpenTpl device (argc, argv);
    return device.run ();
}
