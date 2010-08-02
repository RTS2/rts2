#include <sdvpci.h>

static int ccddev = 0;
static int ccd_serial_block_size = 0;

#define DBG2 PDVLIB_MSG_INFO_2

void
ccd_setdev (dev)
     int dev;
{
  ccddev = dev;
}

void
ccd_multibuf (fd, num)
     PdvDev *fd;
     int num;
{
  pdv_multibuf (fd, num);
}

void
ccd_setsize (fd, width, height)
     PdvDev *fd;
     int width, height;
{
  pdv_setsize ((PdvDev *) fd, width, height);
  return;
}

int
pdv_resize (fd, width, height)
     PdvDev *fd;
     int width, height;
{
  int ret, nbufs;
  u_int size, dir, ii;
  dir = EDT_READ;
  nbufs = fd->ring_buffer_numbufs;
  size = width * height * 2;
  printf ("PDV - resize\n");
  for (ii = 0; ii < nbufs; ii++)
    {
      /* if (fd->ring_buffers[ii])
         {
         * detach buffer from DMA resources *
         edt_ioctl(fd, EDTS_FREEBUF, &ii);
         if (Edt_debug > 1)
         printf("free buf %d\n", ii);
         }
       */
      ret = edt_set_buffer_size (fd, ii, size, dir);
      if (ret < 0)
	{
	  return -1;
	}
    }
  return 0;
}

void
ccd_start_hardware_continuous (fd, nbufs)
     PdvDev *fd;
     int nbufs;
{
  pdv_setup_continuous (fd);
}

void
ccd_stop_hardware_continuous (fd)
     PdvDev *fd;
{
  pdv_stop_continuous (fd);

}

void
pdv_waitdone (fd, waitcount)
     PdvDev *fd;
     int waitcount;
{
}

void
ccd_waitdone (fd, waitcount)
     EdtDev *fd;
     int waitcount;
{
  pdv_waitdone (fd);
}

caddr_t
ccd_next_data (fd)
     PdvDev *fd;
{
  caddr_t buf;
  buf = pdv_wait_image (fd);
  return buf;
}

int
ccd_timeouts (fd)
     PdvDev *fd;
{
  int count;
  count = pdv_timeouts (fd);
  return count;
}

int
ccd_overruns (fd)
     PdvDev *fd;
{
  return (edt_ring_buffer_overrun (fd));
}

int
ccd_lastcount (fd)
     PdvDev *fd;
{
  return (edt_get_bytecount (fd));
}

int
ccd_donecount (fd)
     PdvDev *fd;
{
  return (edt_done_count (fd));
}

void
ccd_flush_fifo (fd)
     PdvDev *fd;
{
  pdv_flush_fifo (fd);
/* Added from samato 11/7/03 - prevent guider hangups? (with keithley) */
  pdv_reset_serial (fd);
  edt_reg_write (fd, PDV_CMD, PDV_RESET_INTFC);
  return;
}

void
ccd_crst (fd)
     PdvDev *fd;
{
/* linux really means REV 2 PDV card - not linux will mean 
   rev 0 PDV cards in packrat/shadow - never used these routines
*/
#ifdef __linux__
  pdv_reset_serial (fd);
  edt_reg_write (fd, PDV_CMD, PDV_RESET_INTFC);
  edt_reg_write (fd, PDV_MODE_CNTL, 1);
  edt_msleep (10);
  edt_reg_write (fd, PDV_MODE_CNTL, 0);
#endif
}

caddr_t
ccd_image (fd)
     PdvDev *fd;
{
  return ((caddr_t) pdv_image (fd));
}

void
ccd_close (fd)
     PdvDev *fd;
{
  pdv_close (fd);
  return;
}

void
ccd_lockoff (fd)
     PdvDev *fd;
{
  edt_lockoff (fd);
  return;
}

void
ccd_start_image (fd)
     PdvDev *fd;
{
  pdv_start_images (fd, 0);
  return;
}

void
ccd_picture_timeout (fd, timeout)
     PdvDev *fd;
     int timeout;
{
  pdv_picture_timeout (fd, (timeout * 100));
}

void
pdv_set_serial_delay (fd, delay)
     PdvDev *fd;
     int delay;
{
}

void
ccd_set_serial_delay (fd, delay)
     PdvDev *fd;
     int delay;
{
  pdv_set_serial_delay ((PdvDev *) fd, delay);
}

/* OLD really means the Solaris devices runnting EDT 1.0 or
  something that haven't implemented the proper FOI 
  commmunication
*/
int
old_ccd_serial_write (fd, value, count)
     PdvDev *fd;
     u_char *value;
     int count;
{
  int ret = 3;
  int ii;
  int icount = 0;
  u_char sendbuffer[16];

  if (count > 4)
    perror ("write command too long (>4)");
  if (fd->devid == PDVFOI_ID)
    {
      sendbuffer[icount++] = 'c';
      sendbuffer[icount++] = 't';
    }
  for (ii = 0; ii < count; ii++)
    sendbuffer[icount + ii] = value[ii];

  pdv_serial_write (fd, (char *) sendbuffer, count + icount);
  ret = pdv_serial_wait (fd, 0, 0);
  return ret;
}

/* NEW really means the Linux devices running EDT 3.3.4.2 or
  greater that have the "flagged" option for fast commands
  that don't wait for response.  (flagged routine not available 
  in old releases (on Sun)
*/
int
new_ccd_serial_write (fd, value, count)
     PdvDev *fd;
     u_char *value;
     int count;
{
  int ret = 3;
  int ii;
  int icount = 0;
  u_char sendbuffer[16];

  if (count > 4)
    perror ("write command too long (>4)");
  if (fd->devid == PDVFOI_ID)
    {
      sendbuffer[icount++] = 'c';
      sendbuffer[icount++] = 't';
    }
  for (ii = 0; ii < count; ii++)
    sendbuffer[icount + ii] = value[ii];

  ret = pdv_serial_write_one (fd, (char *) sendbuffer, count + icount);
  if (ret > 0)
    {
      return ret;
    }
  ret = ccd_serial_wait_done (fd);
/*
        ret = pdv_serial_wait(fd, 0, 0) ;
 	pdv_serial_binary_command(fd,(char *)value,count);
*/
  return ret;
}

int
ccd_serial_write (fd, value, count)
     PdvDev *fd;
     u_char *value;
     int count;
{
  int ret = 3;
#ifdef __linux__
  /* ret=new_ccd_serial_write(fd,value,count); */
  ret = new_ccd_serial_write (fd, value, count);
#else
  ret = old_ccd_serial_write (fd, value, count);
#endif

  return ret;
}

int
pdv_serial_write_one (PdvDev * pdv_p, char *buf, int size)
{
  int avail = 0;
  int ret = 0;
  int speed = pdv_get_baud (pdv_p);
  int sleepval;
  int loop = 0;

  int chunk = ccd_serial_block_size;

  if (size > chunk)
    {
      printf ("pdv_serial_write_one: command too long\n");
      return ret;
    }

  sleepval = 1;
  avail = pdv_serial_write_available (pdv_p);
  while (avail < 2 * size && loop <= 10000)
    {
      edt_msg (DBG2, "avail: %d size: %d chunk: %d\n", avail, size, chunk);
      SAOusleep (0.0001);
      avail = pdv_serial_write_available (pdv_p);
      loop++;
    }
  if (avail > size)
    {
      return pdv_serial_write_single_block (pdv_p, buf, size);
    }
  else
    {
      printf ("PDV: NOT ENOUGH AVAILABLE: %d size: %d loops: %d\n",
	      avail, size, loop);
    }
  return 2;
}

int
ccd_serial_wait_done (PdvDev * pdv_p)
{
  int avail = 0;
  int loop = 0;
  /* int bufsize = pdv_serial_block_size; */
  int bufsize;
  bufsize = pdv_get_serial_block_size ();

  avail = pdv_serial_write_available (pdv_p);
  while (avail < 4 * bufsize - 1 && loop <= 100000)
    {
      edt_msg (DBG2, "avail: %d loop: %d \n", avail, loop);
      SAOusleep (0.0001);
      avail = pdv_serial_write_available (pdv_p);
      loop++;
    }
  if (loop >= 100000)
    {
      printf ("PDV: BUFFER NOT EMPTY - avail: %d loops: %d bufsize: %d\n",
	      avail, loop, bufsize);
      return 2;
    }
  else
    {
      return 0;
    }
}

void
ccd_serial_read (fd, value, count)
     PdvDev *fd;
     u_char *value;
     int count;
{
  pdv_serial_read (fd, (char *) value, count);
}


int
ccd_format_height (fd)
     PdvDev *fd;
{
  return (pdv_get_height (fd));
}

int
ccd_format_width (fd)
     PdvDev *fd;
{
  return (pdv_get_width (fd));
}

PdvDev *
ccd_open (dvname, unit)
     char *dvname;
     int unit;
{
  char devname[30];
  PdvDev *pd;
  sprintf (devname, "%s%d", dvname, unit);
  if ((pd = pdv_open (devname, 0)) == NULL)
    {
      edt_perror (devname);
      exit (1);
    }
  else
    {
      ccd_crst (pd);
    }
  if (pd->devid == PDVFOI_ID)
    ccd_serial_block_size = 6;
  else
    ccd_serial_block_size = 4;
  /* pdv_set_serial_block_size(ccd_serial_block_size); */
  return pd;
}

void
ccd_serial_wait (fd, time, number)
     PdvDev *fd;
     int time;
     int number;
{
  pdv_serial_wait (fd, time, number);
}

PdvDev *
ccd_gopen (dvname, unit)
     char *dvname;
     int unit;
{
  char devname[30];
  PdvDev *pd;
  int sdelay = 0;
  if (dvname[0] == '\0')
    strcpy (dvname, "/dev/pdv");
  sprintf (devname, "%s%d", dvname, unit);
  /* printf("devname: %s\n",devname); */
  if ((pd = pdv_open (devname, 0)) == NULL)
    {
/*
               edt_perror(devname);
               strcpy(devname,"/dev/sdv");
               sprintf(dvname,"%s%d",devname,unit);
               if( (pd=sdv_open(dvname,0)) == NULL )
               {
*/
      edt_perror (dvname);
      exit (1);
/*
               }
*/
    }
  if (pd->devid == PDVFOI_ID)
    ccd_serial_block_size = 6;
  else
    ccd_serial_block_size = 4;
  /* pdv_set_serial_block_size(ccd_serial_block_size); */

  ccd_set_serial_delay (pd, sdelay);
  return pd;
}

int
ccd_modecntl (fd, value)
     PdvDev *fd;
/*int	*value; */
     int value;
{
/*
	if( ioctl(fd,PDV_MODE_CNTL,value) == -1 )
*/
  int foo;
/*	foo=value[0]; */
  foo = value;
  printf ("modecntl\n %x\n", foo);
  fflush (stdout);
  /* dep_wait(fd); */
  edt_reg_write (fd, PDV_MODE_CNTL, foo);
  /* dep_wait(fd); */
  return (0);
}


int
ccd_ckstatus (fd)
     PdvDev *fd;
{
  int status;
/*
	if( ioctl(fd,PDV_MODE_CNTL,value) == -1 )
*/
  status = edt_reg_read (fd, PDV_STAT);
  return (status);
}

int
ccd_get_depth (fd)
     PdvDev *fd;
{
  int status;
  status = pdv_get_depth (fd);
  return (status);
}

int
ccd_get_imagesize (fd)
     PdvDev *fd;
{
  int status;
  status = pdv_get_imagesize (fd);
  return (status);
}
