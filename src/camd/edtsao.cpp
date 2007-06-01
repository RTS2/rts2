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
  unsigned short *dest;
  unsigned short *dest_top;
  char *send_top;

  // split mode
  int split_mode;

  u_int status;
  bool shutter;
  bool overrun;

  bool verbose;

  int channels;
  int depth;

  int numbufs;

  u_char **bufs;
  int imagesize;
  int dsub;

  int edtwrite (unsigned long lval);
  int writeBinFile (char *filename);

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
  int setDAC ();

  void probe ();

  int fclr (int num);
public:
  CameraEdtSaoChip (Rts2DevCamera * in_cam, int in_chip_id, PdvDev * in_pd,
		    int in_splitMode);
  virtual ~ CameraEdtSaoChip (void);
  virtual int init ();
  virtual int startExposure (int light, float exptime);
  virtual int stopExposure ();
  virtual long isExposing ();
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  virtual int readoutOneLine ();
  virtual void cancelPriorityOperations ();
  virtual int endReadout ();
};

int
CameraEdtSaoChip::edtwrite (unsigned long lval)
{
  unsigned long lsval = lval;
  if (ft_byteswap ())
    swap4 ((char *) &lsval, (char *) &lval, sizeof (lval));
  ccd_serial_write (pd, (u_char *) (&lsval), 4);
  return 0;
}

int
CameraEdtSaoChip::writeBinFile (char *filename)
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

void
CameraEdtSaoChip::reset ()
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
CameraEdtSaoChip::setDAC ()
{
  // values taken from ccdsetup script
  int ret;
  unsigned long edtVal[] = {
    0xa0384732,			// RD = 9
    0x00000001,			// sleep 1
    0xa0080800,			// Vhi = 5
    0xa0084333,			// Phi = 2
    0xa00887ff,			// Rhi = 5
    0xa008c4cc,			// Shi = 3
    0xa0180b32,			// Slo = -7
    0xa0184e65,			// Plo = -9
    0xa0188000,			// Vlo = 0
    0xa018c7ff,			// Rlo = -5
    0xa0288b32,			// OG2 = -2
    0xa028cfff,			// OG1 = -5
    0xa0380bfe,			// DD = 15
    0xa0388ccc,			// OD2 = 20
    0xa038cccc,			// OD1 = 20
    0x30080100,			// a/d offset channel 1
    0x30180100,			// a/d offset channel 2
    0x30280200,			// a/d offset channel 3
    0x30380200,			// a/d offset channel 4
    0x51000040,			// ioram channel order
    0x51000141,
    0x51000202,
    0x51008303,
    0x00000000
  };				// end
  unsigned long *valp = edtVal;
  while (*valp != 0x00000000)
    {
      if (*valp == 0x000000001)
	{
	  sleep (1);
	}
      else
	{
	  ret = edtwrite (*valp);
	  if (ret)
	    {
	      logStream (MESSAGE_ERROR) << "error writing value " << *valp <<
		sendLog;
	      return -1;
	    }
	}
      valp++;
    }
  return 0;
}

void
CameraEdtSaoChip::probe ()
{
  status = edt_reg_read (pd, PDV_STAT);
  shutter = status & PDV_CHAN_ID0;
  overrun = status & PDV_OVERRUN;
}

int
CameraEdtSaoChip::fclr (int num)
{
  time_t now;
  time_t end_time;
  edt_reg_write (pd, PDV_CMD, PDV_RESET_INTFC);
  for (int i = 0; i < num; i++)
    {
      int j;
      edtwrite (0x4c000000);
      edtwrite (0x00000000);
      for (j = 0; j < 10; j++)
	probe ();
      time (&end_time);
      end_time += 7;
      while ((!overrun && (status & PDV_CHAN_ID1)) || overrun)
	{
	  probe ();
	  time (&now);
	  if (now > end_time)
	    {
	      logStream (MESSAGE_ERROR) <<
		"timeout during fclr, phase 1. Overrun: " << overrun <<
		" status & PDV_CHAN_ID1 " << (status & PDV_CHAN_ID1) <<
		sendLog;
	      return -1;
	    }
	}
      time (&end_time);
      end_time += 7;
      while ((!overrun && !(status & PDV_CHAN_ID1)) || overrun)
	{
	  probe ();
	  time (&now);
	  if (now > end_time)
	    {
	      logStream (MESSAGE_ERROR) << "timeout during fclr, phase 2" <<
		sendLog;
	      return -1;
	    }
	}
      pdv_serial_wait (pd, 10, 4);
    }
  return 0;
}

CameraEdtSaoChip::CameraEdtSaoChip (Rts2DevCamera * in_cam, int in_chip_id, PdvDev * in_pd, int in_splitMode):
CameraChip (in_cam, in_chip_id, 2040, 520, 0, 0)
{
  dest = new unsigned short[2040 * 520];
  verbose = false;
  channels = 2;
  depth = 2;
  split_mode = in_splitMode;
  pd = in_pd;
}

CameraEdtSaoChip::~CameraEdtSaoChip (void)
{
  delete dest;
}

int
CameraEdtSaoChip::init ()
{
  int ret;

  // do initialization
  reset ();

  ret = setEDTGain (false);
  if (ret)
    return ret;
  ret = edtwrite (SAO_PARALER_SP);
  if (ret)
    return ret;
  ret = setEDTSplit (true);
  if (ret)
    return ret;
  ret = setEDTUni (false);
  if (ret)
    return ret;

  ret = writeBinFile ("e2vsc.bin");
  if (ret)
    return ret;

  ret = writeBinFile ("e2vpc.bin");
  if (ret)
    return ret;

  ret = writeBinFile ("e2v_pidlesc.bin");
  if (ret)
    return ret;

  ret = setDAC ();
  return ret;
}

int
CameraEdtSaoChip::startExposure (int light, float exptime)
{
  // taken from readout script
  edtwrite (0x51000040);	//  channel order
  edtwrite (0x51008141);

  writeBinFile ("e2vpc.bin");
  writeBinFile ("e2v_nidlesc.bin");
  if (fclr (5))
    return -1;
  writeBinFile ("e2v_freezesc.bin");

  // taken from expose.c
  edtwrite (0x52000000 | (long) (exptime * 100));	/* set time */

  edtwrite (0x50030000);	/* open shutter */

  return 0;
}

int
CameraEdtSaoChip::stopExposure ()
{
  edtwrite (0x50020000);	/* close shutter */
  return CameraChip::stopExposure ();
}

long
CameraEdtSaoChip::isExposing ()
{
  int ret;
  ret = CameraChip::isExposing ();
  if (ret)
    return ret;
  // taken from expose.c
  probe ();
  if ((!overrun && shutter) || overrun)
    return 100;
  pdv_serial_wait (pd, 100, 4);
  writeBinFile ("e2v_unfreezesc.bin");
  return 0;
}

int
CameraEdtSaoChip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  int ret;
  int width, height;
  int i;
  int db;
  int set_timeout = 0;
  int timeout_val = 7000;
  int bufsize;

  numbufs = 4;
  dsub = 1;

  dest_top = dest;
  send_top = (char *) dest;

  ret = CameraChip::startReadout (dataConn, conn);
  if (ret)
    return ret;

  // taken from kepler.c
  pdv_set_header_dma (pd, 0);
  pdv_set_header_size (pd, 0);
  pdv_set_header_offset (pd, 0);

  /*
   * SET SIZE VARIABLES FOR IMAGE
   */
  width = 1020;
  height = 520;
  /* width = chipUsedReadout->width;
     width /= channels;
     height = chipUsedReadout->height; */
  ret = pdv_setsize (pd, width * channels * dsub, height);
  if (ret == -1)
    {
      logStream (MESSAGE_ERROR) << "startExposure failed in pdv_setsize" <<
	sendLog;
      return -1;
    }
  pdv_set_width (pd, width * channels * dsub);
  pdv_set_height (pd, height);
  chipUsedReadout->width = pdv_get_width (pd);
  chipUsedReadout->height = pdv_get_height (pd);
  depth = pdv_get_depth (pd);
  db = bits2bytes (depth);
  imagesize = chipUsedReadout->width * chipUsedReadout->height * db;

  /*
   * SET TIMEOUT
   * make about 1.5 * readout time + 2 sec
   */
  if (!set_timeout)
    timeout_val =
      (chipUsedReadout->width / channels) * chipUsedReadout->height / 1000 *
      timeout_val * 5 / 2000 + 2000;
  pdv_set_timeout (pd, timeout_val);
  printf ("timeout_val: %d millisecs\n", timeout_val);
  printf ("width: %d height: %d imagesize: %d depth %i\n",
	  chipUsedReadout->width, chipUsedReadout->height, imagesize, depth);
  fflush (stdout);

  /*
   * ALLOCATE MEMORY for the image, and make sure it's aligned on a page
   * boundary
   */
  imagesize = imagesize / dsub;
  pdv_flush_fifo (pd);		/* MC - add a flush */
  numbufs = 1;
  bufsize = imagesize * dsub;
  if (verbose)
    printf ("number of buffers: %d bufsize: %d\n", numbufs, bufsize);
  fflush (stdout);
  bufs = (u_char **) malloc (numbufs * sizeof (u_char *));
  for (i = 0; i < numbufs; i++)
    bufs[i] = (u_char *) pdv_alloc (bufsize);
  pdv_set_buffers (pd, numbufs, bufs);
  return 0;
}

int
CameraEdtSaoChip::readoutOneLine ()
{
  int i, j;
  int ret;
  u_int dsx1, dsx2;
  short diff;
  int x = -1;

  if (readoutLine < chipUsedReadout->height)
    {
      int size =
	(chipUsedReadout->width / usedBinningHorizontal) *
	(chipUsedReadout->height / usedBinningVertical);

      /*
       *  ACQUIRE THE IMAGE
       */
      logStream (MESSAGE_DEBUG)
	<< "reading image from " << pdv_get_cameratype (pd)
	<< " width " << pdv_get_width (pd)
	<< " height " << pdv_get_height (pd)
	<< " depth " << pdv_get_depth (pd) << sendLog;

      fflush (stdout);

      int tb = pdv_timeouts (pd);
      pdv_start_image (pd);

      // sendread
      edtwrite (0x4e000000);
      edtwrite (0x00000000);

      /* READ AND PROCESS ONE BUFFER = IMAGE */

      bufs[0] = pdv_wait_image (pd);
      pdv_flush_fifo (pd);
      int ta = pdv_timeouts (pd);
      if (ta > tb)
	{
	  logStream (MESSAGE_ERROR) << "got a timeout" << sendLog;
	  return -1;
	}

      /* BYTE SWAP FOR UNIX */

      if (!ft_byteswap ())
	{
	  for (j = 0; j < imagesize; j += 2)
	    {
	      u_char bufa = bufs[0][j];
	      u_char bufb = bufs[0][j + 1];
	      bufs[0][j] = bufb;
	      bufs[0][j + 1] = bufa;
	    }
	}

      /* DO DOUBLE CORRELATED SUBTRACTION IF SPECIFIED */

      if (dsub > 1)
	{
	  if (channels == 1)
	    {
	      for (j = 0, i = 0; j < imagesize; j += 2, i += 4)
		{
		  dsx1 = (bufs[0][i + 1] << 8) + bufs[0][i];
		  dsx2 = (bufs[0][i + 3] << 8) + bufs[0][i + 2];
		  diff = dsx2 - dsx1;
		  send_top[j] = (u_char) ((short) diff);
		  send_top[j + 1] = (u_char) ((short) diff >> 8);
		}
	    }
	  else
	    {
	      for (j = 0, i = 0; j < imagesize; j += 4, i += 8)
		{
		  dsx1 = (bufs[0][i + 1] << 8) + bufs[0][i];
		  dsx2 = (bufs[0][i + 5] << 8) + bufs[0][i + 4];
		  diff = dsx2 - dsx1;
		  send_top[j] = (u_char) ((int) diff);
		  send_top[j + 1] = (u_char) ((int) diff >> 8);
		  dsx1 = (bufs[0][i + 3] << 8) + bufs[0][i + 2];
		  dsx2 = (bufs[0][i + 7] << 8) + bufs[0][i + 6];
		  diff = dsx2 - dsx1;
		  send_top[j + 2] = (u_char) ((int) diff);
		  send_top[j + 3] = (u_char) ((int) diff >> 8);
		}
	    }
	}
      else
	for (j = 0; j < imagesize; j++)
	  send_top[j] = bufs[0][j];

      /* PRINTOUT FIRST AND LAST PIXELS - DEBUG */

      if (verbose)
	{
	  printf ("first 8 pixels\n");
	  for (j = 0; j < 16; j += 2)
	    {
	      x = (send_top[j + 1] << 8) + send_top[j];
	      printf ("%d ", x);
	    }
	  printf ("\nlast 8 pixels\n");
	  for (j = imagesize - 16; j < imagesize; j += 2)
	    {
	      x = (send_top[j + 1] << 8) + send_top[j];
	      printf ("%d ", x);
	    }
	  printf ("\n");
	}

      for (i = 0; i < numbufs; i++)
	pdv_free (bufs[i]);	/* free buf memory */

      // swap for split mode

      int width = chipUsedReadout->width / dsub;

      if (channels == 2)
	{
	  int pixd = 0;
	  int pixt;
	  int row;

	  int pixsize = chipUsedReadout->width * chipUsedReadout->height;
	  uint16_t *dx = (uint16_t *) malloc (pixsize * 4);

	  logStream (MESSAGE_DEBUG) << "split mode" << sendLog;
	  while (pixd < pixsize)
	    {			/* do all the even pixels */
	      row = pixd / width;
	      pixt = pixd / 2 + row * width / 2;
	      i = pixd * 2;
	      j = pixt;
	      if (j > pixsize)
		{
		  printf ("j: %i i: %i pixd: %i pixels: %i\n", j, i, pixd,
			  pixsize);
		}
	      else
		{
		  dx[j] = (send_top[i + 1] << 8) + send_top[i];
		}
	      pixd += 2;
	    }
	  pixd = 1;
	  while (pixd < pixsize)
	    {			/* do all the odd pixels */
	      row = pixd / width;
	      pixt = (3 * row * width / 2 + width) - (pixd + 1) / 2;
	      i = pixd * 2;
	      j = pixt;
	      if (j > pixsize)
		{
		  printf ("j: %i i: %i pixd: %i pixels: %i\n", j, i, pixd,
			  pixsize);
		}
	      else
		{
		  dx[j] = (send_top[i + 1] << 8) + send_top[i];
		}
	      pixd += 2;
	    }

	  memcpy (send_top, dx, pixsize * 2);

	  free (dx);

	  /* split mode - assumes 16 bit/pixel and 2 channels */
	  // do it in place, without allocating second memory
	  dx = (uint16_t *) send_top;
	  for (int r = 0; r < chipUsedReadout->height; r++)
	    {
	      int fp, fp2;
	      uint16_t v;
	      // split first half
	      for (int c = 0; c < chipUsedReadout->width / 4; c++)
		{
		  // swap values
		  fp = r * chipUsedReadout->width + c;
		  fp2 = fp + chipUsedReadout->width / 4;
		  v = dx[fp];
		  dx[fp] = dx[fp2];
		  dx[fp2] = v;
		}
	      // and then do second half..
	      for (int c = chipUsedReadout->width / 2;
		   c <
		   chipUsedReadout->width / 2 + chipUsedReadout->width / 4;
		   c++)
		{
		  // swap values
		  fp = r * chipUsedReadout->width + c;
		  fp2 = fp + chipUsedReadout->width / 4;
		  v = dx[fp];
		  dx[fp] = dx[fp2];
		  dx[fp2] = v;
		}
	    }
	}
      else
	{
	  logStream (MESSAGE_DEBUG) << "single channel" << sendLog;
	  // /* already stored loend
	  j = 0;
	  for (i = 0; i < imagesize; i += 2)
	    {
	      send_top[j++] = (send_top[i + 1] << 8) + send_top[i];
	    }
	}

      dest_top += size;
      readoutLine = chipUsedReadout->width;
      return 0;
    }
  if (sendLine == 0)
    {
      ret = CameraChip::sendFirstLine ();
      if (ret)
	return ret;
    }
  int send_data_size;
  sendLine++;
  send_data_size = sendReadoutData (send_top, (char *) dest_top - send_top);
  if (send_data_size < 0)
    return -1;
  send_top += send_data_size;
  if (send_top < (char *) dest_top)
    return 0;
  return -2;
}

void
CameraEdtSaoChip::cancelPriorityOperations ()
{
  CameraChip::cancelPriorityOperations ();
  init ();
}

int
CameraEdtSaoChip::endReadout ()
{
  pdv_stop_continuous (pd);
  pdv_flush_fifo (pd);
  pdv_reset_serial (pd);
  edt_reg_write (pd, PDV_CMD, PDV_RESET_INTFC);
  writeBinFile ("e2v_pidlesc.bin");
  return CameraChip::endReadout ();
}

/**
 * This is main control class for EDT-SAO cameras.
 */
class Rts2CamdEdtSao:public Rts2DevCamera
{
private:
  PdvDev * pd;
  char devname[16];
  int devunit;
  bool notimeout;
  int sdelay;

  // 0 - unsplit, 1 - left, 2 - right
  Rts2ValueInteger *splitMode;

  bool verbose;

  int edtwrite (unsigned long lval);
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

  createValue (splitMode, "SPL_MODE", "split mode of the readout", true);
  splitMode->setValueInteger (0);
}

Rts2CamdEdtSao::~Rts2CamdEdtSao (void)
{
  edt_close (pd);
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

  chipNum = 1;

  chips[0] =
    new CameraEdtSaoChip (this, 0, pd, splitMode->getValueInteger ());
  chips[1] = NULL;

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
