/* Library to calculate dome target AZ and ZD  */
/* Copyright (C) 2006-2010, Markus Wildi */

/* The transformations are based on the paper Matrix Method for  */
/* Coodinates Transformation written by Toshimi Taki  */
/* (http://www.asahi-net.or.jp/~zs3t-tk).  */

/* Documentation: */
/* https://azug.minpet.unibas.ch/wikiobsvermes/index.php/Robotic_ObsVermes#Telescope */

/* This library is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU Lesser General Public */
/* License as published by the Free Software Foundation; either */
/* version 2.1 of the License, or (at your option) any later version. */

/* This library is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU */
/* Lesser General Public License for more details. */

/* You should have received a copy of the GNU Lesser General Public */
/* License along with this library; if not, write to the Free Software */
/* Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301  USA */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <libnova/libnova.h>
#include <slitazimuth.h>

#define Csc(x) (1./sin(x))
#define Sec(x) (1./cos(x))

#define ABOVE_HORIZON  0
#define BELOW_HORIZON -1


double LDRAtoHA( double RA, double longitude) ;
int    LDRAtoDomeAZ( struct ln_equ_posn tmp_equ, struct ln_lnlat_posn obs_location, struct geometry obs, double *Az, double *ZD) ;
int    LDRAtoStarAZ( struct ln_equ_posn tmp_equ, struct ln_lnlat_posn obs_location, struct geometry obs, double *Az, double *ZD) ;
int    LDHAtoDomeAZ( double latitude, double HA, double dec, struct geometry obs, double *Az, double *ZD) ;
int    LDHAtoStarAZ( double latitude, double HA, double dec, double *Az, double *ZD) ;
double LDStarOnDomeTX( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd) ;
double LDStarOnDomeTY( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd) ;
double LDStarOnDomeZ( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd) ;
int    LDCheckHorizon( double HA, double dec, double phi) ;


/* the main entry function */
/* longitude positive to the East */
/* West: DECaxis== HA + M_PI/2  RA=RA, DEC=DEC  */
/* East: DECaxis== HA - M_PI/2, RA=RA, DEC= DEC + M_PI */
double dome_target_az( struct ln_equ_posn tel_equ, struct ln_lnlat_posn obs_location, struct geometry obs)
{
  double target_az ;
  double target_ZD ;
  struct ln_equ_posn tmp_equ ;

  if(( tel_equ.dec > 90.) && (  tel_equ.dec <= 270.)) { // EAST: DECaxis==HA - M_PI/2

    tmp_equ.ra =  tel_equ.ra - 180. ;
    tmp_equ.dec= -tel_equ.dec  ;
  } else {// WEST: DECaxis==HA + M_PI/2
    
    tmp_equ.ra =  tel_equ.ra ;
    tmp_equ.dec=  tel_equ.dec  ;
  }
  tmp_equ.ra  *= M_PI/180. ;
  tmp_equ.dec *= M_PI/180. ;

  obs_location.lng *= M_PI/180. ;
  obs_location.lat *= M_PI/180. ;

  LDRAtoDomeAZ( tmp_equ, obs_location, obs, &target_az, &target_ZD) ;

  // This call is for a quick check
  //double star_az ;
  //double star_ZD ;
  //ret= LDRAtoStarAZ( tmp_equ, obs_location, obs, &star_az, &star_ZD) ;

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
/* I copied the whole expressions from the Mathematica notebook */

double LDStarOnDomeTX( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd)
{
    return  xd*pow(cos(HA),2)*pow(cos(dec),2)*pow(cos(phi),2) + xd*pow(cos(dec),2)*pow(sin(HA),2) + 
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
}

double LDStarOnDomeTY( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd)
{
  return -(Rdec*pow(cos(HA),3)*pow(cos(dec),2)) + 
   cos(HA)*(-(Rdec*pow(sin(dec),2)) + pow(cos(dec),2)*sin(HA)*
       (zd*cos(phi) - Rdec*sin(HA) + xd*sin(phi))) - 
   cos(dec)*sin(HA)*(xd*cos(phi)*sin(dec) - zd*sin(dec)*sin(phi) + 
      sqrt(-pow(Rdec,2) + pow(Rdome,2) - pow(xd,2) - pow(zd,2) + 
        pow(xd,2)*pow(cos(phi),2)*pow(sin(dec),2) + 2*Rdec*xd*sin(HA)*sin(phi) + 
        pow(zd,2)*pow(sin(dec),2)*pow(sin(phi),2) + 
        pow(cos(HA),2)*pow(cos(dec),2)*pow(zd*cos(phi) + xd*sin(phi),2) + 
        2*zd*cos(phi)*(Rdec*sin(HA) - xd*pow(sin(dec),2)*sin(phi)) - 
        (cos(HA)*sin(2*dec)*(2*xd*zd*cos(2*phi) + (pow(xd,2) - pow(zd,2))*sin(2*phi)))/2.));
}

double LDStarOnDomeZ( double HA, double dec, double phi, double Rdome,  double Rdec, double xd , double zd)
{
  return sqrt( pow(Rdome,2.)- (pow(LDStarOnDomeTX(HA, dec, phi, Rdome, Rdec, xd , zd),2.) + pow(LDStarOnDomeTY(HA, dec, phi, Rdome, Rdec, xd , zd), 2.))) ;
}

int LDRAtoDomeAZ( struct ln_equ_posn tmp_equ, struct ln_lnlat_posn obs_location, struct geometry obs, double *Az, double *ZD) 
{
    double HA ;

    HA= LDRAtoHA( tmp_equ.ra, obs_location.lng) ;

    LDHAtoDomeAZ( obs_location.lat, HA, tmp_equ.dec, obs, Az, ZD) ;
    return 0 ;
}
int LDRAtoStarAZ( struct ln_equ_posn tmp_equ, struct ln_lnlat_posn obs_location, struct geometry obs, double *Az, double *ZD) 
{
    double HA ;

    HA= LDRAtoHA( tmp_equ.ra, obs_location.lng) ;
    
    LDHAtoStarAZ( obs_location.lat, HA, tmp_equ.dec, Az, ZD) ;
    
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
int LDHAtoDomeAZ( double latitude, double HA, double dec, struct geometry obs, double *Az, double *ZD)
{
  *Az= -atan2( LDStarOnDomeTY( HA, dec, latitude, obs.rdome,  obs.rdec, obs.xd, obs.zd), LDStarOnDomeTX( HA, dec, latitude, obs.rdome,  obs.rdec, obs.xd, obs.zd)) ;

  *Az= fmodl( *Az + M_PI , 2 *M_PI) ;
  *Az= *Az * 180./ M_PI;

  *ZD= acos( LDStarOnDomeZ( HA, dec, latitude, obs.rdome,  obs.rdec, obs.xd, obs.zd)/obs.rdome) ;
  *ZD= *ZD * 180./ M_PI;
  //fprintf( stderr, "LDHAtoDomeAZ  Az, ZD, radius %+010.5f, %+010.5f, %+010.5f\n", *Az, *ZD, obs.rdome) ;

  return 0 ;
}
int LDHAtoStarAZ( double latitude, double HA, double dec, double *Az, double *ZD)
{
  int res ;

  *Az= -atan2( LDStarOnDomeTY( HA, dec, latitude, 1.,  0., 0., 0.), LDStarOnDomeTX( HA, dec, latitude, 1.,  0., 0., 0.)) ;

  *Az= fmodl( *Az + M_PI , 2 *M_PI) ;
  *Az= *Az * 180./ M_PI;

  *ZD= acos( LDStarOnDomeZ( HA, dec, latitude, 1.,  0., 0., 0.)) ;
  if(( res=LDCheckHorizon( HA, dec, latitude))==BELOW_HORIZON)
    {
      *ZD= 180. - *ZD * 180./ M_PI;
    } 
  else
    {
      *ZD= *ZD * 180./ M_PI;
    }
  //fprintf( stderr, "LDHAtoStarAZ  Az, ZD, radius %+010.5f, %+010.5f, %+010.5f\n", *Az, *ZD, 1.) ;

  return 0 ;
}
int LDCheckHorizon( double HA, double dec, double latitude)
{
  double sinh_value ;

  sinh_value= cos(latitude) * cos(HA) * cos(dec) + sin(latitude) * sin(dec) ;

  if( sinh_value >= 0.)
    {
      return ABOVE_HORIZON ;
    }
  else
    {      
      return BELOW_HORIZON ;
    }
}
