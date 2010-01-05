#if 0
   Library to calculate dome target AZ and ZD 
   Copyright (C) 2006-2010, Markus Wildi

   The transformations are based on the paper Matrix Method for 
   Coodinates Transformation written by Toshimi Taki 
   (http://www.asahi-net.or.jp/~zs3t-tk). 

   Documentation:
   https://azug.minpet.unibas.ch/wikiobsvermes/index.php/Robotic_ObsVermes#Telescope

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
   Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301  USA
#endif

/* Standard headers */

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <libnova/libnova.h>

#define Csc(x) (1./sin(x))
#define Sec(x) (1./cos(x))

#define IS_NOT_EAST 2 
#define IS_EAST 1
#define IS_WEST -1
#define IS_NOT_WEST -2
#define BELOW -1


double LDRAtoHA( double RA, double longitude) ;
double LDHAtoRA( double HA, double longitude) ;
int    LDRAtoDomeAZ( double longitude, double latitude, double RA, double dec, double *Az, double *ZD) ;
int    LDRAtoStarAZ( double longitude, double latitude, double RA, double dec, double *Az, double *ZD) ;
int    LDHAtoDomeAZ( double latitude, double HA, double dec, double *Az, double *ZD) ;
int    LDHAtoStarAZ( double latitude, double HA, double dec, double *Az, double *ZD) ;
void   LDUUpdateDomeSetpoint( double interval, char *msg) ;
void   LDSetMovementTypeOnSlew( char *msg) ;
void   LDSetMovementTypeOnTrack( char *msg) ;
void   LDSetMovementTypeOnZero( char *msg) ;
void   LDSetMovementTypeOnTrackMode( int switchnr, char *msg) ;
void   LDSetMovementTypeOnEvent( char *msg) ;
void   LDUpdateMovementType() ;
void   ISUpdateDomeSetpoint() ;
double LDStarOnDomeTEX( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd) ;
double LDStarOnDomeTWX( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd) ;
double LDStarOnDomeTEY( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd) ;
double LDStarOnDomeTWY( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd) ;
double LDStarOnDomeZ( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd) ;
int    LDCheckHorizon( double HA, double dec, double phi) ;


    struct Decaxis_plus_minus {
      int east ;
      int west ;

    } dapm ;

/* static ISwitch CollisionS[] = {   */
/*   {"COLLIDING", "colliding", ISS_OFF, 0, 0},                          */
/*   {"NOW", "now", ISS_OFF, 0, 0}, */
/* }; */

/* Stars equatorial coordinates in the dome azimutal system */
/* INumber StarHAN[] = { */
/*     {"HA",  "HA  H:M:S", "%10.6m",  0.,  24., 0., 19., 0, 0, 0}, */
/*     {"Dec", "Dec D:M:S", "%10.6m", -90., 90., 0., 60., 0, 0, 0}, */
/* }; */

/* INumberVectorProperty StarHANP = { */
/*     mydev, "HOURANGLE", "Star s Hour Angle", CONTROL_GROUP, IP_RW, 0, IPS_IDLE, StarHAN, NARRAY(StarHAN), "", 0}; */

/* INumber StarAZN[] = { */
/*     {"Az",  "Az  D:M:S", "%10.6m",  0., 360., 0., 0., 0, 0, 0}, */
/*     {"ZD",  "ZD D:M:S", "%10.6m", 0., 90., 0., 0., 0, 0, 0}, */
/* }; */

/* INumberVectorProperty StarAZNP = { */
/*     mydev, "STARAZIMUT", "Star s Azimut", CONTROL_GROUP, IP_RO, 0, IPS_IDLE, StarAZN, NARRAY(StarAZN), "", 0}; */

/* INumber DomeAZSetPointN[] = { */
/*     {"Az",  "Az  D:M:S", "%10.6m",  0., 360., 0., 0., 0, 0, 0}, */
/*     {"ZD",  "ZD D:M:S", "%10.6m", 0., 90., 0., 0., 0, 0, 0}, */
/* }; */

/* INumberVectorProperty DomeAZSetPointNP = { */
/*     mydev, "DOMEAZIMUTSETPOINT", "Dome Azimut Set Point", CONTROL_GROUP, IP_RW, 0, IPS_IDLE, DomeAZSetPointN, NARRAY(DomeAZSetPointN), "", 0}; */


/* Observatory parameters  */
/* the origin is the dome center, pos. x is South, y East and z zenith. */

/* static ITextVectorProperty OriginTP= {  */
/*   mydev, "ORIGIN", "Toshimi Taki", OBSERVATORY_GROUP, IP_RO, ISR_1OFMANY, IPS_IDLE, OriginT, NARRAY(OriginT), "", 0 */
/* } ; */

/* static INumber DomeRadiusN[] = { */
/*     { "DomeRadius", "radius [m]", "%6.3f", 0., 100.0, 0., 1.265, 0, 0, 0},}; */


    double DomeRadius ;

/* static INumber PierN[] = { */
/*      { "RADIUS",   "radius [m]", "%6.3f", 0., 100.0, 0., 0.135, 0, 0, 0}, */
/*      { "WEDGE",   "wedge [m]", "%6.3f", -100., 100.0, 0., -0.6684, 0, 0, 0}, */
/*      { "FLOOR",   "floor [m]", "%6.3f", -100., 100.0, 0., -1.9534, 0, 0, 0}, */
/*      { "DANGERBELOW",   "Danger zone below [m]", "%6.3f", -100., 100.0, 0., -2.9534, 0, 0, 0}, */
/*      { "DANGERABOVE",   "Danger zone above [m]", "%6.3f", -100., 100.0, 0., -0.1, 0, 0, 0}, */


    struct pier {
      double radius ;
      double wedge ;
      double floor ;
      double danger_zone_above ;
      double danger_zone_below ;
    }  pr;

/* static INumber MountingN[] = { */
/*      { "xd",   "xd [m]", "%6.3f", -100., 100.0, 0., -0.0684, 0, 0, 0}, */
/*      { "zd",   "zd [m]", "%6.3f", -100., 100.0, 0., -0.1934, 0, 0, 0}, */
/*      { "rdec", "rdec [m]", "%6.3f", -100., 100.0, 0., 0.338, 0, 0, 0},}; */

    struct mount {
      double xd ;
      double zd ;
      double rdec ;

     } mt ;
/* static INumber TelescopeN[] = { */
/*      { "RADIUS",   "radius [m]", "%6.3f", 0., 100.0, 0., 0.123, 0, 0, 0}, */
/*      { "LENGTH",   "rear length [m]", "%6.3f", 0., 100.0, 0., 0.8, 0, 0, 0}, */

/* }; */


    struct telescope {
      double radius ;
      double rear_length ;
      
    } tel ;

    /* the main entry function */
    double dome_target_az( struct ln_equ_posn tel_eq)
    {
      double ret ;
      double target_az ;
      double target_ZD ;
      double longitude= 7.5 ; //wildi ToDo value,  positive to the East
      double latitude= 47.2 ; //wildi ToDo

      dapm.east= IS_EAST ;
      dapm.west= IS_NOT_WEST ;

      DomeRadius= 1.3 ;

      pr.radius= 0.135;
      pr.wedge= -0.6684;
      pr.floor= -1.9534;
      pr.danger_zone_above= -2.9534;
      pr.danger_zone_below= -0.1;

      mt.xd= -0.0684;
      mt.zd= -0.1934;
      mt.rdec= 0.338;

      tel.radius= 0.123;
      tel.rear_length= 0.8;


      ret= LDRAtoDomeAZ( longitude, latitude, tel_eq.ra, tel_eq.dec, &target_az, &target_ZD) ;

      return target_az ;
    }

/* The equation 5.6.1-1 os = ted . (pqe + k  qse ) + opd */
/* was solved for os=( x, y, sqrt( Rdome^2- x^2 -y^2)). The */
/* matrix ted is the transformation from the equatorial system to */
/* azimuth system, os is the vector from the center of the dome */
/* to the star, pqe is the declination axis in the equatorial */
/* system and gse the direction vector of the optical axis pointing */
/* to the star in the same system and opd is the offset of the dome */
/* center to the intersection point of the polar and the declination */
/* axis.       */
/* The eq. 5.6.5-1 might be wrong, because the vectors */
/* PS and PQ are not colinear. */
/* The parameter k has a value which is similar to Rdome. */
/* Therefore the dome Az might acceptable */
/* I solved the equations with Mathematica and hope that the telescope hits always the middle*/


/* There are expressions like sqrt(pow(Rdec,2)). I know that that I should */
/* replace them with Rdec or better +/- Rdec. But for the sake of simplicity I */
/* copied the whole expressions from the Mathematica notebook to the place here */
/* and made only the minimum of replacements.*/
/* I will change that, when the code seems to be correct */

double LDStarOnDomeTEX( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd)
{
    double correct ;

    correct=  xd*pow(cos(HA),2)*pow(cos(dec),2)*pow(cos(phi),2) + xd*pow(cos(dec),2)*pow(sin(HA),2) + 
   zd*cos(HA)*cos(dec)*pow(cos(phi),2)*sin(dec) + Rdec*cos(HA)*cos(dec)*cos(phi)*sin(HA)*sin(dec) - 
   Rdec*cos(HA)*cos(dec)*pow(cos(phi),3)*sin(HA)*sin(dec) - 
   zd*pow(cos(HA),2)*pow(cos(dec),2)*cos(phi)*sin(phi) - 
   Rdec*pow(cos(HA),2)*pow(cos(dec),2)*sin(HA)*sin(phi) - 
   Rdec*pow(cos(dec),2)*pow(sin(HA),3)*sin(phi) + 2*xd*cos(HA)*cos(dec)*cos(phi)*sin(dec)*sin(phi) + 
   zd*cos(phi)*pow(sin(dec),2)*sin(phi) - Rdec*pow(cos(phi),2)*sin(HA)*pow(sin(dec),2)*sin(phi) - 
   zd*cos(HA)*cos(dec)*sin(dec)*pow(sin(phi),2) - 
   Rdec*cos(HA)*cos(dec)*cos(phi)*sin(HA)*sin(dec)*pow(sin(phi),2) + 
   xd*pow(sin(dec),2)*pow(sin(phi),2) - Rdec*sin(HA)*pow(sin(dec),2)*pow(sin(phi),3) - 
   (cos(phi)*sin(dec)*sqrt(-4*(pow(Rdec,2) - pow(Rdome,2) + pow(xd,2) + pow(zd,2) - 
           2*Rdec*zd*cos(phi)*sin(HA) - 2*Rdec*xd*sin(HA)*sin(phi)) + 
        4*pow(cos(HA)*cos(dec)*(zd*cos(phi) + xd*sin(phi)) + sin(dec)*(-(xd*cos(phi)) + zd*sin(phi)),2)))/
    2. + (cos(HA)*cos(dec)*sin(phi)*sqrt(-4*
         (pow(Rdec,2) - pow(Rdome,2) + pow(xd,2) + pow(zd,2) - 
           2*Rdec*zd*cos(phi)*sin(HA) - 2*Rdec*xd*sin(HA)*sin(phi)) + 
        4*pow(cos(HA)*cos(dec)*(zd*cos(phi) + xd*sin(phi)) + sin(dec)*(-(xd*cos(phi)) + zd*sin(phi)),2)))/
    2.;

    return correct ;
}

double LDStarOnDomeTWX( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd)
{

    double correct ;

   correct= xd*pow(cos(HA),2)*pow(cos(dec),2)*pow(cos(phi),2) + xd*pow(cos(dec),2)*pow(sin(HA),2) + 
   zd*cos(HA)*cos(dec)*pow(cos(phi),2)*sin(dec) - Rdec*cos(HA)*cos(dec)*cos(phi)*sin(HA)*sin(dec) + 
   Rdec*cos(HA)*cos(dec)*pow(cos(phi),3)*sin(HA)*sin(dec) - 
   zd*pow(cos(HA),2)*pow(cos(dec),2)*cos(phi)*sin(phi) + 
   Rdec*pow(cos(HA),2)*pow(cos(dec),2)*sin(HA)*sin(phi) + 
   Rdec*pow(cos(dec),2)*pow(sin(HA),3)*sin(phi) + 2*xd*cos(HA)*cos(dec)*cos(phi)*sin(dec)*sin(phi) + 
   zd*cos(phi)*pow(sin(dec),2)*sin(phi) + Rdec*pow(cos(phi),2)*sin(HA)*pow(sin(dec),2)*sin(phi) - 
   zd*cos(HA)*cos(dec)*sin(dec)*pow(sin(phi),2) + 
   Rdec*cos(HA)*cos(dec)*cos(phi)*sin(HA)*sin(dec)*pow(sin(phi),2) + 
   xd*pow(sin(dec),2)*pow(sin(phi),2) + Rdec*sin(HA)*pow(sin(dec),2)*pow(sin(phi),3) - 
   (cos(phi)*sin(dec)*sqrt(-4*(pow(Rdec,2) - pow(Rdome,2) + pow(xd,2) + pow(zd,2) + 
           2*Rdec*zd*cos(phi)*sin(HA) + 2*Rdec*xd*sin(HA)*sin(phi)) + 
        4*pow(cos(HA)*cos(dec)*(zd*cos(phi) + xd*sin(phi)) + sin(dec)*(-(xd*cos(phi)) + zd*sin(phi)),2)))/
    2. + (cos(HA)*cos(dec)*sin(phi)*sqrt(-4*
         (pow(Rdec,2) - pow(Rdome,2) + pow(xd,2) + pow(zd,2) + 
           2*Rdec*zd*cos(phi)*sin(HA) + 2*Rdec*xd*sin(HA)*sin(phi)) + 
        4*pow(cos(HA)*cos(dec)*(zd*cos(phi) + xd*sin(phi)) + sin(dec)*(-(xd*cos(phi)) + zd*sin(phi)),2)))/
    2.
;
 
   return correct ;
}

double LDStarOnDomeTEY( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd)
{
    double correct ;


  correct= -(Rdec*pow(cos(HA),3)*pow(cos(dec),2)) + 
   cos(HA)*(-(Rdec*pow(sin(dec),2)) + pow(cos(dec),2)*sin(HA)*
       (zd*cos(phi) - Rdec*sin(HA) + xd*sin(phi))) - 
   cos(dec)*sin(HA)*(xd*cos(phi)*sin(dec) - zd*sin(dec)*sin(phi) + 
      sqrt(-pow(Rdec,2) + pow(Rdome,2) - pow(xd,2) - pow(zd,2) + 
        pow(xd,2)*pow(cos(phi),2)*pow(sin(dec),2) + 2*Rdec*xd*sin(HA)*sin(phi) + 
        pow(zd,2)*pow(sin(dec),2)*pow(sin(phi),2) + 
        pow(cos(HA),2)*pow(cos(dec),2)*pow(zd*cos(phi) + xd*sin(phi),2) + 
        2*zd*cos(phi)*(Rdec*sin(HA) - xd*pow(sin(dec),2)*sin(phi)) - 
        (cos(HA)*sin(2*dec)*(2*xd*zd*cos(2*phi) + (pow(xd,2) - pow(zd,2))*sin(2*phi)))/2.));

  return correct ; 
}

double LDStarOnDomeTWY( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd)
{
    double correct ;

  correct= Rdec*pow(cos(HA),3)*pow(cos(dec),2) + 
   cos(HA)*(Rdec*pow(sin(dec),2) + pow(cos(dec),2)*sin(HA)*
       (zd*cos(phi) + Rdec*sin(HA) + xd*sin(phi))) - 
   cos(dec)*sin(HA)*(xd*cos(phi)*sin(dec) - zd*sin(dec)*sin(phi) + 
      sqrt(-pow(Rdec,2) + pow(Rdome,2) - pow(xd,2) - pow(zd,2) + 
        pow(xd,2)*pow(cos(phi),2)*pow(sin(dec),2) - 2*Rdec*xd*sin(HA)*sin(phi) + 
        pow(zd,2)*pow(sin(dec),2)*pow(sin(phi),2) + 
        pow(cos(HA),2)*pow(cos(dec),2)*pow(zd*cos(phi) + xd*sin(phi),2) - 
        2*zd*cos(phi)*(Rdec*sin(HA) + xd*pow(sin(dec),2)*sin(phi)) - 
        (cos(HA)*sin(2*dec)*(2*xd*zd*cos(2*phi) + (pow(xd,2) - pow(zd,2))*sin(2*phi)))/2.)) ;

  return correct ;
}

// OPEN ISSUE EAST and WEST see LDHAtoDomeAZ!!!!!!!!!
double LDStarOnDomeZ( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd)
{
    if(( dapm.west == IS_WEST) &&( dapm.east == IS_NOT_EAST))
    {
	return sqrt( pow(Rdome,2.)- (pow(LDStarOnDomeTEX(HA, dec, phi, Rdome, Rdec, xd , zd),2.) + pow(LDStarOnDomeTEY(HA, dec, phi, Rdome, Rdec, xd , zd), 2.))) ;
    }
    else if(( dapm.east == IS_EAST)  &&( dapm.west == IS_NOT_WEST))
    {
	return sqrt( pow(Rdome,2.)- (pow(LDStarOnDomeTWX(HA, dec, phi, Rdome, Rdec, xd , zd),2.) + pow(LDStarOnDomeTWY(HA, dec, phi, Rdome, Rdec, xd , zd), 2.))) ;
    }
    return -1. ;
}

int LDRAtoDomeAZ( double longitude, double latitude, double RA, double dec, double *Az, double *ZD) 
{
    int res ;

    double HA ;

    HA= LDRAtoHA( RA, longitude) ;

    res= LDHAtoDomeAZ( latitude, HA, dec, Az, ZD) ;
    
    return 0 ;
}
int LDRAtoStarAZ( double longitude, double latitude, double RA, double dec, double *Az, double *ZD) 
{
    int res ;
    double HA ;

    HA= LDRAtoHA( RA, longitude) ;

    res= LDHAtoStarAZ( latitude, HA, dec, Az, ZD) ;
    
    return 0 ;
}
double LDRAtoHA( double RA, double longitude)
{
    double HA ;
    double JD ;
    double theta_0= 0. ;
    
    JD=  ln_get_julian_from_sys() ;

    theta_0= 15./180. * M_PI * ln_get_mean_sidereal_time( JD) ; 
/* negative to the West, Kstars neg., XEpehem pos */
    HA =  fmod(theta_0 + longitude - RA + 2. * M_PI,  2. * M_PI) ;

    return HA ;
} 
double LDHAtoRA( double HA, double longitude)
{
    double RA ;
    double JD ;
    double theta_0= 0. ;
    
    JD=  ln_get_julian_from_sys() ;

    theta_0= 15. /180. * M_PI * ln_get_mean_sidereal_time( JD) ;
/* negative to the West, Kstars neg., XEpehem pos */
    RA =  fmod(theta_0 + longitude - HA + 2. * M_PI,  2. * M_PI) ;

    return RA ;
} 

int LDHAtoDomeAZ( double latitude, double HA, double dec, double *Az, double *ZD)
{

// OPEN ISSUE EAST and WEST see LDStarOnDomeZ!!!!!!!!! 

    if(( dapm.west == IS_WEST) &&( dapm.east == IS_NOT_EAST))
    {
	*Az= -atan2( LDStarOnDomeTEY( HA, dec, latitude, DomeRadius,  mt.rdec, mt.xd, mt.zd), LDStarOnDomeTEX( HA, dec, latitude, DomeRadius,  mt.rdec, mt.xd, mt.zd)) ;
    }
    else if(( dapm.east == IS_EAST)  &&( dapm.west == IS_NOT_WEST))
    {
	*Az= -atan2( LDStarOnDomeTWY( HA, dec, latitude, DomeRadius,  mt.rdec, mt.xd, mt.zd), LDStarOnDomeTWX( HA, dec, latitude, DomeRadius,  mt.rdec, mt.xd, mt.zd)) ;
    }
    else
    {
	//IDMessage( mydev, "LDHAtoDomeAZ: neither EAST nor WEST");
        return -1 ;
    }

    *ZD= acos( LDStarOnDomeZ( HA, dec, latitude, DomeRadius,  mt.rdec, mt.xd, mt.zd)/DomeRadius) ;


/*     IDMessage( mydev, "X %f Y %f Z %f Rdome %f", LDStarOnDomeX( HA, dec, latitude, DomeRadius,  mt.rdec, mt.xd, mt.zd), LDStarOnDomeY( HA, dec, latitude, DomeRadius,  mt.rdec, mt.xd, mt.zd), LDStarOnDomeZ( HA, dec, latitude, DomeRadius,  mt.rdec, mt.xd, mt.zd), DomeRadius) ; */

    /* Define the offset */

    *Az += M_PI ;

    if( *Az < 0.)
    {
	*Az += 2. * M_PI ;
    }
    else if( *Az >= 2. * M_PI)
    {
	*Az -= 2. * M_PI ;
    }

    *Az= *Az * 180./ M_PI;
    *ZD= *ZD * 180./ M_PI;

    return 0 ;
}
int LDHAtoStarAZ( double latitude, double HA, double dec, double *Az, double *ZD)
{
    int res ;


    *Az= -atan2( LDStarOnDomeTEY( HA, dec, latitude, 1.,  0., 0., 0.), LDStarOnDomeTEX( HA, dec, latitude, 1.,  0., 0., 0.)) ;
    *ZD= acos( LDStarOnDomeZ( HA, dec, latitude, 1.,  0., 0., 0.)) ;

    /* Define the offset */

    *Az += M_PI ;

    if( *Az < 0.)
    {
        *Az += 2. * M_PI ;
    }
    else if( *Az >= 2. * M_PI)
    {
        *Az -= 2. * M_PI ;
    }

  
    *Az= *Az * 180./ M_PI;

    if(( res=LDCheckHorizon( HA, dec, latitude))==BELOW)
    {
	*ZD= 180. - *ZD * 180./ M_PI;
    } 
    else
    {
	*ZD= *ZD * 180./ M_PI;
    }
    return 0 ;
}
int LDCheckHorizon( double HA, double dec, double latitude)
{
  double sinh ;

  sinh= cos(latitude) * cos(HA) * cos(dec) + sin(latitude) * sin(dec) ;

  if( sinh >= 0.)
    {
      return 0 ;
    }
  else
    {      
      return BELOW ;
    }
}
