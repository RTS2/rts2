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

enum AAGC_SETTINGS { IMP_DELTA, IMP_MIN, IMP_DURATION_WET, IMP_DURATION_RAIN, IMP_CYCLE_WET, IMP_CYCLE_RAIN } ;

/* rts2 specific constants */
#define WEATHER_TIMEOUT 60
#define WEATHER_TIMEOUT_BAD 120
#define POLLING_TIME 10.

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
#define MAX_RAIN_SENSOR_TEMPERATURE 40. /* above this rain sensor temperature no heating occurs */ 
#define MAX_OPERATING_TEMPERATURE 30. /* above this ambient temperature no heating occurs */ 
#define HEAT  1.               /* the heat amount [0.,1.] */
#define THRESHOLD_DRY_OUT -3.   /* the increase of the rain_frequency required*/
				/* for further heating in state IS_RAIN*/
                                /* It should only avoid heating if rain frequency really drops*/

#define TEMPERATURE_DIFFFERENCE_AT_THRESHOLD_DRY 10.
#define THRESHOLD_DROP_WHILE_DRY -10.
#define THRESHOLD_MAX  2070.   /* do not cook */
#define THRESHOLD_DRY  2030.  
#define THRESHOLD_WET  1900.
#define IS_MAX  1
#define IS_DRY  2
#define IS_WET  3  
#define IS_RAIN 4
#define HEAT_REGULAR 0
#define HEAT_MAX 1
#define HEAT_MIN 2
#define HEAT_NOT 3

/* define the clound states, not yet implemented */

#define THRESHOLD_CLEAR -25.
#define THRESHOLD_CLOUDY -5.
#define IS_CLEAR  1
#define IS_CLOUDY  2  
#define IS_OVERCAST 3

/* define the brightness states, not yet implemented */
#define THRESHOLD_NIGHT 3500.
#define THRESHOLD_DAWN 6.
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

/* Heating characteristics */
#define THRESHOLD_0  0. 
#define THRESHOLD_1 20.

#define DELTA_0 6.
#define DELTA_1 4.

#define IMPULSE_0  10.
#define IMPULSE_1  10./100.
#define IMPULSE_2  59.
#define IMPULSE_3  60.
#define IMPULSE_4  61.
#define IMPULSE_5 601.
