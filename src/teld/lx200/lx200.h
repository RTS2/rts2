/* @file Header file for LX200 telescope driver
* $Id$
* @author petr 
*/

#ifndef __TELD_LX200__
#define __TELD_LX200__

#define RATE_SLEW	'S'
#define RATE_FIND	'M'
#define RATE_CENTER	'C'
#define RATE_GUIDE	'G'
#define DIR_NORTH	'n'
#define DIR_EAST	'e'
#define DIR_SOUTH	's'
#define DIR_WEST	'w'
#define PORT_TIMEOUT	5

// basic functions
int tel_connect (const char *devptr);
int tel_is_ready (void);
int tel_set_to (double ra, double dec);
int tel_move_to (double ra, double dec);
int tel_read_dec (double *decptr);
int tel_read_ra (double *raptr);
int tel_park (void);

// extended functions
int tel_read_longtitude (double *tptr);
int tel_read_latitude (double *tptr);
int tel_read_siderealtime (double *tptr);
int tel_read_localtime (double *tptr);
int tel_disconnect (void);

// slew functions
int tel_stop_slew (char direction);
int tel_start_slew (char direction);
int tel_set_rate (char new_rate);

extern pthread_mutex_t mov_mutex;
extern pthread_mutex_t tel_mutex;
extern double park_dec;

#endif // __TELD_LX200__
