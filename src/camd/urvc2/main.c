#define _MAIN_C

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>
#include <sys/io.h>

#include "urvc.h"
#include "fitsio.h"
#include "tcf.h"

#ifdef USE_READLINE
// readline - why to reinvent wheel
#include <readline.h>
#include <history.h>
#endif

fd_set read_fds;

#define RED 0
#define GREEN 1
#define BLUE 2

#define MAX_FITS_PARAMS		20
#define BRIGHT_VALUE		50000
#define BRIGHT_RATIO	  	0.66

int filen = 1;

PAR_ERROR
print_error (PAR_ERROR e)
{
  if (e <= CE_NEXT_ERROR)
    printf ("%s ", CE_names[e]);
  printf ("(error 0x%02x)\n", e);
  return e;
}

static double
getf (char *s, float default_value)
{
  char buffer[80];

  printf ("%s [%03.1f]: ", s, default_value);
  fflush (stdout);
  fgets (buffer, 79, stdin);
  if (buffer[0] == '\n')
    return default_value;
  return atof (buffer);
}

static int
geti (char *s, int default_value)
{
  char buffer[80];

  printf ("%s [%d]: ", s, default_value);
  fflush (stdout);
  fgets (buffer, 79, stdin);
  if (buffer[0] == '\n')
    return default_value;
  return atoi (buffer);
}

void
printerror (int status)
{
  if (status)
    fits_report_error (stdout, status);	// print error report
}

static int
write_fits (char *filename, long exposure, int w, int h, unsigned short *data)
{
  fitsfile *fptr = NULL;
  int status = 0;
  long naxes[2] = { w, h };

//      printf("A"); fflush(stdout);
  if ((!strlen (filename))
      || ((strlen (filename) == 1) && (*filename == '!')))
    return 1;
//      printf("B"); fflush(stdout);

  fits_clear_errmsg ();
//      printf("C %d,%s,%d",fptr, filename, status); fflush(stdout);
  if (fits_create_file (&fptr, filename, &status))
    printerror (status);
//      printf("D"); fflush(stdout);
  if (fits_create_img (fptr, USHORT_IMG, 2, naxes, &status))
    printerror (status);
//      printf("E"); fflush(stdout);
  if (fits_update_key
      (fptr, TLONG, "EXPOSURE", &exposure, "Total exposure time", &status))
    printerror (status);
//      printf("F"); fflush(stdout);
  if (fits_write_img (fptr, TUSHORT, 1, w * h, data, &status))
    printerror (status);
//      printf("G"); fflush(stdout);
  if (fits_close_file (fptr, &status))
    printerror (status);
//      printf("H"); fflush(stdout);

  return 0;
}

void
getmeandisp (unsigned short *img, int len, double *mean, double *disp)
{
  int i;
  double d;

  if (len < 1)
    return;
  if (len == 1)
    {
      *mean = *img;
      *disp = 0;
      return;
    }

  *mean = 0;
  i = len;
  while (i)
    *mean += img[--i];
  *mean /= len;

  *disp = 0;
  i = len;
  while (i)
    {
      d = *mean - img[--i];
      *disp += d * d;
    }
  *disp /= len - 1;
  *disp = sqrt (*disp);
}

/*int cmd_update_clock() {
  int ret;
  ret=sbig_update_clock();
  if (ret < 0)
  sbigerror(ret);
  return ret;
  }   
  */

PAR_ERROR
cmd_status ()
{
  PAR_ERROR ret = CE_NO_ERROR;
  StatusResults gvr;
  QueryTemperatureStatusResults qtsr;

  GetVersionResults st;
  if ((ret = MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &st)))
    return ret;

  if ((ret = MicroCommand (MC_STATUS, st.cameraID, NULL, &gvr)))
    ;				//     return ret;
  if ((ret = MicroCommand (MC_TEMP_STATUS, st.cameraID, NULL, &qtsr)))
    ;				//     return ret;

  printf ("imaging_ccd_status = %d\n", gvr.imagingState);
  if (st.cameraID == ST7_CAMERA)
    printf ("tracking_ccd_status = %d\n", gvr.trackingState);
  printf ("fan_on = %d\n", gvr.fanEnabled);
  if (st.cameraID == ST7_CAMERA)
    printf ("shutter_state = %d\n", gvr.shutterStatus);
  if (st.cameraID == ST7_CAMERA)
    printf ("led_state = %d\n", gvr.ledStatus);
  if (st.cameraID == ST7_CAMERA)
    printf ("shutter_edge = %d\n", gvr.shutterEdge);
  printf ("relays =%s%s%s%s%s\n",
	  gvr.xPlusActive ? " x+" : "",
	  gvr.xMinusActive ? " x-" : "",
	  gvr.yPlusActive ? " y+" : "",
	  gvr.yMinusActive ? " y-" : "",
	  gvr.xPlusActive && gvr.xMinusActive && gvr.yPlusActive
	  && gvr.yMinusActive ? "" : " -");
  // printf("pulse_active = %d\n", gvr.pulse_active); //--
  printf ("CFW_activity = %d\n", gvr.CFW6Active);
  printf ("temperature_regulation = %d\n", qtsr.enabled);
  printf ("temperature_setpoint = %03.1f (Thermistor val. %d)\n",
	  ccd_ad2c (qtsr.ccdSetpoint), qtsr.ccdSetpoint >> 4);
  printf ("cooling_power = %d\n", qtsr.power);
  printf ("air_temperature = %.1f (Thermistor val. %d)\n",
	  ambient_ad2c (qtsr.ambientThermistor), qtsr.ambientThermistor >> 4);
  printf ("ccd_temperature = %.1f (Thermistor val. %d)\n",
	  ccd_ad2c (qtsr.ccdThermistor), qtsr.ccdThermistor >> 4);

  return ret;
}

// make exposure and readout square image
PAR_ERROR
ccd_image (unsigned short *img, int exposure, int shutter, int bin, int x)
{
  PAR_ERROR ret = CE_NO_ERROR;
  CAMERA *C;
  int tx = 0, ty = 0, tw = 1536, th = 1024;

  if ((ret = OpenCCD (0, &C)))
    return ret;

  th = C->vertImage / bin;
  tw = C->horzImage / bin;
  tx = tw / 2;
  ty = th / 2;
  tw = x;
  th = x;
  tx -= tw / 2;
  ty -= th / 2;

  if ((ret = CCDExpose (C, exposure, shutter)))
    return ret;
  do
    TimerDelay (100000);
  while (CCDImagingState (C));
  if ((ret = CCDReadout (img, C, tx, ty, tw, th, bin)))
    return ret;

  return ret;
}

// Specialni funkce pro regresi funkce z=Ax^2+Cx+F
// vektory predpoklada struct v[i] : i je pocet, v pointer na zacatek dat,
// zbytek je pro navratove hodnoty.
#define DIM 3
void
regr_q (double *vX, double *vZ, long i, double *rA, double *rB, double *rC)
{
  double A = 0, B = 0, C = 0, D = 0, N = 0, Q, V = 0, X = 0, Z =
    0, M[DIM][DIM + 1], _B, _C, _D, _Z;
  long a, b, c;

  // spocitame koeficienty
  for (a = 0; a < i; a++)
    {
      _D = vX[a];
      _C = _D * _D;
      _B = _D * _C;
      _Z = vZ[a];
      A += _C * _C;
      B += _B;
      C += _C;
      D += _D;
      N += 1;
      V += _C * _Z;
      X += _D * _Z;
      Z += _Z;
    }

  // naplnime matici
  M[0][0] = A;
  M[0][1] = B;
  M[0][2] = C;
  M[0][DIM] = V;
  M[1][0] = B;
  M[1][1] = C;
  M[1][2] = D;
  M[1][DIM] = X;
  M[2][0] = C;
  M[2][1] = D;
  M[2][2] = N;
  M[2][DIM] = Z;

  // udelame gaussovu eliminaci
  for (a = 0; a < (DIM - 1); a++)	// faze 1. - anulovat cleny pod diagonalou
    for (b = a + 1; b < DIM; b++)
      {				// takze tady odecitam od b. radku a. radek, podil v a. sloupci
	Q = -M[a][a] / M[b][a];
	for (c = 0; c < (DIM + 1); c++)
	  M[b][c] = Q * M[b][c] + M[a][c];
      }
  for (a = 0; a < (DIM - 1); a++)	// faze 2. - anulovat cleny nad diagonalou
    for (b = a + 1; b < DIM; b++)
      {				// tady odecitam od a. radku b. radek - podil clenu v b. sloupci
	Q = -M[b][b] / M[a][b];
	for (c = 0; c < (DIM + 1); c++)
	  M[a][c] = Q * M[a][c] + M[b][c];
      }

  *rA = M[0][DIM] / M[0][0];
  *rB = M[1][DIM] / M[1][1];
  *rC = M[2][DIM] / M[2][2];
}


double run_sex (char *);
#define USTEP 60
#define MAXTRIES 60

PAR_ERROR
cmd_focus ()
{
  PAR_ERROR ret = CE_NO_ERROR;
  int bin, x, i, j, tcfret;
  int Time, add = 0;
  unsigned short *img1 = NULL;
  unsigned short *img2 = NULL;
  unsigned short *img3 = NULL;
  double M = 0, Q = 0;
  double fwhm[MAXTRIES];
  double posi[MAXTRIES], pp = 0;
  double A, B, C = 0, min = 0;

  char filename[64];

  Time = 100 * getf ("Exposure time (0 -- 655.35s)", 0.01) + 0.5;
  bin = geti ("Readout mode (0 = 1x1, 1 = 2x2, ...)", 0);
  x = geti ("Readout mode (0 = 1x1, 1 = 2x2, ...)", 256);

  // Make space for the image
  img1 = (unsigned short *) malloc (x * x * sizeof (unsigned short));
  img2 = (unsigned short *) malloc (x * x * sizeof (unsigned short));
  img3 = (unsigned short *) malloc (x * x * sizeof (unsigned short));

  // init focuser
  tcfret = tcf_set_manual ();
  if (tcfret < 1)
    {
      printf ("focuser: unable to initialize\n");
      return 0;
    }

  posi[0] = 0;

  ccd_image (img2, Time, 2, bin, x);	// Dark

  for (j = 0; j < 100;)
    {
      // save previous image if needed
      if (add)
	{
	  i = x * x;
	  while (i--)
	    img3[i] = img1[i];
	}

      ccd_image (img1, Time, 1, bin, x);	// img
      i = x * x;
      while (i--)
	img1[i] -= img2[i];

      // add previous image if requested
      if (add)
	{
	  i = x * x;
	  while (i--)
	    img1[i] += img3[i];
	}

      getmeandisp (img1, x * x, &M, &Q);
//      printf("Pixel value: %lf +/- %lf\n", M, Q);

      sprintf (filename, "!fo%03di.fits", filen++);
      write_fits (filename, Time, x, x, img1);

      fwhm[j] = run_sex (filename + 1);
      if (fwhm[j] < 0)
	{
	  printf ("Coadding images\n");
	  add = 1;
	}
      else
	{
	  add = 0;		// enough stars:= reset adding
	  switch (j)
	    {
	    case 0:		// setup first step
	      pp = USTEP;
	      break;
	    case 1:
	      // it was worse before
	      if (fwhm[0] > fwhm[1])
		pp = USTEP;
	      // it was better
	      else
		pp = -2 * USTEP;
	      break;

	    case 2:
	      if (fwhm[0] > fwhm[1])	// x10 1x0 10x
		{
		  if (fwhm[2] > fwhm[0])
		    pp = (posi[1] + posi[0]) / 2;	// 102 
		  else
		    pp = (posi[1] + posi[2]) / 2;	// 120 210
		}
	      else		// 0x1 x01 01x
		{
		  if (fwhm[2] > fwhm[1])
		    pp = (posi[1] + posi[0]) / 2;	// 012
		  else
		    pp = (posi[2] + posi[0]) / 2;	// 201 021

		}
	      pp = pp - posi[2];
	      break;
	    default:		// We have 3 values, we may start fitting...

	      //if(j>2)
	      regr_q (posi, fwhm, j + 1, &A, &B, &C);
	      /*else
	         {
	         A=( (fwhm[0]-fwhm[1])/(posi[0]-posi[1]) - 
	         (fwhm[0]-fwhm[2])/(posi[0]-posi[2]))
	         /(posi[1]-posi[2]);
	         B=( (fwhm[1]-fwhm[1])/(posi[0]-posi[1]) - 
	         A*(posi[0]+posi[1]));
	         } */

	      min = -B / (2 * A);

	      // ted jsem na pos[j], chci se dostat na min, pokud
	      // fabs(min-pos[j])<3*USTEP

	      pp = min - posi[j];
	      if (pp > 3 * USTEP)
		pp = 3 * USTEP;
	      if (pp < (-3 * USTEP))
		pp = -3 * USTEP;

	      break;

	    }

	  printf ("%02d: posi:%f, fwhm:%f -> min=%f, pp=%f\n", j, posi[j],
		  fwhm[j], min, pp);

	  if (j > 6 && pp < 1.0)
	    break;

	  posi[j + 1] = posi[j] + pp;

	  tcfret = tcf_step_out ((pp > 0) ? pp : -pp, (pp > 0) ? 1 : -1);
	  j++;
	}
    }

  if (img1)
    free (img1);
  if (img2)
    free (img2);
  return ret;
}

PAR_ERROR
cmd_imaging ()
{
  PAR_ERROR ret = CE_NO_ERROR;
  int i, x, tx = 0, ty = 0, tw = 1536, th = 1024;
  static int Time = 10, Shutter = 1, Chip = 0, bin = 1;
  unsigned short *img = NULL, *imgd = NULL;
  double M = 0, Q = 0;
  char filename[64];

  CAMERA *C;

  Chip = geti ("Chip (0,1 :)", Chip);
  Time =
    100 * getf ("Exposure time (0 -- 655.35s)", ((float) Time) / 100) + 0.5;
  Shutter =
    geti ("Shutter (0 = tracking, 1 = imaging, 2 = dark, 3=image-dark)",
	  Shutter);
  bin = geti ("Readout mode (1 = 1x1, 2 = 2x2, ...)", bin);

  if ((ret = OpenCCD (Chip, &C)))
    goto imaging_end;

  th = C->vertImage / bin;
  tw = C->horzImage / bin;
  tx = geti ("X center", tw / 2);
  ty = geti ("Y center", th / 2);
  tw = geti ("X size", tw);
  th = geti ("Y size", th);
  tx -= tw / 2;
  ty -= th / 2;

  // Make space for the image
  img = (unsigned short *) malloc (tw * th * sizeof (unsigned short));

  x = ((Shutter == 3) ? 1 : Shutter);

  if ((ret = CCDExpose (C, Time, x)))
    goto imaging_end;
  do
    TimerDelay (100000);
  while (CCDImagingState (C));
  if ((ret = CCDReadout (img, C, tx, ty, tw, th, bin)))
    goto imaging_end;


  if (Shutter == 3)
    {
      imgd = (unsigned short *) malloc (tw * th * sizeof (unsigned short));

      if ((ret = CCDExpose (C, Time, 2)))
	goto imaging_end;
      do
	TimerDelay (100000);
      while (CCDImagingState (C));
      if ((ret = CCDReadout (imgd, C, tx, ty, tw, th, bin)))
	goto imaging_end;

      i = tw * th;
      while (i--)
	img[i] -= imgd[i];
      if (imgd)
	free (imgd);
    }

  CloseCCD (C);

  getmeandisp (img, tw * th, &M, &Q);
  printf ("Pixel value: %lf +/- %lf\n", M, Q);

  sprintf (filename, "!f%03di.fits", filen++);

  printf ("Saving as %s\n", filename);
  write_fits (filename, Time, tw, th, /* NULL, */ img);

imaging_end:

  if (img)
    free (img);
  return ret;
}

PAR_ERROR
cmd_port ()
{
  int tt, uu;

  tt = geti ("Parport number (0, 1, ...)", 0);

  if ((uu = getbaseaddr (tt)) == 0)
    {
      printf ("-p: parport%d not known to kernel\n", tt);
      return CE_BAD_PARAMETER;
    }
  else
    {
      CameraInit (uu);
      return CE_NO_ERROR;
    }
}

PAR_ERROR
cmd_ver ()
{
  PAR_ERROR ret = CE_NO_ERROR;
  EEPROMContents eePtr;
  GetVersionResults st;

  if ((ret = MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &st)))
    return ret;

  printf ("ID  0x%X 0x%X\n", st.cameraID, st.firmwareVersion);

  if ((ret = GetEEPROM (st.cameraID, &eePtr)))
    return ret;

  printf ("SBIG %s (firmware v.%d.%02d serial %s)\n",
	  Cams[eePtr.model].fullName, st.firmwareVersion >> 8,
	  st.firmwareVersion & 0xff, eePtr.serialNumber);
  printf ("Chip_0:\t%s\t(array:%dx%d pixel:%d.%02dx%d.%02dum)\n",
	  Cams[eePtr.model].chipName, Cams[eePtr.model].horzImage,
	  Cams[eePtr.model].vertImage, Cams[eePtr.model].pixelX / 100,
	  Cams[eePtr.model].pixelX % 100, Cams[eePtr.model].pixelY / 100,
	  Cams[eePtr.model].pixelY % 100);
  if (Cams[eePtr.model].hasTrack)
    printf ("Chip_1:\t%s\t(array:%dx%d pixel:%d.%02dx%d.%02dum)\n",
	    Cams[0].chipName, Cams[0].horzImage, Cams[0].vertImage,
	    Cams[0].pixelX / 100, Cams[0].pixelX % 100,
	    Cams[0].pixelY / 100, Cams[0].pixelY % 100);

  return ret;
}

// DUMP EEPROM contents to the screen
PAR_ERROR
cmd_eeprom ()
{
  PAR_ERROR ret = CE_NO_ERROR;
  EEPROMContents eePtr;
  int reply = 0, x;

  GetVersionResults st;
  if ((ret = MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &st)))
    return ret;

  if ((ret = GetEEPROM (st.cameraID, &eePtr)))
    printf ("WARNING: EEPROM problem, results may be unreliable!\n");

  printf ("[0] EEPROM version= %d\n", eePtr.version);
  printf ("[1]          model= %d (=%s)\n", eePtr.model,
	  Cams[eePtr.model].fullName);
  printf ("[2]        abgType= %d\n", eePtr.abgType);
  printf ("[3]     badColumns= %d\n", eePtr.badColumns);
  printf ("[4] trackingOffset= %d\n", eePtr.trackingOffset);
  printf ("[5]   trackingGain= %d\n", eePtr.trackingGain);
  printf ("[6-13]     columns= %d, %d, %d, %d\n", eePtr.columns[0],
	  eePtr.columns[1], eePtr.columns[2], eePtr.columns[3]);
  printf ("[14-30] serialNum = \"%s\"\n", eePtr.serialNumber);
  printf ("[31-32]  checksum = %d\n", eePtr.checksum);
  printf ("[-] real checksum = %d\n", CalculateEEPROMChecksum (&eePtr));

  if (st.cameraID == ST7_CAMERA)
    {
      reply = geti ("Change contents? (0 = no, 1 = yes)", reply);

      if (reply)
	{
	  ret = CE_NO_ERROR;
	  //eePtr.version=geti("[0] EEPROM version (should be 1)",eePtr.version);
	  eePtr.model = geti ("(4=st7, 5=st8, 6=st5c, 7=TCE,"
			      " 8=st237, 9=stk, 10=st9, 11=stv, 12=st10, 13=st1001)\n model",
			      eePtr.model);
	  eePtr.abgType = geti ("abgType", eePtr.abgType);
	  eePtr.badColumns = geti ("Number of bad columns", eePtr.badColumns);
	  eePtr.trackingOffset =
	    geti ("Tracking offset", eePtr.trackingOffset);
	  eePtr.trackingGain = geti ("Tracking gain", eePtr.trackingGain);
	  for (x = 0; x < 4; x++)
	    {
	      if (x < eePtr.badColumns)
		eePtr.columns[x] =
		  geti ("Bad column number\n", eePtr.columns[x]);
	      else
		eePtr.columns[x] = 0;
	    }

	  if ((ret = PutEEPROM (st.cameraID, &eePtr)))
	    printf ("WARNING: EEPROM writing problem...\n");
	}
    }
  return ret;
}

PAR_ERROR
cmd_misc ()
{
  PAR_ERROR ret = CE_NO_ERROR;

  MiscellaneousControlParams ctrl;
  StatusResults sr;
  GetVersionResults st;


  if ((ret = MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &st)))
    return ret;
  if ((ret = MicroCommand (MC_STATUS, st.cameraID, NULL, &sr)))
    return ret;

  ctrl.fanEnable = geti ("Fan (0 = off, 1 = on)", sr.fanEnabled);
  if (st.cameraID == ST7_CAMERA)
    {
      ctrl.shutterCommand =
	geti ("Shutter (0 = nothing, 1 = open, 2 = close, 3 = init)", 0);
      ctrl.ledState =
	geti ("LED (0 = off, 1 = on, 2 = slow, 3 = fast)", sr.ledStatus);
    }
  else
    {
      // not to send stupid numbers to st237
      ctrl.shutterCommand = 0;
      ctrl.ledState = 0;
    }

  ret = MicroCommand (MC_MISC_CONTROL, st.cameraID, &ctrl, NULL);
  // ret uz netestuju - stejne ho vracim...
  return ret;
}

/*
   int cmd_relay() {
   int i,ret;
   struct sbig_relay relay;
   i = geti("X time? [+- 0.1 s] ", 0);
   if (i < 0) {
   relay.x_plus_time = 0;
   relay.x_minus_time = -i;
   } 
   else {
   relay.x_plus_time = i;
   relay.x_minus_time = 0;
   }
   i = geti("Y time? [+- 0.1 s] ", 0);
   if (i < 0) {
   relay.y_plus_time = 0;
   relay.y_minus_time = -i;
   } else {
   relay.y_plus_time = i;
   relay.y_minus_time = 0;
   }
   sync();
   ret = sbig_activate_relay(&relay);
   printf("sbig_activate_relay() returned %d\n", ret);
   if (ret < 0)
   sbigerror(ret);
   return ret;
   }
*/

PAR_ERROR
cmd_pulse ()
{
  int ret;
  PulseOutParams pop;
  GetVersionResults st;

  pop.numberPulses = geti ("nmbr of pulses: ", 60);
  pop.pulseWidth = geti ("pulse width in us [> 8]: ", 500);
  pop.pulsePeriod = geti ("pulse period in us [> 28+width]: ", 18270);

  if ((ret = MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &st)))
    return ret;
  ret = MicroCommand (MC_PULSE, st.cameraID, &pop, NULL);

  return ret;
}

PAR_ERROR
cmd_filter ()
{
  int i, ret;
  PulseOutParams pop;

  GetVersionResults st;
  if ((ret = MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &st)))
    return ret;

  i = geti ("CFW8 position [1 to 5]: ", 1);
  if (i < 1 || i > 5)
    {
      printf ("filter position is 1, 2, 3, 4 or 5\n");
      fflush (stdout);
      return 1;
    }

  pop.pulsePeriod = 18270;
  pop.pulseWidth = 500 + 300 * (i - 1);
  pop.numberPulses = 60;

  ret = MicroCommand (MC_PULSE, st.cameraID, &pop, NULL);

  return ret;
}

PAR_ERROR
cmd_cool ()
{
  PAR_ERROR ret = CE_NO_ERROR;
  int i;
  double f, g;
  MicroTemperatureRegulationParams cool;

  GetVersionResults st;
  if ((ret = MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &st)))
    return ret;
  cool.regulation = geti ("Cooling (0 = off, 1 = temp, 2 = power)", 1);

  switch (cool.regulation)
    {
    case 0:
      cool.preload = 0;
      cool.ccdSetpoint = 0;
      break;
    case 1:
      f = getf ("Setpoint (-150 -- +140oC)", 0.0);
      i = ccd_c2ad (f) + 0x7;	// zaokrohlovat a neorezavat!
      g = ccd_ad2c (i & 0xff0);
      printf ("Teplotu nastavim na %.2f (chyba = %.2f, ad = %d)\n", g, f - g,
	      i >> 4);

      cool.ccdSetpoint = i;
      cool.preload = 0xaf;
      break;
    case 2:
      // cool.ccdSetpoint = 0;
      cool.ccdSetpoint = geti ("Power (0 -- 255)", 200);
      // cool.preload = geti("Power (0 -- 255)", 200);
      cool.preload = 0;
      break;
    default:
      printf ("Eek?\n");
      break;
    }

  ret = MicroCommand (MC_REGULATE_TEMP, st.cameraID, &cool, NULL);
  return ret;
}

PAR_ERROR
cmd_on ()
{
  PAR_ERROR ret = CE_NO_ERROR;

  MiscellaneousControlParams ctrl;
  MicroTemperatureRegulationParams cool;
  PulseOutParams pop;
  GetVersionResults st;
  if ((ret = MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &st)))
    return ret;

  cool.regulation = 2;
  cool.preload = 0;
  cool.ccdSetpoint = 200;

  ctrl.fanEnable = 1;
  ctrl.shutterCommand = 3;
  ctrl.ledState = 1;

  pop.pulsePeriod = 18270;
  pop.pulseWidth = 500;
  pop.numberPulses = 60;

  ret = MicroCommand (MC_MISC_CONTROL, st.cameraID, &ctrl, NULL);
  printf ("Eek1?\n");
  if (ret)
    return ret;
//    usleep(500000);
  ret = MicroCommand (MC_REGULATE_TEMP, st.cameraID, &cool, NULL);
  printf ("Eek2?\n");
  if (ret)
    return ret;
  usleep (2000000);
  ret = MicroCommand (MC_PULSE, st.cameraID, &pop, NULL);

  return ret;
}

PAR_ERROR
cmd_off ()
{
  PAR_ERROR ret = CE_NO_ERROR;

  MiscellaneousControlParams ctrl;
  MicroTemperatureRegulationParams cool;
  GetVersionResults st;
  if ((ret = MicroCommand (MC_GET_VERSION, ST7_CAMERA, NULL, &st)))
    return ret;

  cool.regulation = 0;
  cool.preload = 0;
  cool.ccdSetpoint = 0;

  ctrl.fanEnable = 0;
  ctrl.shutterCommand = 0;
  ctrl.ledState = 0;

  ret = MicroCommand (MC_MISC_CONTROL, st.cameraID, &ctrl, NULL);
  ret = MicroCommand (MC_REGULATE_TEMP, st.cameraID, &cool, NULL);

  return ret;
}

PAR_ERROR
cmd_quit ()
{

  end_realtime ();
  exit (0);
}

typedef PAR_ERROR Cmd ();

typedef struct
{
  char *text;
  Cmd *cmd;
  char *description;
}
COMMAND;

int CMD_NUM;

PAR_ERROR cmd_help ();

COMMAND commands[] = {
  {"status", cmd_status, "Dumps some status information"},
  {"eeprom", cmd_eeprom,
   "Dumps EPROM as is and allows for it's change (use with care)"},
  // { "expose", cmd_expose },
  // { "update_clock", cmd_update_clock },
  {"misc", cmd_misc, "Controls LED, Fan and shutter"},
  {"ver", cmd_ver, "Basic camera info"},
  // { "relay", cmd_relay },
  //{"Flat", cmd_flat, "flat field gen"},
  {"x", cmd_focus, "sextractor/optec focusing"},
  {"filter", cmd_filter, "filter wheel"},
  {"Pulse", cmd_pulse, "Send pulses to device port"},
  {"cool", cmd_cool, "Cooling control"},
  // { "init", cmd_init },
  // {"track", cmd_tracking, "Exposure with tracking chip"},
//  {"dotrack", cmd_do_tracking, "Exposure with tracking (not ready)"},
  {"image", cmd_imaging, "Exposure with imaging chip"},
  {"quit", cmd_quit, "Leave program"},
  {"bye", cmd_quit, "Another way to leave program"},
  {"help", cmd_help, "Writes this"},
  {"port", cmd_port, "Changes parallel port being used"},
  {"on", cmd_on, "Perform actions to reset the camera after OFF"},
  {"off", cmd_off, "Perform actions to set camera to OFF-like mode"},
  {"?", cmd_help, "Synonym of help"}
//    {"\x4", cmd_quit}
};

PAR_ERROR
cmd_help ()
{
  PAR_ERROR ret = CE_NO_ERROR;
  int i;

  for (i = 0; i < CMD_NUM; i++)
    printf ("%s\t... %s\n", commands[i].text, commands[i].description);
  return ret;
}

int
main (int argc, char *argv[])
{
  int i, j, k, l = 0;
  //double timer2, timer3, timer4;
  char *buffer;
  PAR_ERROR ret = CE_NO_ERROR;
  unsigned int tt, base;

  CMD_NUM = sizeof (commands) / sizeof (COMMAND);

  if ((tt = getbaseaddr (0)) == 0)
    base = 0x378;
  else
    base = tt;

  for (i = 1; i < argc; ++i)
    {
      if (!strcmp (argv[i], "-a"))
	{
	  sscanf (argv[++i], "%x", &tt);
	  base = tt;
	}

      if (!strcmp (argv[i], "-port"))
	{
	  sscanf (argv[++i], "%x", &tt);
	  base = tt;
	}

      if (!strcmp (argv[i], "-p"))
	{
	  sscanf (argv[++i], "%d", &tt);
	  if ((base = getbaseaddr (tt)) == 0)
	    {
	      printf ("-p: parport%d not known to kernel\n", tt);
	      exit (0);
	    }
	}
    }


  sync ();
  begin_realtime ();
  CameraInit (base);
  //measure_pp();

  while (1)
    {

      fflush (stdout);

#ifdef USE_READLINE
      buffer = readline ("> ");
      add_history (buffer);
#else
      printf ("> ");
      fflush (stdout);
      buffer = (char *) malloc (1024);
      fgets (buffer, 1024, stdin);

      if (feof (stdin))
	{
	  fputc ('\n', stdout);
	  cmd_quit ();
	}

      i = strlen (buffer);
      if (i > 0)
	buffer[i - 1] = 0;
#endif

      k = strlen (buffer);

      if (k != 0)
	{
	  for (i = j = 0; i < CMD_NUM; i++)
	    if (!strncmp (buffer, commands[i].text, k))
	      {
		j++;
		l = i;
	      }
	  switch (j)
	    {
	    case 1:
	      ret = commands[l].cmd ();
	      if (ret)
		print_error (ret);
	      // printf(">Ready\n");
	      break;

	    case 0:
	      printf ("Unrecognized command \'%s\'\n", buffer);
	      break;
	    default:
	      for (i = 0; i < CMD_NUM; i++)
		if (!strncmp (buffer, commands[i].text, k))
		  printf ("%s ", commands[i].text);
	      printf ("<- There are more possibilities.\n");
	      break;
	    }
	}
      free (buffer);
    }
  end_realtime ();
  exit (0);
}
