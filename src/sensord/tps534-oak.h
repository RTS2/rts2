/*
 *
 * Copyright (C) 2010 Markus Wildi, version for RTS2, minor adaptions
 * Copyright (C) 2009 Lukas Zimmermann, Basel, Switzerland
 *
 *
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Or visit http://www.gnu.org/licenses/gpl.html.
 *
 */
// Default device, if not specified as an argument

#define TPS534_THRESHOLD_CLOUDY -10.
#define TPS534_POLLING_TIME    1.      /* seconds */
#define TPS534_WEATHER_TIMEOUT 60.      /* seconds */
#define TPS534_WEATHER_TIMEOUT_BAD 300. /* seconds */
#define OAK_CONNECTION_FAILED 1
/** \brief the data read from Oak ADC 
*/
typedef struct _tps534_stat {
  double analogIn[5] ;
  double calcIn[4] ;
} tps534_state;

/** \brief Sets up the connection to the oak tps534 device 
*/
#ifdef __cplusplus
extern "C"
{
#endif
  int connectDevice( char *device_file, int connecting);
#ifdef __cplusplus
}
#endif
