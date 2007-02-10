#include "camd.h"
#include "edtsao/interface.h"

/*
 * Depends on libedtsao.a, which is build from edtsao directory. This
 * contains unmodified files from EDT-SAO distribution, and should be
 * update regulary.
 */

/** 
 * Class for EDT-SAO chip.
 */
class CameraEdtSaoChip:public CameraChip
{
private:
  PdvDev * pd;
public:
  CameraEdtSaoChip (Rts2DevCamera * in_cam, int in_chip_id, PdvDev * in_pd);
    virtual ~ CameraEdtSaoChip (void);
};

CameraEdtSaoChip::CameraEdtSaoChip (Rts2DevCamera * in_cam, int in_chip_id,
				    PdvDev * in_pd):
CameraChip (in_cam, in_chip_id)
{
  pd = in_pd;
}

CameraEdtSaoChip::~CameraEdtSaoChip (void)
{
}

/**
 * This is main control class for EDT-SAO cameras.
 */
class Rts2CamdEdtSao:public Rts2DevCamera
{
private:
  PdvDev * pd;
  char *devname;
  int devunit;
  bool notimeout;
  int sdelay;

  bool verbose;

  int edtwrite (unsigned long lval);

  // perform camera-specific functions
  /** perform camera reset */
  void reset ();
  /** set high or low gain */
  int setEDTGain (bool high)
  {
    if (high)
      return edtwrite (SAO_GAIN_HIGH);
    return edtwrite (SAO_GAIN_LOW);
  }
  int setEDTSplit (bool on)
  {
    if (on)
      return edtwrite (SAO_SPLIT_ON);
    return edtwrite (SAO_SPLIT_OFF);
  }
  int setEDTUni (bool on)
  {
    if (on)
      return edtwrite (SAO_UNI_ON);
    return edtwrite (SAO_UNI_OFF);
  }
  int writeBinFile (char *filename);

protected:
  virtual int processOption (int in_opt);
public:
  Rts2CamdEdtSao (int in_argc, char **in_argv);
  virtual ~ Rts2CamdEdtSao (void);

  virtual int init ();
  virtual int initValues ();
  virtual int initChips ();
};

Rts2CamdEdtSao::Rts2CamdEdtSao (int in_argc, char **in_argv):
Rts2DevCamera (in_argc, in_argv)
{
  devname[0] = '\0';
  devunit = 0;
  pd = NULL;
  verbose = false;

  notimeout = 0;
  sdelay = 0;

  addOption ('p', "devname", 1, "device name");
  addOption ('n', "devunit", 1, "device unit number");
  addOption ('t', "notimeout", 0, "don't timeout");
  addOption ('s', "sdelay", 1, "serial delay");
  addOption ('v', "verbose", 0, "verbose report");
}

Rts2CamdEdtSao::~Rts2CamdEdtSao (void)
{
  edt_close (pd);
}

int
Rts2CamdEdtSao::edtwrite (unsigned long lval)
{
  unsigned long lsval = lval;
  if (ft_byteswap ())
    swap4 ((char *) &lsval, (char *) &lval, sizeof (lval));
  ccd_serial_write (pd, (u_char *) (&lsval), 4);
  return 0;
}

void
Rts2CamdEdtSao::reset ()
{
  pdv_flush_fifo (pd);
  pdv_reset_serial (pd);
  edt_reg_write (pd, PDV_CMD, PDV_RESET_INTFC);
  edt_reg_write (pd, PDV_MODE_CNTL, 1);
  edt_msleep (10);
  edt_reg_write (pd, PDV_MODE_CNTL, 0);
  sleep (1);
}

int
Rts2CamdEdtSao::writeBinFile (char *filename)
{
  // taken from edtwriteblk.c, heavily modified
  FILE *fp;
  struct stat stbuf;
  u_int cbuf[256 * 4];
  u_int *cptr;
  int loops;
  int nwrite;

  fp = fopen (filename, "r");
  if (!fp)
    {
      logStream (MESSAGE_ERROR) << "cannot open file " << filename << sendLog;
      return -1;
    }
  if (stat (filename, &stbuf) == -1)
    {
      logStream (MESSAGE_ERROR) << "fsize: can't access " << filename <<
	sendLog;
      return -1;
    }
  logStream (MESSAGE_DEBUG) << "writing " << filename << "  - " << stbuf.
    st_size << " bytes" << sendLog;
  cptr = cbuf;
  loops = 0;
  /*pdv_reset_serial(pd); */
  while ((nwrite = fread (cbuf, 4, 1, fp)) > 0)
    {
      ccd_serial_write (pd, (u_char *) (cptr), nwrite * 4);
      if (verbose)
	{
	  sao_print_command (cptr, nwrite);
	}
      loops++;
    }
  fclose (fp);
  logStream (MESSAGE_DEBUG) << "Total number of serial commands: " << loops <<
    sendLog;
  return 0;
}

int
Rts2CamdEdtSao::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'p':
      strncpy (devname, optarg, 16);
      if (devname[5] == 's')
	ccd_setdev (1);
      break;
    case 'n':
      devunit = atoi (optarg);
      break;
    case 't':
      notimeout = true;
      break;
    case 's':
      sdelay = atoi (optarg);
      break;
    case 'v':
      verbose = true;
      break;
    default:
      return Rts2DevCamera::processOption (in_opt);
    }
  return 0;
}

int
Rts2CamdEdtSao::init ()
{
  int ret;
  ret = Rts2DevCamera::init ();
  if (ret)
    return ret;

  // open CCD
  pd = ccd_gopen (devname, devunit);
  if (pd == NULL)
    {
      logStream (MESSAGE_ERROR) << "cannot init " << devname << " unit " <<
	devunit << sendLog;
      return -1;
    }

  if (notimeout)
    ccd_picture_timeout (pd, 0);

  ccd_set_serial_delay (pd, sdelay);

  // do initialization
  reset ();

  return Rts2CamdEdtSao::initChips ();
}

int
Rts2CamdEdtSao::initValues ()
{
  addConstValue ("DEVNAME", "device name", devname);
  addConstValue ("DEVNUM", "device unit number", devunit);
  addConstValue ("DEVNT", "device no timeout", notimeout);
  addConstValue ("SDELAY", "device serial delay", sdelay);
  return Rts2DevCamera::initValues ();
}

int
Rts2CamdEdtSao::initChips ()
{
  return Rts2DevCamera::initChips ();
}

int
main (int argc, char **argv)
{
  Rts2CamdEdtSao device = Rts2CamdEdtSao (argc, argv);
  return device.run ();
}
