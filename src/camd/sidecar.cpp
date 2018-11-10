/*
 * Teledyne SIDECAR driver. Needs IDL server to connect to SIDECAR hardware.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "camd.h"
#include "dirsupport.h"
#include "utilsfunc.h"
#include "connection/tcp.h"

#include <dirent.h>

#define OPT_IMAGE_DIR                OPT_LOCAL + 727
#define OPT_IMAGE_SUFFIX_FSRAMP      OPT_LOCAL + 728
#define OPT_IMAGE_SUFFIX_UPTHERAMP   OPT_LOCAL + 729

namespace rts2camd
{


/**
 * Sidecar connection class. Utility class to handle connections to the
 * sidecar. Please note that you can easily create two instances (objects)
 * inside a single Camera, if you need it for synchronizing the commanding.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SidecarConn:public rts2core::ConnTCP
{
    public:
        SidecarConn (rts2core::Block * _master, std::string _hostname, int _port):ConnTCP (_master, _hostname.c_str (), _port) {}
        virtual ~SidecarConn () {}

        /**
         * Send command over TCP/IP, get response.
         *
         * @param cmd    command to be send
         * @param _is    reply from the device
         * @param wtime  time to wait for reply in seconds
         */
        void sendCommand (const char *cmd, std::istringstream **_is, int wtime = 10);

        void sendCommand (const char *cmd, double p1, std::istringstream **_is, int wtime = 10);

        /**
         * Call ping command, return camera status. Camera status values:
         *
         *  - <b>0</b> The system is idle
         *  - <b>1</b> Exposure is in progress
         *  - <b>2</b> Ramp acqusition succeeded
         *
         * @return camera status. See discussion above for success codes. On error, -1 is returned.
         */
        int pingCommand ();

        /**
         * Call method on server.
         *
         * @param method   method name
         * @param p1       first parameter
         * @param _is      string output (return of the call)
         * @param wtime    wait time (in seconds)
         */
        // this does not return anything. It only returns possible output of command in _is. You might want to parse
        // method output to get more, e.g. status of the call
        void callMethod (const char *method, int p1, std::istringstream **_is, int wtime = 10);

        /**
         * Call method on server with 5 parameters of arbitary type.
         *
         * @see callMethod(cmd,p1,_is,wtime)
         */
        template < typename t1, typename t2, typename t3, typename t4, typename t5 > void callMethod (const char *cmd, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, std::istringstream **_is, int wtime = 10)
        {
            std::ostringstream os;
            os << std::fixed << cmd << "(" << p1 << "," << p2 << "," << p3 << "," << p4 << "," << p5 << ")";
            sendCommand (os.str().c_str (), _is, wtime);
        }
};

/**
 * Class for sidecar camera.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Sidecar:public Camera
{
    public:
        Sidecar (int in_argc, char **in_argv);
        virtual ~Sidecar ();


    protected:
        /**
         * Process -t option (Teledyne server IP).
         */
        virtual int processOption (int in_opt);

        virtual int init ();
        virtual int initChips ();
        virtual int info ();

        // called when variable is changed
        virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);
        virtual void setExposure (double exp);

        virtual int startExposure ();
        virtual long isExposing ();
        virtual int stopExposure ();
        virtual int doReadout ();

    private:
        int width;
        int height;

        HostString *sidecarServer;
        SidecarConn *sidecarConn;

        // path to the last data
        rts2core::ValueString *imageDir;
            rts2core::ValueString *lastDataDir;
        rts2core::ValueString *fileSuffixFSRamp;
        rts2core::ValueString *fileSuffixUpTheRamp;

        // internal variables
        rts2core::ValueSelection *fsMode;
        rts2core::ValueInteger *nResets;
        rts2core::ValueInteger *nReads;
        rts2core::ValueInteger *nGroups;
        rts2core::ValueInteger *nDropFrames;
        rts2core::ValueInteger *nRamps;
        rts2core::ValueSelection *gain;
        rts2core::ValueSelection *ktcRemoval;
        rts2core::ValueSelection *warmTest;
        rts2core::ValueSelection *idleMode;
        rts2core::ValueSelection *enhancedClocking;

        int parseConfig (std::istringstream *is);
};

};

using namespace rts2camd;

void SidecarConn::sendCommand (const char *cmd, std::istringstream **_is, int wtime)
{
    sendData (cmd);
    sendData ("\r\n");
    receiveData (_is, wtime, '\n');
}

void SidecarConn::sendCommand (const char *cmd, double p1, std::istringstream **_is, int wtime)
{
    std::ostringstream os;
    os << cmd << " " << p1;
    sendCommand (os.str ().c_str (), _is, wtime);
}

int SidecarConn::pingCommand ()
{
    std::istringstream *is = NULL;
    sendCommand ("Ping", &is);

    char lineb[200];
    is->getline (lineb, 200);

    int ret = -1;

    if (is->fail ())
    {
        logStream (MESSAGE_ERROR) << "cannot parse ping reply" << sendLog;
        ret = -1;
    }
    else
    {
        const char *responses[3] = {"0:The system is idle\r", "-1:Exposure is in progress\r", "0:Ramp acquisition succeeded\r"};
        for (ret = 0; ret < 3; ret++)
        {
            if (strcmp(responses[ret],lineb) == 0)
                break;
        }
        if (ret == 3)
        {
            logStream (MESSAGE_ERROR) << "unknown ping response: " << lineb << sendLog;
            delete is;
            return -1;
        }
    }

    delete is;
    return ret;
}

void SidecarConn::callMethod (const char *method, int p1, std::istringstream **_is, int wtime)
{
    std::ostringstream os;
    os << method << "(" << p1 << ")";
    sendCommand (os.str ().c_str (), _is, wtime);
}

Sidecar::Sidecar (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
    sidecarServer = NULL;
    sidecarConn = NULL;

    createTempCCD ();
    createExpType ();

    createValue (imageDir, "image_dir", "current image directory", false, RTS2_VALUE_WRITABLE);
    createValue (lastDataDir, "last_data", "last data directory", false);
    createValue (fileSuffixFSRamp, "file_suffix_fsramp", "file suffix for FS Ramp (will be added to last data directory)", false, RTS2_VALUE_WRITABLE);
    createValue (fileSuffixUpTheRamp, "file_suffix_uptheramp", "file suffix for UpTheRamp (will be added to last data directory)", false, RTS2_VALUE_WRITABLE);

    // name for fsMode, as shown in rts2-mon, will be fs_mode, "mode of.. " is the comment.
    // false means that it will not be recorded to FITS
    // RTS2_VALUE_WRITABLE means you can change value from monitor
    // CAM_WORKING means the value can only be set if camera is not exposing or reading the image
    createValue (fsMode, "fs_mode", "mode of the chip exposure (Up The Ramp[0] vs Fowler[1])", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    fsMode->addSelVal ("0 Up The Ramp");
    fsMode->addSelVal ("1 Fowler Sampling");
    fsMode->setValueInteger(0);

    createValue (nResets, "n_resets", "number of reset frames", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    nResets->setValueInteger(0);
    createValue (nReads, "n_reads", "number of read frames", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    nReads->setValueInteger(1);
    createValue (nGroups, "n_groups", "number of groups", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    nGroups->setValueInteger(1);
    createValue (nDropFrames, "n_drop_frames", "number of drop frames", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    nDropFrames->setValueInteger(1);
    createValue (nRamps, "n_ramps", "number of ramps", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    nRamps->setValueInteger(1);

    createValue (gain, "gain", "set the gain. 0-15", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    gain->addSelVal ("0   -3dB, small Cin");
    gain->addSelVal ("1    0dB, small Cin");
    gain->addSelVal ("2    3dB, small Cin");
    gain->addSelVal ("3    6dB, small Cin");
    gain->addSelVal ("4    6dB, large Cin");
    gain->addSelVal ("5    9dB, small Cin");
    gain->addSelVal ("6    9dB, large Cin");
    gain->addSelVal ("7   12dB, small Cin");
    gain->addSelVal ("8   12dB, large Cin");
    gain->addSelVal ("9   15dB, small Cin");
    gain->addSelVal ("10  15dB, large Cin");
    gain->addSelVal ("11  18dB, small Cin");
    gain->addSelVal ("12  18dB, large Cin");
    gain->addSelVal ("13  21dB, large Cin");
    gain->addSelVal ("14  24dB, large Cin");
    gain->addSelVal ("15  27dB, large Cin");
    gain->setValueInteger(13);

    createValue (ktcRemoval, "preamp_ktc_removal", "turn on (1) or off (0) the preamp KTC removal", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    ktcRemoval->addSelVal ("0 Off");
    ktcRemoval->addSelVal ("1 On");
    ktcRemoval->setValueInteger(0);

    createValue (warmTest, "warm_test", "set for warm (1) or cold (0)", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    warmTest->addSelVal ("0 Cold");
    warmTest->addSelVal ("1 Warm");
    warmTest->setValueInteger(0);

    // Not sure why, but changing the IdleModeOption in the rts2-mon does not affect a change
    // in the HxRG Socket Server. The SS does show receiving the TCP command, though.
    createValue (idleMode, "idle_mode", "do nothing (0), continuously reset (1), or continuously read-reset (2)", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    idleMode->addSelVal ("0 Do Nothing");
    idleMode->addSelVal ("1 Continuous Resets");
    idleMode->addSelVal ("2 Continuous Read-Resets");
    idleMode->setValueInteger(1);

    createValue (enhancedClocking, "clocking", "normal (0) or enhanced (1)", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    enhancedClocking->addSelVal ("0 Normal");
    enhancedClocking->addSelVal ("1 Enhanced");
    enhancedClocking->setValueInteger(1);

    width = 2048;
    height = 2048;

    addOption ('t', NULL, 1, "Teledyne host name and double colon separated port (port defaults to 5000)");
    addOption (OPT_IMAGE_DIR, "image-dir", 1, "path to images");
    addOption (OPT_IMAGE_SUFFIX_FSRAMP, "image-suffix-fsramp", 1, "suffix for FSRamp mode to add to newly found directory to get to image");
    addOption (OPT_IMAGE_SUFFIX_UPTHERAMP, "image-suffix-uptheramp", 1, "suffix for UpTheRamp mode to add to newly found directory to get to image");
}

Sidecar::~Sidecar ()
{
}

int Sidecar::processOption (int in_opt)
{
    switch (in_opt)
    {
        case 't':
            sidecarServer = new HostString (optarg, "5000");
            break;
        case OPT_IMAGE_DIR:
            {
                std::string imgd (optarg);
                if (imgd[imgd.length() - 1] != '/')
                    imgd += "/";
                imageDir->setValueCharArr (imgd.c_str ());
            }
            break;
        case OPT_IMAGE_SUFFIX_FSRAMP:
            fileSuffixFSRamp->setValueCharArr (optarg);
            break;
        case OPT_IMAGE_SUFFIX_UPTHERAMP:
            fileSuffixUpTheRamp->setValueCharArr (optarg);
            break;
        default:
            return Camera::processOption (in_opt);
    }
    return 0;
}

int Sidecar::init ()
{
    int ret;
    ret = Camera::init ();
    if (ret)
        return ret;

    ccdRealType->setValueCharArr ("SIDECAR");
    serialNumber->setValueCharArr ("1");

    if (sidecarServer == NULL)
    {
        std::cerr << "Invalid port or IP address of Teledyne server" << std::endl;
        return -1;
    }

    try
    {
        sidecarConn = new SidecarConn (this, sidecarServer->getHostname (), sidecarServer->getPort ());
        // Images are scp'd back to linux rts2 machine via a python TCP server running on the windows machine.
        // The port address of this image retrieval server is one higher (5001) than the port of the
        // HxRG Socket Server (5000).
        sidecarConn->init ();
        sidecarConn->setDebug ();
        // initialize system..
        sidecarConn->pingCommand ();
        // std::istringstream *is;
        // sidecarConn->sendCommand ("Initialize3", &is, 45);
        // delete is;
    }
    catch (rts2core::ConnError &er)
    {
        logStream (MESSAGE_ERROR) << er << sendLog;
        return -1;
    }

    return initChips ();
}

int Sidecar::initChips ()
{
    initCameraChip (width, height, 0, 0);
    return Camera::initChips ();
}

int Sidecar::parseConfig (std::istringstream *is)
{
    while (!is->eof ())
    {
        std::string var;
        *is >> var;
        logStream (MESSAGE_DEBUG) << "got " << var << sendLog;
        // parse it..
        size_t pos = var.find('=');
        if (pos == std::string::npos)
        {
            logStream (MESSAGE_DEBUG) << "cannot find = in '" << var << "', ignoring this line" << sendLog;
            continue;
        }
        // variable name and variable value
        std::string vn = var.substr (0, pos - 1);
        std::string vv = var.substr (pos + 1);
        rts2core::Value *vp = getOwnValue (vn.c_str ());
        if (vp == NULL)
        {
            logStream (MESSAGE_DEBUG) << "cannot find value with name " << vn << ", creating it" << sendLog;
            rts2core::ValueDouble *vd;
            createValue (vd, vn.c_str (), "created by parseConfing", false);
            updateMetaInformations (vd);
            vp = vd;
        }
        vp->setValueCharArr (vv.c_str ());
    }
    return 0;
}

int Sidecar::info ()
{
    sidecarConn->pingCommand ();
    // std::istringstream *is;
    // sidecarConn->sendCommand ("GetConfig", &is);
    // parseConfig (is);
    // delete is;
    // sidecarConn->sendCommand ("GetTelemetry", &is, 120);
    // delete is;
    return Camera::info ();
}

int Sidecar::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
    std::istringstream *is = NULL;
    if (old_value == fsMode)
    {
        // this will send SetFSMode(0) (or 1..) on sidecar connection
        sidecarConn->callMethod ("SetFSMode",new_value->getValueInteger (), &is);
        // if that will return anything, we shall process output found in is..can be done in callMethod
        delete is;
        return 0;
    }

    else if (old_value == nResets)
    {
        if (new_value->getValueInteger () > 300)
            return -2;
        sidecarConn->callMethod ("SetRampParam", new_value->getValueInteger (), nReads->getValueInteger (), nGroups->getValueInteger (), nDropFrames->getValueInteger (), nRamps->getValueInteger (), &is);
        delete is;
        return 0;
    }

    else if (old_value == nReads)
    {
        if (new_value->getValueInteger () < 1 || new_value->getValueInteger () > 32)
            return -2;
        sidecarConn->callMethod ("SetRampParam", nResets->getValueInteger (), new_value->getValueInteger (), nGroups->getValueInteger (), nDropFrames->getValueInteger (), nRamps->getValueInteger (), &is);
        delete is;
        return 0;
    }

    else if (old_value == nGroups)
    {
        if (new_value->getValueInteger () > 32)
            return -2;
        sidecarConn->callMethod ("SetRampParam", nResets->getValueInteger (), nReads->getValueInteger (), new_value->getValueInteger (), nDropFrames->getValueInteger (), nRamps->getValueInteger (), &is);
        delete is;
        return 0;
    }

    else if (old_value == nDropFrames)
    {
        if (new_value->getValueInteger () > 128)
            return -2;
        sidecarConn->callMethod ("SetRampParam", nResets->getValueInteger (), nReads->getValueInteger (), nGroups->getValueInteger (), new_value->getValueInteger (), nRamps->getValueInteger (), &is);
        delete is;
        return 0;
    }

    else if (old_value == nRamps)
    {
        if (new_value->getValueInteger () > 32)
            return -2;
        sidecarConn->callMethod ("SetRampParam", nResets->getValueInteger (), nReads->getValueInteger (), nGroups->getValueInteger (), nDropFrames->getValueInteger (), new_value->getValueInteger (), &is);
        delete is;
        return 0;
    }

    else if (old_value == gain)
    {
        sidecarConn->callMethod ("SetGain",new_value->getValueInteger (), &is);
        delete is;
        return 0;
    }

    else if (old_value == ktcRemoval)
    {
        sidecarConn->callMethod ("SetKTCRemoval",new_value->getValueInteger (), &is);
        delete is;
        return 0;
    }

    else if (old_value == warmTest)
    {
        sidecarConn->callMethod ("SetWarmTest",new_value->getValueInteger (), &is);
        delete is;
        return 0;
    }

    else if (old_value == idleMode)
    {
        sidecarConn->callMethod ("SetIdleModeOption",new_value->getValueInteger (), &is);
        delete is;
        return 0;
    }

    else if (old_value == enhancedClocking)
    {
        sidecarConn->callMethod ("SetEnhancedClk",new_value->getValueInteger (), &is);
        delete is;
        return 0;
    }

    return Camera::setValue (old_value, new_value);
}

void Sidecar::setExposure (double exp)
{
    std::istringstream *is = NULL;
    sidecarConn->callMethod ("SetFSParam", nResets->getValueInteger (), nReads->getValueInteger (), nGroups->getValueInteger (), exp, nRamps->getValueInteger (), &is);
    delete is;

    Camera::setExposure (exp);
}

int Sidecar::startExposure ()
{
    setFitsTransfer ();
    std::istringstream *is = NULL;
    sidecarConn->sendCommand ("AcquireRamp", &is, getExposure () + 100);
    delete is;
    // data are stored in local directory
    return 0;
}

long Sidecar::isExposing ()
{
    int ret = Camera::isExposing ();
    if (ret > 0)
        return ret;

    ret = sidecarConn->pingCommand ();
    std::cout << "isExposing ret " << ret << std::endl;
    switch (ret)
    {
        case 1:
            return 100;
    }
    return -2;
}

int Sidecar::stopExposure ()
{
    return 0;
}

int Sidecar::doReadout ()
{
    // get filename
    struct dirent **res;
    // construct dirname - it depends on mode
    std::string imgdir = imageDir->getValueString () + (fsMode->getValueInteger () == 0 ? "UpTheRamp" : "FSRamp");
    int nd = scandir (imgdir.c_str (), &res, NULL, alphasort);

    int i;

    if (nd < 0)
    {
        logStream (MESSAGE_ERROR) << "cannot find files in " << imgdir << ":" << strerror (errno) << sendLog;
        return -1;
    }

    for (i = 0; i < nd; i++)
    {
        if (strcoll (res[i]->d_name, lastDataDir->getValueString ().c_str ()) > 0)
        {
            lastDataDir->setValueCharArr (res[i]->d_name);
        }
        free (res[i]);
    }

    sendValueAll (lastDataDir);

    // pass it as FITS data..

    std::ostringstream os;
    os << imgdir << "/" << lastDataDir->getValueString () << "/" << (fsMode->getValueInteger () == 0 ? fileSuffixUpTheRamp->getValueString () : fileSuffixFSRamp->getValueString ());

    fitsDataTransfer (os.str ().c_str ());

    free (res);

    return -2;
}

int main (int argc, char **argv)
{
    Sidecar device (argc, argv);
    return device.run ();
}
