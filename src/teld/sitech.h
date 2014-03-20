/*  This is from John Kielkopf's xmtel files for sitech system with 
    telescope encoders 
    May or may not work with sitech systems without 'scope encoders 
    'lifted' from xmtel by <shashikiran.ganesh@gmail.com> - 2012 
*/
/* Telescope status files */


/* ---------------doubt------------------- 
#define FOCUSFILE "/usr/local/observatory/status/telfocus"
#define TEMPERATUREFILE "/usr/local/observatory/status/teltemperature"
#define ROTATEFILE "/usr/local/observatory/status/rotate"
#define MAXPATHLEN 100

----------------------------------------*/

/* Motor encoder counts per degree                */

/* Motor: Pittman GM9236S021                      */
/* Motor encoder: 500 cpr                         */
/* Motor multiplier: 4                            */
/* Motor gearbox: ~19.7:1                         */
/* Motor cog belt: 2:1                            */
/* Estimated motor RA:  28307692 counts/rev       */
/* Estimated motor Dec: 28307692 counts/rev       */

/* Motor RA  - Sidtech Y                          */

#define MOTORAZMCOUNTPERDEG   78632.47778

/* Motor Dec - Sidtech X                          */

#define MOTORALTCOUNTPERDEG -78632.47778      


/* Mount encoder counts per degree                */
/* Exact values will vary from mount to mount     */

/* Encoder: Renishaw RESM                         */
/* Diameter: 255 mm                               */
/* Reseau: 40 microns exactly                     */
/* Counts: 40000 full turn exactly                */
/* Signum multiplier: 400x interpolator           */
/* Effective reseau: 0.1 micron or 100 nm         */
/* Calculated scale: 44444.44444 counts/deg       */
/* Measured scale: ~40000 counts/deg              */

/* Mount encoders for Moore A200HR                */
/*                                                */
/* Planewave values                               */
/*   RA:  14357088 counts/rev                     */
/*   Dec: 14352882 counts/rev                     */
/*                                                */
/* Fitted values to motor encoder                 */
/*   RA:  14355467.5 counts/rev                   */
/*   Dec: 14352048.0 counts/rev                   */
/*                                                */
/* Fitted calibration                             */
/*   RA:    39876.298 counts/deg                  */
/*   Dec:  -39866.800 counts/deg                  */
/*                                                */
/* Working calibration                            */
/*   RA:    39836.064 counts/deg                  */
/*   Dec:  -39866.800 counts/deg                  */

/* Mount encoders for Mount Kent A200HR           */
/*                                                */
/* Fitted values                                  */
/*   RA:  14346318.5 counts/rev                   */
/*   Dec: 14360945.1 counts/rev                   */
/*                                                */
/* Fitted Calibration                             */
/*   RA:    39850.885 counts/deg                  */
/*   Dec:  -39891.514 counts/deg                  */
/*                                                */
/* Working calibration                            */
/*   RA:    39850.885 counts/deg                  */
/*   Dec:  -39891.514 counts/deg                  */


/* Mount RA  - Sidtech Y                          */

#define MOUNTAZMCOUNTPERDEG  39836.064 

/* Mount Dec - Sidtech X                          */

#define MOUNTALTCOUNTPERDEG  -39866.800

/* Sidereal rates                                 */

#define SIDEREALDAY 86164.0989

/* For Sitech controller and the A200HR           */
/*   rate = 33.5565796176 * counts/second         */
/*   where the sidereal counts/second is          */
/*   360. * counts/degree / sidereal day          */

#define AZMSIDEREALRATE   11024
#define ALTSIDEREALRATE   0  

/* UTC time correction */

#define LEAPSECONDS    35.0

/* Important constant                                                        */

#define PI             3.14159265358

/* The following parameters are used internally to set speed and direction.  */
/* Do not change these values.                                               */

/* Slew motion speeds                                                        */

#define	GUIDE		1
#define	CENTER		2
#define	FIND		4
#define	SLEW		8
 
/* Slew directions                                                           */

#define	NORTH		1
#define	SOUTH		2
#define	EAST		4
#define	WEST		8

/* Focus commands                                                            */
/* Distance from the CCD to the focal plane                                  */
/* Direction is + from the CCD toward the sky                                */
/* Used by Focus(focuscmd, focusspd)                                         */

#define FOCUSSPD4       8   /* Fast    */
#define FOCUSSPD3       4   /* Medium  */
#define FOCUSSPD2       2   /* Slow    */
#define FOCUSSPD1       1   /* Precise */

#define FOCUSCMDOUT     1   /* CCD moves away from sky */
#define FOCUSCMDOFF     0   /* CCD does not move */
#define FOCUSCMDIN     -1   /* CCD moves toward the  sky */


/* Rotator commands                                                          */

#define ROTATESPDFAST       3   /* Fast set */
#define ROTATESPDSLOW       2   /* Slow set */
#define ROTATESPDSIDEREAL   1   /* Sidereal rate */
#define ROTATECMDCW         1   /* Camera rotates CW looking toward the sky */
#define ROTATECMDOFF        0   /* Camera does not rotate */
#define ROTATECMDCCW       -1   /* Camera rotates CCW looking toward the sky */


/* Fan commands                                                              */

#define FANCMDHIGH      2   /* Cooling fan on high speed */
#define FANCMDLOW       1   /* Cooling fan on low speed */
#define FANCMDOFF       0   /* Cooling fan off */


/* Dew heater commands                                                       */

#define HEATERCMDHIGH      2   /* dew heater on high */
#define HEATERCMDLOW       1   /* dew heater on low */
#define HEATERCMDOFF       0   /* dew heater off */


/* Pointing models (bit mapped and additive)                                 */

#define RAW       0      /* Unnmodified but assumed zero corrected */
#define OFFSET    1      /* Correct for target offset*/
#define REFRACT   2      /* Correct for atmospheric refraction */
#define POLAR     4      /* Correct for polar axis misalignment */ 
#define DYNAMIC   8      /* Correct for errors in real time */


/* Display epochs (selected with GUI button)                                 */

#define J2000    0      /* Epoch 2000.0 as entered or precessed without pm */
#define EOD      1      /* Epoch of date with pm if available  (default) */


/* Mounts                                                                    */

#define ALTAZ  0        /* Alt-azimuth mount with at least 2-axis tracking */ 
#define EQFORK 1        /* Equatorial fork */
#define GEM    2        /* German equatorial */


/* Slew tolerances in arcseconds */

#define SLEWTOLRA  60.      /* 60 arcseconds */
#define SLEWTOLDEC 60.      /* 60 arcseconds */
#define SLEWFAST   0         /* Set to 1 for fast slew with caution !! */


/* Slew software limits  */

#define MINTARGETALT   10.   /* Minimum target altitude in degrees */


/* Defaults for site may be modified by prefs file */

#define LONGITUDE      85.5300
#define LATITUDE       38.3334
#define ALTITUDE      230.0000
#define TEMPERATURE    20.0
#define PRESSURE      760.0
#define PARKRA          0
#define PARKDEC         89.999
