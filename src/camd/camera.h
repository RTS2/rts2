#ifndef __RTS_CAMERA__
#define __RTS_CAMERA__

/* image types, taken from fitsio.h */
#define CAMERA_BYTE_IMG		8
#define CAMERA_SHORT_IMG	16
#define CAMERA_LONG_IMG		32
#define CAMERA_FLOAT_IMG	-32
#define CAMERA_DOUBLE_IMG	-64
#define CAMERA_USHORT_IMG	20
#define CAMERA_ULONG_IMG	40

#define CAMERA_COOL_OFF		0
#define CAMERA_COOL_MAX		1
#define CAMERA_COOL_HOLD	2
#define CAMERA_COOL_SHUTDOWN	3

struct chip_info
{
  int width;
  int height;
  int binning_vertical;
  int binning_horizontal;
  int image_type;		/* see above for types */
  int gain;			/* in 0.01 e / ADU */
};

struct camera_info
{
  char name[64];
  char serial_number[64];
  int chips;
  struct chip_info *chip_info;
  struct readout_mode *readout_mode;
  int temperature_regulation;
  float temperature_setpoint;	/* o C */
  float air_temperature;	/* o C */
  float ccd_temperature;	/* o C */
  short cooling_power;		/* 0 - 1000, -1 for unknow */
  int fan;
};

extern int camera_init (char *device_name, int camera_id);
extern void camera_done ();

extern int camera_info (struct camera_info *info);

extern int camera_fan (int fan_state);

extern int camera_expose (int chip, float *exposure, int light);
extern int camera_end_expose (int chip);

extern int camera_binning (int chip_id, int vertical, int horizontal);

extern int camera_readout_line (int chip_id, short start, short length,
				void *data);
extern int camera_dump_line (int chip_id);
extern int camera_end_readout (int chip_id);

extern int camera_cool_max ();	/* try to max temperature */
extern int camera_cool_hold ();	/* hold on that temperature */
extern int camera_cool_shutdown ();	/* ramp to ambient */
extern int camera_cool_setpoint (float coolpoint);	/* set direct setpoint */

#endif /* __RTS_CAMERA__ */
