/*!
 * Driver for Apogee camera.
 *
 * Based on original apogge driver.
 *
 * @author petr
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "../camera.h"

#define APP_PCI
//#define APP_ISA
//#define APP_PPI

/* Apogee registers */
#define REG_COMMAND		0
#define	REG_DATA		0	// distinguish by STARTNEXTLINE/DONENEXTLINE

#define REG_LINE_COUNTER	6

#define REG_STATUS		10

/* registers which differs */
#ifdef APP_PCI

#define REG_TIMER		0x4

#endif
#ifdef APP_ISA

#define REG_TIMER		0x2

#endif
#ifdef APP_PPI

#endif


/* register bits */
#define TDI_MODE		0x1
#define START_TIMER		0x2
#define FIFO_CACHE		0x10
#define SHUTTER_OVERRIDE	0x4
#define SHUTTER_ENABLE		0x80
#define DONEREADING		0x200
#define TIMERLOAD		0x400
#define STARTNEXTLINE		0x800

/* status */
#define LINEDONE		0x2

/* appogee readouts */
#define APP_READ_USHORT       _IOR('a', 0x01, unsigned int)
#define APP_READ_LINE         _IOR('a', 0x02, unsigned int)
#define APP_WRITE_USHORT      _IOW('a', 0x03, unsigned int)

//! IOCTL data
struct apIOparam
{
  unsigned int reg;
  unsigned long param1, param2;
};

int dev;

// internals
int
reg_read (struct apIOparam *param)
{
  return ioctl (dev, APP_READ_USHORT, (unsigned long) &param);
}

int
reg_write (struct apIOparam *param)
{
  return ioctl (dev, APP_WRITE_USHORT, (unsigned long) &param);
}

/* Register clear/set */
int
reg_set (unsigned int reg, int p1, int set)
{
  struct apIOparam param;
  int _p1;
  param.reg = reg;
  param.param1 = (unsigned long) &_p1;
  if (reg_read (&param))
    return -1;
  if (set)
    _p1 |= p1;
  else
    _p1 &= ~p1;
  return reg_write (&param);
}

extern int
camera_init (char *device_name, int camera_id)
{
  dev = open (device_name, O_RDONLY);
  if (!dev)
    return -1;
  return 0;
}

extern void
camera_done ()
{
  close (dev);
}

extern int
camera_info (struct camera_info *info)
{

}

extern int
camera_fan (int fan_state)
{
  return 0;
}

extern int
camera_expose (int chip, float *exposure, int light)
{
  struct apIOparam param;

  long valTimer;

//   if (*exposure <= 1048.575)
//   {
//      valTimer = long ((*exposure * 1000) + 0.5);
//              
//   }

  valTimer = (long) ((*exposure * 100) + 0.5);
  // safety..
  if (valTimer <= 0)
    valTimer = 0;

  if (reg_set (REG_COMMAND, TIMERLOAD, 1))
    return -1;

  param.reg = REG_TIMER;
  valTimer = valTimer & 0xFFFF;	// only 16bit
  param.param1 = (unsigned long) &valTimer;

  if (reg_write (&param))
    return -1;

  if (reg_set (REG_COMMAND, TIMERLOAD, 0))
    return -1;

  if (reg_set (REG_COMMAND, SHUTTER_ENABLE, light))
    return -1;

  if (reg_set (REG_COMMAND, START_TIMER, 1)
      || reg_set (REG_COMMAND, START_TIMER, 0))
    return -1;

  return 0;
}

extern int
camera_end_expose (int chip)
{

}

extern int
camera_binning (int chip_id, int vertical, int horizontal)
{

}

extern int
camera_readout_line (int chip_id, short start, short length, void *data)
{
  clock_t stoptime = clock () + CLOCKS_PER_SEC;	// at most 1 sec
  struct apIOparam param;
  int retval;
  int i;

  // clock out next line
  if (reg_set (REG_COMMAND, STARTNEXTLINE, 1)
      || reg_set (REG_COMMAND, STARTNEXTLINE, 0))
    return -1;

  // skip pixels..
  param.reg = REG_DATA;
  param.param1 = (unsigned long) &retval;

  for (i = 0; i < start; i++)
    {
      if (reg_read (&param))
	return -1;
    }

  for (i = 0; i < length; i++)
    {
      if (reg_read (&param))
	return -1;
      *(unsigned short *) data++ = (unsigned short) retval;
    }

  if (reg_set (REG_COMMAND, DONEREADING, 1)
      || reg_set (REG_COMMAND, DONEREADING, 0))
    return -1;

  // wait until camera is done clocking
  param.reg = REG_STATUS;
  while (1)
    {
      reg_read (&param);
      if (retval & LINEDONE)
	break;
      if (clock () > stoptime)
	{
	  errno = ETIMEDOUT;
	  //Flush()
	  return -1;
	}
    }
  return 0;
}

extern int
camera_dump_line (int chip_id)
{

}

extern int
camera_end_readout (int chip_id)
{
  struct apIOparam param;
  int retval;
  clock_t stoptime;

  if (reg_set (REG_COMMAND, DONEREADING, 1)
      || reg_set (REG_COMMAND, DONEREADING, 0))
    return -1;

  // wait until camera is done clocking
  param.reg = REG_STATUS;
  param.param1 = (unsigned long) &retval;

  while (1)
    {
      reg_read (&param);
      if (retval & LINEDONE)
	break;
      if (clock () > stoptime)
	{
	  errno = ETIMEDOUT;
	  //Flush()
	  return -1;
	}
    }
  return 0;
}

extern int
camera_cool_max ()
{

}

extern int
camera_cool_hold ()
{

}

extern int
camera_cool_shutdown ()
{

}

extern int
camera_cool_setpoint (float coolpoint)
{

}

extern int
camera_set_filter (int filter)
{
  return 0;
}
