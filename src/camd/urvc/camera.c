/*!
 * Alternative driver for SBIG camera.
 *
 * @author mates
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>
#include <sys/io.h>
#include <syslog.h>
#include <errno.h>
#include <pthread.h>

#include <sys/ipc.h>
#include <sys/sem.h>

#include "mardrv.h"
#include "../camera.h"

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun
{
  int val;			/* value for SETVAL */
  struct semid_ds *buf;		/* buffer for IPC_STAT, IPC_SET */
  unsigned short int *array;	/* array for GETALL, SETALL */
  struct seminfo *__buf;	/* buffer for IPC_INFO */
};
#endif

int camera_reg;

int semid = -1;			// semaphore ID 

int chips = 0;
struct chip_info *ch_info = NULL;

unsigned short baseAddress = 0x378;

PAR_ERROR
print_error (PAR_ERROR e)
{
  if (e <= CE_NEXT_ERROR)
    printf ("%s ", CE_names[e]);
  printf ("(error 0x%02x)\n", e);
  return e;
}

void
sem_lock ()
{
  struct sembuf sem_buf;
  sem_buf.sem_num = 0;
  sem_buf.sem_op = -1;
  sem_buf.sem_flg = SEM_UNDO;
  if (semop (semid, &sem_buf, 1) == -1)
    {
      syslog (LOG_ERR, "locking semid: %i %m %i", semid, errno);
      //  exit (EXIT_FAILURE);
    }
}

void
sem_unlock ()
{
  struct sembuf sem_buf;
  sem_buf.sem_num = 0;
  sem_buf.sem_op = 1;
  sem_buf.sem_flg = SEM_UNDO;
  if (semop (semid, &sem_buf, 1) == -1)
    {
      syslog (LOG_ERR, "unlocking semid: %i", semid);
    }
}

unsigned int
ccd_c2ad (double t)
{
  double r = 3.0 * exp (0.94390589891 * (25.0 - t) / 25.0);
  unsigned int ret = 4096.0 / (10.0 / r + 1.0);
  if (ret > 4096)
    ret = 4096;
  return ret;
}

double
ccd_ad2c (unsigned int ad)
{
  double r;
  if (ad == 0)
    return 1000;
  r = 10.0 / (4096.0 / (double) ad - 1.0);
  return 25.0 - 25.0 * (log (r / 3.0) / 0.94390589891);
}

double
ambient_ad2c (unsigned int ad)
{
  double r;
  r = 3.0 / (4096.0 / (double) ad - 1.0);
  return 25.0 - 45.0 * (log (r / 3.0) / 2.0529692213);
}

#define GETBASEADDR_BUFLEN 128
int
getbaseaddr (int parport_num)
{
  FILE *fufu;
  int a;
  char buffer[400];
  snprintf (buffer, GETBASEADDR_BUFLEN,
	    "/proc/sys/dev/parport/parport%d/base-addr", parport_num);

  fufu = fopen (buffer, "r");
  if (fufu == NULL)
    return 0;
  fscanf (fufu, "%d", &a);
  fclose (fufu);
  return a;
}

void
measure_pp ()
{
  double timer2, timer3, timer4;
  int i;

  timer2 = mSecCount ();
  for (i = 0; i < 1000; i++)
    {
      inportb (baseAddress);
      inportb (baseAddress);
      inportb (baseAddress);
      inportb (baseAddress);
      inportb (baseAddress);
      inportb (baseAddress);
      inportb (baseAddress);
      inportb (baseAddress);
      inportb (baseAddress);
      inportb (baseAddress);
    }
  timer3 = mSecCount ();
  for (i = 0; i < 1000; i++)
    {
      outportb (baseAddress, 0);
      outportb (baseAddress, 0);
      outportb (baseAddress, 0);
      outportb (baseAddress, 0);
      outportb (baseAddress, 0);
      outportb (baseAddress, 0);
      outportb (baseAddress, 0);
      outportb (baseAddress, 0);
      outportb (baseAddress, 0);
      outportb (baseAddress, 0);
    }
  timer4 = mSecCount ();

#ifdef DEBUG
  printf ("I/O cycle time: %.2f/%.2fus\n", (timer3 - timer2) / 10,
	  (timer4 - timer3) / 10);
#endif
}

extern int
camera_init (char *device_name, int camera_id)
{
  int uu;
  int i;
  union semun sem_un;
  unsigned short int sem_arr[] = { 1 };
  EEPROMContents eePtr;
  StatusResults gvr;
  QueryTemperatureStatusResults qtsr;
  PAR_ERROR ret = CE_NO_ERROR;

  if ((semid = semget (ftok (device_name, camera_id), 1, 0644)) == -1)
    {
      if ((semid =
	   semget (ftok (device_name, camera_id), 1, IPC_CREAT | 0644)) == -1)
	{
	  syslog (LOG_ERR, "semget %m");
	  // exit (EXIT_FAILURE);
	}
      sem_un.array = sem_arr;
      if (semctl (semid, 0, SETALL, sem_un) < 0)
	{
	  syslog (LOG_ERR, "semctl init: %m");
	  // exit (EXIT_FAILURE);
	}
      sem_lock ();
      begin_realtime ();
      if ((uu = getbaseaddr (camera_id)) == 0)
	{
	  uu = 0x378;		// defualt
	}
      baseAddress = uu;
      CameraOut (0x60, 1);
      ForceMicroIdle ();

      DisableLPTInterrupts (baseAddress);
#ifdef DEBUG
      printf ("Using SPP on 0x%x\n", baseAddress);
#endif
      measure_pp ();
      if ((ret = MicroCommand (MC_STATUS, ST7_CAMERA, NULL, &gvr)) ||
	  (ret = MicroCommand (MC_TEMP_STATUS, ST7_CAMERA, NULL, &qtsr)))
	{
	  sem_unlock ();
	  return -1;
	}
      // determine temperature regulation state
      switch (qtsr.enabled)
	{
	case 0:
	  camera_reg = (qtsr.power > 250) ? CAMERA_COOL_MAX : CAMERA_COOL_OFF;
	  break;
	case 1:
	  camera_reg = CAMERA_COOL_HOLD;
	  break;
	  break;
	default:
	  camera_reg = CAMERA_COOL_OFF;
	}
      // get camera info
      if ((ret = GetEEPROM (ST7_CAMERA, &eePtr)))
	{
	  sem_unlock ();
	  return -1;
	}

      if (Cams[eePtr.model].hasTrack)
	chips = 2;
      else
	chips = 1;

      ch_info =
	(struct chip_info *) malloc (sizeof (struct chip_info) * chips);

      for (i = 0; i < chips; i++)
	{
	  ch_info[i].width = Cams[eePtr.model].horzImage;
	  ch_info[i].height = Cams[eePtr.model].vertImage;
	  ch_info[i].binning_vertical = 1;
	  ch_info[i].binning_horizontal = 1;
	  ch_info[i].pixelX = Cams[eePtr.model].pixelX;
	  ch_info[i].pixelY = Cams[eePtr.model].pixelY;
	  if (i)
	    ch_info[i].gain = eePtr.trackingGain;
	  else
	    ch_info[i].gain = eePtr.imagingGain;
	}
      sem_unlock ();
    }
  return 0;
}

extern void
camera_done ()
{
  free (ch_info);
  chips = 0;
}

extern int
camera_info (struct camera_info *info)
{
  PAR_ERROR ret = CE_NO_ERROR;
  EEPROMContents eePtr;

  StatusResults gvr;
  QueryTemperatureStatusResults qtsr;

  sem_lock ();
  if ((ret = MicroCommand (MC_STATUS, ST7_CAMERA, NULL, &gvr)))
    goto err;
  if ((ret = MicroCommand (MC_TEMP_STATUS, ST7_CAMERA, NULL, &qtsr)))
    goto err;
  // get camera info
  if ((ret = GetEEPROM (ST7_CAMERA, &eePtr)))
    goto err;
  strcpy (info->type, Cams[eePtr.model].fullName);
  strcpy (info->serial_number, eePtr.serialNumber);
  info->chips = chips;
  info->chip_info = ch_info;
  info->temperature_regulation = camera_reg;
  info->temperature_setpoint = ccd_ad2c (qtsr.ccdSetpoint);
  info->cooling_power = (qtsr.power == 255) ? 1000 : qtsr.power * 3.906;
  info->air_temperature = ambient_ad2c (qtsr.ambientThermistor);
  info->ccd_temperature = ccd_ad2c (qtsr.ccdThermistor);
  info->fan = gvr.fanEnabled;
  sem_unlock ();
  return 0;
err:
  sem_unlock ();
  return -1;
}

extern int
camera_fan (int fan_state)
{
  MiscellaneousControlParams ctrl;
  StatusResults sr;

  sem_lock ();
  if ((MicroCommand (MC_STATUS, ST7_CAMERA, NULL, &sr)))
    goto err;
  ctrl.fanEnable = fan_state;
  ctrl.shutterCommand = 0;
  ctrl.ledState = 0;
  if (MicroCommand (MC_MISC_CONTROL, ST7_CAMERA, &ctrl, NULL))
    goto err;
  sem_unlock ();
  return 0;
err:
  sem_unlock ();
  return -1;
}

extern int
camera_expose (int chip, float *exposure, int light)
{
  PAR_ERROR ret = CE_NO_ERROR;
  StartExposureParams sep;
  EndExposureParams eep;
  StatusResults sr;
  double timer, timer1, timer4;
  int bin = 0;
  int ty = 0;
  EEPROMContents eePtr;

  sem_lock ();
  begin_realtime ();

  if ((ret = GetEEPROM (ST7_CAMERA, &eePtr)))
    goto imaging_end;
  sep.ccd = eep.ccd = 0;
  sep.exposureTime = (int) *exposure * 100;
  sep.openShutter = light;
  if ((ret =
       ClearImagingArray (Cams[eePtr.model].vertBefore +
			  Cams[eePtr.model].vertImage + 1,
			  Cams[eePtr.model].horzBefore +
			  Cams[eePtr.model].horzImage + 32, 4, 6)))
    goto imaging_end;		// height, times, [left]

  if ((ret = MicroCommand (MC_START_EXPOSURE, ST7_CAMERA, &sep, NULL)))
    goto imaging_end;

  sem_unlock ();
  // wait for completing exp
  do
    {
      sem_lock ();
      if ((ret = MicroCommand (MC_STATUS, ST7_CAMERA, NULL, &sr)))
	goto imaging_end;
//      usleep(100000);
      sem_unlock ();
      pthread_testcancel ();
      TimerDelay (100000);
    }
  while (sr.imagingState);

  timer1 = 0;
  timer4 = timer = mSecCount ();

  if ((ret = DumpImagingLines (800, 4, 1)))
    goto imaging_end;		// width, len, vertBin
  if ((ret = DumpImagingLines (800 / (bin + 1), ty, bin + 1)))
    goto imaging_end;		// width, len, vertBin
  sem_unlock ();
  return 0;
imaging_end:
  sem_unlock ();
  return -1;
}

extern int
camera_end_expose (int chip)
{
  EndExposureParams eep;
  eep.ccd = chip;
  sem_lock ();
  begin_realtime ();
  if (MicroCommand (MC_END_EXPOSURE, ST7_CAMERA, &eep, NULL))
    {
      sem_unlock ();
      return -1;
    }
  sem_unlock ();
  return 0;
};

extern int
camera_binning (int chip_id, int vertical, int horizontal)
{
  if (chip_id >= chips || chip_id < 0 || vertical < 1 || vertical > 3
      || horizontal < 1 || horizontal > 3)
    {
      errno = EINVAL;
      return -1;
    }
  ch_info[chip_id].binning_vertical = vertical;
  ch_info[chip_id].binning_horizontal = horizontal;
  return 0;
};

extern int
camera_readout_line (int chip_id, short start, short length, void *data)
{
  PAR_ERROR ret = CE_NO_ERROR;
  int bin = ch_info[chip_id].binning_vertical;
  sem_lock ();
  disable ();
  CameraOut (0x60, 1);
  SetVdd (1);
  if ((ret = DigitizeImagingLine (chip_id, bin, start, length, data)))
    {
      enable ();
      sem_unlock ();
      printf ("digitize line:%i", ret);
      return -1;
    }
  enable ();
  sem_unlock ();
#ifdef DEBUG
  printf ("get line\n");
#endif /* DEBUG */
  return 0;
};

extern int
camera_dump_line (int chip_id)
{
  sem_lock ();
  if (chip_id == 1)
    {
      if (DumpImagingLines (800, 1, 1))
	goto err;
    }
  else
    {
      if (DumpTrackingLines (800, 1, 1))
	goto err;
    }
  sem_unlock ();
  return 0;
err:
  sem_lock ();
  return 0;
};

extern int
camera_end_readout (int chip_id)
{
  return 0;
};

extern int
camera_cool_max ()		/* try to max temperature */
{
  MicroTemperatureRegulationParams cool;
  cool.regulation = 2;
  cool.ccdSetpoint = 255;
  cool.preload = 0;

  camera_fan (1);
  sem_lock ();
  if (MicroCommand (MC_REGULATE_TEMP, ST7_CAMERA, &cool, NULL))
    {
      sem_unlock ();
      return -1;
    }
  sem_unlock ();
  camera_reg = CAMERA_COOL_MAX;
  return 0;
};

extern int
camera_cool_hold ()		/* hold on that temperature */
{
  QueryTemperatureStatusResults qtsr;
  float ot;			// optimal temperature
  if (camera_reg == CAMERA_COOL_HOLD)
    return 0;			// already cooled
  sem_lock ();
  if (MicroCommand (MC_TEMP_STATUS, ST7_CAMERA, NULL, &qtsr))
    {
      sem_unlock ();
      return -1;
    }
  ot = ccd_ad2c (qtsr.ccdThermistor);
  ot = ((int) (ot + 5) / 5) * 5;
  camera_fan (1);
  sem_unlock ();
  return camera_cool_setpoint (ot);
};

extern int
camera_cool_shutdown ()		/* ramp to ambient */
{
  MicroTemperatureRegulationParams cool;
  cool.regulation = 0;
  cool.ccdSetpoint = 0;
  cool.preload = 0;
  camera_reg = CAMERA_COOL_OFF;
  camera_fan (0);
  sem_lock ();
  if (MicroCommand (MC_REGULATE_TEMP, ST7_CAMERA, &cool, NULL))
    {
      sem_unlock ();
      return -1;
    }
  sem_unlock ();
  return 0;
};

extern int
camera_cool_setpoint (float coolpoint)	/* set direct setpoint */
{
  MicroTemperatureRegulationParams cool;
  int i;

  cool.regulation = 1;
  i = ccd_c2ad (coolpoint) + 0x7;	// zaokrohlovat a neorezavat!
  cool.ccdSetpoint = i;
  cool.preload = 0xaf;
  camera_fan (1);
  sem_lock ();
  if (MicroCommand (MC_REGULATE_TEMP, ST7_CAMERA, &cool, NULL))
    {
      sem_unlock ();
      return -1;
    }
  camera_reg = CAMERA_COOL_HOLD;
  sem_unlock ();
  return 0;
};
