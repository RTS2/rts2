/* Library to calculate pier telescope collision.  */
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

/* Standard headers */

#include <stdio.h>
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
#define NO_COLLISION 0
#define WITHIN_DANGER_ZONE_ABOVE 1
#define WITHIN_DANGER_ZONE_BELOW 2
#define COLLIDING 3
#define UNDEFINED_DEC_AXIS_POSITION 4

double LDRAtoHA( double RA, double longitude) ;
int    LDCollision( double RA, double dec, double lambda, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier, int EastOfPier, int WestOfPier) ;
double LDTangentPlaneLineP(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double tp, int EastOfPier, int WestOfPier) ;
double LDTangentPlaneLineM(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double tp, int EastOfPier, int WestOfPier) ;
double LDCutPierLineP1(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier, int EastOfPier, int WestOfPier) ;
double LDCutPierLineM1(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier, int EastOfPier, int WestOfPier) ;
double LDCutPierLineP3(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier, int EastOfPier, int WestOfPier) ;
double LDCutPierLineM3(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier, int EastOfPier, int WestOfPier) ;

struct Decaxis_plus_minus {
  int east ;
  int west ;
  
} dapm ;

struct pier {
  double radius ;
  double wedge ;
  double floor ;
  double danger_zone_above ;
  double danger_zone_below ;
}  pr;

struct mount {
  double xd ;
  double zd ;
  double rdec ;
  
} mt ;

struct telescope {
  double radius ;
  double rear_length ;

} tel ;
/* the main entry function */
/* struct ln_equ_posn tel_eq telescope target coordinates */
/* int angle (IS_WEST|IS_EAST) IS_WEST: DEC axis points to HA +M_PI/2, IS_EAST: DEC axis points to HA - M_PI/2*/
/* DEC axis is the vector which points from the intersection of the HA axis with DEC axis tp the intersection*/
/* of the DEC axis with the optical axis, the optical axis points to HA, DEC) */
int  pier_collision( struct ln_equ_posn tel_eq, int angle)
{
  double longitude= 7.5 /180. * M_PI ; //wildi ToDo value,  positive to the East
  double latitude= 47.2 /180. * M_PI ; //wildi ToDo
  
  if( angle== IS_WEST)
    {
      dapm.east= IS_NOT_EAST ;
      dapm.west= IS_WEST ;
    }
  else if( angle== IS_EAST)
    {
      dapm.east= IS_EAST ;
      dapm.west= IS_NOT_WEST ;
    }
  else
    {
      return UNDEFINED_DEC_AXIS_POSITION ;
    }

  pr.radius= 0.135;
  pr.wedge= -0.6684;
  pr.floor= -1.9534;
  pr.danger_zone_above= -0.1;
  pr.danger_zone_below= -2.9534;
  
  mt.xd= -0.0684;
  mt.zd= -0.1934;
  mt.rdec= 0.338;

  tel.radius= 0.123;
  tel.rear_length= 0.8;

  return LDCollision( tel_eq.ra/180. * M_PI, tel_eq.dec/180. * M_PI, longitude, latitude, mt.zd, mt.xd, mt.rdec, tel.radius, pr.radius, dapm.east, dapm.west) ;
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

int LDCollision( double RA, double dec, double lambda, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier, int EastOfPier, int WestOfPier)
{
    double tpp1 ;
    double tpm1 ;
    double czp1 ;
    double czm1 ;

    double tpp3 ;
    double tpm3 ;
    double czp3 ;
    double czm3 ;
    
    double HA ;

    HA= LDRAtoHA( RA, lambda) ;

/* Parameter of the straight line */

    tpp1= LDCutPierLineP1(HA, dec, phi, zd, xd, Rdec, Rtel, Rpier, EastOfPier, WestOfPier);
    tpm1= LDCutPierLineM1(HA, dec, phi, zd, xd, Rdec, Rtel, Rpier, EastOfPier, WestOfPier);

    tpp3= LDCutPierLineP3(HA, dec, phi, zd, xd, Rdec, Rtel, Rpier, EastOfPier, WestOfPier);
    tpm3= LDCutPierLineM3(HA, dec, phi, zd, xd, Rdec, Rtel, Rpier, EastOfPier, WestOfPier);

/* z componet of the intersection */

    czp1= LDTangentPlaneLineP(HA, dec, phi, zd, xd, Rdec, Rtel, tpp1, EastOfPier, WestOfPier) ;
    czm1= LDTangentPlaneLineM(HA, dec, phi, zd, xd, Rdec, Rtel, tpm1, EastOfPier, WestOfPier) ;

    czp3= LDTangentPlaneLineP(HA, dec, phi, zd, xd, Rdec, Rtel, tpp3, EastOfPier, WestOfPier) ;
    czm3= LDTangentPlaneLineM(HA, dec, phi, zd, xd, Rdec, Rtel, tpm3, EastOfPier, WestOfPier) ;


    /* fprintf( stderr, "HA %f, lambda %f\n", HA/M_PI * 180., lambda /M_PI * 180.) ; */
    /* fprintf( stderr, "HA %f, dec %f, phi %f, zd %f, xd %f, Rdec %f, Rtel %f, Rpier %f, EastOfPier %d, WestOfPier %d\n", HA * 180/M_PI, dec* 180/M_PI, phi* 180/M_PI, zd, xd, Rdec, Rtel, Rpier, EastOfPier, WestOfPier) ; */
    /* fprintf( stderr,  "tpp1 %f\ntpm1 %f\ntpp3 %f\ntpm3 %f\nczp1 %f\nczm1 %f\nczp3 %f\nczm3 %f\n", tpp1, tpm1, tpp3, tpm3, czp1, czm1, czp3, czm3) ; */
    /* fprintf( stderr, "\n") ; */

    if((czp1 > pr.floor) && ( czp1 < pr.wedge))
    {
	if( fabs(tpp1) > tel.rear_length)
	{
	  return NO_COLLISION ;
	}

	fprintf(stderr, "Collision 1 on the positive side\n" ) ;
	return COLLIDING ;
    } 
    else  if((czp1 > pr.danger_zone_above) && ( czp1 <= pr.floor))
      {
	fprintf(stderr, "Within danger zone below 1 on the positive side\n") ;
	return WITHIN_DANGER_ZONE_BELOW ;
      }
    else  if((czp1 < pr.danger_zone_below) && ( czp1 >= pr.wedge))
    {
	fprintf(stderr, "Within danger zone above 1 on the positive side\n") ;
	return WITHIN_DANGER_ZONE_ABOVE ;
    }

    if((czm1 > pr.floor) && ( czm1 < pr.wedge))
    {
	if( fabs(tpm1) > tel.rear_length)
	{
	  return NO_COLLISION ;
	}
	fprintf(stderr, "Collision 1 on the negative side\n" ) ;
	return COLLIDING ;
    }
    else  if((czm1 > pr.danger_zone_above) && ( czm1 <= pr.floor))
    {
	fprintf(stderr, "Within danger zone below 1 on the negative side\n") ;
	return WITHIN_DANGER_ZONE_BELOW ;
    }
    else  if((czm1 < pr.danger_zone_below) && ( czm1 >= pr.wedge))
    {
	fprintf(stderr, "Within danger zone above 1 on the negative side\n") ;
	return WITHIN_DANGER_ZONE_ABOVE ;
    }

    if((czp3 > pr.floor) && ( czp3 < pr.wedge))
    {
	if( fabs(tpp3) > tel.rear_length)
	{
	  return NO_COLLISION ;
	}
	fprintf(stderr, "Collision 3 on the positive side\n" ) ;
	return COLLIDING ;
    }
    else  if((czp3 > pr.danger_zone_above) && ( czp3 <= pr.floor))
    {
	fprintf(stderr, "Within danger zone below 3 on the positive side\n") ;
	return WITHIN_DANGER_ZONE_BELOW ;
    }
    else  if((czp3 < pr.danger_zone_below) && ( czp3 >= pr.wedge))
    {
	fprintf(stderr, "Within danger zone above 3 on the positive side\n") ;
	return WITHIN_DANGER_ZONE_ABOVE ;
    }

    if((czm3 > pr.floor) && ( czm3 < pr.wedge))
    {
	if( fabs(tpm3) > tel.rear_length)
	{
	  return NO_COLLISION ;
	}
	fprintf(stderr, "Collision 3 on the negative side\n" ) ;
	return COLLIDING ;
    }

    if((czm3 > pr.danger_zone_above) && ( czm3 <= pr.floor))
    {
	fprintf(stderr, "Within danger zone below 3 on the negative side\n") ;
	return WITHIN_DANGER_ZONE_BELOW ;
    }
    else  if((czm3 < pr.danger_zone_below) && ( czm3 >= pr.wedge))
    {
	fprintf(stderr, "Within danger zone above 3 on the negative side\n") ;
	return WITHIN_DANGER_ZONE_ABOVE ;
    }
    return NO_COLLISION ;
}
double LDTangentPlaneLineP(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double tp, int EastOfPier, int WestOfPier)
{
    if(( WestOfPier == IS_WEST) &&( EastOfPier == IS_NOT_EAST))
    {
	return zd - sqrt(pow(Rdec,2))*cos(phi)*sin(HA) + (Rdec*Rtel*cos(phi)*sin(HA))/sqrt(pow(Rdec,2)) + 
    (Rdec*Rtel*(cos(HA)*cos(phi)*sin(dec) - cos(dec)*sin(phi)))/sqrt(pow(Rdec,2)) + tp*(cos(HA)*cos(dec)*cos(phi) + sin(dec)*sin(phi)) ;
    }
    else if(( EastOfPier == IS_EAST)  &&( WestOfPier == IS_NOT_WEST))
    {
	return zd + sqrt(pow(Rdec,2))*cos(phi)*sin(HA) - (Rdec*Rtel*cos(phi)*sin(HA))/sqrt(pow(Rdec,2)) + 
	(Rdec*Rtel*(-(cos(HA)*cos(phi)*sin(dec)) + cos(dec)*sin(phi)))/sqrt(pow(Rdec,2)) + tp*(cos(HA)*cos(dec)*cos(phi) + sin(dec)*sin(phi));
    } 
    else 
    {
	// LDTangentPlaneLineP neither WEST nor EAST
    }
    return 1./0.;
}

double LDTangentPlaneLineM(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double tp, int EastOfPier, int WestOfPier)
{
    if(( WestOfPier == IS_WEST) &&( EastOfPier == IS_NOT_EAST))
    {
	return zd - sqrt(pow(Rdec,2))*cos(phi)*sin(HA) + (Rdec*Rtel*cos(phi)*sin(HA))/sqrt(pow(Rdec,2)) + 
    (Rdec*Rtel*(-(cos(HA)*cos(phi)*sin(dec)) + cos(dec)*sin(phi)))/sqrt(pow(Rdec,2)) + tp*(cos(HA)*cos(dec)*cos(phi) + sin(dec)*sin(phi));
    }
    else if(( EastOfPier == IS_EAST)  &&( WestOfPier == IS_NOT_WEST))
    {
	return zd + sqrt(pow(Rdec,2))*cos(phi)*sin(HA) - (Rdec*Rtel*cos(phi)*sin(HA))/sqrt(pow(Rdec,2)) + 
    (Rdec*Rtel*(cos(HA)*cos(phi)*sin(dec) - cos(dec)*sin(phi)))/sqrt(pow(Rdec,2)) + tp*(cos(HA)*cos(dec)*cos(phi) + sin(dec)*sin(phi));
    }
    else
    {
	// LDTangentPlaneLineM neither WEST nor EAST
    }
    return 1./0. ;
}


double LDCutPierLineP1(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier, int EastOfPier, int WestOfPier)
{

    if(( WestOfPier == IS_WEST) &&( EastOfPier == IS_NOT_EAST))
    {
	return (Rdec*Rpier*xd*cos(phi)*sin(dec) + sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) - 
     sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(sin(HA),2)*sin(dec) - 
     Rdec*sqrt(pow(Rdec,2))*Rpier*cos(phi)*sin(HA)*sin(dec)*sin(phi) + sqrt(pow(Rdec,2))*Rpier*Rtel*cos(phi)*sin(HA)*sin(dec)*sin(phi) - 
     sqrt(pow(Rdec,2))*Rpier*Rtel*pow(cos(HA),2)*cos(dec)*sin(dec)*pow(sin(phi),2) + 
     Rpier*cos(HA)*(-(sqrt(pow(Rdec,2))*Rtel*pow(cos(dec),2)*cos(phi)*sin(phi)) + 
        sqrt(pow(Rdec,2))*Rtel*cos(phi)*pow(sin(dec),2)*sin(phi) - 
        cos(dec)*(sqrt(pow(Rdec,2))*(Rdec - Rtel)*pow(cos(phi),2)*sin(HA) + Rdec*xd*sin(phi))) - 
     Csc(HA)*Sec(dec)*sqrt(pow(Rpier,2)*pow(cos(dec),2)*pow(sin(HA),2)*
        (-(pow(Rdec,2)*pow(Rtel,2)*pow(cos(dec),4)*pow(cos(phi),2)*pow(sin(HA),2)) + 
          pow(Rdec,2)*pow(cos(phi),2)*pow(sin(dec),2)*
           (pow(Rpier,2) - pow(Rdec - Rtel,2)*pow(cos(HA),2) - 2*Rdec*Rtel*cos(HA)*sin(HA)*sin(dec) + 
             pow(Rtel,2)*sin(2*HA)*sin(dec) - pow(Rtel,2)*pow(sin(HA),2)*pow(sin(dec),2)) + 
          2*Rdec*Rtel*pow(cos(dec),3)*cos(phi)*sin(HA)*(-(sqrt(pow(Rdec,2))*xd*sin(HA)) + Rdec*(Rdec - Rtel)*sin(phi)) - 
          Rdec*pow(cos(dec),2)*(-2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(cos(HA),2)*sin(HA)*sin(phi) - 
             2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(sin(HA),3)*sin(phi) + 
             Rdec*pow(Rdec - Rtel,2)*pow(sin(HA),4)*pow(sin(phi),2) + 
             Rdec*pow(sin(HA),2)*(-pow(Rpier,2) + pow(xd,2) + 2*pow(Rtel,2)*pow(cos(phi),2)*pow(sin(dec),2) + 
                2*pow(Rdec - Rtel,2)*pow(cos(HA),2)*pow(sin(phi),2)) + 
             Rdec*((Rdec - Rtel)*Rtel*pow(cos(phi),2)*sin(2*HA)*sin(dec) + 
                pow(cos(HA),2)*(-pow(Rpier,2) + pow(Rdec - Rtel,2)*pow(cos(HA),2))*pow(sin(phi),2))) + 
          cos(dec)*sin(dec)*(Rdec*sqrt(pow(Rdec,2))*Rtel*xd*pow(cos(HA),2)*cos(phi)*sin(dec) - 
             2*cos(HA)*cos(phi)*(pow(pow(Rdec,2),1.5)*xd*sin(HA) - 
                pow(Rdec,2)*(pow(Rdec,2) - pow(Rpier,2) - 2*Rdec*Rtel + pow(Rtel,2))*sin(phi)) + 
             Rdec*Rtel*(cos(phi)*(sqrt(pow(Rdec,2))*xd*sin(2*HA) - 
                   sin(dec)*(sqrt(pow(Rdec,2))*xd + sqrt(pow(Rdec,2))*xd*pow(sin(HA),2) + 2*Rdec*Rtel*sin(HA)*sin(phi))) + 
                pow(Rdec,2)*sin(HA)*sin(dec)*sin(2*phi))))))/
   (Rdec*Rpier*(pow(cos(phi),2)*pow(sin(dec),2) - 2*cos(HA)*cos(dec)*cos(phi)*sin(dec)*sin(phi) + 
       pow(cos(dec),2)*(pow(sin(HA),2) + pow(cos(HA),2)*pow(sin(phi),2))));
    }
    else if(( EastOfPier == IS_EAST)  &&( WestOfPier == IS_NOT_WEST))
    {

    return (Rdec*Rpier*xd*cos(phi)*sin(dec) - sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) + 
     sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(sin(HA),2)*sin(dec) + 
     Rdec*sqrt(pow(Rdec,2))*Rpier*cos(phi)*sin(HA)*sin(dec)*sin(phi) - sqrt(pow(Rdec,2))*Rpier*Rtel*cos(phi)*sin(HA)*sin(dec)*sin(phi) + 
     sqrt(pow(Rdec,2))*Rpier*Rtel*pow(cos(HA),2)*cos(dec)*sin(dec)*pow(sin(phi),2) - 
     Rpier*cos(HA)*(-(sqrt(pow(Rdec,2))*Rtel*pow(cos(dec),2)*cos(phi)*sin(phi)) + 
        sqrt(pow(Rdec,2))*Rtel*cos(phi)*pow(sin(dec),2)*sin(phi) + 
        cos(dec)*(-(sqrt(pow(Rdec,2))*(Rdec - Rtel)*pow(cos(phi),2)*sin(HA)) + Rdec*xd*sin(phi))) - 
     Csc(HA)*Sec(dec)*sqrt(pow(Rpier,2)*pow(cos(dec),2)*pow(sin(HA),2)*
        (-(pow(Rdec,2)*pow(Rtel,2)*pow(cos(dec),4)*pow(cos(phi),2)*pow(sin(HA),2)) + 
          pow(Rdec,2)*pow(cos(phi),2)*pow(sin(dec),2)*
           (pow(Rpier,2) - pow(Rdec - Rtel,2)*pow(cos(HA),2) - 2*Rdec*Rtel*cos(HA)*sin(HA)*sin(dec) + 
             pow(Rtel,2)*sin(2*HA)*sin(dec) - pow(Rtel,2)*pow(sin(HA),2)*pow(sin(dec),2)) + 
          2*Rdec*Rtel*pow(cos(dec),3)*cos(phi)*sin(HA)*(sqrt(pow(Rdec,2))*xd*sin(HA) + Rdec*(Rdec - Rtel)*sin(phi)) - 
          Rdec*pow(cos(dec),2)*(2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(cos(HA),2)*sin(HA)*sin(phi) + 
             2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(sin(HA),3)*sin(phi) + 
             Rdec*pow(Rdec - Rtel,2)*pow(sin(HA),4)*pow(sin(phi),2) + 
             Rdec*pow(sin(HA),2)*(-pow(Rpier,2) + pow(xd,2) + 2*pow(Rtel,2)*pow(cos(phi),2)*pow(sin(dec),2) + 
                2*pow(Rdec - Rtel,2)*pow(cos(HA),2)*pow(sin(phi),2)) + 
             Rdec*((Rdec - Rtel)*Rtel*pow(cos(phi),2)*sin(2*HA)*sin(dec) + 
                pow(cos(HA),2)*(-pow(Rpier,2) + pow(Rdec - Rtel,2)*pow(cos(HA),2))*pow(sin(phi),2))) + 
          cos(dec)*sin(dec)*(-(Rdec*sqrt(pow(Rdec,2))*Rtel*xd*pow(cos(HA),2)*cos(phi)*sin(dec)) - 
             2*Rdec*cos(HA)*cos(phi)*(sqrt(pow(Rdec,2))*Rtel*xd*sin(HA) - 
                Rdec*(pow(Rdec,2) - pow(Rpier,2) - 2*Rdec*Rtel + pow(Rtel,2))*sin(phi)) + 
             cos(phi)*(pow(pow(Rdec,2),1.5)*xd*sin(2*HA) + 
                Rdec*Rtel*sin(dec)*(sqrt(pow(Rdec,2))*xd + sqrt(pow(Rdec,2))*xd*pow(sin(HA),2) - 2*Rdec*Rtel*sin(HA)*sin(phi)))\
              + pow(Rdec,3)*Rtel*sin(HA)*sin(dec)*sin(2*phi)))))/
   (Rdec*Rpier*(pow(cos(phi),2)*pow(sin(dec),2) - 2*cos(HA)*cos(dec)*cos(phi)*sin(dec)*sin(phi) + 
       pow(cos(dec),2)*(pow(sin(HA),2) + pow(cos(HA),2)*pow(sin(phi),2))));
    }
    else
    {
    }
    return 1./0. ;
}
double LDCutPierLineM1(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier, int EastOfPier, int WestOfPier)
{

    if(( WestOfPier == IS_WEST) &&( EastOfPier == IS_NOT_EAST))
    {

	return 
	    (Rdec*Rpier*xd*cos(phi)*sin(dec) - sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) + 
	     sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(sin(HA),2)*sin(dec) - 
	     Rdec*sqrt(pow(Rdec,2))*Rpier*cos(phi)*sin(HA)*sin(dec)*sin(phi) + sqrt(pow(Rdec,2))*Rpier*Rtel*cos(phi)*sin(HA)*sin(dec)*sin(phi) + 
	     sqrt(pow(Rdec,2))*Rpier*Rtel*pow(cos(HA),2)*cos(dec)*sin(dec)*pow(sin(phi),2) + 
	     Rpier*cos(HA)*(sqrt(pow(Rdec,2))*Rtel*pow(cos(dec),2)*cos(phi)*sin(phi) - 
			    sqrt(pow(Rdec,2))*Rtel*cos(phi)*pow(sin(dec),2)*sin(phi) - 
			    cos(dec)*(sqrt(pow(Rdec,2))*(Rdec - Rtel)*pow(cos(phi),2)*sin(HA) + Rdec*xd*sin(phi))) - 
	     Csc(HA)*Sec(dec)*sqrt(pow(Rpier,2)*pow(cos(dec),2)*pow(sin(HA),2)*
				   (-(pow(Rdec,2)*pow(Rtel,2)*pow(cos(dec),4)*pow(cos(phi),2)*pow(sin(HA),2)) + 
				    pow(Rdec,2)*pow(cos(phi),2)*pow(sin(dec),2)*
				    (pow(Rpier,2) - pow(Rdec - Rtel,2)*pow(cos(HA),2) - 2*pow(Rtel,2)*cos(HA)*sin(HA)*sin(dec) + 
				     Rdec*Rtel*sin(2*HA)*sin(dec) - pow(Rtel,2)*pow(sin(HA),2)*pow(sin(dec),2)) - 
				    2*Rdec*Rtel*pow(cos(dec),3)*cos(phi)*sin(HA)*(-(sqrt(pow(Rdec,2))*xd*sin(HA)) + Rdec*(Rdec - Rtel)*sin(phi)) - 
				    Rdec*pow(cos(dec),2)*(-2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(cos(HA),2)*sin(HA)*sin(phi) - 
							  2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(sin(HA),3)*sin(phi) + 
							  Rdec*pow(Rdec - Rtel,2)*pow(sin(HA),4)*pow(sin(phi),2) + 
							  Rdec*pow(sin(HA),2)*(-pow(Rpier,2) + pow(xd,2) + 2*pow(Rtel,2)*pow(cos(phi),2)*pow(sin(dec),2) + 
									       2*pow(Rdec - Rtel,2)*pow(cos(HA),2)*pow(sin(phi),2)) + 
							  Rdec*(-((Rdec - Rtel)*Rtel*pow(cos(phi),2)*sin(2*HA)*sin(dec)) + 
								pow(cos(HA),2)*(-pow(Rpier,2) + pow(Rdec - Rtel,2)*pow(cos(HA),2))*pow(sin(phi),2))) + 
				    cos(dec)*sin(dec)*(-(Rdec*sqrt(pow(Rdec,2))*Rtel*xd*pow(cos(HA),2)*cos(phi)*sin(dec)) - 
						       2*cos(HA)*cos(phi)*(pow(pow(Rdec,2),1.5)*xd*sin(HA) - 
									   pow(Rdec,2)*(pow(Rdec,2) - pow(Rpier,2) - 2*Rdec*Rtel + pow(Rtel,2))*sin(phi)) + 
						       Rdec*Rtel*(cos(phi)*(sqrt(pow(Rdec,2))*xd*sin(2*HA) + 
									    sin(dec)*(sqrt(pow(Rdec,2))*xd + sqrt(pow(Rdec,2))*xd*pow(sin(HA),2) - 2*pow(Rdec,2)*sin(HA)*sin(phi))) + 
								  Rdec*Rtel*sin(HA)*sin(dec)*sin(2*phi))))))/
	    (Rdec*Rpier*(pow(cos(phi),2)*pow(sin(dec),2) - 2*cos(HA)*cos(dec)*cos(phi)*sin(dec)*sin(phi) + 
			 pow(cos(dec),2)*(pow(sin(HA),2) + pow(cos(HA),2)*pow(sin(phi),2)))) ;

    } 
    else if(( EastOfPier == IS_EAST)  &&( WestOfPier == IS_NOT_WEST))
    {

    return -((-(Rdec*Rpier*xd*cos(phi)*sin(dec)) - sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) + 
       sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(sin(HA),2)*sin(dec) - 
       Rdec*sqrt(pow(Rdec,2))*Rpier*cos(phi)*sin(HA)*sin(dec)*sin(phi) + 
       sqrt(pow(Rdec,2))*Rpier*Rtel*cos(phi)*sin(HA)*sin(dec)*sin(phi) + 
       sqrt(pow(Rdec,2))*Rpier*Rtel*pow(cos(HA),2)*cos(dec)*sin(dec)*pow(sin(phi),2) + 
       Rpier*cos(HA)*(sqrt(pow(Rdec,2))*Rtel*pow(cos(dec),2)*cos(phi)*sin(phi) - 
          sqrt(pow(Rdec,2))*Rtel*cos(phi)*pow(sin(dec),2)*sin(phi) + 
          cos(dec)*(-(sqrt(pow(Rdec,2))*(Rdec - Rtel)*pow(cos(phi),2)*sin(HA)) + Rdec*xd*sin(phi))) + 
       Csc(HA)*Sec(dec)*sqrt(pow(Rpier,2)*pow(cos(dec),2)*pow(sin(HA),2)*
          (-(pow(Rdec,2)*pow(Rtel,2)*pow(cos(dec),4)*pow(cos(phi),2)*pow(sin(HA),2)) + 
            pow(Rdec,2)*pow(cos(phi),2)*pow(sin(dec),2)*
             (pow(Rpier,2) - pow(Rdec - Rtel,2)*pow(cos(HA),2) - 2*pow(Rtel,2)*cos(HA)*sin(HA)*sin(dec) + 
               Rdec*Rtel*sin(2*HA)*sin(dec) - pow(Rtel,2)*pow(sin(HA),2)*pow(sin(dec),2)) - 
            2*Rdec*Rtel*pow(cos(dec),3)*cos(phi)*sin(HA)*(sqrt(pow(Rdec,2))*xd*sin(HA) + Rdec*(Rdec - Rtel)*sin(phi)) - 
            Rdec*pow(cos(dec),2)*(2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(cos(HA),2)*sin(HA)*sin(phi) + 
               2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(sin(HA),3)*sin(phi) + 
               Rdec*pow(Rdec - Rtel,2)*pow(sin(HA),4)*pow(sin(phi),2) + 
               Rdec*pow(sin(HA),2)*(-pow(Rpier,2) + pow(xd,2) + 2*pow(Rtel,2)*pow(cos(phi),2)*pow(sin(dec),2) + 
                  2*pow(Rdec - Rtel,2)*pow(cos(HA),2)*pow(sin(phi),2)) + 
               Rdec*(-((Rdec - Rtel)*Rtel*pow(cos(phi),2)*sin(2*HA)*sin(dec)) + 
                  pow(cos(HA),2)*(-pow(Rpier,2) + pow(Rdec - Rtel,2)*pow(cos(HA),2))*pow(sin(phi),2))) + 
            cos(dec)*sin(dec)*(Rdec*sqrt(pow(Rdec,2))*Rtel*xd*pow(cos(HA),2)*cos(phi)*sin(dec) - 
               2*Rdec*cos(HA)*cos(phi)*(sqrt(pow(Rdec,2))*Rtel*xd*sin(HA) - 
                  Rdec*(pow(Rdec,2) - pow(Rpier,2) - 2*Rdec*Rtel + pow(Rtel,2))*sin(phi)) + 
               cos(phi)*(pow(pow(Rdec,2),1.5)*xd*sin(2*HA) - 
                  Rdec*Rtel*sin(dec)*(sqrt(pow(Rdec,2))*xd + sqrt(pow(Rdec,2))*xd*pow(sin(HA),2) + 
                     2*pow(Rdec,2)*sin(HA)*sin(phi))) + pow(Rdec,2)*pow(Rtel,2)*sin(HA)*sin(dec)*sin(2*phi)))))/
     (Rdec*Rpier*(pow(cos(phi),2)*pow(sin(dec),2) - 2*cos(HA)*cos(dec)*cos(phi)*sin(dec)*sin(phi) + 
         pow(cos(dec),2)*(pow(sin(HA),2) + pow(cos(HA),2)*pow(sin(phi),2)))));


    } else {
	//return nan ;
    }
    return 1./0. ;
}
double LDCutPierLineP3(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier, int EastOfPier, int WestOfPier)
{
    if(( WestOfPier == IS_WEST) &&( EastOfPier == IS_NOT_EAST))
    {
        return  (Rdec*Rpier*xd*cos(phi)*sin(dec) + sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) - 
     sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(sin(HA),2)*sin(dec) - 
     Rdec*sqrt(pow(Rdec,2))*Rpier*cos(phi)*sin(HA)*sin(dec)*sin(phi) + sqrt(pow(Rdec,2))*Rpier*Rtel*cos(phi)*sin(HA)*sin(dec)*sin(phi) - 
     sqrt(pow(Rdec,2))*Rpier*Rtel*pow(cos(HA),2)*cos(dec)*sin(dec)*pow(sin(phi),2) + 
     Rpier*cos(HA)*(-(sqrt(pow(Rdec,2))*Rtel*pow(cos(dec),2)*cos(phi)*sin(phi)) + 
        sqrt(pow(Rdec,2))*Rtel*cos(phi)*pow(sin(dec),2)*sin(phi) - 
        cos(dec)*(sqrt(pow(Rdec,2))*(Rdec - Rtel)*pow(cos(phi),2)*sin(HA) + Rdec*xd*sin(phi))) + 
     Csc(HA)*Sec(dec)*sqrt(pow(Rpier,2)*pow(cos(dec),2)*pow(sin(HA),2)*
        (-(pow(Rdec,2)*pow(Rtel,2)*pow(cos(dec),4)*pow(cos(phi),2)*pow(sin(HA),2)) + 
          pow(Rdec,2)*pow(cos(phi),2)*pow(sin(dec),2)*
           (pow(Rpier,2) - pow(Rdec - Rtel,2)*pow(cos(HA),2) - 2*Rdec*Rtel*cos(HA)*sin(HA)*sin(dec) + 
             pow(Rtel,2)*sin(2*HA)*sin(dec) - pow(Rtel,2)*pow(sin(HA),2)*pow(sin(dec),2)) + 
          2*Rdec*Rtel*pow(cos(dec),3)*cos(phi)*sin(HA)*(-(sqrt(pow(Rdec,2))*xd*sin(HA)) + Rdec*(Rdec - Rtel)*sin(phi)) - 
          Rdec*pow(cos(dec),2)*(-2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(cos(HA),2)*sin(HA)*sin(phi) - 
             2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(sin(HA),3)*sin(phi) + 
             Rdec*pow(Rdec - Rtel,2)*pow(sin(HA),4)*pow(sin(phi),2) + 
             Rdec*pow(sin(HA),2)*(-pow(Rpier,2) + pow(xd,2) + 2*pow(Rtel,2)*pow(cos(phi),2)*pow(sin(dec),2) + 
                2*pow(Rdec - Rtel,2)*pow(cos(HA),2)*pow(sin(phi),2)) + 
             Rdec*((Rdec - Rtel)*Rtel*pow(cos(phi),2)*sin(2*HA)*sin(dec) + 
                pow(cos(HA),2)*(-pow(Rpier,2) + pow(Rdec - Rtel,2)*pow(cos(HA),2))*pow(sin(phi),2))) + 
          cos(dec)*sin(dec)*(Rdec*sqrt(pow(Rdec,2))*Rtel*xd*pow(cos(HA),2)*cos(phi)*sin(dec) - 
             2*cos(HA)*cos(phi)*(pow(pow(Rdec,2),1.5)*xd*sin(HA) - 
                pow(Rdec,2)*(pow(Rdec,2) - pow(Rpier,2) - 2*Rdec*Rtel + pow(Rtel,2))*sin(phi)) + 
             Rdec*Rtel*(cos(phi)*(sqrt(pow(Rdec,2))*xd*sin(2*HA) - 
                   sin(dec)*(sqrt(pow(Rdec,2))*xd + sqrt(pow(Rdec,2))*xd*pow(sin(HA),2) + 2*Rdec*Rtel*sin(HA)*sin(phi))) + 
                pow(Rdec,2)*sin(HA)*sin(dec)*sin(2*phi))))))/
   (Rdec*Rpier*(pow(cos(phi),2)*pow(sin(dec),2) - 2*cos(HA)*cos(dec)*cos(phi)*sin(dec)*sin(phi) + 
       pow(cos(dec),2)*(pow(sin(HA),2) + pow(cos(HA),2)*pow(sin(phi),2))));
    }
    else if(( EastOfPier == IS_EAST)  &&( WestOfPier == IS_NOT_WEST))
    {
	return  (Rdec*Rpier*xd*cos(phi)*sin(dec) - sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) + 
     sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(sin(HA),2)*sin(dec) + 
     Rdec*sqrt(pow(Rdec,2))*Rpier*cos(phi)*sin(HA)*sin(dec)*sin(phi) - sqrt(pow(Rdec,2))*Rpier*Rtel*cos(phi)*sin(HA)*sin(dec)*sin(phi) + 
     sqrt(pow(Rdec,2))*Rpier*Rtel*pow(cos(HA),2)*cos(dec)*sin(dec)*pow(sin(phi),2) - 
     Rpier*cos(HA)*(-(sqrt(pow(Rdec,2))*Rtel*pow(cos(dec),2)*cos(phi)*sin(phi)) + 
        sqrt(pow(Rdec,2))*Rtel*cos(phi)*pow(sin(dec),2)*sin(phi) + 
        cos(dec)*(-(sqrt(pow(Rdec,2))*(Rdec - Rtel)*pow(cos(phi),2)*sin(HA)) + Rdec*xd*sin(phi))) + 
     Csc(HA)*Sec(dec)*sqrt(pow(Rpier,2)*pow(cos(dec),2)*pow(sin(HA),2)*
        (-(pow(Rdec,2)*pow(Rtel,2)*pow(cos(dec),4)*pow(cos(phi),2)*pow(sin(HA),2)) + 
          pow(Rdec,2)*pow(cos(phi),2)*pow(sin(dec),2)*
           (pow(Rpier,2) - pow(Rdec - Rtel,2)*pow(cos(HA),2) - 2*Rdec*Rtel*cos(HA)*sin(HA)*sin(dec) + 
             pow(Rtel,2)*sin(2*HA)*sin(dec) - pow(Rtel,2)*pow(sin(HA),2)*pow(sin(dec),2)) + 
          2*Rdec*Rtel*pow(cos(dec),3)*cos(phi)*sin(HA)*(sqrt(pow(Rdec,2))*xd*sin(HA) + Rdec*(Rdec - Rtel)*sin(phi)) - 
          Rdec*pow(cos(dec),2)*(2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(cos(HA),2)*sin(HA)*sin(phi) + 
             2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(sin(HA),3)*sin(phi) + 
             Rdec*pow(Rdec - Rtel,2)*pow(sin(HA),4)*pow(sin(phi),2) + 
             Rdec*pow(sin(HA),2)*(-pow(Rpier,2) + pow(xd,2) + 2*pow(Rtel,2)*pow(cos(phi),2)*pow(sin(dec),2) + 
                2*pow(Rdec - Rtel,2)*pow(cos(HA),2)*pow(sin(phi),2)) + 
             Rdec*((Rdec - Rtel)*Rtel*pow(cos(phi),2)*sin(2*HA)*sin(dec) + 
                pow(cos(HA),2)*(-pow(Rpier,2) + pow(Rdec - Rtel,2)*pow(cos(HA),2))*pow(sin(phi),2))) + 
          cos(dec)*sin(dec)*(-(Rdec*sqrt(pow(Rdec,2))*Rtel*xd*pow(cos(HA),2)*cos(phi)*sin(dec)) - 
             2*Rdec*cos(HA)*cos(phi)*(sqrt(pow(Rdec,2))*Rtel*xd*sin(HA) - 
                Rdec*(pow(Rdec,2) - pow(Rpier,2) - 2*Rdec*Rtel + pow(Rtel,2))*sin(phi)) + 
             cos(phi)*(pow(pow(Rdec,2),1.5)*xd*sin(2*HA) + 
                Rdec*Rtel*sin(dec)*(sqrt(pow(Rdec,2))*xd + sqrt(pow(Rdec,2))*xd*pow(sin(HA),2) - 2*Rdec*Rtel*sin(HA)*sin(phi)))\
              + pow(Rdec,3)*Rtel*sin(HA)*sin(dec)*sin(2*phi)))))/
   (Rdec*Rpier*(pow(cos(phi),2)*pow(sin(dec),2) - 2*cos(HA)*cos(dec)*cos(phi)*sin(dec)*sin(phi) + 
       pow(cos(dec),2)*(pow(sin(HA),2) + pow(cos(HA),2)*pow(sin(phi),2))));
    } else {
        //return nan ;
    }
    return 1./0. ;

}
double LDCutPierLineM3(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier, int EastOfPier, int WestOfPier)
{
    if(( WestOfPier == IS_WEST) &&( EastOfPier == IS_NOT_EAST))
    {
        return (Rdec*Rpier*xd*cos(phi)*sin(dec) - sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) + 
     sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(sin(HA),2)*sin(dec) - 
     Rdec*sqrt(pow(Rdec,2))*Rpier*cos(phi)*sin(HA)*sin(dec)*sin(phi) + sqrt(pow(Rdec,2))*Rpier*Rtel*cos(phi)*sin(HA)*sin(dec)*sin(phi) + 
     sqrt(pow(Rdec,2))*Rpier*Rtel*pow(cos(HA),2)*cos(dec)*sin(dec)*pow(sin(phi),2) + 
     Rpier*cos(HA)*(sqrt(pow(Rdec,2))*Rtel*pow(cos(dec),2)*cos(phi)*sin(phi) - 
        sqrt(pow(Rdec,2))*Rtel*cos(phi)*pow(sin(dec),2)*sin(phi) - 
        cos(dec)*(sqrt(pow(Rdec,2))*(Rdec - Rtel)*pow(cos(phi),2)*sin(HA) + Rdec*xd*sin(phi))) + 
     Csc(HA)*Sec(dec)*sqrt(pow(Rpier,2)*pow(cos(dec),2)*pow(sin(HA),2)*
        (-(pow(Rdec,2)*pow(Rtel,2)*pow(cos(dec),4)*pow(cos(phi),2)*pow(sin(HA),2)) + 
          pow(Rdec,2)*pow(cos(phi),2)*pow(sin(dec),2)*
           (pow(Rpier,2) - pow(Rdec - Rtel,2)*pow(cos(HA),2) - 2*pow(Rtel,2)*cos(HA)*sin(HA)*sin(dec) + 
             Rdec*Rtel*sin(2*HA)*sin(dec) - pow(Rtel,2)*pow(sin(HA),2)*pow(sin(dec),2)) - 
          2*Rdec*Rtel*pow(cos(dec),3)*cos(phi)*sin(HA)*(-(sqrt(pow(Rdec,2))*xd*sin(HA)) + Rdec*(Rdec - Rtel)*sin(phi)) - 
          Rdec*pow(cos(dec),2)*(-2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(cos(HA),2)*sin(HA)*sin(phi) - 
             2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(sin(HA),3)*sin(phi) + 
             Rdec*pow(Rdec - Rtel,2)*pow(sin(HA),4)*pow(sin(phi),2) + 
             Rdec*pow(sin(HA),2)*(-pow(Rpier,2) + pow(xd,2) + 2*pow(Rtel,2)*pow(cos(phi),2)*pow(sin(dec),2) + 
                2*pow(Rdec - Rtel,2)*pow(cos(HA),2)*pow(sin(phi),2)) + 
             Rdec*(-((Rdec - Rtel)*Rtel*pow(cos(phi),2)*sin(2*HA)*sin(dec)) + 
                pow(cos(HA),2)*(-pow(Rpier,2) + pow(Rdec - Rtel,2)*pow(cos(HA),2))*pow(sin(phi),2))) + 
          cos(dec)*sin(dec)*(-(Rdec*sqrt(pow(Rdec,2))*Rtel*xd*pow(cos(HA),2)*cos(phi)*sin(dec)) - 
             2*cos(HA)*cos(phi)*(pow(pow(Rdec,2),1.5)*xd*sin(HA) - 
                pow(Rdec,2)*(pow(Rdec,2) - pow(Rpier,2) - 2*Rdec*Rtel + pow(Rtel,2))*sin(phi)) + 
             Rdec*Rtel*(cos(phi)*(sqrt(pow(Rdec,2))*xd*sin(2*HA) + 
                   sin(dec)*(sqrt(pow(Rdec,2))*xd + sqrt(pow(Rdec,2))*xd*pow(sin(HA),2) - 2*pow(Rdec,2)*sin(HA)*sin(phi))) + 
                Rdec*Rtel*sin(HA)*sin(dec)*sin(2*phi))))))/
   (Rdec*Rpier*(pow(cos(phi),2)*pow(sin(dec),2) - 2*cos(HA)*cos(dec)*cos(phi)*sin(dec)*sin(phi) + 
       pow(cos(dec),2)*(pow(sin(HA),2) + pow(cos(HA),2)*pow(sin(phi),2))));
    }
    else if(( EastOfPier == IS_EAST)  &&( WestOfPier == IS_NOT_WEST))
    {
        return (Rdec*Rpier*xd*cos(phi)*sin(dec) + sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) - 
     sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(sin(HA),2)*sin(dec) + 
     Rdec*sqrt(pow(Rdec,2))*Rpier*cos(phi)*sin(HA)*sin(dec)*sin(phi) - sqrt(pow(Rdec,2))*Rpier*Rtel*cos(phi)*sin(HA)*sin(dec)*sin(phi) - 
     sqrt(pow(Rdec,2))*Rpier*Rtel*pow(cos(HA),2)*cos(dec)*sin(dec)*pow(sin(phi),2) - 
     Rpier*cos(HA)*(sqrt(pow(Rdec,2))*Rtel*pow(cos(dec),2)*cos(phi)*sin(phi) - 
        sqrt(pow(Rdec,2))*Rtel*cos(phi)*pow(sin(dec),2)*sin(phi) + 
        cos(dec)*(-(sqrt(pow(Rdec,2))*(Rdec - Rtel)*pow(cos(phi),2)*sin(HA)) + Rdec*xd*sin(phi))) + 
     Csc(HA)*Sec(dec)*sqrt(pow(Rpier,2)*pow(cos(dec),2)*pow(sin(HA),2)*
        (-(pow(Rdec,2)*pow(Rtel,2)*pow(cos(dec),4)*pow(cos(phi),2)*pow(sin(HA),2)) + 
          pow(Rdec,2)*pow(cos(phi),2)*pow(sin(dec),2)*
           (pow(Rpier,2) - pow(Rdec - Rtel,2)*pow(cos(HA),2) - 2*pow(Rtel,2)*cos(HA)*sin(HA)*sin(dec) + 
             Rdec*Rtel*sin(2*HA)*sin(dec) - pow(Rtel,2)*pow(sin(HA),2)*pow(sin(dec),2)) - 
          2*Rdec*Rtel*pow(cos(dec),3)*cos(phi)*sin(HA)*(sqrt(pow(Rdec,2))*xd*sin(HA) + Rdec*(Rdec - Rtel)*sin(phi)) - 
          Rdec*pow(cos(dec),2)*(2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(cos(HA),2)*sin(HA)*sin(phi) + 
             2*sqrt(pow(Rdec,2))*(Rdec - Rtel)*xd*pow(sin(HA),3)*sin(phi) + 
             Rdec*pow(Rdec - Rtel,2)*pow(sin(HA),4)*pow(sin(phi),2) + 
             Rdec*pow(sin(HA),2)*(-pow(Rpier,2) + pow(xd,2) + 2*pow(Rtel,2)*pow(cos(phi),2)*pow(sin(dec),2) + 
                2*pow(Rdec - Rtel,2)*pow(cos(HA),2)*pow(sin(phi),2)) + 
             Rdec*(-((Rdec - Rtel)*Rtel*pow(cos(phi),2)*sin(2*HA)*sin(dec)) + 
                pow(cos(HA),2)*(-pow(Rpier,2) + pow(Rdec - Rtel,2)*pow(cos(HA),2))*pow(sin(phi),2))) + 
          cos(dec)*sin(dec)*(Rdec*sqrt(pow(Rdec,2))*Rtel*xd*pow(cos(HA),2)*cos(phi)*sin(dec) - 
             2*Rdec*cos(HA)*cos(phi)*(sqrt(pow(Rdec,2))*Rtel*xd*sin(HA) - 
                Rdec*(pow(Rdec,2) - pow(Rpier,2) - 2*Rdec*Rtel + pow(Rtel,2))*sin(phi)) + 
             cos(phi)*(pow(pow(Rdec,2),1.5)*xd*sin(2*HA) - 
                Rdec*Rtel*sin(dec)*(sqrt(pow(Rdec,2))*xd + sqrt(pow(Rdec,2))*xd*pow(sin(HA),2) + 
                   2*pow(Rdec,2)*sin(HA)*sin(phi))) + pow(Rdec,2)*pow(Rtel,2)*sin(HA)*sin(dec)*sin(2*phi)))))/
   (Rdec*Rpier*(pow(cos(phi),2)*pow(sin(dec),2) - 2*cos(HA)*cos(dec)*cos(phi)*sin(dec)*sin(phi) + 
       pow(cos(dec),2)*(pow(sin(HA),2) + pow(cos(HA),2)*pow(sin(phi),2))))
 ;
    } else {
	//return nan ;
    }
    return 1./0. ;
}
