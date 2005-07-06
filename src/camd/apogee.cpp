/*!
 * Driver for Apogee camera.
 *
 * Based on original apogee driver.
 *
 * @author petr
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* !_GNU_SOURCE */

#include <math.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define BOOLEAN  bool
#define HANDLE   int

#define MAXTOTALROWS     5000
#define MAXTOTALCOLUMNS  5000

#include "apogee/CameraIO_Linux.h"

#include "camera_cpp.h"
#include "filter_ifw.h"

// Error codes returned from config_load
const int CCD_OPEN_NOERR = 0;	// No error detected
const int CCD_OPEN_CFGNAME = 1;	// No config file specified
const int CCD_OPEN_CFGDATA = 2;	// Config missing or missing required data
const int CCD_OPEN_LOOPTST = 3;	// Loopback test failed, no camera found
const int CCD_OPEN_ALLOC = 4;	// Memory alloc failed - system error
const int CCD_OPEN_NTIO = 5;	// NT I/O driver not present

class CameraApogeeChip:public CameraChip
{
  CCameraIO *camera;
  short unsigned int *dest;
  short unsigned int *dest_top;
  char *send_top;
public:
    CameraApogeeChip (CCameraIO * in_camera);
    virtual ~ CameraApogeeChip (void);
  virtual int init ();
  virtual int setBinning (int in_vert, int in_hori);
  virtual int startExposure (int light, float exptime);
  virtual long isExposing ();
  virtual int endExposure ();
  virtual int stopExposure ();
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  virtual int readoutOneLine ();
  virtual int endReadout ();
};

CameraApogeeChip::CameraApogeeChip (CCameraIO * in_camera):CameraChip (0)
{
  camera = in_camera;
  dest = NULL;
}

CameraApogeeChip::~CameraApogeeChip (void)
{
  delete dest;
}

int
CameraApogeeChip::init ()
{
  setSize (camera->m_ImgColumns, camera->m_ImgRows, 0, 0);
  binningHorizontal = camera->m_BinX;
  binningVertical = camera->m_BinY;
  pixelX = camera->m_PixelXSize;
  pixelY = camera->m_PixelYSize;
  gain = camera->m_Gain;

  dest = new short unsigned int[chipSize->width * chipSize->height];
  return 0;
}

int
CameraApogeeChip::setBinning (int in_vert, int in_hori)
{
  camera->m_BinX = in_hori;
  camera->m_BinY = in_vert;
  return 0;
}

int
CameraApogeeChip::startExposure (int light, float exptime)
{
  bool ret;
  Camera_Status status;
  ret = camera->Expose (exptime, light);

  if (!ret)
    {
      syslog (LOG_ERR, "CameraApogeeChip::startExposure exposure error");
      return -1;
    }

  chipUsedReadout = new ChipSubset (chipReadout);
  usedBinningVertical = binningVertical;
  usedBinningHorizontal = binningHorizontal;
  return 0;
}

long
CameraApogeeChip::isExposing ()
{
  long ret;
  Camera_Status status;
  ret = CameraChip::isExposing ();
  if (ret > 0)
    return ret;

  status = camera->read_Status ();

  if (status != Camera_Status_ImageReady)
    return 2000000;
  // exposure has ended.. 
  return -2;
}

int
CameraApogeeChip::endExposure ()
{
  camera->m_WaitingforTrigger = false;
  return CameraChip::endExposure ();
}

int
CameraApogeeChip::stopExposure ()
{
  camera->Reset ();
  return CameraChip::stopExposure ();
}

int
CameraApogeeChip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  int ret;
  ret = CameraChip::startReadout (dataConn, conn);
  dest_top = dest;
  send_top = (char *) dest;
  return ret;
}

int
CameraApogeeChip::readoutOneLine ()
{
  int ret;

  if (readoutLine < 0)
    return -1;

  if (readoutLine <
      (chipUsedReadout->y + chipUsedReadout->height) / usedBinningVertical)
    {
      bool status;
      short int width, height;
      width = chipUsedReadout->width;
      height = chipUsedReadout->height;
      status = camera->GetImage (dest_top, width, height);
      if (!status)
	return -3;
      dest_top += width * height;
      readoutLine = chipUsedReadout->height + chipUsedReadout->y;
    }
  if (sendLine == 0)
    {
      int ret;
      ret = CameraChip::sendFirstLine ();
      if (ret)
	return ret;
    }
  if (!readoutConn)
    {
      return -3;
    }
  if (send_top < (char *) dest_top)
    {
      int send_data_size;
      sendLine++;
      send_data_size =
	sendReadoutData (send_top, (char *) dest_top - send_top);
      if (send_data_size < 0)
	return -2;

      send_top += send_data_size;
      return 0;
    }
  endReadout ();
  return -2;
}

int
CameraApogeeChip::endReadout ()
{
  camera->Reset ();
  return CameraChip::endReadout ();
}

class Rts2DevCameraApogee:public Rts2DevCamera
{
  CCameraIO *camera;
  int device_id;
  char *cfgname;
  int config_load (short BaseAddress, short RegOffset);
  bool CfgGet (FILE * inifp, char *inisect, char *iniparm, char *retbuff,
	       short bufflen, short *parmlen);
public:
    Rts2DevCameraApogee (int argc, char **argv);
    virtual ~ Rts2DevCameraApogee (void);
  virtual int processOption (int in_opt);
  virtual int init ();

  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();

  virtual int camChipInfo (int chip);

  virtual int camCoolMax ();
  virtual int camCoolHold ();
  virtual int camCoolTemp (float new_temp);
  virtual int camCoolShutdown ();
};

//-------------------------------------------------------------
// CfgGet
//
// Retrieve a parameter from an INI file. Returns a status code
// and the paramter string in retbuff.
//-------------------------------------------------------------
bool
  Rts2DevCameraApogee::CfgGet (FILE * inifp,
			       char *inisect,
			       char *iniparm,
			       char *retbuff, short bufflen, short *parmlen)
{
  short gotsect;
  char tbuf[256];
  char *ss, *eq, *ps, *vs, *ptr;

  rewind (inifp);

  // find the target section

  gotsect = 0;
  while (fgets (tbuf, 256, inifp) != NULL)
    {
      if ((ss = strchr (tbuf, '[')) != NULL)
	{
	  if (strncasecmp (ss + 1, inisect, strlen (inisect)) == 0)
	    {
	      gotsect = 1;
	      break;
	    }
	}
    }

  if (!gotsect)
    {				// section not found
      return false;
    }

  while (fgets (tbuf, 256, inifp) != NULL)
    {				// find parameter in sect

      if ((ptr = strrchr (tbuf, '\n')) != NULL)	// remove newline if there
	*ptr = '\0';

      ps = tbuf + strspn (tbuf, " \t");	// find the first non-blank

      if (*ps == ';')		// Skip line if comment
	continue;

      if (*ps == '[')
	{			// Start of next section
	  return false;
	}

      eq = strchr (ps, '=');	// Find '=' sign in string

      if (eq)
	vs = eq + 1 + strspn (eq + 1, " \t");	// Find start of value str
      else
	continue;

      // found the target parameter

      if (strncasecmp (ps, iniparm, strlen (iniparm)) == 0)
	{

	  if ((ptr = strchr (vs, ';')) != NULL)	// cut off an EOL comment
	    *ptr = '\0';

	  if (short (strlen (vs)) > bufflen - 1)
	    {			// not enough buffer space
	      strncpy (retbuff, vs, bufflen - 1);
	      retbuff[bufflen - 1] = '\0';	// put EOL in string
	      *parmlen = bufflen;
	      return true;
	    }
	  else
	    {
	      strcpy (retbuff, vs);	// got it
	      ptr = retbuff;
	      while (*ptr && *ptr != '\n' && *ptr != '\r')
		ptr++;
	      *ptr = '\0';
	      *parmlen = strlen (retbuff);
	      return true;
	    }
	}
    }

  return false;			// parameter not found
}


// Initializes internal variables to their default value and reads the parameters in the
// specified INI file. Then initializes the camera using current settings. If BaseAddress
// or RegOffset parameters are specified (not equal to -1) then one or both of these
// values are used for the m_BaseAddress and m_RegisterOffset properties overriding those
// settings in the INI file.
int
Rts2DevCameraApogee::config_load (short BaseAddress, short RegOffset)
{
  short plen;
  char retbuf[256];

  if ((strlen (cfgname) == 0) || (cfgname[0] == '\0'))
    return CCD_OPEN_CFGNAME;

  // attempt to open INI file
  FILE *inifp = NULL;

  if ((inifp = fopen (cfgname, "r")) == NULL)
    return CCD_OPEN_CFGDATA;

  // System
  if (CfgGet (inifp, "system", "interface", retbuf, sizeof (retbuf), &plen))
    {
      // Assume cam is currently null
      if (strcasecmp ("isa", retbuf) == 0)
	{
	  return CCD_OPEN_NTIO;
	}
      else if (strcasecmp ("ppi", retbuf) == 0)
	{
	  return CCD_OPEN_NTIO;
	}
      else if (strcasecmp ("pci", retbuf) == 0)
	{
	  camera = new CCameraIO;
	}

      if (camera == NULL)
	{
	  fclose (inifp);
	  return CCD_OPEN_ALLOC;
	}
    }
  else
    {
      fclose (inifp);
      return CCD_OPEN_CFGDATA;
    }

  /////////////////////////////////////////////////////////////////////////////////
  // Settings which are stored in a class member (not in firmware) are already set
  // to a default value in the constructor. Settings accessed by get/put functions
  // must be set to a default value in this routine, after the base address and
  // communication protocal is setup.

  /////////////////////////////////////////////////////////////////////////////////
  // These settings must done first since they affect communication with the camera

/*  if (BaseAddress == -1)
    {
      if (CfgGet (inifp, "system", "base", retbuf, sizeof (retbuf), &plen))
	camera->m_BaseAddress = strtol (retbuf, NULL, 0) & 0xFFF;
      else
	{
	  fclose (inifp);
	  delete camera;
	  camera = NULL;
	  return CCD_OPEN_CFGDATA;	// base address MUST be defined
	}
    }
  else
    camera->m_BaseAddress = BaseAddress & 0xFFF; */

  if (RegOffset == -1)
    {
      if (CfgGet
	  (inifp, "system", "reg_offset", retbuf, sizeof (retbuf), &plen))
	{
	  unsigned short val = strtol (retbuf, NULL, 0);
//        if (val >= 0x0 && val <= 0xF0)
//          camera->m_RegisterOffset = val & 0xF0;
	}
    }
  else
    {
      if (RegOffset >= 0x0 && RegOffset <= 0xF0)
	camera->m_RegisterOffset = RegOffset & 0xF0;
    }

  /////////////////////////////////////////////////////////////////////////////////
  // Necessary geometry settings

  if (CfgGet (inifp, "geometry", "rows", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 1 && val <= MAXTOTALROWS)
	camera->m_Rows = val;
    }
  else
    {
      fclose (inifp);
      delete camera;
      camera = NULL;
      return CCD_OPEN_CFGDATA;	// rows MUST be defined
    }

  if (CfgGet (inifp, "geometry", "columns", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, 0, 0);
      if (val >= 1 && val <= MAXTOTALCOLUMNS)
	camera->m_Columns = val;
    }
  else
    {
      fclose (inifp);
      delete camera;
      camera = NULL;
      return CCD_OPEN_CFGDATA;	// columns MUST be defined
    }

  /////////////////////////////////////////////////////////////////////////////////

  if (CfgGet (inifp, "system", "pp_repeat", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val > 0 && val <= 1000)
	camera->m_PPRepeat = val;
    }

  /////////////////////////////////////////////////////////////////////////////////
  // First actual communication with camera if in PPI mode
  if (!camera->InitDriver (device_id))
    {
      delete camera;
      camera = NULL;
      fclose (inifp);
      return CCD_OPEN_LOOPTST;
    }
  /////////////////////////////////////////////////////////////////////////////////
  // First actual communication with camera if in ISA mode
  camera->Reset ();		// Read in command register to set shadow register known state
  /////////////////////////////////////////////////////////////////////////////////

  if (CfgGet (inifp, "system", "cable", retbuf, sizeof (retbuf), &plen))
    {
      if (!strcasecmp ("LONG", retbuf))
	camera->write_LongCable (true);
      else if (!strcasecmp ("SHORT", retbuf))
	camera->write_LongCable (false);
    }
  else
    camera->write_LongCable (false);	// default

  if (!camera->read_Present ())
    {
      delete camera;
      camera = NULL;
      fclose (inifp);

      return CCD_OPEN_LOOPTST;
    }
  /////////////////////////////////////////////////////////////////////////////////
  // Set default setting and read other settings from ini file

  camera->write_UseTrigger (false);
  camera->write_ForceShutterOpen (false);

  if (CfgGet
      (inifp, "system", "high_priority", retbuf, sizeof (retbuf), &plen))
    {
      if (!strcasecmp ("ON", retbuf) || !strcasecmp ("TRUE", retbuf)
	  || !strcasecmp ("1", retbuf))
	{
	  camera->m_HighPriority = true;
	}
      else if (!strcasecmp ("OFF", retbuf) || !strcasecmp ("FALSE", retbuf)
	       || !strcasecmp ("0", retbuf))
	{
	  camera->m_HighPriority = false;
	}
    }

  if (CfgGet (inifp, "system", "data_bits", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 8 && val <= 18)
	camera->m_DataBits = val;
    }

  if (CfgGet (inifp, "system", "sensor", retbuf, sizeof (retbuf), &plen))
    {
      if (strcasecmp ("ccd", retbuf) == 0)
	{
	  camera->m_SensorType = Camera_SensorType_CCD;
	}
      else if (strcasecmp ("cmos", retbuf) == 0)
	{
	  camera->m_SensorType = Camera_SensorType_CMOS;
	}
    }

  if (CfgGet (inifp, "system", "mode", retbuf, sizeof (retbuf), &plen))
    {
      unsigned short val = strtol (retbuf, NULL, 0) & 0xF;
      camera->write_Mode (val);
    }
  else
    camera->write_Mode (0);	// default

  if (CfgGet (inifp, "system", "test", retbuf, sizeof (retbuf), &plen))
    {
      unsigned short val = strtol (retbuf, NULL, 0) & 0xF;
      camera->write_TestBits (val);
    }
  else
    camera->write_TestBits (0);	//default

  if (CfgGet (inifp, "system", "test2", retbuf, sizeof (retbuf), &plen))
    {
      unsigned short val = strtol (retbuf, NULL, 0) & 0xF;
      camera->write_Test2Bits (val);
    }
  else
    camera->write_Test2Bits (0);	// default

  camera->write_FastReadout (false);	//default

  if (CfgGet
      (inifp, "system", "shutter_speed", retbuf, sizeof (retbuf), &plen))
    {
      if (!strcasecmp ("normal", retbuf))
	{
	  camera->m_FastShutter = false;
	  camera->m_MaxExposure = 10485.75;
	  camera->m_MinExposure = 0.01;
	}
      else if (!strcasecmp ("fast", retbuf))
	{
	  camera->m_FastShutter = true;
	  camera->m_MaxExposure = 1048.575;
	  camera->m_MinExposure = 0.001;
	}
      else if (!strcasecmp ("dual", retbuf))
	{
	  camera->m_FastShutter = true;
	  camera->m_MaxExposure = 10485.75;
	  camera->m_MinExposure = 0.001;
	}
    }

  if (CfgGet
      (inifp, "system", "shutter_bits", retbuf, sizeof (retbuf), &plen))
    {
      unsigned short val = strtol (retbuf, NULL, 0);
      camera->m_FastShutterBits_Mode = val & 0x0F;
      camera->m_FastShutterBits_Test = (val & 0xF0) >> 4;
    }

  if (CfgGet (inifp, "system", "maxbinx", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 1 && val <= MAXHBIN)
	camera->m_MaxBinX = val;
    }

  if (CfgGet (inifp, "system", "maxbiny", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 1 && val <= MAXVBIN)
	camera->m_MaxBinY = val;
    }

  if (CfgGet
      (inifp, "system", "guider_relays", retbuf, sizeof (retbuf), &plen))
    {
      if (!strcasecmp ("ON", retbuf) || !strcasecmp ("TRUE", retbuf)
	  || !strcasecmp ("1", retbuf))
	{
	  camera->m_GuiderRelays = true;
	}
      else if (!strcasecmp ("OFF", retbuf) || !strcasecmp ("FALSE", retbuf)
	       || !strcasecmp ("0", retbuf))
	{
	  camera->m_GuiderRelays = false;
	}
    }

  if (CfgGet (inifp, "system", "timeout", retbuf, sizeof (retbuf), &plen))
    {
      double val = atof (retbuf);
      if (val >= 0.0 && val <= 10000.0)
	camera->m_Timeout = val;
    }

  /////////////////////////////////////////////////////////////////////////////////
  // Geometry

  if (CfgGet (inifp, "geometry", "bic", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 1 && val <= MAXCOLUMNS)
	camera->m_BIC = val;
    }

  if (CfgGet (inifp, "geometry", "bir", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 1 && val <= MAXROWS)
	camera->m_BIR = val;
    }

  if (CfgGet (inifp, "geometry", "skipc", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 0 && val <= MAXCOLUMNS)
	camera->m_SkipC = val;
    }

  if (CfgGet (inifp, "geometry", "skipr", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 0 && val <= MAXROWS)
	camera->m_SkipR = val;
    }

  if (CfgGet (inifp, "geometry", "imgcols", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 1 && val <= MAXTOTALCOLUMNS)
	camera->m_ImgColumns = val;
    }
  else
    camera->m_ImgColumns =
      camera->m_Columns - camera->m_BIC - camera->m_SkipC;

  if (CfgGet (inifp, "geometry", "imgrows", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 1 && val <= MAXTOTALROWS)
	camera->m_ImgRows = val;
    }
  else
    camera->m_ImgRows = camera->m_Rows - camera->m_BIR - camera->m_SkipR;

  if (CfgGet (inifp, "geometry", "hflush", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 1 && val <= MAXHBIN)
	camera->m_HFlush = val;
    }

  if (CfgGet (inifp, "geometry", "vflush", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 1 && val <= MAXVBIN)
	camera->m_VFlush = val;
    }

  // Default to full frame
  camera->m_NumX = camera->m_ImgColumns;
  camera->m_NumY = camera->m_ImgRows;

  /////////////////////////////////////////////////////////////////////////////////
  // Temperature

  if (CfgGet (inifp, "temp", "control", retbuf, sizeof (retbuf), &plen))
    {
      if (!strcasecmp ("ON", retbuf) || !strcasecmp ("TRUE", retbuf)
	  || !strcasecmp ("1", retbuf))
	{
	  camera->m_TempControl = true;
	}
      else if (!strcasecmp ("OFF", retbuf) || !strcasecmp ("FALSE", retbuf)
	       || !strcasecmp ("0", retbuf))
	{
	  camera->m_TempControl = false;
	}
    }

  if (CfgGet (inifp, "temp", "cal", retbuf, sizeof (retbuf), &plen))
    {
      short val = strtol (retbuf, NULL, 0);
      if (val >= 1 && val <= 255)
	camera->m_TempCalibration = val;
    }

  if (CfgGet (inifp, "temp", "scale", retbuf, sizeof (retbuf), &plen))
    {
      double val = atof (retbuf);
      if (val >= 1.0 && val <= 10.0)
	camera->m_TempScale = val;
    }

  if (CfgGet (inifp, "temp", "target", retbuf, sizeof (retbuf), &plen))
    {
      double val = atof (retbuf);
      if (val >= -60.0 && val <= 40.0)
	camera->write_CoolerSetPoint (val);
      else
	camera->write_CoolerSetPoint (-10.0);
    }
  else
    camera->write_CoolerSetPoint (-10.0);	//default

  /////////////////////////////////////////////////////////////////////////////////
  // CCD

  if (CfgGet (inifp, "ccd", "sensor", retbuf, sizeof (retbuf), &plen))
    {
      if (plen > 256)
	plen = 256;
      memcpy (camera->m_Sensor, retbuf, plen);
    }

  if (CfgGet (inifp, "ccd", "color", retbuf, sizeof (retbuf), &plen))
    {
      if (!strcasecmp ("ON", retbuf) || !strcasecmp ("TRUE", retbuf)
	  || !strcasecmp ("1", retbuf))
	{
	  camera->m_Color = true;
	}
      else if (!strcasecmp ("OFF", retbuf) || !strcasecmp ("FALSE", retbuf)
	       || !strcasecmp ("0", retbuf))
	{
	  camera->m_Color = false;
	}
    }

  if (CfgGet (inifp, "ccd", "noise", retbuf, sizeof (retbuf), &plen))
    {
      camera->m_Noise = atof (retbuf);
    }

  if (CfgGet (inifp, "ccd", "gain", retbuf, sizeof (retbuf), &plen))
    {
      camera->m_Gain = atof (retbuf);
    }

  if (CfgGet (inifp, "ccd", "pixelxsize", retbuf, sizeof (retbuf), &plen))
    {
      camera->m_PixelXSize = atof (retbuf);
    }

  if (CfgGet (inifp, "ccd", "pixelysize", retbuf, sizeof (retbuf), &plen))
    {
      camera->m_PixelYSize = atof (retbuf);
    }

  fclose (inifp);
  return CCD_OPEN_NOERR;
}


Rts2DevCameraApogee::Rts2DevCameraApogee (int argc, char **argv):
Rts2DevCamera (argc, argv)
{
  addOption ('n', "device_id", 1,
	     "device ID (ussualy 0, which is also default)");
  addOption ('c', "config_name", 1,
	     "device ini config file (default to /etc/rts2/apogee.ini");
  addOption ('I', "IFW wheel port", 1,
	     "dev entry for IFW (Optec) filter wheel, if camera is equiped with such");
  device_id = 0;
  cfgname = "/etc/rts2/apogee.ini";

  camera = NULL;

  fan = 0;
  canDF = 1;
}

Rts2DevCameraApogee::~Rts2DevCameraApogee (void)
{
  delete camera;
}

int
Rts2DevCameraApogee::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'n':
      device_id = atoi (optarg);
      break;
    case 'c':
      cfgname = optarg;
      break;
    case 'I':
//      filter = new Rts2FilterIfw (optarg);
      break;
    default:
      return Rts2DevCamera::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevCameraApogee::init ()
{
  int ret;
  ret = Rts2DevCamera::init ();
  if (ret)
    return ret;

  Camera_Status status;

  ret = config_load (-1, -1);
  if (ret != CCD_OPEN_NOERR)
    return -1;

  camera->Flush ();

  status = camera->read_Status ();

  if (status < 0)
    return -1;

  chipNum = 1;
  chips[0] = new CameraApogeeChip (camera);

  return Rts2DevCamera::initChips ();
}

int
Rts2DevCameraApogee::ready ()
{
  int ret;
  ret = camera->read_Present ();
  if (!ret)
    return -1;
  return 0;
}

int
Rts2DevCameraApogee::baseInfo ()
{
  strcpy (ccdType, "Apogee ");
  strncat (ccdType, camera->m_Sensor, 10);
  strcpy (serialNumber, "007");
  return 0;
}

int
Rts2DevCameraApogee::info ()
{

  tempRegulation = camera->read_CoolerMode ();
  tempSet = camera->read_CoolerSetPoint ();
  tempCCD = camera->read_Temperature ();
  tempAir = nan ("f");
  coolingPower = 5000;
  return 0;
}

int
Rts2DevCameraApogee::camChipInfo (int chip)
{
  return 0;
}

int
Rts2DevCameraApogee::camCoolMax ()
{
  camera->write_CoolerSetPoint (-100);
  camera->write_CoolerMode (Camera_CoolerMode_On);
  return 0;
}

int
Rts2DevCameraApogee::camCoolHold ()
{
  camera->write_CoolerSetPoint (-20);
  camera->write_CoolerMode (Camera_CoolerMode_On);
  return 0;
}

int
Rts2DevCameraApogee::camCoolShutdown ()
{
  camera->write_CoolerMode (Camera_CoolerMode_Shutdown);
  return 0;
}

int
Rts2DevCameraApogee::camCoolTemp (float new_temp)
{
  camera->write_CoolerSetPoint (new_temp);
  camera->write_CoolerMode (Camera_CoolerMode_On);
  return 0;
}

int
main (int argc, char **argv)
{
  Rts2DevCameraApogee *device = new Rts2DevCameraApogee (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize apogee camera - exiting!");
      exit (1);
    }
  device->run ();
  delete device;
}
