#ifndef _SBIG_H
#define _SBIG_H

#define VERSION  2.0

#include "pardrv.h"

typedef struct
{
  unsigned short mode;
  unsigned short width;		/* pixels */
  unsigned short height;	/* height */
  unsigned short gain;		/* in 0.01 e-/ADU */
  unsigned long pixel_width;	/* in nanometers */
  unsigned long pixel_height;	/* in nanometers */
}
readout_mode_t;

struct sbig_init
{
  float linux_version;
  unsigned int nmbr_bad_columns;	/* bad columns in imaging CCD */
  unsigned int bad_columns[4];
  IMAGING_ABG imaging_abg_type;	/* 0 no ABG, 1 ABG present */
  char serial_number[10];
  unsigned int firmware_version;
  char camera_name[64];
  CAMERA_TYPE camera_type;
  unsigned int nmbr_chips;
  struct camera_info
  {
    unsigned int nmbr_readout_modes;
    readout_mode_t readout_mode[20];
  }
  camera_info[2];
  int ST5_AD_size;
  int ST5_filter_type;
};
extern int sbig_init (int port, int options, struct sbig_init *);

struct sbig_status
{
  int imaging_ccd_status;
  int tracking_ccd_status;
  int fan_on;
  int shutter_state;
  int led_state;
  int shutter_edge;
  int plus_x_relay;
  int minus_x_relay;
  int plus_y_relay;
  int minus_y_relay;
  int pulse_active;
  int temperature_regulation;
  int temperature_setpoint;
  unsigned short cooling_power;
  int air_temperature;
  int ccd_temperature;
};
extern int sbig_get_status (struct sbig_status *);

struct sbig_control
{
  unsigned short fan_on;
  unsigned short shutter;
  unsigned short led;
};
extern int sbig_control (struct sbig_control *);

struct sbig_pulse
{
  unsigned short nmbr_pulses;	/* 0 to 255 */
  unsigned short pulse_width;	/* microsec, min 9 */
  unsigned short pulse_interval;	/* microsec, min 27+pulse_width */
};
extern int sbig_pulse (struct sbig_pulse *);

struct sbig_relay
{
  unsigned short x_plus_time;
  unsigned short x_minus_time;
  unsigned short y_plus_time;
  unsigned short y_minus_time;
};
extern int sbig_activate_relay (struct sbig_relay *);

struct sbig_expose
{
  unsigned short ccd;
  unsigned long exposure_time;
  unsigned short abg_state;
  unsigned short shutter;
};

extern int sbig_expose (struct sbig_expose *);

extern int sbig_end_expose (unsigned short);

struct sbig_readout_line
{
  unsigned short /* CCD_REQUEST */ ccd;
  unsigned short readoutMode;
  unsigned short pixelStart;
  unsigned short pixelLength;
  unsigned short *data;
};

extern int sbig_readout_line (struct sbig_readout_line *readout_line);

struct sbig_dump_lines
{
  unsigned short /* CCD_REQUEST */ ccd;
  unsigned short readoutMode;
  unsigned short lineLength;
};

extern int sbig_dump_lines (struct sbig_dump_lines *dump_lines);

struct sbig_readout
{
  unsigned int ccd;
  unsigned int binning;
  unsigned short x, y;
  unsigned short width, height;
  unsigned short *data;
  int data_size_in_bytes;
};

extern int sbig_end_readout (unsigned int ccd);

extern int sbig_update_clock ();

struct sbig_cool
{
  int regulation;		/* 0 off, 1 on, 2 direct_drive */
  int temperature;		/* in 0.1 deg C, if 'on' */
  int direct_drive;		/* power [0..255], direct_drive */
};
extern int sbig_set_cooling (struct sbig_cool *);

extern int sbig_set_ao7_deflection (int x_deflection, int y_deflection);

extern int sbig_set_ao7_focus (int type);

#define SBIG_AO7_FOCUS_SOFT_CENTER 4
#define SBIG_AO7_FOCUS_HARD_CENTER  3
#define SBIG_AO7_FOCUS_STEP_TOWARD_SCOPE 2
#define SBIG_AO7_FOCUS_STEP_FROM_SCOPE 1

/*
 *	Return Error Codes
 *
 *	These are the error codes returned by the driver
 *	function.  They are prefixed with SBIG_ to designate
 *	them as camera errors.
 *
 *      The return codes from the sbig_xxx() routines will
 *      be the NEGATIVE of these on an error.
 *
 */
#define SBIG_CAMERA_NOT_FOUND	-1
#define SBIG_EXPOSURE_IN_PROGRESS -2
#define SBIG_NO_EXPOSURE_IN_PROGRESS -3
#define SBIG_UNKNOWN_COMMAND	-4
#define SBIG_BAD_CAMERA_COMMAND	-5
#define SBIG_BAD_PARAMETER	-6
#define SBIG_TX_TIMEOUT		-7
#define SBIG_RX_TIMEOUT		-8
#define SBIG_NAK_RESBIGIVED	-9
#define SBIG_CAN_RESBIGIVED	-10
#define SBIG_UNKNOWN_RESPONSE	-11
#define SBIG_BAD_LENGTH		-12
#define SBIG_AD_TIMEOUT		-13
#define SBIG_CHECKSUM_ERROR	-14
#define SBIG_EEPROM_ERROR	-15
#define SBIG_SHUTTER_ERROR	-16
#define SBIG_UNKNOWN_CAMERA	-17
#define SBIG_DRIVER_NOT_FOUND	-18
#define SBIG_DRIVER_NOT_OPEN	-19
#define SBIG_DRIVER_NOT_CLOSED	-20
#define SBIG_SHARE_ERROR	-21
#define SBIG_TSBIG_NOT_FOUND	-22
#define SBIG_NEXT_ERROR		-23
#define SBIG_NOT_ROOT		-24
char *sbig_show_error (int);

#endif /* S_BIG_H */
