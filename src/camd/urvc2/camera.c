/*!
 * Alternative driver for SBIG camera.
 *
 * rts2 interface
 * 
 * @author mates
 */

#define _GNU_SOURCE

#include <ctype.h>
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

#include "urvc.h"
//#include "../camera.h"
#include "camera.h"

#define DEFAULT_CAMERA	ST8_CAMERA	// in case geteeprom fails
#define CAMERA_KEY	25

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

// space for an image, readout globals
unsigned short *img[2] = { 0, 0 }, **lin[2] =

{
0, 0};
int readout_line[2] = { 0, 0 }, max_line[2] =

{
0, 0};

int camera_filter = -1;
int camera_reg = -1;
int cameraID = -1;		// default camera communicating mode
int chips = -1;
int is_init = 0;

EEPROMContents eePtr;		// global to prevent multiple EEPROM calls

int semid = -1;			// semaphore ID 

struct chip_info *ch_info = NULL;

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

void
get_eeprom ()
{
  if (!is_init)
    return;

  if (camera_reg == -1)
    {
      if (GetEEPROM (cameraID, &eePtr) != CE_NO_ERROR)
	{
	  eePtr.version = 0;
	  eePtr.model = DEFAULT_CAMERA;	// ST8 camera
	  eePtr.abgType = 0;
	  eePtr.badColumns = 0;
	  eePtr.trackingOffset = 0;
	  eePtr.trackingGain = 304.0;
	  eePtr.imagingOffset = 0;
	  eePtr.imagingGain = 560.0;
	  memset (eePtr.columns, 0, 4);
	  strcpy (eePtr.serialNumber, "EEEE");
	}
      else
	{
	  // check serial number
	  char *b = eePtr.serialNumber;
	  for (; *b; b++)
	    if (!isalnum (*b))
	      {
		*b = '0';
		break;
	      }
	}
    }
}

void
init_shutter ()
{
  MiscellaneousControlParams ctrl;
  StatusResults sr;

  if (!is_init)
    return;

  if (cameraID == ST237_CAMERA || cameraID == ST237A_CAMERA)
    return;

  if (MicroCommand (MC_STATUS, cameraID, NULL, &sr) == CE_NO_ERROR)
    {
      ctrl.fanEnable = sr.fanEnabled;
      ctrl.shutterCommand = 3;
      ctrl.ledState = sr.ledStatus;
      MicroCommand (MC_MISC_CONTROL, cameraID, &ctrl, NULL);
      sleep (2);
    }
}

int
cond_init (char *device_name, int camera_id)
{
  short base;
  int i;
  //StatusResults sr;
  QueryTemperatureStatusResults qtsr;
  GetVersionResults gvr;

  if (is_init)
    return 0;

  base = getbaseaddr (0);	// sbig_port...?

  // lock the device
  sem_lock ();

  // This is a standard driver init procedure 
  begin_realtime ();
  CameraInit (base);

  if (MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &gvr))
    goto err;

  cameraID = gvr.cameraID;

  //if(MicroCommand (MC_STATUS, cameraID, NULL, &sr)) goto err;
  if (MicroCommand (MC_TEMP_STATUS, cameraID, NULL, &qtsr))
    goto err;
  get_eeprom ();


  if (Cams[eePtr.model].hasTrack)
    chips = 2;
  else
    chips = 1;

  ch_info = (struct chip_info *) malloc (sizeof (struct chip_info) * chips);

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

  // make space for the read out image
  {
    CAMERA *C;
    int j, k;

    for (j = 0; j < chips; j++)
      {
	OpenCCD (j, &C);

	free (img[j]);
	free (lin[j]);

	img[j] = (unsigned short *)
	  malloc (C->horzTotal * C->vertTotal * sizeof (unsigned short));
	lin[j] = (unsigned short **)
	  malloc (C->vertTotal * sizeof (unsigned short *));

	for (k = 0; k < C->vertTotal; k++)
	  lin[j][k] = img[j] + k * C->horzTotal;

	max_line[j] = C->vertTotal;

	CloseCCD (C);
      }
  }

  // determine temperature regulation state
  switch (qtsr.enabled)
    {
    case 0:
      // a bit strange: may have different
      // meanings... (freeze)
      camera_reg = (qtsr.power > 250) ? CAMERA_COOL_MAX : CAMERA_COOL_OFF;
      break;
    case 1:
      camera_reg = CAMERA_COOL_HOLD;
      break;
      break;
    default:
      camera_reg = CAMERA_COOL_OFF;
    }

  is_init = 1;

  sem_unlock ();

  return 0;

err:
  sem_unlock ();
  errno = ENODEV;
  return -1;
}

extern int
camera_init (char *device_name, int camera_id)
{
  union semun sem_un;
  unsigned short int sem_arr[] = { 1 };

  // init semaphores
  if ((semid == -1)
      && (semid = semget (CAMERA_KEY + camera_id, 1, 0644)) == -1)
    {
      if ((semid =
	   semget (CAMERA_KEY + camera_id, 1, IPC_CREAT | 0644)) == -1)
	{
	  syslog (LOG_ERR, "semget %m");
	  // exit (EXIT_FAILURE);
	}
      else
	// we create a new semaphore
	{
	  sem_un.array = sem_arr;
	  if (semctl (semid, 0, SETALL, sem_un) < 0)
	    {
	      syslog (LOG_ERR, "semctl init: %m");
	      // exit (EXIT_FAILURE);
	    }
	}
    }

  return cond_init (device_name, camera_id);

}

extern void
camera_done ()
{
  free (ch_info);
  chips = -1;
}

extern int
camera_info (struct camera_info *info)
{
  //PAR_ERROR ret = CE_NO_ERROR;


  StatusResults gvr;
  QueryTemperatureStatusResults qtsr;

  if (!is_init)
    return -1;

  sem_lock ();

  if (MicroCommand (MC_STATUS, cameraID, NULL, &gvr))
    goto err;
  if (MicroCommand (MC_TEMP_STATUS, cameraID, NULL, &qtsr))
    goto err;

  // get camera info
  get_eeprom ();
  sem_unlock ();

  strcpy (info->type, Cams[eePtr.model].fullName);
  strcpy (info->serial_number, eePtr.serialNumber);

  if (Cams[eePtr.model].hasTrack)
    chips = 2;
  else
    chips = 1;

  info->chips = chips;
  info->chip_info = ch_info;
//  memcpy(info->chip_info,&(ch_info[0]),sizeof(struct chip_info));
  info->temperature_regulation = camera_reg;
  info->temperature_setpoint = ccd_ad2c (qtsr.ccdSetpoint);
  info->cooling_power = (qtsr.power == 255) ? 1000 : qtsr.power * 3.906;
  info->air_temperature = ambient_ad2c (qtsr.ambientThermistor);
  info->ccd_temperature = ccd_ad2c (qtsr.ccdThermistor);
  info->fan = gvr.fanEnabled;
  info->filter = camera_filter;
  info->can_df = 1;
  return 0;

err:
  sem_unlock ();
  errno = ENODEV;
  return -1;
}

extern int
camera_expose (int chip, float *exposure, int light)
{
  CAMERA *C;
  int imstate;
  if (!is_init)
    return -1;

  sem_lock ();

#ifdef INIT_SHUTTER
  init_shutter ();
#else
  if (!light)			// init shutter only for dark images
    init_shutter ();
#endif

  if (OpenCCD (chip, &C))
    goto err;
  if (CCDExpose (C, (int) 100 * (*exposure) + 0.5, light))
    goto err;

  sem_unlock ();

  // wait for completing exp
  do
    {
      sem_lock ();
      imstate = CCDImagingState (C);
      sem_unlock ();

      pthread_testcancel ();
      TimerDelay (100000);
    }
  while (imstate);

  sem_lock ();
  readout_line[chip] = 0;
  if (CCDReadout
      (img[chip], C, 0, 0, C->vertImage, C->horzImage,
       ch_info[chip].binning_vertical))
    goto err;
  CloseCCD (C);
  sem_unlock ();

  return 0;

err:
  CloseCCD (C);
  sem_unlock ();
  errno = ENODEV;
  return -1;
}

extern int
camera_end_expose (int chip)
{
  EndExposureParams eep;
  eep.ccd = chip;
  sem_lock ();

  if (!is_init)
    return -1;

  if (MicroCommand (MC_END_EXPOSURE, cameraID, &eep, NULL))
    {
      sem_unlock ();
      errno = ENODEV;
      return -1;
    }
  sem_unlock ();
  return 0;
};

extern int
camera_binning (int chip_id, int vertical, int horizontal)
{
  if (!is_init)
    return -1;
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
camera_dump_line (int chip_id)
{
  if (!is_init)
    return -1;
  readout_line[chip_id]++;
  return 0;
}

extern int
camera_readout_line (int chip_id, short start, short length, void *data)
{
  if (!is_init)
    return -1;
  if (readout_line[chip_id & 1] >= max_line[chip_id & 1])
    return (-1);
  // has to be locked !
  sem_lock ();
  memcpy (data, lin[chip_id & 1][readout_line[chip_id & 1]++] + start,
	  length * sizeof (unsigned short));
  sem_unlock ();
  return 0;
};

extern int
camera_end_readout (int chip_id)
{
  if (!is_init)
    return -1;
  return 0;
};

extern int
camera_fan (int fan_state)
{
  MiscellaneousControlParams ctrl;
  StatusResults sr;
  if (!is_init)
    return -1;

  ctrl.fanEnable = fan_state;
  ctrl.shutterCommand = 0;

  sem_lock ();
  if ((MicroCommand (MC_STATUS, cameraID, NULL, &sr)))
    goto err;
  ctrl.ledState = sr.ledStatus;
  if (MicroCommand (MC_MISC_CONTROL, cameraID, &ctrl, NULL))
    goto err;
  sem_unlock ();

  return 0;

err:
  sem_unlock ();
  errno = ENODEV;
  return -1;
}

int
_camera_setcool (int reg, int setpt, int prel, int fan, int state)
{
  MicroTemperatureRegulationParams cool;
  //GetVersionResults st;
  if (!is_init)
    return -1;

  cool.regulation = reg;
  cool.ccdSetpoint = setpt;
  cool.preload = prel;

  camera_fan (fan);

  sem_lock ();
  if (MicroCommand (MC_REGULATE_TEMP, cameraID, &cool, NULL))
    goto err;
  camera_reg = state;
  sem_unlock ();

  return 0;

err:
  sem_unlock ();
  errno = ENODEV;
  return -1;
}

extern int
camera_cool_max ()		/* try to max temperature */
{
  return _camera_setcool (2, 255, 0, FAN_ON, CAMERA_COOL_MAX);
};

extern int
camera_cool_shutdown ()		/* ramp to ambient */
{
  return _camera_setcool (0, 0, 0, FAN_OFF, CAMERA_COOL_OFF);
}

extern int
camera_cool_setpoint (float coolpoint)	/* set direct setpoint */
{
  int i;
  i = ccd_c2ad (coolpoint) + 0x7;	// zaokrohlovat a neorezavat!
  return _camera_setcool (1, i, 0xaf, FAN_ON, CAMERA_COOL_HOLD);
}

extern int
camera_cool_hold ()		/* hold on that temperature */
{
  QueryTemperatureStatusResults qtsr;
  float ot;			// optimal temperature

  if (!is_init)
    return -1;
  if (camera_reg == CAMERA_COOL_HOLD)
    return 0;			// already cooled

  sem_lock ();
  if (MicroCommand (MC_TEMP_STATUS, cameraID, NULL, &qtsr))
    goto err;
  sem_unlock ();

  ot = ccd_ad2c (qtsr.ccdThermistor);
  ot = ((int) (ot + 5) / 5) * 5;

  camera_fan (1);

  return camera_cool_setpoint (ot);

err:
  sem_unlock ();
  errno = ENODEV;
  return -1;
}

/* Should be improved to support st237 filter wheel */
extern int
camera_set_filter (int filter)	/* set camera filter */
{
  PulseOutParams pop;

  if (!is_init)
    return -1;

  if (filter < 1 || filter > 5)
    {
      errno = EINVAL;
      return -1;
    }

  pop.pulsePeriod = 18270;
  pop.pulseWidth = 500 + 300 * (filter - 1);
  pop.numberPulses = 60;

  sem_lock ();
  if (MicroCommand (MC_PULSE, cameraID, &pop, NULL))
    goto err;
  camera_filter = filter;
  sem_unlock ();

  return 0;

err:
  sem_unlock ();
  errno = ENODEV;
  return -1;
}
