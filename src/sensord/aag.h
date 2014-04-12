#if 0
    AAG Cloud watcher driver
    Copyright (C) 2009 Markus Wildi, markus.wildi@one-arcsec.org

    For further details visit
    https://azug.minpet.unibas.ch/wikiobsvermes/index.php/Main_Page

    The original software for MS windows was developed by 
    António Alberto  Peres Gomes (AAG):
    http://www.aagware.eu/aag_cloudwatcher.htm 

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


#endif
#define OPT_AAG_DEVICE             OPT_LOCAL + 53
#define OPT_AAG_DRY_TRIGGER        OPT_LOCAL + 54
#define OPT_AAG_WET_TRIGGER        OPT_LOCAL + 55
#define OPT_AAG_CLEAR_TRIGGER      OPT_LOCAL + 56
#define OPT_AAG_CLOUD_TRIGGER      OPT_LOCAL + 57
#define OPT_AAG_NO_SNOW_TRIGGER    OPT_LOCAL + 58

enum AAGC_SETTINGS { IMP_DELTA, IMP_MIN, IMP_DURATION_WET, IMP_DURATION_RAIN, IMP_CYCLE_WET, IMP_CYCLE_RAIN } ;

/* rts2 specific constants */
#define AAG_WEATHER_TIMEOUT 60.      /* 60 seconds */
#define AAG_WEATHER_TIMEOUT_BAD 300. /* 300 seconds */
#define AAG_POLLING_TIME 10.        /* seconds */

/* device constants */
#define AAGC_TIMEOUT 1
#define BLOCK_LENGTH 15
#define ZENER_CONSTANT 3.         /* reading from Antonio's software:    3 */
#define LDRPULLUP_RESISTANCE 56.  /* reading from Antonio's software:   56 */
#define RAINPULLUP_RESISTANCE 1.  /* reading from Antonio's software:    1 */
#define RAIN_RES_AT_25 1.         /* reading from Antonio's software:    1 */
#define RAIN_BETA 3450.           /* reading from Antonio's software: 3450 */
#define ABSZERO 273.15
#define NUMBER_ELECTRICAL_CONSTANTS 6

/* Operation */
#define REAL_RAIN_TIMEOUT    60 /* [sec] period after a weather is set to bad due to (real) rain*/
#define MAX_RAIN_SENSOR_TEMPERATURE 40. /* deg C, above this rain sensor temperature no heating occurs */ 
#define MAX_OPERATING_TEMPERATURE 30.   /* deg C, above this ambient temperature no heating occurs */ 
#define HEAT  1.                        /* [0.,1.], the heat amount */
#define HEAT_FACTOR_DROP_WHILE_DRY .99  /* [0.,1.], if the rain sensor is dry, sometimes temperature suddenly drops*/
#define THRESHOLD_DROP_WHILE_DRY -10.
#define THRESHOLD_MAX  2065.            /* arb. units, do not cook */
#define THRESHOLD_DRY  2025.           
#define THRESHOLD_WET  1960.
#define IS_MAX  1
#define IS_DRY  2
#define IS_WET  3  
#define IS_RAIN 4
#define HEAT_REGULAR 0
#define HEAT_MAX 1
#define HEAT_MIN 2
#define HEAT_NOT 3
#define HEAT_DROP_WHILE_DRY 4

/* define the clound states, not yet implemented */

#define THRESHOLD_CLEAR  -25. /* deg C */
#define THRESHOLD_CLOUD  -12. /* deg C */
#define THRESHOLD_NO_SNOW 5. /* deg C, if snow lies on the detector and ambient temperature (tempIRSensor) is below THRESHOLD_CLOUDY it is considered as bad weather (to be on the safe side)*/
#define IS_CLEAR  1
#define IS_CLOUDY  2  
#define IS_OVERCAST 3

/* define the brightness states, not yet implemented */
#define THRESHOLD_NIGHT 3500. /* arb. units */
#define THRESHOLD_DAWN 6.     /* arb. units */
#define IS_NIGHT 1
#define IS_DAWN 2
#define IS_DAY 3

/* Relay status, not yet implemented */
#define IS_OPEN 1
#define IS_CLOSED 2
#define IS_UNDEFINED -1

/* Constants which will be later configurable e.g. via rts2-mon*/
/* sky temperature correction model */
#define COR_0  33.
#define COR_1   0.
#define COR_2   4.
#define COR_3 100. 
#define COR_4 100.
