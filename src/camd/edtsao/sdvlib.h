//#pragma ident "@(#)sdvlib.h	1.8 02/07/02"

/*
 * sdvlib.h
 *
 * Copyright (c) 1997, Engineering Design Team, Inc.
 *
 * DESCRIPTION
 *	SDV to PCI DV library porting layer for Solaris -- header file
 *
 *	Include this file in SDV applications when porting to PCI DV
 *	or when writing code that is to run on both platforms
 */

#ifndef _SDVLIB_H
#define _SDVLIB_H

#include "edtinc.h"

#define SDV_NOLOCKDEV PDV_NOLOCKDEV
#define SDV_LOCKDEV PDV_LOCKDEV

#define SDV_LOCK_NONE PDV_LOCK_NONE
#define SDV_LOCK_VCO  PDV_LOCK_VCO
#define SDV_LOCK_DIG  PDV_LOCK_DIG

/* system flags -- N/A for PCI but dv checks for it so needs to be defined */
#define SDV_FSUN4M  0x1

/* typedef dvu_window sdv_window; */
typedef PdvDev SdvDev;
typedef PdvDependent sdv_methods;

/*
 * prototypes
 */
SdvDev *sdv_open (char *dev_name, int lock);
SdvDev *sdv_simple_open (char *dev_name, int lock);
SdvDev *sdv_init (char *dev_name, int lock);
char *sdv_camera_type (SdvDev * sd);
void sdv_clr_ref ();
int sdv_close (SdvDev * sd);
int sdv_ibuf (SdvDev * sd, int enable);
caddr_t sdv_data (SdvDev * sd);
caddr_t sdv_check_image (SdvDev * sd);
caddr_t sdv_image (SdvDev * sd);
caddr_t sdv_wait_image (SdvDev * sd);
caddr_t sdv_wait_images (SdvDev * sd, int n);
int sdv_abort_current (SdvDev * sd);
int sdv_wait_ready (SdvDev * sd);
void sdv_start_image (SdvDev * sd);
void sdv_start_images (SdvDev * sd, int images);
caddr_t sdv_next_data (SdvDev * sd);
int sdv_stopcont_abort_and_lock (SdvDev * sd);
int sdv_bufnum (SdvDev * sd);
int sdv_donecount (SdvDev * sd);
u_int sdv_appsdone (SdvDev * sd);
int sdv_waitdone (SdvDev * sd, int count);
int sdv_setsize (SdvDev * sd, int xsize, int ysize);
caddr_t sdv_grab (SdvDev * sd);
caddr_t sdv_grab_average (SdvDev * sd, int N);
int sdv_get_format (SdvDev * sd);
int sdv_set_exposure (SdvDev * sd, int value);
int sdv_get_exposure (SdvDev * sd);
int sdv_enable_lock (SdvDev * sd, int flag);
int sdv_enable_continuous (SdvDev * sd, int flag);
int sdv_enable_lockstep (SdvDev * sd, int flag, int numbufs);
int sdv_set_buf (SdvDev * sd, caddr_t buf);
int sdv_set_image (SdvDev * sd, caddr_t buf);
int sdv_set_remap (SdvDev * sd, int remap);
int sdv_set_interlace (SdvDev * sd, int interlace);
int sdv_interlace_method (SdvDev * sd);
int sdv_multidone (SdvDev * sd, int done);
int sdv_multibuf (SdvDev * sd, int bufs);
int sdv_assignbuf (SdvDev * sd, int bufs, caddr_t addr);
int sdv_lockstep (SdvDev * sd, int proceed);
int sdv_lockoff (SdvDev * sd);
int sdv_set_slop (SdvDev * sd, int slop);
int sdv_serial_read (SdvDev * sd, u_char * buf, u_int size);
int sdv_serial_write (SdvDev * sd, u_char * buf, u_int size);
int sdv_set_gain (SdvDev * sd, int gain);
int sdv_set_blacklvl (SdvDev * sd, int blacklvl);
int sdv_get_gain (SdvDev * sd, int *gain);
int sdv_get_blacklvl (SdvDev * sd, int *blacklvl);
int sdv_set_aperture (SdvDev * sd, int aperture);
int sdv_get_aperture (SdvDev * sd, int *aperture);
void sdv_invert (SdvDev * sd, int val);
void sdv_setup_continuous (SdvDev * sd);
void sdv_stop_continuous (SdvDev * sd);
int sdv_timeouts (SdvDev * sd);
int sdv_overruns (SdvDev * sd);
int sdv_lastcount (SdvDev * sd);
int sdv_overrun (SdvDev * sd);
int sdv_picture_timeout (SdvDev * sd, int tenths);
int sdv_get_picture_timeout (SdvDev * sd);
int sdv_acquire_wait (SdvDev * sd);
#if 0
sfoi_regs *sfoi_register_map (SdvDev * sdv_p);
#endif
int sdv_mmap (SdvDev * sdv_p);
int sdv_foi_set_serial_baud (SdvDev * sdv_p, int baud);
int sdv_foi_get_serial_baud (SdvDev * sdv_p);
int sdv_foi_set_serial_bits (SdvDev * sdv_p, int bits);
int sdv_foi_get_serial_bits (SdvDev * sdv_p);

#ifdef _MERGED_
#define sdv_format_width(sd)  (sd->dd_p->width)
#define sdv_format_height(sd) (sd->dd_p->height)
#define sdv_format_depth(sd)  (sd->dd_p->depth)
#define sdv_interlace(sd) (sd->dd_p->interlace)
#else
#define sdv_format_width(sd)  (sd->x_size)
#define sdv_format_height(sd) (sd->y_size)
#define sdv_format_depth(sd)  (sd->depth)
#define sdv_interlace(sd) (sd->interlace)
#endif
#endif							 /* !_SDVLIB_H */
