#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <libnova/libnova.h>
#include <math.h>
#include <malloc.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/sem.h>

#include <syslog.h>

#include "../telescope.h"

#include "status.h"

#include "libmks3.h"

#define LONGITUDE +6.732778
#define LATITUDE 37.1
#define ALTITUDE 30

#define DEF_SLEWRATE 1080000000.0
#define RA_SLEW_SLOWER 0.75
#define DEC_SLEW_SLOWER 0.90

#define PARK_DEC -2.079167
#define PARK_HA -29.94128

/* RAW model: Tpoint models should be computed as addition to this one */
#define DEC_CPD -20880.0	// DOUBLE - how many counts per degree...
#define HA_CPD -32000.0		// 8.9 counts per arcsecond
// Podle dvou ruznych modelu je to bud <-31845;-31888>, nebo <-31876;-31928>, => -31882
// pripadne <32112;32156> a <32072;32124> => -32118 (8.9216c/") podle toho, jestli se
// to ma pricist nebo odecist, coz nevim. -m.

//#define DEC_ZERO 41760.0      // DEC homing: 2 deg
#define DEC_ZERO (2.0*DEC_CPD)	// DEC homing: 2 deg
#define HA_ZERO (-30.0*HA_CPD)	// AFTERNOON ZERO: -30 deg


//#define DEC_ZERO -43413               // homing mark in DEC
//#define HA_ZERO 958121                // AFTERNOON ZERO (The other is HA_ZERO + HA_CPD * 360)

//#define DUMMY

struct ln_lnlat_posn observer = {
  +6.732778,
  37.104444
};

typedef struct
{
  MKS3Id axis0, axis1;
  double lastra, lastdec;
} T9;

T9 *mount = NULL;

/* some fancy functions to handle angles */
double
in180 (double x)
{
  for (; x > 180; x -= 360);
  for (; x < -180; x += 360);
  return x;
}

double
in360 (double x)
{
  for (; x > 360; x -= 360);
  for (; x < 0; x += 360);
  return x;
}

#define RAD (3.1415927/180)

/*
#define TP_IH	-1213.51
#define TP_ID	-103.20
#define TP_NP	+406.77
#define TP_CH	-292.55
#define TP_ME	+210.87
#define TP_MA	-5.14
#define TP_PDD	+305.37
#define	TP_PHH	+605.12
#define	TP_A1D	-819.17
#define	TP_A1H	-233.45
*/

#define TP_IH	-590.34		// 590/293
#define TP_ID	+54.24		//  54/74
#define TP_NP	+16.97		//  17/144
#define TP_CH	-270.84		// 271/277
#define TP_MA	+76.91		//  77/100
#define TP_ME	-360.0		//  *** to test ***
#define TP_PHH	+100.0		//  *** to test ***
#define TP_PDD	+88.07		//  88/72
#define	TP_A1D	-541.47		// 541/154
#define	TP_A1H	-828.83		// 828/88

#define PI 3.1415927
#define DEG_TO_RAD (180/PI)

void
applyME (double h0, double d0, double *ph1, double *pd1, double ME)
{
  double h1, d1, M, N;

  h0 = in180 (h0);

  fprintf (stderr, "%f %f\n", h0, d0);

  h0 /= DEG_TO_RAD;
  d0 /= DEG_TO_RAD;

  N = asin (sin (h0) * cos (d0));
  M = asin (sin (d0) / cos (N));

  if ((h0 > (PI / 2)) || (h0 < (-PI / 2)))
    {
      if (M > 0)
	M = PI - M;
      else
	M = -PI + M;
    }

  M = M + (ME / (3600 * DEG_TO_RAD));

  if (M > PI)
    M -= 2 * PI;
  if (M < -PI)
    M += 2 * PI;

  fprintf (stderr, "%f %f\n", DEG_TO_RAD * N, DEG_TO_RAD * M);

  d1 = asin (sin (M) * cos (N));
  h1 = asin (sin (N) / cos (d1));
  if (M > (PI / 2))
    h1 = PI - h1;
  if (M < (-PI / 2))
    h1 = -PI + h1;

  if (h1 > PI)
    h1 -= 2 * PI;
  if (h1 < -PI)
    h1 += 2 * PI;

  *ph1 = DEG_TO_RAD * h1;
  *pd1 = DEG_TO_RAD * d1;

  fprintf (stderr, "%f %f\n", *ph1, *pd1);
}

// return sid time since homing
double
sid_home ()
{
  long pos0, en0;

  if (MKS3PosEncoderGet (mount->axis0, &en0))
    return 0;
  if (MKS3PosCurGet (mount->axis0, &pos0))
    return 0;

  return (double) (en0 - pos0) / HA_CPD;
}

// *** return siderial time in degrees ***
// once libnova will use struct timeval in
// ln_ln_get_julian_from_sys, this will be completely OK
double
sid_time ()
{
  return in360 (15 *
		ln_get_apparent_sidereal_time (ln_get_julian_from_sys ()) -
		LONGITUDE);
}

double
gethoming ()
{
  return sid_time () - sid_home ();
}

// Use the libnova one once it works well..
double
ln_get_refraction (double altitude)
{
  double R;

  //altitude = ln_deg_to_rad (altitude);

  /* eq. 5.27 (Telescope Control,M.Trueblood & R.M. Genet) */
  R = 1.02 /
    tan (ln_deg_to_rad (altitude + 10.3 / (altitude + 5.11) + 0.0019279));

  // for gnuplot (in degrees)
  // R(a) = 1.02 / tan( (a + 10.3 / (a + 5.11) + 0.0019279) / 57.2958 ) / 60

  /* take into account of atm press and temp */
  //R *= ((atm_pres / 1010) * (283 / (273 + temp)));
  R *= ((1013.6 / 1010) * (283 / (273 + 10)));

  /* convert from arcminutes to degrees */
  R /= 60.0;

  return (R);
}


/* compute RAW trasformation (no Tpoint) */
void
counts2sky (long ac, long dc, double *ra, double *dec, int *flip)
{
  double _ra, _dec;

  // Base transform to raw RA, DEC    
  _dec = (double) (dc - DEC_ZERO) / DEC_CPD;
  _ra = (double) (ac - HA_ZERO) / HA_CPD;
  _ra = -_ra + gethoming ();

  // Flip
  if (_dec > 90)
    {
      *flip = 1;
      _dec = 180 - _dec;
      _ra = 180 + _ra;
    }
  else
    *flip = 0;

  _dec = in180 (_dec);
  _ra = in360 (_ra);

  *dec = _dec;
  *ra = _ra;
}

void
sky2counts (double ra, double dec, long *ac, long *dc)
{
  long _dc, _ac, flip = 0;
  double ha, JD;
  struct ln_equ_posn aber_pos, pos;
  struct ln_hrz_posn hrz;

  pos.ra = ra;
  pos.dec = dec;
  JD = ln_get_julian_from_sys ();

// Aberation
  ln_get_equ_aber (&pos, JD, &aber_pos);
// Precession
  ln_get_equ_prec (&aber_pos, JD, &pos);
// Refraction 
  ln_get_hrz_from_equ (&pos, &observer, JD, &hrz);
  hrz.alt += ln_get_refraction (hrz.alt);
  ln_get_equ_from_hrz (&hrz, &observer, JD, &pos);

  ra = pos.ra;
  dec = pos.dec;

  // True Hour angle
  ha = in180 (sid_time () - ra);
  ra = in180 (gethoming () - ra);

// xxx FLIP xxx
  if (ha < 0)
    flip = 1;

//    apply_tpoint_model(&ra, &dec, flip);

  if (flip)
    {
      dec = 180 - dec;
      ra = 180 + ra;
    }

  _dc = DEC_ZERO + (long) (DEC_CPD * dec);
  _ac = HA_ZERO + (long) (HA_CPD * ra);

  *dc = _dc;
  *ac = _ac;
}

void
display_status (long status, int color)
{
  if (status & MOTOR_HOMING)
    printf ("\033[%dmHOG\033[m ", 30 + color);
  //if(status&MOTOR_SERVO)printf("\033[31mMOTOR_SERVO\033[m ");
  if (status & MOTOR_INDEXING)
    printf ("\033[%dmIDX\033[m ", 30 + color);
  if (status & MOTOR_SLEWING)
    printf ("\033[%dmSLW\033[m ", 30 + color);
  if (!(status & MOTOR_HOMED))
    printf ("\033[%dm!HO\033[m ", 30 + color);
  if (status & MOTOR_JOYSTICKING)
    printf ("\033[%dmJOY\033[m ", 30 + color);
  if (status & MOTOR_OFF)
    printf ("\033[%dmOFF\033[m ", 30 + color);
}

int
getradec (double *ra, double *dec, int *flip)
{
  long pos0, pos1;
  int ret = 0;

  if ((ret = MKS3PosCurGet (mount->axis0, &pos0)))
    goto rr;
  if ((ret = MKS3PosCurGet (mount->axis1, &pos1)))
    goto rr;

  counts2sky (pos0, pos1, ra, dec, flip);

rr:
  if (ret)
    fprintf (stderr, "Error:%d\n", ret);
  return ret;
}

void
statuswatch (MKS3Id axis0, MKS3Id axis1)
{
//#ifdef S_WATCH
  long pos0, pos1;
  unsigned short stat0, stat1;
  double ra, dec;
  int flip;

  MKS3StatusGet (axis0, &stat0);
  MKS3StatusGet (axis1, &stat1);

  MKS3PosCurGet (axis0, &pos0);
  MKS3PosCurGet (axis1, &pos1);
  counts2sky (pos0, pos1, &ra, &dec, &flip);

  printf ("\033[31mPos: %ld (%fd)\033[m ", pos0, ra);
  printf ("\033[32mPos: %ld (%fd)\033[m ", pos1, dec);

  display_status (stat0, 1);
  display_status (stat1, 2);


  printf ("                      \r");
  fflush (stdout);
//#endif
}


/* Call to home the mount: let the mount find it's position mark and reinitialize the counters */
/* should be called whenever the status flags do not show MOTOR_HOMED, as most probably the mount has been restarted */

int
home ()
{
  int i, q, ret = 0;
  unsigned short stat0, stat1;
//    struct sembuf sem_buf;

//    sem_buf.sem_num = SEM_MOVE;
//    sem_buf.sem_op = -1;
//    sem_buf.sem_flg = SEM_UNDO;
//    if (semop (semid, &sem_buf, 1) < 0)
//          return -1;

  if ((ret = MKS3Home (mount->axis0, 0)))
    goto rr;
  if ((ret = MKS3Home (mount->axis1, 0)))
    goto rr;

  for (i = 0, q = 1;; i++)
    {
      statuswatch (mount->axis0, mount->axis1);

      if ((ret = MKS3StatusGet (mount->axis1, &stat1)))
	goto rr;
      if ((ret = MKS3StatusGet (mount->axis0, &stat0)))
	goto rr;

      if ((stat0 & MOTOR_HOMED) && (stat1 & MOTOR_HOMED))
	break;
      usleep (100000);
    }

rr:
//    sem_buf.sem_op = 1;
//    semop (semid, &sem_buf, 1);

  return ret;
}

int
opthome ()
{
  int ret;
  unsigned short stat0, stat1;
//    struct sembuf sem_buf;

  // Lock the port for writing 
//    sem_buf.sem_num = SEM_MOVE;
//    sem_buf.sem_flg = SEM_UNDO;
//    sem_buf.sem_op = -1;
//    if (semop (semid, &sem_buf, 1) < 0) return -1;


  if ((ret = MKS3StatusGet (mount->axis0, &stat0)))
    goto rr;
  if ((ret = MKS3StatusGet (mount->axis1, &stat1)))
    goto rr;

  if ((!(stat0 & MOTOR_HOMED)) || (!(stat1 & MOTOR_HOMED)))
    {
      fprintf (stderr, "para: homing needed\n");
      ret = home (mount);
    }

rr:
//    sem_buf.sem_op = 1;
//    semop (semid, &sem_buf, 1);
  if (ret)
    fprintf (stderr, "%s: mks_error: %d\n", __FUNCTION__, ret);
  return ret;
}

/* basic GoTo call */

/* should apply all the corrections needed to get the telescope on a desired
 * object, i.e. astrometry, refraction, mount modeling */

int
move (double ra, double dec)
{
  long ac, dc;
  unsigned short stat0, stat1;
//    struct sembuf sem_buf;
  int ret = 0;

  sky2counts (ra, dec, &ac, &dc);

  mount->lastra = ra;
  mount->lastdec = dec;

  if ((ret = opthome ()))
    return ret;

  // Lock the port for writing 
//    sem_buf.sem_num = SEM_MOVE;
//    sem_buf.sem_flg = SEM_UNDO;
//    sem_buf.sem_op = -1;
//    if (semop (semid, &sem_buf, 1) < 0) return -1;

  // Set up the speed of the mount 
  if ((ret =
       MKS3RateSlewSet (mount->axis0,
			(unsigned long) (DEF_SLEWRATE * RA_SLEW_SLOWER))))
    goto rr;
  if ((ret =
       MKS3RateSlewSet (mount->axis1,
			(unsigned long) (DEF_SLEWRATE * DEC_SLEW_SLOWER))))
    goto rr;;

  // Send it to a requested position
  if ((ret = MKS3PosTargetSet (mount->axis1, dc)))
    goto rr;
  if ((ret = MKS3PosTargetSet (mount->axis0, ac)))
    goto rr;

  // wait to finish the slew
  for (;;)
    {
      //struct sembuf sem_buf_write;

      // Lock the mount for reading
      //sem_buf_write.sem_num = SEM_TEL;
      //sem_buf_write.sem_flg = SEM_UNDO;
      //sem_buf_write.sem_op = -1;

      //if (semop (semid, &sem_buf_write, 1) < 0)
      //      break;

      if ((ret = MKS3StatusGet (mount->axis0, &stat0)))
	goto rr;
      if ((ret = MKS3StatusGet (mount->axis1, &stat1)))
	goto rr;

      // Unlock
      //sem_buf_write.sem_op = 1;
      //semop(semid, &sem_buf_write, 1);

      if ((stat1 & MOTOR_SLEWING) || (stat0 & MOTOR_SLEWING))
	usleep (100000);
      else
	break;
    }

  // Unlock    
rr:
//    sem_buf.sem_op = 1;
//    semop (semid, &sem_buf, 1);
  return ret;
}

int
optwake ()
{
  int ret = 0;
  unsigned short stat0, stat1;

  if ((ret = MKS3StatusGet (mount->axis0, &stat0)))
    goto rr;
  if ((ret = MKS3StatusGet (mount->axis1, &stat1)))
    goto rr;

  // Try both axes without GoTo's, not really the best solution, but...
  if (stat0 & MOTOR_OFF)
    ret = MKS3MotorOn (mount->axis0);
  if (stat1 & MOTOR_OFF)
    ret = MKS3MotorOn (mount->axis1);

rr:
  return ret;
}

int
zdechni ()
{
  int ret = 0;
  unsigned short stat0, stat1;

  if ((ret = MKS3StatusGet (mount->axis0, &stat0)))
    goto rr;
  if ((ret = MKS3StatusGet (mount->axis1, &stat1)))
    goto rr;

  if ((!(stat0 & MOTOR_OFF)) || (!(stat1 & MOTOR_OFF)))
    {
      if ((ret = move (sid_time () + PARK_HA, PARK_DEC)))
	goto rr;
      ret = MKS3MotorOff (mount->axis0);
      ret = MKS3MotorOff (mount->axis1);
    }

rr:
  return ret;
}

/* ******************************************
*  EXPORTED FUNCTIONS..
*  ******************************************/

/* Open device...
*/

int
init (const char *device)
{
//    T9 *mount;
  int ret;

  mount = (T9 *) malloc (sizeof (T9));

  mount->axis0.unitId = 0x64;
  mount->axis1.unitId = 0x64;

  mount->axis0.axisId = 0;
  mount->axis1.axisId = 1;

// A co kdyz je teleskop vypnuty! - MKS3Init vraci MKS_OK, jinak kod chyby...

  fprintf (stderr, "init: %s\n", device);

  MKS3Init (device);

  // If needed, perform homing
  ret = opthome ();

  return ret;
}

/* Init the telescope - mainly init the comm. protocol and locks */

extern int
telescope_init (const char *device_name, int telescope_id)
{
//  union semun sem_un;
//  unsigned short int sem_arr[] = { 1, 1 };

/*  if ((semid = semget (ftok (device_name, 0), 2, 0644)) < 0)
    {
      if ((semid = semget (ftok (device_name, 0), 2, IPC_CREAT | 0644)) < 0)
	{
	  syslog (LOG_ERR, "semget: %m");
	  return -1;
	}
      sem_un.array = sem_arr;

      if (semctl (semid, 0, SETALL, sem_un) < 0)
	{
	  syslog (LOG_ERR, "semctl init: %m");
	  return -1;
	}
*/
  fprintf (stderr, "%s\n", __FUNCTION__);
  fflush (stderr);
  init (device_name);
//      home (mount);
  return 0;
//     }
//    if (!mount)
//      init (device_name);

  return 0;
}

/* shut down the telescope driver */
extern void
telescope_done ()
{
//      semctl (semid, 1, IPC_RMID);
//      syslog (LOG_DEBUG, "para: telescope_done called %i", semid);
  fprintf (stderr, "%s\n", __FUNCTION__);
  fflush (stderr);
  commFree ();
  free (mount);
}

/* info request handlers */
extern int
telescope_base_info (struct telescope_info *info)
{
  fprintf (stderr, "%s\n", __FUNCTION__);
  fflush (stderr);
  strcpy (info->type, "PARAMOUNT_TC-300");
  strcpy (info->serial_number, "0007");
  info->park_dec = PARK_DEC;
  info->longtitude = LONGITUDE;
  info->latitude = LATITUDE;
  info->altitude = ALTITUDE;
  return 0;
}

extern int
telescope_info (struct telescope_info *info)
{
  long pos0, pos1;
  unsigned short stat0, stat1;
  double ra, dec;
  int noreply = 0, flip;

  fprintf (stderr, "%s\n", __FUNCTION__);
  fflush (stderr);
//    struct sembuf sem_buf;

  //printf("Getting the port to take the info\n"); fflush(stdout);
//    sem_buf.sem_num = SEM_TEL;
//    sem_buf.sem_op = -1;
//    sem_buf.sem_flg = SEM_UNDO;

//    semop (semid, &sem_buf, 1);

  //printf("Getting info\n"); fflush(stdout);

  //statuswatch(mount->axis0, mount->axis1);

  if (MKS_OK != MKS3StatusGet (mount->axis0, &stat0))
    noreply = 1;
  if (MKS_OK != MKS3StatusGet (mount->axis1, &stat1))
    noreply = 1;

  if (MKS_OK != MKS3PosCurGet (mount->axis0, &pos0))
    noreply = 1;
  if (MKS_OK != MKS3PosCurGet (mount->axis1, &pos1))
    noreply = 1;

  //printf("Info taken, releasing the port\n"); fflush(stdout);
//    sem_buf.sem_op = 1;

//    semop (semid, &sem_buf, 1);

  counts2sky (pos0, pos1, &ra, &dec, &flip);

  if (noreply)
    return -1;

  info->ra = ra;
  info->dec = dec;
  info->siderealtime = sid_time () / 15.0;
  info->localtime = 0;
  info->flip = flip;

  info->axis0_counts = pos0;
  info->axis1_counts = pos1;

  return 0;
}

/* GoTo driver call */
extern int
telescope_move_to (double ra, double dec)
{
  fprintf (stderr, "%s\n", __FUNCTION__);
  fflush (stderr);
  optwake ();
  move (ra, dec);
  return 0;
}

/* obsolete (used for meade repointing) */
extern int
telescope_set_to (double ra, double dec)
{
  fprintf (stderr, "%s\n", __FUNCTION__);
  fflush (stderr);
  return 0;
}

/* obsolete */
extern int
telescope_correct (double ra, double dec)
{
  fprintf (stderr, "%s\n", __FUNCTION__);
  fflush (stderr);
  return 0;
}

/* stop the telescope movement - do not really know why */
extern int
telescope_stop ()
{
  fprintf (stderr, "%s\n", __FUNCTION__);
  fflush (stderr);
  return 0;
}

extern int
telescope_park ()
{
  fprintf (stderr, "%s\n", __FUNCTION__);
  fflush (stderr);
  return zdechni ();
}

/* switch off the mount - switch off everything what's possible to be switched off by sw */
extern int
telescope_off ()
{
  fprintf (stderr, "%s\n", __FUNCTION__);
  fflush (stderr);
  return zdechni ();
}

extern int
telescope_change (double ra, double dec)
{
  return 0;
}

extern int
telescope_start_move (char a)
{
  return 0;
}

extern int
telescope_stop_move (char a)
{
  return 0;
}
