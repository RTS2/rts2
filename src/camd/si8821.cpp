/*
 * Driver for Spectral Instruments SI8821 PCI board CCD
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "camd.h"

#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h> // APMONTERO: new

#include "si8821.h"
#include "si_app.h"

namespace rts2camd
{
/**
 * Driver for SI 8821 CCD.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SI8821:public Camera
{
	public:
		SI8821 (int argc, char **argv);
		virtual ~SI8821 ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual int info ();
		
		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int startExposure ();
		virtual int endExposure (int ret);

		virtual int closeShutter ();

		virtual int stopExposure ();

		virtual int doReadout ();

	private:
		const char *devicePath;
		const char *cameraCfg;
		const char *cameraSet;

		//int camera_fd;
		struct SI_CAMERA *camera; // APMONTERO: new

		int verbose;

		int init_com (int baud, int parity, int bits, int stopbits, int buffersize);

		int load_cfg( struct CFG_ENTRY **e, const char *fname, const char *var ); // APMONTERO: new
		int load_camera_cfg( struct SI_CAMERA *c, const char *fname); // APMONTERO: new
		int parse_cfg_string( struct CFG_ENTRY *entry ); // APMONTERO: new
		int setfile_readout( struct SI_CAMERA *c, const char *file ); // APMONTERO: new
		int receive_n_ints( int fd, int n, int *data ); // APMONTERO: new
		int send_n_ints( int fd, int n, int *data ); // APMONTERO: new
		struct CFG_ENTRY *find_readout( struct SI_CAMERA *c, char *name ); // APMONTERO: new
		void send_readout( struct SI_CAMERA *c ); // APMONTERO: new

		void write_dma_data( unsigned char *ptr, int total ); // APMONTERO: new
		void dma_unmap( struct SI_CAMERA *c ); // APMONTERO: new
		void camera_image( struct SI_CAMERA *c, int cmd ); // APMONTERO: new

		int clear_buffer ();
		int clear_buffer(int fd);

		int send_command (int cmd);
		int send_command( int fd, int cmd );// APMONTERO: new

		/**
		 * @return -1 error, 0 no, 1 yes
		 */
		int send_command_yn (int cmd);
		int send_command_yn(int fd, int data );// APMONTERO: new

		int send_char (int data);
		int send_char(int fd, int data);
		int receive_char ();
		int receive_char(int fd);
	
		int expect_yn ();
		int expect_yn( int fd );
		void expect_y( int fd );

		int swapl( int *d );

};

}

using namespace rts2camd;

SI8821::SI8821 (int argc, char **argv):Camera (argc, argv)
{
	devicePath = "/dev/SIPCI_0";
	//cameraCfg = NULL; APMONTERO: line commented
	cameraCfg = NULL; // APMONTERO: file .cfg setted
	cameraSet = NULL;

	//camera_fd = 0; APMONTERO: line commented
	camera = ( struct SI_CAMERA *)malloc(sizeof(struct SI_CAMERA )); // APMONTERO: new
	bzero(camera, sizeof(struct SI_CAMERA)); // APMONTERO: new

	addOption ('f', NULL, 1, "SI device path; default to /dev/SIPCI_0");
	addOption ('c', NULL, 1, "SI camera configuration file");
	addOption ('s', NULL, 1, "SI camera configuration set (values) file");
	logStream (MESSAGE_DEBUG) << "## APMONTERO_DEBUG ## ----> SI8821::SI8821" << sendLog;
}
//------------------------------------------------------------------------

SI8821::~SI8821 ()
{
	if (camera->fd)
		close (camera->fd);
}
//------------------------------------------------------------------------

int SI8821::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			devicePath = optarg;
			break;
		case 'c':
			cameraCfg = optarg;
			break;
		case 's':
			cameraSet = optarg;
			break;
		default:
			return Camera::processOption (opt);
	}
	return 0;
}
//------------------------------------------------------------------------

int SI8821::initHardware ()
{
	if (cameraCfg == NULL)
	{
		logStream (MESSAGE_ERROR) << "missing -c parameter for SI configuration" << sendLog;
		return -1;
	}

	if (cameraSet == NULL)
	{
		logStream (MESSAGE_ERROR) << "missing -s parameter for SI configuration set" << sendLog;
		return -1;
	}

	//camera_fd = open (devicePath, O_RDWR, 0);
	camera->fd = open (devicePath, O_RDWR, 0);
	//if (camera_fd < 0)
	if (camera->fd < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot open " << devicePath << ":" << strerror (errno) << sendLog;
		return -1;
	}

	verbose = 1;
	if (ioctl (camera->fd, SI_IOCTL_VERBOSE, &verbose) < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot set camera to verbose mode" << sendLog;
		return -1;
	}

	if (init_com (250000, 0, 8, 1, 9000)) /* setup uart for camera */
	{
		logStream (MESSAGE_ERROR) << "cannot init uart for camera: " << strerror << sendLog;
		return -1;
	}

	if (load_camera_cfg (camera, cameraCfg))
	{
		logStream (MESSAGE_ERROR) << "cannot load camera configuration file " << cameraCfg << sendLog;
		return -1;
	}
	logStream (MESSAGE_INFO) << "loaded configuration file " << cameraCfg << sendLog;

	///////////////
	setfile_readout( camera, cameraSet);
	send_readout( camera );
	send_command(camera->fd, 'H');     // H    - load readout
	receive_n_ints( camera->fd, 32, (int *)&camera->readout );
	expect_y( camera->fd );
	//////////////

	logStream (MESSAGE_DEBUG) << "## APMONTERO_DEBUG ## ----> SI8821::initHardware return 0" << sendLog;
	setSize (camera->readout[READOUT_SERLEN_IX], camera->readout[READOUT_PARLEN_IX], 0, 0);
	logStream (MESSAGE_DEBUG) << "READOUT_SERLEN_IX " << camera->readout[READOUT_SERLEN_IX] << " READOUT_PARLEN_IX " << camera->readout[READOUT_PARLEN_IX] << sendLog;
	return 0;
}
//------------------------------------------------------------------------

int SI8821::info ()
{
	return Camera::info ();
}
//------------------------------------------------------------------------

int SI8821::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	return Camera::setValue (oldValue, newValue);
}
//------------------------------------------------------------------------

int SI8821::startExposure ()
{
	logStream (MESSAGE_DEBUG) << "## APMONTERO_DEBUG ## ----> SI8821::startExposure ENTRA" << sendLog;
	// open shutter
	if (send_command_yn ('A') != 1)
		return -1;
	logStream (MESSAGE_DEBUG) << "## APMONTERO_DEBUG ## ----> SI8821::startExposure return 0" << sendLog;
	return 0;
}
//------------------------------------------------------------------------

int SI8821::endExposure (int ret)
{
	logStream (MESSAGE_DEBUG) << "## APMONTERO_DEBUG ## ----> SI8821::endExposure ENTRA" << sendLog;
	// close shutter
	if (send_command_yn ('B') != 1)
		return -1;
	return Camera::endExposure (ret);
}
//------------------------------------------------------------------------

// TODO APMONTERO BEGIN: Temporary Function, to test closing the shutter
int SI8821::closeShutter ()
{
	logStream (MESSAGE_DEBUG) << "## APMONTERO_DEBUG ## ----> SI8821::closeShutter ENTRA" << sendLog;
	// close shutter
	if (send_command_yn ('B') != 1)
		return -1;
	return 0;
}
//------------------------------------------------------------------------
// TODO APMONTERO END: Temporary Function, to test closing the shutter independiently

int SI8821::stopExposure ()
{
	return Camera::stopExposure ();
}
//------------------------------------------------------------------------

int SI8821::doReadout ()
{
	logStream (MESSAGE_DEBUG) << "## APMONTERO_DEBUG ## ----> SI8821::doReadout" << sendLog;
////////////
//	// else if( strncmp( buf, "image", 5 )== 0 ) {
//	int cmd;
//	strtok( buf, delim );
//	if( (s = strtok( NULL, delim ) ))
//		cmd = *s;
//	else
//		cmd = 'D';
	int cmd = 'D';
	camera_image( camera, cmd );
////////////
	return -2;
}
//------------------------------------------------------------------------


// APMONTERO: begin

void SI8821::dma_unmap( struct SI_CAMERA *c )
//struct SI_CAMERA *c;
{
  int nbufs;

  if( !c->dma_active )
    return;

  nbufs = c->dma_config.total / c->dma_config.buflen ;
  if( c->dma_config.total % c->dma_config.buflen )
    nbufs += 1;

  if(munmap( c->ptr, c->dma_config.buflen*nbufs ) )
    perror("failed to unmap");

  c->dma_active = 0;
}
//------------------------------------------------------------------------

void SI8821::write_dma_data( unsigned char *ptr, int total )
//unsigned char *ptr;
//int total;
{
  FILE *fd;
  //int tmm;
  long int tmm; // APMONTERO: Changed to solve compiling error
  char buf[256];

  time(&tmm);
  //sprintf( buf, "dma_%d.cam", tmm );
  sprintf( buf, "dma_%d.cam", (int)tmm ); // APMONTERO: Changed to solve compiling error

  printf("WRITING DATA %s\n",buf);
  if( !(fd = fopen( buf, "w+" ))) {
    printf("cant write dma.cam\n");
    return;
  }
  fwrite(ptr, total, 1, fd ); 
  fclose(fd);
  printf("wrote %d bytes to %s\n", total, buf );
}
//------------------------------------------------------------------------

void SI8821::camera_image( struct SI_CAMERA *c, int cmd )
//struct SI_CAMERA *c;
//int cmd;
{
  int nbufs, ret; // i, last, 
//  unsigned short *flip;
//  fd_set wait;
  int tot, serlen, parlen; // sel

  if( cmd != 'C' && cmd != 'D' && cmd != 'E' && cmd != 'Z' ) {
    printf("camera_image needs C, D, E, Z type ? for help\n");
    return;
  }
  printf("starting camera_image %c\n", cmd );

  if( c->ptr )
    dma_unmap(c);

  serlen = c->readout[READOUT_SERLEN_IX];
  parlen = c->readout[READOUT_PARLEN_IX];
  printf("SERLEN %i\n",serlen); 
  printf("PARLEN %i\n",parlen); 

  //tot = serlen*parlen*2*4; /* 2 bytes per short, 4 quadrants */
  tot = serlen*parlen*2*2; /* 2 bytes per short, 2 readout?? */
  printf("TOT %i\n",tot);

  //c->dma_config.maxever = 16*2048*4111;
  c->dma_config.maxever = 16*2048*4111;
  c->dma_config.total = tot;
  c->dma_config.buflen = 1024*1024; /* power of 2 makes it easy to mmap */
  c->dma_config.timeout = 50000;
//  c->dma_config.config = SI_DMA_CONFIG_WAKEUP_EACH;
  c->dma_config.config = SI_DMA_CONFIG_WAKEUP_ONEND;

  if( ioctl( c->fd, SI_IOCTL_DMA_INIT, &c->dma_config )<0 ){
    perror("dma init");
    return;
  }
  nbufs = c->dma_config.total / c->dma_config.buflen ;
  if( c->dma_config.total % c->dma_config.buflen )
    nbufs += 1;
  c->ptr = (unsigned short *)mmap( 0, c->dma_config.maxever, PROT_READ, MAP_SHARED, c->fd, 0);
  if(!c->ptr) {
        perror("mmap");
        return;
  }
//      print_mem_changes( c->ptr, c->dma_config.total/sizeof(short) );
  printf("c->ptr: %p\n",c->ptr);
  c->dma_active = 1;

  if( ioctl( c->fd, SI_IOCTL_DMA_START, &c->dma_status )<0 ){
    perror("dma start");
    return;
  }

  bzero(&c->dma_status, sizeof(struct SI_DMA_STATUS));
  send_command_yn(c->fd, cmd );

/* wait for DMA done */

  printf("sent command %c, waiting\n", cmd );
  ret = ioctl( c->fd, SI_IOCTL_DMA_NEXT, &c->dma_status );

  if( ret < 0 ) {
    perror("ioctl dma_next");
  }
  if( c->dma_status.transferred != c->dma_config.total )
   printf("NEXT wakeup xfer %d bytes\n", c->dma_status.transferred );

  if( c->dma_status.status & SI_DMA_STATUS_DONE )
    printf("done\n" );
  else
    printf("timeout\n" );

  //write_dma_data(c->ptr, tot);
  write_dma_data((unsigned char *)c->ptr, tot); // APMONTERO: Changed to solve compiling error
#ifdef NOT
  flip = (unsigned short *)malloc(4096*4096*2);
  bzero( flip, 4096*4096*2 );

  if( serlen < 2047 ) {
    //camera_demux_gen( flip, c->ptr, 2048, serlen, parlen );
    write_dma_data( flip, 2048*2048*2 );
  } else {
    //camera_demux_gen( flip, c->ptr, 4096, serlen, parlen );
    write_dma_data( flip, 4096*4096*2 );
  }
  free(flip);
#endif
}
//------------------------------------------------------------------------

void SI8821::expect_y( int fd )
//int fd;
{
  if( expect_yn(fd) != 1 )
    printf("ERROR expected yes got no from uart\n");
}
//------------------------------------------------------------------------

int SI8821::expect_yn( int fd )
//int fd;
{
  int got;

  if( (got = receive_char(fd)) < 0 )
   return -1;

  if( got != 'Y' && got != 'N')
   return -1;

  return (got == 'Y');
}
//------------------------------------------------------------------------

/* read n bigendian integers and return it */
int SI8821::receive_n_ints( int fd, int n, int *data )
//int fd;
//int n;
//int *data;
{
  int len, i;
  int ret;

  len = n * sizeof(int);

  if( fd < 0 ) {
    bzero( data, len );
    return 0;
  }

  if( (ret = read( fd, data, len )) != len )
    return -1;

  for( i=0; i<n; i++ )
    swapl(&data[i]);
  return 0;
}
//------------------------------------------------------------------------

int SI8821::send_n_ints( int fd, int n, int *data )
//int fd;
//int n;
//int *data;
{
  int len, i;
  int *d;

  len = n * sizeof(int);
  d = (int *)malloc( len );
  memcpy( d, data, len );
  for( i=0; i<n; i++ )
    swapl(&d[i]);

  if( (i = write( fd, d, len )) != len )
    return -1;

//  memset( d, 0, len );
//  if( receive_n_ints( fd, len, d ) < 0 )
//    return -1;

//  if( memcmp( d, data, len ) != 0 )
//    return -1;
  free(d);

  return len;
}
//------------------------------------------------------------------------

struct CFG_ENTRY *SI8821::find_readout( struct SI_CAMERA *c, char *name )
//struct SI_CAMERA *c;
//char *name;
{
  struct CFG_ENTRY *cfg;
  int i;

  for( i=0; i<SI_READOUT_MAX; i++ ) {
    cfg = c->e_readout[i];

    if( cfg && cfg->name ) {
      if( strncasecmp( cfg->name, name, strlen(cfg->name))==0 )
	{
	  printf("PARAMETER FOUND!!!! %s\n",name);
	  return cfg;
	}
    }
  }
  return NULL;
}
//------------------------------------------------------------------------

void SI8821::send_readout( struct SI_CAMERA *c )
//struct SI_CAMERA *c;
{
  send_command(c->fd, 'F'); // F    - Send Readout Parameters
  send_n_ints( c->fd, 32, (int *)&c->readout );
  expect_yn( c->fd );
}
//------------------------------------------------------------------------

int SI8821::setfile_readout( struct SI_CAMERA *c, const char *file )
//struct SI_CAMERA *c;
//char *file;
{
	FILE *fd;
	const char *delim = "=\n";
	char *s;
	char buf[256];
	struct CFG_ENTRY *cfg;

	if( !(fd = fopen( file, "r" ))) {
		return -1;
	}

	while( fgets( buf, 256, fd )) {
		printf("BUF: %s",buf);
		if( !(cfg = find_readout( c, buf )))
			continue;
	
		strtok( buf, delim );
		// if( s = strtok( NULL, delim ))
		s = strtok( NULL, delim );
		if( s )
		{
			printf("PARAM... %s\n",s);
			c->readout[ cfg->index ] = atoi(s);
		}
	}
	fclose(fd);
	return 0;
}
//------------------------------------------------------------------------

int SI8821::load_camera_cfg( struct SI_CAMERA *c, const char *fname)
//int load_camera_cfg( c, fname )
//struct SI_CAMERA *c;
//char *fname;
{
	load_cfg( c->e_status, fname, "SP" );
	load_cfg( c->e_readout, fname, "ESP" );
	load_cfg( c->e_readout, fname, "RFP" );
	load_cfg( c->e_config, fname, "CP" );

	return 0;
}
//------------------------------------------------------------------------

int SI8821::load_cfg( struct CFG_ENTRY **e, const char *fname, const char *var )
//struct CFG_ENTRY **e;
//char *fname;
//char *var;
{
	int len, varlen, index, pindex;
	FILE *fd;
	char buf[256];
	struct CFG_ENTRY *entry;

	varlen = strlen(var);

	if( !(fd = fopen(fname,"r")) ) {
		printf("unable to open %s\n", fname );
		return -1;
	}

	pindex = 0;
	while( fgets( buf, 256, fd )) {
		if( strncmp( buf, var, varlen ) == 0 ) {
			index = atoi( &buf[varlen] );
			len = strlen(buf);
			len -=1;
			buf[len] = 0;
			entry = (struct CFG_ENTRY *)malloc( sizeof(struct CFG_ENTRY ));
			bzero( entry, sizeof(struct CFG_ENTRY ));
			entry->index = index;
			entry->cfg_string = (char *)malloc( len );
			strcpy( entry->cfg_string, buf );
			parse_cfg_string( entry );
			if( index!=pindex || index > 32 ) {
				printf("CFG error\n");
				index = pindex;
			}
	      
			e[pindex] = entry; /* warning here no bounds check */
			pindex++;
			if( pindex >= 32 )
				break;
		}
  	}

	fclose(fd);
	return -1; // APMONTERO: new
}
//------------------------------------------------------------------------

int SI8821::parse_cfg_string( struct CFG_ENTRY *entry )
//struct CFG_ENTRY *entry;
{
	const char *delim = "=\",\n\r";
	char *s;
	char buf[512];
	int i, tot;//, min, max; APMONTERO unused variables
	unsigned int mask;

	strcpy( buf, entry->cfg_string );
	if( !(s = strtok( buf, delim ))) 
		return -1;

	if( !(s = strtok( NULL, delim ))) 
		return -1;

	int oritype = atoi(s);

	if (oritype == 1 || oritype == 2 || oritype == 3 || oritype == 4 || oritype == 5
	|| oritype == 6 || oritype == 7 || oritype == 11 || oritype == 12 
	|| oritype == 13 || oritype == 14)
		oritype = 1;
	else if (oritype == 8)
		oritype = 8;
	else if (oritype == 9)
		oritype = 9;
	else if (oritype == 10)
		oritype = 3;
	else if (oritype == 0)
		oritype = 0;
	else 
	{
		printf("CFG type error\n");
		exit(1);
	}
  
	entry->type = oritype;

	if( entry->type == CFG_TYPE_NOTUSED )
		return 0;

	/* AI:20152021 Security val does not exist any longer
	if( !(s = strtok( NULL, delim ))) 
		return -1;
	entry->security = atoi(s);
	*/

	if( !(s = strtok( NULL, delim ))) 
		return -1;

	entry->name = (char*)malloc(strlen(s)+1);
	strcpy( entry->name, s );
	printf("READING entry->NAME %s\n",entry->name);
  
	printf("READING ENTRY %i\n",entry->type);

	switch( entry->type ) {
		case CFG_TYPE_INPUTD:
			if( !(s = strtok( NULL, delim ))) 
				return -1;
			entry->u.iobox.min = atoi(s);
			if( !(s = strtok( NULL, delim ))) 
				return -1;
			entry->u.iobox.max = atoi(s);

			if( !(s = strtok( NULL, delim ))) 
				return -1;
			entry->u.iobox.units = (char*)malloc(strlen(s)+1); // APMONTERO: (char*) added to compile
			strcpy( entry->u.iobox.units, s );

			if( !(s = strtok( NULL, delim )))  {
				entry->u.iobox.mult = 1.0;
				return -1;
			}
			entry->u.iobox.mult = atof(s);

			if( !(s = strtok( NULL, delim ))) 
				return -1;
			entry->u.iobox.offset = atof(s);

			if( !(s = strtok( NULL, delim ))) 
				return -1;
			entry->u.iobox.status = atoi(s);
		break;
		case CFG_TYPE_DROPDI:
			if( !(s = strtok( NULL, delim ))) 
				return -1;
			tot = atoi(s);
			entry->u.dropi.min = 0;

			entry->u.dropi.max = tot-1;
			if( tot <= 0 )
				return -1;
			entry->u.dropi.list = (char **)malloc( sizeof(char *)*tot);
			bzero( entry->u.dropi.list, sizeof(char *)*tot);
			for( i=0; i<tot; i++ ){
				if( !(s = strtok( NULL, delim ))) 
					return -1;
				printf("DROP %s\n",s);
				entry->u.dropi.list[i] = (char *)malloc( strlen(s)+1);
				strcpy( entry->u.dropi.list[i], s );
			}
		break;
		case CFG_TYPE_DROPDP:
			if( !(s = strtok( NULL, delim ))) 
				return -1;
			tot = atoi(s);

			entry->u.dropp.list = (char **)malloc( sizeof(char *)*tot);
			bzero( entry->u.dropp.list, sizeof(char *)*tot);
			for( i=0; i<tot; i++ ){
				if( !(s = strtok( NULL, delim ))) 
					return -1;
				entry->u.dropp.val = atoi(s);
				printf("DROPP %s\n",s);
				if( !(s = strtok( NULL, delim ))) 
					return -1;
				entry->u.dropp.list[i] = (char *)malloc( strlen(s)+1);
				printf("DROPP STRING %s\n",s);
				strcpy( entry->u.dropp.list[i], s );
			}

		break;
		case CFG_TYPE_BITF:
			if( !(s = strtok( NULL, delim ))) 
				return -1;
			mask = entry->u.bitf.mask = atoi(s);
			tot = 0;
			while( mask &1 ) {
				tot++;
				mask = (mask>>1);
			}
			entry->u.bitf.list = (char **)malloc( sizeof(char *)*tot);
			bzero( entry->u.bitf.list, sizeof(char *)*tot);
			for( i=0; i<tot; i++ ){
				if( !(s = strtok( NULL, delim )))
					return -1;
				entry->u.bitf.list[i] = (char *)malloc( strlen(s)+1);
				strcpy( entry->u.bitf.list[i], s );
			}
		break;
	}
	return -1; // APMONTERO: new
}
//------------------------------------------------------------------------
// APMONTERO: end

int SI8821::init_com (int baud, int parity, int bits, int stopbits, int buffersize)
{
	struct SI_SERIAL_PARAM serial;

	bzero( &serial, sizeof(struct SI_SERIAL_PARAM));

	serial.baud = baud;
	serial.buffersize = buffersize;
	serial.fifotrigger = 8;
	serial.bits = bits;
	serial.stopbits = stopbits;
	serial.parity = parity;

	serial.flags = SI_SERIAL_FLAGS_BLOCK;
	serial.timeout = 1000;
  
	return ioctl (camera->fd, SI_IOCTL_SET_SERIAL, &serial);
}
//------------------------------------------------------------------------

int SI8821::clear_buffer ()
{
	if (ioctl (camera->fd, SI_IOCTL_SERIAL_CLEAR, 0))
		return -1;
	else
		return 0;
}
//------------------------------------------------------------------------

int SI8821::send_char(int fd, int data) 
//int fd;
//int data;
{
  unsigned char wbyte, rbyte;
  int n;

  if( fd < 0 )
    return 0;

  wbyte = ( unsigned char) data;
  if( write( fd,  &wbyte, 1 ) != 1 ) {
    perror("write");
    return -1;
  }

  if( (n = read( fd,  &rbyte, 1 )) != 1 ) {
    if( n < 0 )
      perror("read");
    return -1;
  }

  if( wbyte != rbyte )
    return -1;
  else
    return 0;
}
//------------------------------------------------------------------------

int SI8821::send_command( int fd, int cmd )
//int fd;
//int cmd;
{
  int  ret;

  if( fd < 0 )
    return 0;

  clear_buffer(fd);    
  ret = send_char(fd, cmd);

  return (ret);
}
//------------------------------------------------------------------------

int SI8821::clear_buffer( int fd) 
{
  if( ioctl( fd, SI_IOCTL_SERIAL_CLEAR, 0 ))
    return -1;
  else
    return 0;
}
//------------------------------------------------------------------------

int SI8821::send_command (int cmd)
{
  	clear_buffer ();    
	return send_char (cmd);
}
//------------------------------------------------------------------------

int SI8821::send_command_yn (int cmd)
{
	logStream (MESSAGE_DEBUG) << "## APMONTERO_DEBUG ## ----> SI8821::send_command_yn ENTRA" << sendLog;
	send_command (cmd);

	return expect_yn ();
}
//------------------------------------------------------------------------

int SI8821::send_command_yn(int fd, int data )
//int fd;
//int data;
{
  int ex;

send_command(fd, data);

  ex = expect_yn( fd );
  if( ex < 0  )
    printf("error expected Y/N uart\n");
  else if (ex)
    printf("got yes from uart\n");
  else
    printf("got no from uart\n");
return 0; // APMOTERO: Added to solve compiling warnings
}
//------------------------------------------------------------------------

int SI8821::send_char (int data)
{
	unsigned char wbyte, rbyte;
	int n;

	wbyte = (unsigned char) data;
	if (write (camera->fd,  &wbyte, 1) != 1)
	{
		logStream (MESSAGE_ERROR) << "cannot send char " << data << ":" << strerror << sendLog;
		return -1;
	}

	if ((n = read (camera->fd,  &rbyte, 1)) != 1)
	{
		logStream (MESSAGE_ERROR) << "cannot read reply to command " << data << ":" << strerror << sendLog;
		return -1;
	}

	return 0;
}
//------------------------------------------------------------------------

int SI8821::receive_char ()
{
	unsigned char rbyte;
	int n;

	if ((n = read (camera->fd,  &rbyte, 1)) != 1)
	{
		if (n<0)
		      logStream (MESSAGE_ERROR) << "receive_char read:" << strerror << sendLog;
		return -1;
	}
	else
	{
		return (int)rbyte;
	}
}
//------------------------------------------------------------------------

int SI8821::receive_char(int fd) 
//int fd;
{
  unsigned char rbyte;
  int n;

  if( fd < 0 )
    return 0;

  if( (n = read( fd,  &rbyte, 1 )) != 1 ) {
    if( n<0)
      perror("read");
    return -1;
  } else
    return (int)rbyte;
}
//------------------------------------------------------------------------

int SI8821::expect_yn ()
{
	int got;
	logStream (MESSAGE_DEBUG) << "## APMONTERO_DEBUG ## ----> SI8821::expect_yn ENTRA" << sendLog;
	if ((got = receive_char()) < 0)
		return -1;

	if (got != 'Y' && got != 'N')
		return -1;

	return (got == 'Y');
}
//------------------------------------------------------------------------

int SI8821::swapl( int *d )
//int *d;
{
  union {
    int i;
    unsigned char c[4];
  }u;
  unsigned char tmp;

  u.i = *d;
  tmp = u.c[0];
  u.c[0] = u.c[3];
  u.c[3] = tmp;

  tmp = u.c[1];
  u.c[1] = u.c[2];
  u.c[2] = tmp;

  *d = u.i;
return 0; // APMOTERO: Added to solve compiling warnings
}
//------------------------------------------------------------------------

int main (int argc, char **argv)
{
	SI8821 device (argc, argv);
	return device.run ();
}
//------------------------------------------------------------------------

