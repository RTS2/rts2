#include <libpdv.h>
#include <sdvlib.h>
#include <sdvpci.h>


static int ccddev = 0;
static int nbufs = 4;

ccd_setdev (dev)
     int dev;
{
  ccddev = dev;
}

ccd_multibuf (fd, num)
/*PdvDev *fd; */
     EdtDev *fd;
     int num;
{
/*
	ccddev ? sdv_multibuf((SdvDev *)fd,num) : pdv_multibuf((PdvDev *)fd,num);
*/
  pdv_multibuf ((PdvDev *) fd, num);
  pdv_multibuf (fd, num);
}

ccd_setsize (fd, width, height)
     EdtDev *fd;
     int width, height;
{
  ccddev ? sdv_setsize ((SdvDev *) fd, width,
			height) : pdv_setsize ((PdvDev *) fd, width, height);
  return;
}


ccd_start_hardware_continuous (fd, nbufs)
/* PdvDev *fd; */
     EdtDev *fd;
     int nbufs;
{
  void sdv_start_hardware_continuous ();
  void edt_startdma_action ();
  ccddev ? sdv_start_hardware_continuous ((SdvDev *) fd) :
    pdv_start_hardware_continuous ((PdvDev *) fd);
  if (ccddev)
/*		PK changed */
/*		 edt_startdma_action((PdvDev *)fd); */
    pdv_start_hardware_continuous (fd);
  edt_startdma_action (fd, EDT_ACT_ONCE);
}

ccd_stop_hardware_continuous (fd)
/* PdvDev *fd; */
     EdtDev *fd;
{
  void pdv_stop_hardware_continuous ();
  void sdv_stop_hardware_continuous ();
  ccddev ? sdv_stop_hardware_continuous ((SdvDev *) fd) :
    pdv_stop_hardware_continuous ((PdvDev *) fd);

/*
	pdv_stop_hardware_continuous(fd);
*/
}

ccd_waitdone (fd, waitcount)
     EdtDev *fd;
     int waitcount;
{
  ccddev ? sdv_waitdone ((SdvDev *) fd,
			 waitcount) : pdv_waitdone ((PdvDev *) fd, waitcount);
/*
	pdv_waitdone(fd);
*/
}

pdv_waitdone (fd, waitcount)
     PdvDev *fd;
     int waitcount;
{
}

caddr_t
ccd_next_data (fd)
/* PdvDev *fd; */
     EdtDev *fd;
{
  caddr_t buf;
/*
    caddr_t pdv_next_data();
*/
  ccddev ? (buf = sdv_next_data ((SdvDev *) fd)) : (buf =
						    pdv_wait_image ((PdvDev *)
								    fd));
/*
	buf=pdv_wait_image(fd);
	buf=edt_wait_for_buffers(fd,1);
*/
  return buf;
}

int
ccd_timeouts (fd)
/* PdvDev *fd; */
     EdtDev *fd;
{
  int count;
  ccddev ? (count = sdv_timeouts ((SdvDev *) fd)) : (count =
						     pdv_timeouts ((PdvDev *)
								   fd));
/*
	count=pdv_timeouts(fd);
*/
  return count;
}

int
ccd_overruns (fd)
     EdtDev *fd;
{
  int count;
  ccddev ? (count = sdv_overruns ((SdvDev *) fd)) : (count =
						     edt_ring_buffer_overrun ((PdvDev *) fd));
/*
	ccddev ? (count=sdv_overruns((SdvDev *)fd)) : (count=pdv_overruns((PdvDev *)fd));
	count=edt_ring_buffer_overrun(fd);
*/
  return count;
}

int
ccd_lastcount (fd)
     EdtDev *fd;
{
  int count;
  ccddev ? (count = sdv_lastcount ((SdvDev *) fd)) : (count =
						      edt_get_bytecount ((PdvDev *) fd));
/*
	count=edt_get_bytecount(fd);
*/
  return count;
}

int
ccd_donecount (fd)
     EdtDev *fd;
{
  int count;
  ccddev ? (count = sdv_donecount ((SdvDev *) fd)) : (count =
						      edt_done_count ((PdvDev
								       *)
								      fd));
/*	
	count=fd->donecount;
	count=edt_done_count((PdvDev *)fd);
*/
  return count;
}

ccd_flush_fifo (fd)
/* PdvDev *fd; */
     EdtDev *fd;
{
  void sdv_flush_fifo ();
  ccddev ? (sdv_flush_fifo ((SdvDev *) fd))
    : (pdv_flush_fifo ((PdvDev *) fd));
/*
	pdv_flush_fifo(fd);
*/
  return;
}

caddr_t
ccd_image (fd)
     EdtDev *fd;
{
  caddr_t buf;
  ccddev ? (buf = (caddr_t) sdv_image ((SdvDev *) fd)) : (buf =
							  (caddr_t)
							  pdv_image ((PdvDev
								      *) fd));
/*
	buf=(caddr_t)pdv_image(fd);
*/
  return buf;
}

ccd_close (fd)
     EdtDev *fd;
{
  ccddev ? (sdv_close ((SdvDev *) fd)) : (pdv_close ((PdvDev *) fd));
/*
	pdv_close(fd);
*/
  return;
}

ccd_lockoff (fd)
     EdtDev *fd;
{
/*
	void sdv_lockoff();
	void edt_lockoff();
	ccddev ? sdv_lockoff((SdvDev *)fd) : edt_lockoff((PdvDev *)fd);
	edt_lockoff(fd);
*/
  return;
}

ccd_start_image (fd)
     EdtDev *fd;
{
/*
	void sdv_start_image();
	void edt_start_buffers();
	ccddev ? sdv_start_image((SdvDev *)fd) : edt_start_buffers((PdvDev *)fd,0);
	edt_start_buffers(fd,0);
*/
  return;
}

ccd_picture_timeout (fd, timeout)
     EdtDev *fd;
     int timeout;
{
  ccddev ? sdv_picture_timeout ((SdvDev *) fd,
				timeout) : pdv_picture_timeout ((PdvDev *) fd,
								timeout *
								100);
/*
	pdv_picture_timeout(fd,timeout);
*/
}

ccd_set_serial_delay (fd, delay)
     void *fd;
     int delay;
{
  ccddev ? sdv_set_serial_delay ((SdvDev *) fd,
				 delay) : pdv_set_serial_delay ((PdvDev *) fd,
								delay);
}

ccd_serial_write (fd, value, count)
     EdtDev *fd;
     u_char *value;
     int count;
{
  printf ("lib: data: %lx - nbytes: %d\n", (char *) value, count);
  ccddev ? sdv_serial_write ((SdvDev *) fd, (char *) value,
			     count) : pdv_serial_binary_command ((PdvDev *)
								 fd,
								 (char *)
								 value,
								 count);
/*
	pdv_serial_write(fd,(char *)value,count);
 	pdv_serial_binary_command(fd,(char *)value,count);
*/
}

ccd_serial_read (fd, value, count)
     EdtDev *fd;
     u_char *value;
     int count;
{
  ccddev ? sdv_serial_read ((SdvDev *) fd, (char *) value,
			    count) : pdv_serial_read ((PdvDev *) fd,
						      (u_char *) value,
						      count);
/*
	pdv_serial_read(fd,(char *)value,count);
*/
}


int
ccd_format_height (fd)
     EdtDev *fd;
{
  int ht;
/*
	int sdv_format_height():
	int pdv_format_height():
	ccddev ? (ht=sdv_format_height((SdvDev *)fd)) : (ht=pdv_format_height((PdvDev *)fd));
	ht=pdv_get_height(fd);
*/
  return ht;
}

int
ccd_format_width (fd)
     EdtDev *fd;
{
  int wid;
/*
	ccddev ? (wid=sdv_format_width((SdvDev *)fd)) : (wid=pdv_format_width((PdvDev *)fd));
	wid=pdv_get_width(fd);
*/
  return wid;
}

void *
ccd_open (unit, mode)
     int unit;
     int mode;
{
  char devname[30];
  void *fd;
  if (ccddev)
    {
/*
	    PdvDev *pd;
            sprintf(devname,"/dev/pdv%d",unit);
	    if( (pd=pdv_open(devname,mode)) == NULL )
	    {
               printf("detserver: couldn't open %s\n",devname);
               exit(1);
	    }
	    fd=(void *)pd;
*/
    }
  else
    {
/*
	    SdvDev *sd;
            sprintf(devname,"/dev/sdv%d",unit);
	    if( (sd=sdv_open(devname,mode)) == NULL )
	    {
               printf("detserver: couldn't open %s\n",devname);
               exit(1);
	    }
	    fd=(void *)sd;
*/
    }
  return fd;
}
