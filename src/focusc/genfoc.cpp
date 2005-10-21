#include "genfoc.h"

#include <iostream>
#include <iomanip>
#include <vector>

Rts2GenFocCamera::Rts2GenFocCamera (Rts2Conn * in_connection, Rts2GenFocClient * in_master):Rts2DevClientCameraFoc (in_connection,
			in_master->
			getExePath ())
{
  master = in_master;

  exposureTime = master->defaultExpousure ();
  autoDark = master->getAutoDark ();

  average = 0;

  low = med = hig = 0;

  autoSave = master->getAutoSave ();
}

Rts2GenFocCamera::~Rts2GenFocCamera (void)
{
  std::list < fwhmData * >::iterator fwhm_iter;
  for (fwhm_iter = fwhmDatas.begin (); fwhm_iter != fwhmDatas.end ();
       fwhm_iter++)
    {
      fwhmData *dat;
      dat = *fwhm_iter;
      delete dat;
    }
  fwhmDatas.clear ();
}

void
Rts2GenFocCamera::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_START_EXPOSURE:
      exposureCount = (exe != NULL) ? 1 : -1;
      break;
    case EVENT_STOP_EXPOSURE:
      exposureCount = 0;
      break;
    }
  Rts2DevClientCameraFoc::postEvent (event);
}

void
Rts2GenFocCamera::getPriority ()
{
  if (exposureCount)
    queExposure ();
}

void
Rts2GenFocCamera::lostPriority ()
{
  std::cout << "Priority lost" << std::endl;
  exposureCount = 0;
}

void
Rts2GenFocCamera::stateChanged (Rts2ServerState * state)
{
  std::cout << "State changed (" << getName () << "): " << state->
    getName () << " value:" << state->getValue () << std::endl;
  Rts2DevClientCameraFoc::stateChanged (state);
}

Rts2Image *
Rts2GenFocCamera::createImage (const struct timeval *expStart)
{
  char *filename;
  Rts2Image *image;
  if (autoSave)
    {
      image = Rts2DevClientCameraFoc::createImage (expStart);
      image->keepImage ();
      return image;
    }
  if (exe)
    {
      asprintf (&filename, "!/tmp/%s_%i.fits", connection->getName (),
		getpid ());
      image = new Rts2Image (filename, expStart);
      image->keepImage ();
      free (filename);
      return image;
    }
  // memory-only image
  image = new Rts2Image ();
  return image;
}

void
Rts2GenFocCamera::processImage (Rts2Image * image)
{
  Rts2DevClientCameraFoc::processImage (image);
  std::cout << "Camera " << getName () << " image_type:";
  switch (image->getType ())
    {
    case IMGTYPE_DARK:
      std::cout << "dark";
      break;
    case IMGTYPE_OBJECT:
      std::cout << "object";
      break;
    default:
      std::cout << "unknow (" << image->getType () << ") ";
      break;
    }
  std::cout << std::endl;
}

void
Rts2GenFocCamera::printFWHMTable ()
{
  std::list < fwhmData * >::iterator dat;
  std::cout << "=======================" << std::endl;
  std::cout << "# stars | focPos | fwhm" << std::endl;
  for (dat = fwhmDatas.begin (); dat != fwhmDatas.end (); dat++)
    {
      fwhmData *d;
      d = *dat;
      std::cout << std::setw (8) << d->num << "| "
	<< std::setw (7) << d->focPos << "| " << d->fwhm << std::endl;
    }
  std::cout << "=======================" << std::endl;
}

void
Rts2GenFocCamera::focusChange (Rts2Conn * focus)
{
  if (images->sexResultNum)
    {
      double fwhm;
      int focPos;
      fwhm = images->getFWHM ();
      focPos = images->getFocPos ();
      fwhmDatas.push_back (new fwhmData (images->sexResultNum, focPos, fwhm));
    }

  printFWHMTable ();

  // if we should query..
  if (master->getFocusingQuery ())
    {
      int change;
      int cons_change;
      char c[12];
      char *end_c;
      change = focConn->getChange ();
      if (change == INT_MAX)
	{
	  std::
	    cout << "Focusing algorithm for camera " << getName () <<
	    " did not converge." << std::endl;
	  std::
	    cout << "Write new value, otherwise hit enter for no change " <<
	    std::endl;
	  change = 0;
	}
      else
	{
	  std::cout << "Focusing algorithm for camera " << getName () <<
	    " recommends to change focus by " << change << std::endl;
	  std::cout << "Hit enter to confirm, or write new value." << std::
	    endl;
	}
      std::cin.getline (c, 200);
      cons_change = strtol (c, &end_c, 10);
      if (end_c != c)
	change = cons_change;
      std::cout << std::endl;
      focConn->setChange (change);
      if (change != 0)
	{
	  std::cout << "Will change by: " << change << std::endl;
	}
    }
  Rts2DevClientCameraFoc::focusChange (focus);
}

void
Rts2GenFocCamera::center (int centerWidth, int centerHeight)
{
  connection->
    queCommand (new Rts2CommandCenter (this, 0, centerWidth, centerHeight));
}

Rts2GenFocClient::Rts2GenFocClient (int in_argc, char **in_argv):
Rts2Client (in_argc, in_argv)
{
  defExposure = 10;
  defCenter = 0;
  defBin = -1;

  centerWidth = -1;
  centerHeight = -1;

  autoSave = 0;

  focExe = NULL;

  autoDark = 0;
  query = 0;
  tarRa = -999.0;
  tarDec = -999.0;

  addOption ('d', "device", 1,
	     "camera device name(s) (multiple for multiple cameras)");

  addOption ('A', "autodark", 0, "take (and use) dark image");
  addOption ('e', "exposure", 1, "exposure (defaults to 10 sec)");
  addOption ('c', "center", 0, "takes only center images");
  addOption ('b', "binning", 1,
	     "default binning (ussually 1, depends on camera setting)");
  addOption ('Q', "query", 0,
	     "query after image end to user input (changing focusing etc..");
  addOption ('R', "ra", 1, "target ra (must come with dec - -d)");
  addOption ('D', "dec", 1, "target dec (must come with ra - -r)");
  addOption ('W', "width", 1, "center width");
  addOption ('H', "height", 1, "center height");
  addOption ('F', "imageprocess", 1,
	     "image processing script (default to NULL - no image processing will be done");
}

Rts2GenFocClient::~Rts2GenFocClient (void)
{

}

int
Rts2GenFocClient::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'd':
      cameraNames.push_back (optarg);
      break;
    case 'A':
      autoDark = 1;
      break;
    case 'e':
      defExposure = atof (optarg);
      break;
    case 'b':
      defBin = atoi (optarg);
      break;
    case 'Q':
      query = 1;
      break;
    case 'R':
      tarRa = atof (optarg);
      break;
    case 'D':
      tarDec = atof (optarg);
      break;
    case 'W':
      centerWidth = atoi (optarg);
      break;
    case 'H':
      centerHeight = atoi (optarg);
      break;
    case 'c':
      defCenter = 1;
      break;
    case 'F':
      focExe = optarg;
      break;
    default:
      return Rts2Client::processOption (in_opt);
    }
  return 0;
}

Rts2GenFocCamera *
Rts2GenFocClient::createFocCamera (Rts2Conn * conn)
{
  return new Rts2GenFocCamera (conn, this);
}

Rts2GenFocCamera *
Rts2GenFocClient::initFocCamera (Rts2GenFocCamera * cam)
{
  std::vector < char *>::iterator cam_iter;
  cam->setSaveImage (autoSave || focExe);
  if (defCenter)
    {
      cam->center (centerWidth, centerHeight);
    }
  if (defBin > 0)
    {
      cam->queCommand (new Rts2CommandBinning (cam, defBin, defBin));
    }
  // post exposure event..if name agree
  for (cam_iter = cameraNames.begin (); cam_iter != cameraNames.end ();
       cam_iter++)
    {
      if (!strcmp (*cam_iter, cam->getName ()))
	{
	  printf ("Get conn: %s\n", cam->getName ());
	  cam->postEvent (new Rts2Event (EVENT_START_EXPOSURE));
	}
    }
  return cam;
}

Rts2DevClient *
Rts2GenFocClient::createOtherType (Rts2Conn * conn, int other_device_type)
{
  switch (other_device_type)
    {
    case DEVICE_TYPE_CCD:
      Rts2GenFocCamera * cam;
      cam = createFocCamera (conn);
      return initFocCamera (cam);
    case DEVICE_TYPE_MOUNT:
      return new Rts2DevClientTelescopeImage (conn);
    case DEVICE_TYPE_FOCUS:
      return new Rts2DevClientFocusFoc (conn);
    case DEVICE_TYPE_PHOT:
      return new Rts2DevClientPhotFoc (conn);
    case DEVICE_TYPE_DOME:
      return new Rts2DevClientDomeImage (conn);
    case DEVICE_TYPE_MIRROR:
      return new Rts2DevClientMirrorImage (conn);
    default:
      return Rts2Client::createOtherType (conn, other_device_type);
    }
}

int
Rts2GenFocClient::run ()
{
  getCentraldConn ()->queCommand (new Rts2Command (this, "priority 137"));
  return Rts2Client::run ();
}
