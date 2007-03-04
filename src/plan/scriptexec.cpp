#include "scriptexec.h"
#include "rts2execcli.h"
#include "rts2devcliphot.h"

#include <iostream>
#include <vector>

// Rts2ScriptForDevice

Rts2ScriptForDevice::Rts2ScriptForDevice (std::string in_deviceName,
					  std::string in_script)
{
  deviceName = in_deviceName;
  script = in_script;
}

Rts2ScriptForDevice::~Rts2ScriptForDevice (void)
{
}

// Rts2ScriptExec class

Rts2ScriptForDevice *
Rts2ScriptExec::findScript (std::string in_deviceName)
{
  for (std::vector < Rts2ScriptForDevice >::iterator iter = scripts.begin ();
       iter != scripts.end (); iter++)
    {
      Rts2ScriptForDevice *ds = &(*iter);
      if (ds->isDevice (in_deviceName))
	return ds;
    }
  return NULL;
}

int
Rts2ScriptExec::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'd':
      deviceName = optarg;
      break;
    case 's':
      if (!deviceName)
	{
	  std::cerr << "unknow device name" << std::endl;
	  return -1;
	}
      scripts.
	push_back (Rts2ScriptForDevice
		   (std::string (deviceName), std::string (optarg)));
      deviceName = NULL;
      break;
    default:
      return Rts2Client::processOption (in_opt);
    }
  return 0;
}

Rts2ScriptExec::Rts2ScriptExec (int in_argc, char **in_argv):Rts2Client (in_argc,
	    in_argv)
{
  scriptCount = new Rts2ValueInteger ("script_count");
  waitState = 0;
  currentTarget = NULL;

  addOption ('d', "device", 1, "name of next script device");
  addOption ('s', "script", 1, "device script (for device specified with d)");
}

Rts2ScriptExec::~Rts2ScriptExec (void)
{

}

int
Rts2ScriptExec::init ()
{
  int ret;
  ret = Rts2Client::init ();
  if (ret)
    return ret;

  // create current target
  currentTarget = new Rts2TargetScr (this);

  return 0;
}

int
Rts2ScriptExec::run ()
{
  int ret;
  ret = init ();
  if (ret)
    return ret;
  return Rts2Client::run ();
}

Rts2DevClient *
Rts2ScriptExec::createOtherType (Rts2Conn * conn, int other_device_type)
{
  Rts2DevClient *cli;
  switch (other_device_type)
    {
    case DEVICE_TYPE_MOUNT:
      cli = new Rts2DevClientTelescopeExec (conn);
      break;
    case DEVICE_TYPE_CCD:
      cli = new Rts2DevClientCameraExec (conn);
      break;
    case DEVICE_TYPE_FOCUS:
      cli = new Rts2DevClientFocusImage (conn);
      break;
/*    case DEVICE_TYPE_PHOT:
      cli = new Rts2DevClientPhotExec (conn);
      break; */
    case DEVICE_TYPE_MIRROR:
      cli = new Rts2DevClientMirrorExec (conn);
      break;
    case DEVICE_TYPE_DOME:
    case DEVICE_TYPE_SENSOR:
      cli = new Rts2DevClientWriteImage (conn);
      break;
    default:
      cli = Rts2Client::createOtherType (conn, other_device_type);
    }
  std::cout << "send EVENT_SET_TARGET to " << conn->getName () << std::endl;
  cli->postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
  return cli;
}

void
Rts2ScriptExec::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_SCRIPT_ENDED:
      scriptCount->setValueInteger (0);
      postEvent (new Rts2Event (EVENT_MOVE_QUESTION, (void *) scriptCount));
      if (scriptCount->getValueInteger () == 0)
	endRunLoop ();
      break;
    case EVENT_MOVE_OK:
      if (waitState)
	{
	  postEvent (new Rts2Event (EVENT_CLEAR_WAIT));
	  break;
	}
      postEvent (new Rts2Event (EVENT_OBSERVE));
      break;
    case EVENT_MOVE_FAILED:
      endRunLoop ();
      break;
    case EVENT_ENTER_WAIT:
      waitState = 1;
      break;
    case EVENT_CLEAR_WAIT:
      waitState = 0;
      break;
    case EVENT_GET_ACQUIRE_STATE:
      *((int *) event->getArg ()) = 1;
//      (currentTarget) ? currentTarget->getAcquired () : -2;
      break;
    }
  Rts2Client::postEvent (event);
}


int
main (int argc, char **argv)
{
  Rts2ScriptExec app = Rts2ScriptExec (argc, argv);
  return app.run ();
}
