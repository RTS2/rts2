#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <libnova.h>
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

#define PARK_DEC 90		//LATITUDE

#define DEC_CPD -20921.9688	// model result
//#define DEC_CPD -20880.0      // DOUBLE - how many counts per degree... seems to be fixed and nice
//#define DEC_ZERO -4286                // LONG - PUVODNI

// Measured: 2.0791667
#define DEC_ZERO -43413		// LONG - where is the homing mark. Not sure, needs precising

#define HA_CPD -32195.52	// model result
//#define HA_CPD -32000.0               // 8.9 counts per arcsecond
//#define HA_CPD -32040.0               // 8.9 counts per arcsecond
//#define HA_ZERO 963615                // AFTERNOON ZERO  PUVODNI
//#define HA_ZERO 949866                // AFTERNOON ZERO (The other is HA_ZERO + HA_CPD * 360)

// Measured: 29.9039
#define HA_ZERO 958121		// AFTERNOON ZERO (The other is HA_ZERO + HA_CPD * 360)

//double get_julian_from_sys();

//#define DUMMY

typedef struct
{
  MKS3Id axis0, axis1;
  double lastra, lastdec;
}
T9;

T9 *mount = NULL;

double stime_home;

int semid;

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

//! one semaphore for direct port control..
#define SEM_TEL 	0
//! and second for control of the move operations
#define SEM_MOVE 	1

void
in180 (double *x)
{
  for (; *x > 180; *x -= 360);
  for (; *x < -180; *x += 360);
}

void
in360 (double *x)
{
  for (; *x > 360; *x -= 360);
  for (; *x < 0; *x += 360);
}

// return sid time since homing
double
sid_home (MKS3Id axis0)
{
  long pos0, en0;
  double ret;

  MKS3PosEncoderGet (axis0, &en0);
  MKS3PosCurGet (axis0, &pos0);

  //    s = sid_time() - stime_home;
  //        stime_home = sid_time() - sid_home(axis0);
  ret = (double) (pos0 - en0) / 32000.0;

  printf ("sid_home() = %f; ", ret);
  return ret;
}

// return siderial time in degrees
double
sid_time ()
{
  double s;

  s = 15 * get_apparent_sidereal_time (get_julian_from_sys ()) - LONGITUDE;
  printf ("sid_time() = %f ", s);

  in360 (&s);

  return s;
}

double
get_refraction (double altitude, double atm_pres, double temp)
{
  double R;

  //altitude = deg_to_rad (altitude);

  /* eq. 5.27 (Telescope Control,M.Trueblood & R.M. Genet) */
  R = 1.02 /
    tan (deg_to_rad (altitude + 10.3 / (altitude + 5.11) + 0.0019279));

  // for gnuplot (in degrees)
  // R(a) = 1.02 / tan( (a + 10.3 / (a + 5.11) + 0.0019279) / 57.2958 ) / 60

  /* take into account of atm press and temp */
  R *= ((atm_pres / 1010) * (283 / (273 + temp)));

  /* convert from arcminutes to degrees */
  R /= 60.0;

  return (R);
}

void
ha_dec (struct ln_equ_posn *mean_position,
	struct ln_equ_posn *app_position, double JD, double atm_pres,
	double temp, struct ln_lnlat_posn *observer)
{
  // struct ln_equ_posn *mean_position     ==> The mean equatorial position in the catalog (libnova)
  // struct ha_dec_pos  *apparent_position ==> The apparent equatorial position (libnova)
  // double JD                             ==> Julian date
  // double atm_pres                       ==> Atmospheric pressure in mbar
  // double temp                           ==> Temperature in ºC
  // struct ln_lnlat_posn *observer        ==> Observer coordinates

  struct ln_equ_posn proper_motion;
  struct ln_hrz_posn h_app_position;
//  struct ha_dec_pos hd_app_position;
  double refraction;

  printf
    ("\n\033[31mInside the conversion function:\n===============================\033[m\n");
  // Transform from catalogue position (J2000 ???) to apparent position
  proper_motion.ra = 0;
  proper_motion.dec = 0;
  get_apparent_posn (mean_position, &proper_motion, JD, app_position);	// Second argument should be proper motion but we ignore it
  printf ("\033[34mcat: %f %f\033[m\n", mean_position->ra,
	  mean_position->dec);
  printf ("\033[34mapp: %f %f\033[m\n", app_position->ra, app_position->dec);
  // Transform to horizontal coordinates
  get_hrz_from_equ (app_position, observer, JD, &h_app_position);
  printf ("\033[34mhor: %f %f\033[m\n", h_app_position.az,
	  h_app_position.alt);
  // Correction for refraction
  refraction = get_refraction (h_app_position.alt, atm_pres, temp);
  h_app_position.alt = h_app_position.alt + refraction;
  refraction *= 60;
  if (refraction > 2)
    printf ("\033[34mThe atmospheric refraction is: \033[m%f\'\n",
	    refraction);
  else
    printf ("\033[34mThe atmospheric refraction is: \033[m%f\"\n",
	    refraction * 60);
  // Convertion to equatorial
  get_equ_from_hrz (&h_app_position, observer, JD, app_position);
  printf ("\033[34mcor: %f %f\033[m\n", app_position->ra, app_position->dec);

  // Convertion to ha_dec
  //get_hd_from_equ (&app_position, observer, JD,apparent_position);



}				// ha_dec

void
astrometric_corr (double *ra, double *dec, double atm_pres, double temp)
{
  double JD;
  struct ln_lnlat_posn observer;	//==> Observer coordinates
  struct ln_equ_posn mean_position;	//==> The mean equatorial position in the catalog (libnova)
  struct ln_equ_posn app_position;	//==> The apparent equatorial position (libnova)

  JD = get_julian_from_sys ();

  observer.lat = 37.1;
  observer.lng = +6.732778;

  mean_position.ra = *ra;
  mean_position.dec = *dec;

  ha_dec (&mean_position, &app_position, JD, atm_pres, temp, &observer);

  *ra = app_position.ra;
  *dec = app_position.dec;
}

void
counts2sky (long ac, long dc, double *ra, double *dec, int *flip)
{
  double _ra, _dec;

  // Base transform to raw RA, DEC    
  _dec = (double) (dc - DEC_ZERO) / DEC_CPD;
  _ra = (double) (ac - HA_ZERO) / HA_CPD;
  _ra = -_ra + stime_home;

  // Flip
  if (_dec > 90)
    {
      *flip = 1;
      _dec = 180 - _dec;
      _ra = 180 + _ra;
    }
  else
    *flip = 0;

  in180 (&_dec);
  in360 (&_ra);

  *dec = _dec;
  *ra = _ra;
}

void
sky2counts (double ra, double dec, long *ac, long *dc)
{
  long _dc, _ac;
  double ha;

//    printf("---\n");
//    astrometric_corr(&ra, &dec, 1013.6, 15.0);
  printf ("---\n");

  // True Hour angle
  ha = sid_time () - ra;

  printf ("Ha: %f ", ha);
  in180 (&ha);

  ra = stime_home - ra;
  in180 (&ra);

  printf ("(ra: %f) ", ra);

  if (ha < 0)
    {
      dec = 180 - dec;
      ra = 180 + ra;

      printf ("FLIP ");
    }
  else
    printf ("NEFLIP ");


  printf ("ra: %f ", ra);

  _dc = DEC_ZERO + (long) (DEC_CPD * dec);
  _ac = HA_ZERO + (long) (HA_CPD * ra);
  printf ("ac: %ld ", _ac);

  *dc = _dc;
  *ac = _ac;
}

void
display_status (long status, int color)
{
  if (status & MOTOR_HOMING)
    printf ("\033[%dmHOMING\033[m ", 30 + color);
  //if(status&MOTOR_SERVO)printf("\033[31mMOTOR_SERVO\033[m ");
  if (status & MOTOR_INDEXING)
    printf ("\033[%dmIDX\033[m ", 30 + color);
  if (status & MOTOR_SLEWING)
    printf ("\033[%dmSLEW\033[m ", 30 + color);
  if (!(status & MOTOR_HOMED))
    printf ("\033[%dmNOT_HOMED\033[m ", 30 + color);
  if (status & MOTOR_JOYSTICKING)
    printf ("\033[%dmJOY\033[m ", 30 + color);
  if (status & MOTOR_OFF)
    printf ("\033[%dmOFF\033[m ", 30 + color);
}

void
statuswatch (MKS3Id axis0, MKS3Id axis1)
{
  long pos0, pos1;
  unsigned short stat0, stat1;
  //double ra, dec;
  //int flip;

  struct sembuf sem_buf;

  sem_buf.sem_num = SEM_TEL;
  sem_buf.sem_op = -1;
  sem_buf.sem_flg = SEM_UNDO;

  semop (semid, &sem_buf, 1);

  MKS3StatusGet (axis0, &stat0);
  MKS3StatusGet (axis1, &stat1);

  MKS3PosCurGet (axis0, &pos0);
  MKS3PosCurGet (axis1, &pos1);

  sem_buf.sem_op = 1;

  semop (semid, &sem_buf, 1);

  //counts2sky(pos0, pos1, &ra, &dec, &flip);

  // printf("\033[31mPos: %ld (%fd)\033[m ", pos0, ra);
  // printf("\033[32mPos: %ld (%fd)\033[m ", pos1, dec);

  // display_status(stat0, 1);
  // display_status(stat1, 2);


  // printf("                      \r");
  // fflush(stdout);
}

void
move (T9 * mount, double ra, double dec)
{
  long pos0, pos1;
  long ac, dc;
  unsigned short stat0, stat1;
  struct sembuf sem_buf;

  sky2counts (ra, dec, &ac, &dc);
  //counts2sky(ac, dc, &ra, &dec);
  //printf("ra: %f, dec: %f;\n", ra, dec);

  mount->lastra = ra;
  mount->lastdec = dec;

  sem_buf.sem_num = SEM_MOVE;
  sem_buf.sem_op = -1;
  sem_buf.sem_flg = SEM_UNDO;

  if (semop (semid, &sem_buf, 1) < 0)
    return;

  MKS3PosTargetSet (mount->axis1, dc);
  MKS3PosTargetSet (mount->axis0, ac);

  for (;;)
    {
      struct sembuf sem_buf_write;

      sem_buf_write.sem_num = SEM_TEL;
      sem_buf_write.sem_op = -1;
      sem_buf_write.sem_flg = SEM_UNDO;

      if (semop (semid, &sem_buf_write, 1) < 0)
	break;

      MKS3StatusGet (mount->axis0, &stat0);
      MKS3StatusGet (mount->axis1, &stat1);
      MKS3PosCurGet (mount->axis0, &pos0);
      MKS3PosCurGet (mount->axis1, &pos1);

      sem_buf_write.sem_op = 1;

      semop (semid, &sem_buf_write, 1);

      //counts2sky(pos0, pos1, &ra, &dec, &flip);

//      printf("\033[31mPos: %ld (%fd)\033[m ", pos0, ra);
//      printf("\033[32mPos: %ld (%fd)\033[m ", pos1, dec);
//      printf("                      \r");
//      fflush(stdout);

      if ((stat1 & MOTOR_SLEWING) || (stat0 & MOTOR_SLEWING))
	usleep (100000);
      else
	break;
    }
  sem_buf.sem_op = 1;
  semop (semid, &sem_buf, 1);
//    printf("\n");
}

T9 *
init (char *device)
{
  unsigned short stat0, stat1;
  T9 *mount;
  int i, q;
  time_t t1;

  mount = (T9 *) malloc (sizeof (T9));

  mount->axis0.unitId = 0x64;
  mount->axis1.unitId = 0x64;

  mount->axis0.axisId = 0;
  mount->axis1.axisId = 1;

// A co kdyz je teleskop vypnuty! - MKS3Init vraci MKS_OK, jinak kod chyby...

  MKS3Init (device);

  MKS3StatusGet (mount->axis0, &stat0);
  if (!(stat0 & MOTOR_HOMED))
    MKS3Home (mount->axis0, 0);

  MKS3StatusGet (mount->axis1, &stat1);
  if (!(stat1 & MOTOR_HOMED))
    MKS3Home (mount->axis1, 0);

  for (i = 0, q = 1;; i++)
    {
      statuswatch (mount->axis0, mount->axis1);

      MKS3StatusGet (mount->axis1, &stat1);
      MKS3StatusGet (mount->axis0, &stat0);

      if ((stat0 & MOTOR_HOMED) && (stat1 & MOTOR_HOMED))
	break;
      usleep (100000);
    }

  // Wait for full second    
  printf ("Waiting for tick...");
  fflush (stdout);
  for (t1 = time (NULL); t1 == time (NULL););
  printf ("got it\n");
  stime_home = sid_time () - sid_home (mount->axis0);
  printf ("stime_home=%f\n", stime_home);
  fflush (stdout);

  return mount;
}

/*
void
home (T9 *mount)
{
    unsigned short stat0, stat1;
    int i, q;
    struct sembuf sem_buf;

    //    if (!(stat0 & MOTOR_HOMED))
    MKS3Home(mount->axis0, 0);

    //    if (!(stat1 & MOTOR_HOMED))
    MKS3Home(mount->axis1, 0);

	for (i = 0, q = 1;; i++) {
		statuswatch(mount->axis0, mount->axis1);

		sem_buf.sem_num = SEM_TEL;
		sem_buf.sem_op = -1;
		sem_buf.sem_flg = SEM_UNDO;

		if (semop (semid, &sem_buf, 1))
			continue;

		MKS3StatusGet(mount->axis1, &stat1);
		MKS3StatusGet(mount->axis0, &stat0);

		sem_buf.sem_op = 1;
		semop (semid, &sem_buf, 1); 

		if ((stat0 & MOTOR_HOMED) && (q)) {
		    q = 0;
		    stime_home = sid_time();
		}
	
		if ((stat0 & MOTOR_HOMED) && (stat1 & MOTOR_HOMED))
		    break;
		usleep(100000);
	    }
}
*/

extern int
telescope_init (const char *device_name, int telescope_id)
{
  union semun sem_un;
  unsigned short int sem_arr[] = { 1, 1 };

  if ((semid = semget (ftok (device_name, 0), 2, 0644)) < 0)
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

      mount = init ("/dev/ttyS0");
//      home (mount);
      return 0;
    }
  if (!mount)
    mount = init ("/dev/ttyS0");

  return 0;
}

extern void
telescope_done ()
{
  semctl (semid, 1, IPC_RMID);
  syslog (LOG_DEBUG, "para: telescope_done called %i", semid);
  printf ("done..?\n");
  fflush (stdout);
  free (mount);
}

extern int
telescope_base_info (struct telescope_info *info)
{
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

  struct sembuf sem_buf;

  sem_buf.sem_num = SEM_TEL;
  sem_buf.sem_op = -1;
  sem_buf.sem_flg = SEM_UNDO;

  semop (semid, &sem_buf, 1);

  if (MKS_OK != MKS3StatusGet (mount->axis0, &stat0))
    noreply = 1;
  if (MKS_OK != MKS3StatusGet (mount->axis1, &stat1))
    noreply = 1;

  if (MKS_OK != MKS3PosCurGet (mount->axis0, &pos0))
    noreply = 1;
  if (MKS_OK != MKS3PosCurGet (mount->axis1, &pos1))
    noreply = 1;

  sem_buf.sem_op = 1;

  semop (semid, &sem_buf, 1);

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

extern int
telescope_move_to (double ra, double dec)
{
  move (mount, ra, dec);
  return 0;
}

extern int
telescope_set_to (double ra, double dec)
{
  return 0;
}

extern int
telescope_correct (double ra, double dec)
{
  return 0;
}

extern int
telescope_stop ()
{

  return 0;
}

extern int
telescope_start_move (char direction)
{
  return 0;
}

extern int
telescope_stop_move (char direction)
{
  return 0;
}

extern int
telescope_park ()
{
  move (mount, sid_time () - 30, PARK_DEC);
  return 0;
}

extern int
telescope_off ()
{
  return 0;
}

/*    long i;

    mount = init("/dev/ttyS0");

    ////homingjd = get_julian_from_sys();

    //move(mount, 81.600792, 7.1465722); // GRB 030509 Center
//    move(mount, 81.6460, 7.1492); // GRB 030509 Center

    for(;1<0;)
    {
	    double ra, ram, ras, dec, dem, des;

    printf("\ncoords, [hh mm ss dd mm ss]:");
    scanf("%lf %lf %lf %lf %lf %lf", &ra, &ram, &ras, &dec, &dem, &des );
    printf("%f %f %f %f %f %f\n", ra, ram, ras, dec, dem, des );

    ra=15*(ra+ram/60+ras/3600);
    dec=dec+dem/60+des/3600;

    printf("move to %f %f\n", ra, dec);
    
    move(mount, ra, dec);

    for (i = 0;i<60; i++) {
	sid_time();
	statuswatch(mount->axis0, mount->axis1);
	usleep(100000);
    }
    
    }
    
    
   move(mount, 213.9153, 19.18241); // Arcturus
   getchar();
   move(mount, 148.9, 69.0666);	// M81
    

    sleep(400);
    move(mount, 208.67116, 18.397717);	// Mufrid
    move(mount, 177.26491, 14.890283); // Denebola
    move(mount, 152.09296, 11.967207); // Regulus
    move(mount, 213.9153, 19.18241); // Arcturus
    move(mount, 114.82549, 5.224993); // Procyon
    move(mount, 279.23474, 38.783692); // Vega

    for (i = 0;; i++) {
	sid_time();
	statuswatch(mount->axis0, mount->axis1);
	usleep(100000);
    }

    return 0;
} */
