#include "sbig.h"
#include "pardrv.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>

#include <math.h>

#include "../writers/imghdr.h"


extern int
sbig_init (int port, int options, struct sbig_init *init)
{
  EstablishLinkParams in;
  EstablishLinkResults out;

  int result;

  GetCCDInfoParams inf_par;
  GetCCDInfoResults0 inf_res0;
  GetCCDInfoResults2 inf_res2;
  //GetCCDInfoResults3    inf_res3;

  int chip = 0;

  if (port > 5)
    {
      in.port = 0;
      in.baseAddress = port;
    }
  else
    in.port = port;

#ifdef DEBUG
  printf ("CC_ESTABLISH_LINK...port %i\n", in.port);
#endif

  if ((result = ParDrvCommand (CC_ESTABLISH_LINK, &in, &out)) != 0)
    return -result;
  //else


  init->camera_type = out.cameraType;

  if ((out.cameraType == ST7_CAMERA) || (out.cameraType == ST8_CAMERA)
      || (out.cameraType == ST9_CAMERA))
    init->nmbr_chips = 2;
  else
    init->nmbr_chips = 1;
#ifdef DEBUG
  printf ("chips..\n");
#endif

  for (chip = 0; chip < init->nmbr_chips; chip++)
    {
      inf_par.request = chip;
      if ((result =
	   ParDrvCommand (CC_GET_CCD_INFO, &inf_par, &inf_res0)) != 0)
	return -result;
      // else
      init->firmware_version = inf_res0.firmwareVersion;
      strcpy (init->camera_name, inf_res0.name);
      init->camera_info[chip].nmbr_readout_modes = inf_res0.readoutModes;
      memcpy (init->camera_info[chip].readout_mode,
	      inf_res0.readoutInfo,
	      sizeof (inf_res0.readoutInfo[0]) * inf_res0.readoutModes);
    }

  if ((out.cameraType == ST7_CAMERA) || (out.cameraType == ST8_CAMERA)
      || (out.cameraType == ST9_CAMERA))
    {

      inf_par.request = 2;
      if ((result =
	   ParDrvCommand (CC_GET_CCD_INFO, &inf_par, &inf_res2)) != 0)
	return -result;
      // else
      init->nmbr_bad_columns = inf_res2.badColumns;
      memcpy (init->bad_columns, inf_res2.columns, sizeof (inf_res2.columns));
      init->imaging_abg_type = inf_res2.imagingABG;
      strcpy (init->serial_number, inf_res2.serialNumber);
    }
  return 0;
}

unsigned int
ccd_c2ad (double t)
{
  double r = 3.0 * exp (0.94390589891 * (25.0 - t) / 25.0);
  unsigned int ret = 4096.0 / (10.0 / r + 1.0);

  if (ret > 4096)
    ret = 4096;
#ifdef DEBUG
  printf ("ad:%i", ret);
#endif
  return ret;
}

double
ccd_ad2c (unsigned int ad)
{
  double r;
#ifdef DEBUG
  printf ("ad:%i\n", ad);
#endif
  if (ad == 0)
    return 1000;
  r = 10.0 / (4096.0 / (double) ad - 1.0);
#ifdef DEBUG
  printf ("r:%f\n", r);
#endif
  return 25.0 - 25.0 * (log (r / 3.0) / 0.94390589891);
}

double
ambient_ad2c (unsigned int ad)
{
  double r;
#ifdef DEBUG
  printf ("ad:%i\n", ad);
#endif
  r = 3.0 / (4096.0 / (double) ad - 1.0);
  return 25.0 - 45.0 * (log (r / 3.0) / 2.0529692213);
}

extern int
sbig_get_status (struct sbig_status *status)
{
  QueryCommandStatusParams in;
  QueryCommandStatusResults out;

  QueryTemperatureStatusResults temp;

  int result;

  in.command = CC_MISCELLANEOUS_CONTROL;

  if ((result = ParDrvCommand (CC_QUERY_COMMAND_STATUS, &in, &out)) != 0)
    return -result;

  status->shutter_edge = (out.status) & 0x000F;
  status->fan_on = (out.status >> 8) & 0x0001;
  status->shutter_state = (out.status >> 9) & 0x0003;
  status->led_state = (out.status >> 11) & 0x0003;

  in.command = CC_START_EXPOSURE;

  if ((result = ParDrvCommand (CC_QUERY_COMMAND_STATUS, &in, &out)) != 0)
    return -result;

  status->imaging_ccd_status = (out.status) & 0x0003;
  status->tracking_ccd_status = (out.status >> 2) & 0x0003;

  if ((result =
       ParDrvCommand (CC_QUERY_TEMPERATURE_STATUS, NULL, &temp)) != 0)
    return -result;

  status->air_temperature = 10 * ambient_ad2c (temp.ambientThermistor);
  status->ccd_temperature = 10 * ccd_ad2c (temp.ccdThermistor);
  status->cooling_power = temp.power;
  status->temperature_setpoint = 10 * ccd_ad2c (temp.ccdSetpoint);
  status->temperature_regulation = temp.enabled;

  in.command = CC_ACTIVATE_RELAY;

  if ((result = ParDrvCommand (CC_QUERY_COMMAND_STATUS, &in, &out)) != 0)
    return -result;

  status->plus_x_relay = (out.status) & 0x01;
  status->minus_x_relay = (out.status >> 1) & 0x01;
  status->plus_y_relay = (out.status >> 2) & 0x01;
  status->minus_y_relay = (out.status >> 3) & 0x01;

  in.command = CC_PULSE_OUT;

  if ((result = ParDrvCommand (CC_QUERY_COMMAND_STATUS, &in, &out)) != 0)
    return -result;

  status->pulse_active = (out.status) & 0x01;


  return 0;
}

extern int
sbig_control (struct sbig_control *control)
{
  return -ParDrvCommand (CC_MISCELLANEOUS_CONTROL, control, NULL);
};

extern int
sbig_pulse (struct sbig_pulse *pulse)
{
  return -ParDrvCommand (CC_PULSE_OUT, pulse, NULL);
};

extern int
sbig_activate_relay (struct sbig_relay *relay)
{
  return -ParDrvCommand (CC_ACTIVATE_RELAY, relay, NULL);
};

int
sbig_start_expose (struct sbig_expose *expose)
{
  StartExposureParams in;

  in.ccd = expose->ccd;
  in.exposureTime = expose->exposure_time;
  in.abgState = expose->abg_state;
  in.openShutter = expose->shutter;

  return -ParDrvCommand (CC_START_EXPOSURE, &in, NULL);
};

extern int
sbig_expose (struct sbig_expose *expose)
{
#define MAX_WAIT_COUNT  100
  struct sbig_status status;
  int i, ret = 0;
  if ((ret = sbig_start_expose (expose)) < 0)
    return ret;
  usleep (expose->exposure_time * 10000);
  for (i = 0; i < MAX_WAIT_COUNT; i++)
    {
      if ((ret = sbig_get_status (&status)) < 0)
	return ret;

      if (expose->ccd == 0)
	{
	  if (status.imaging_ccd_status != 2)
	    break;
	}
      else if (status.tracking_ccd_status != 2)
	break;

      usleep (50000);
      if (i == MAX_WAIT_COUNT)
	return -1;
    }
  return 0;
}

extern int
sbig_end_expose (unsigned short ccd)
{
  EndExposureParams in;
  in.ccd = ccd;
  return -ParDrvCommand (CC_END_EXPOSURE, &in, NULL);
}

extern int
sbig_update_clock ()
{
  return -ParDrvCommand (CC_UPDATE_CLOCK, NULL, NULL);
}

extern int
sbig_readout_line (struct sbig_readout_line *readout_line)
{
  int result;
#ifdef DEBUG
  printf ("Reading line..pixelStart:%i pixelLength:%i",
	  readout_line->pixelStart, readout_line->pixelLength);
#endif
  result =
    ParDrvCommand (CC_READOUT_LINE, (ReadoutLineParams *) readout_line,
		   readout_line->data);

#ifdef DEBUG
  printf ("..complete, result = %i\n", result);
#endif
  return -result;
};

extern int
sbig_dump_lines (struct sbig_dump_lines *dump_lines)
{
#ifdef DEBUG
  printf ("Dumping %i lines...\n", dump_lines->lineLength);
#endif
  return -ParDrvCommand (CC_DUMP_LINES, (DumpLinesParams *) dump_lines, NULL);
};

extern int
sbig_end_readout (unsigned int ccd)
{
  EndReadoutParams in;

  in.ccd = ccd;
  return -ParDrvCommand (CC_END_READOUT, &in, NULL);
};

extern int
sbig_readout (struct sbig_readout *readout)
{
  struct sbig_readout_line line;
  unsigned int y;
  int result;
  int size;
  struct imghdr *header;

  if ((result = sbig_end_expose (readout->ccd)) < 0)
    return result;

  if ((readout->y) > 0)
    {
      struct sbig_dump_lines dump;

      dump.ccd = readout->ccd;
      dump.readoutMode = readout->binning;
      dump.lineLength = readout->y;

      if ((result = sbig_dump_lines (&dump)) < 0)
	return result;
    }

  size = sizeof (struct imghdr) +
    sizeof (unsigned short) * readout->width * readout->height;
  if (readout->data)
    free (readout->data);

  if ((readout->data = malloc (size)) == NULL)
    return -1;

  line.ccd = readout->ccd;
  line.pixelStart = readout->x;
  line.pixelLength = readout->width;
  line.readoutMode = readout->binning;
  line.data =
    (unsigned short *) ((char *) readout->data + sizeof (struct imghdr));

  readout->data_size_in_bytes = sizeof (struct imghdr);

  header = (struct imghdr *) (readout->data);

  header->data_type = 1;
  header->naxes = 2;
  header->sizes[0] = readout->width;
  header->sizes[1] = readout->height;
  header->binnings[0] = 1;
  header->binnings[1] = 1;

  for (y = 0; y < (readout->height); y++)
    {
      if ((result = sbig_readout_line (&line)) != 0)
	return result;

      line.data += line.pixelLength;

      readout->data_size_in_bytes += 2 * line.pixelLength;
      readout->callback (readout->ccd, y / (float) (readout->height - 1));
    }

  readout->callback (readout->ccd, 1);

  return sbig_end_readout (readout->ccd);
};

extern int
sbig_set_cooling (struct sbig_cool *cool)
{
  int result;
  SetTemperatureRegulationParams in;
  in.regulation = cool->regulation;
  if (cool->regulation == 2)
    in.ccdSetpoint = cool->direct_drive;
  else
    in.ccdSetpoint = ccd_c2ad ((double) cool->temperature / 10.0);

  if ((result =
       ParDrvCommand (CC_SET_TEMPERATURE_REGULATION, &in, NULL)) != 0)
    return -result;
  return 0;
};

extern int
sbig_set_ao7_deflection (int x_deflection, int y_deflection)
{
  return 0;
};

extern int
sbig_set_ao7_focus (int type)
{
  return 0;
};

char *
sbig_show_error (int error)
{
  char *err = (char *) malloc (200);
  sprintf (err, "UNKNOW ERROR: %i", error);
  return err;
};
