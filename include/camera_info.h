#ifndef __RTS_CAMERA_INFO__
#define __RTS_CAMERA_INFO__

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
  int pixelX;
  int pixelY;
  int image_type;		/* see above for types */
  float gain;			/* in 0.01 e / ADU */
};

struct camera_info
{
  char type[64];
  char serial_number[64];
  int chips;
  struct chip_info *chip_info;
  float exposure;		/* actual image exposure length */
  int temperature_regulation;
  float temperature_setpoint;	/* o C */
  float air_temperature;	/* o C */
  float ccd_temperature;	/* o C */
  int cooling_power;		/* 0 - 1000, -1 for unknow */
  int fan;
  int filter;			/* filter number */
};

#endif /* __RTS_CAMERA_INFO__ */
