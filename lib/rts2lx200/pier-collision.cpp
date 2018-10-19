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

#include <cmath>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <libnova/libnova.h>

#include "rts2lx200/pier-collision.h"

#include "utilsfunc.h"

#define Csc(x) (1./sin(x))
#define Sec(x) (1./cos(x))


double LDRAtoHA( double RA, double longitude) ;
int    LDCollision( double RA, double dec, double lambda, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier) ;
double LDTangentPlaneLineP(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double tp) ;
double LDTangentPlaneLineM(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double tp) ;
double LDCutPierLineP1(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier) ;
double LDCutPierLineM1(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier) ;
double LDCutPierLineP3(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier) ;
double LDCutPierLineM3(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier) ;

struct pier
{
	double radius;
	double wedge;
	double floor;
	double danger_zone_below;
	double danger_zone_above;
} pr;

struct mount
{
	double xd ;
	double zd ;
	double rdec ;
} mt;

struct telescope
{
	double radius ;
	double rear_length ;
} tel;
/* the main entry function */
/* struct ln_equ_posn tel_eq telescope target coordinates */
/* WEST: DEC axis points to HA +M_PI/2, IS_EAST: DEC axis points to HA - M_PI/2*/
/* DEC axis is the vector which points from the intersection of the HA axis with DEC axis tp the intersection*/
/* of the DEC axis with the optical axis, the optical axis points to HA, DEC) */
/* longitude positive to the East*/
int  pier_collision (struct ln_equ_posn *tel_equ, struct ln_lnlat_posn *obs)
{
	struct ln_equ_posn tmp_equ ;

	pr.radius= 0.135;
	pr.wedge= -0.6684;
	pr.floor= -1.9534;
	pr.danger_zone_below= -2.9534;
	pr.danger_zone_above= -0.1;

	mt.xd = -0.0684;
	mt.zd = -0.1934;
	mt.rdec = 0.338;

	tel.radius= 0.123;
	tel.rear_length = 1.1; // 0.8 + 0.3 for FLI equipment

	if ((tel_equ->dec > 90.) && ( tel_equ->dec <= 270.)) // EAST: DECaxis==HA - M_PI/2
	{
		tmp_equ.ra = tel_equ->ra - 180.;
		tmp_equ.dec = -tel_equ->dec;
	}
	else // WEST: DECaxis==HA + M_PI/2
	{
		tmp_equ.ra =  tel_equ->ra;
		tmp_equ.dec=  tel_equ->dec;
	}
	return LDCollision (tmp_equ.ra/180. * M_PI, tmp_equ.dec/180. * M_PI, obs->lng/180.*M_PI, obs->lat/180.*M_PI, mt.zd, mt.xd, mt.rdec, tel.radius, pr.radius);
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
/* I copied the whole expressions from the Mathematica notebook. */

double LDRAtoHA (double RA, double longitude)
{
	double HA;
	double JD;
	double theta_0 = 0.;

	JD = ln_get_julian_from_sys ();

	theta_0 = 15. / 180. * M_PI * ln_get_mean_sidereal_time (JD);
	/* negative to the West, Kstars neg., XEpehem pos */
	HA =  fmod(theta_0 + longitude - RA + 2. * M_PI,  2. * M_PI) ;
	return HA ;
}

int LDCollision( double RA, double dec, double lambda, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier)
{
  int state_collision= NO_COLLISION ;
  int state_danger   = NO_DANGER ;

  double tpp1= NAN;
  double tpm1= NAN;
  double czp1= NAN;
  double czm1= NAN;

  double tpp3= NAN;
  double tpm3= NAN;
  double czp3= NAN;
  double czm3= NAN;

  double HA ;

  HA= LDRAtoHA( RA, lambda) ;

/* Parameter of the straight line */

  tpp1= LDCutPierLineP1(HA, dec, phi, zd, xd, Rdec, Rtel, Rpier);
  tpm1= LDCutPierLineM1(HA, dec, phi, zd, xd, Rdec, Rtel, Rpier);

  tpp3= LDCutPierLineP3(HA, dec, phi, zd, xd, Rdec, Rtel, Rpier);
  tpm3= LDCutPierLineM3(HA, dec, phi, zd, xd, Rdec, Rtel, Rpier);

/* z componet of the intersection */

  if( !std::isnan(tpp1)){
    czp1= LDTangentPlaneLineP(HA, dec, phi, zd, xd, Rdec, Rtel, tpp1) ;
  } else {
    //fprintf( stderr, "LDCollision tpp1==nan\n") ;
  }
  if( !std::isnan(tpm1)){
    czm1= LDTangentPlaneLineM(HA, dec, phi, zd, xd, Rdec, Rtel, tpm1) ;
  } else {
    //fprintf( stderr, "LDCollision tpm1==nan\n") ;
  }
  if( !std::isnan(tpp3)) {
    czp3= LDTangentPlaneLineP(HA, dec, phi, zd, xd, Rdec, Rtel, tpp3) ;
  } else {
    //fprintf( stderr, "LDCollision tpp3==nan\n") ;
  }
  if( !std::isnan(tpm3)) {
    czm3= LDTangentPlaneLineM(HA, dec, phi, zd, xd, Rdec, Rtel, tpm3) ;
  }  else {
    //fprintf( stderr, "LDCollision tpm3==nan\n") ;
  }

  if( !std::isnan(czp1)) {
    //if((czp1 > PierN[2].value) && ( czp1 < PierN[1].value))
    if((czp1 > pr.floor) && ( czp1 < pr.wedge)) {
		if( !std::isnan(tpp1)){
	if( fabs(tpp1) > tel.rear_length){
	  fprintf( stderr, "LDCollision NO_COLLISION czp1, tpp1 %f> %f\n", fabs(tpp1), tel.rear_length) ;
	  if( state_collision != COLLIDING) {
	    state_collision= NO_COLLISION ;
	  }
	} else {
	  fprintf(stderr, "Collision 1 on the positive side\n" ) ;
	  state_collision= COLLIDING ;
	}
      }
      //else  if((czp1 > PierN[3].value) && ( czp1 <= PierN[2].value))
    } else  if((czp1 > pr.danger_zone_below) && ( czp1 <= pr.floor)){
      fprintf(stderr, "Within danger zone below 1 on the positive side\n") ;
      state_danger= WITHIN_DANGER_ZONE_BELOW ;
      //else  if((czp1 < PierN[4].value) && ( czp1 >= PierN[1].value))
    } else  if((czp1 < pr.danger_zone_above) && ( czp1 >= pr.wedge)){
      fprintf(stderr, "Within danger zone above 1 on the positive side\n") ;
      state_danger= WITHIN_DANGER_ZONE_ABOVE ;
    }
  } else {
    //fprintf( stderr, "LDCollision czp1==nan\n") ;
  }
  if(  !std::isnan(czm1)){
    //if((czm1 > PierN[2].value) && ( czm1 < PierN[1].value))
    if((czm1 > pr.floor) && ( czm1 < pr.wedge)){
		if( !std::isnan(tpm1)){
	if( fabs(tpm1) > tel.rear_length){
	  fprintf( stderr, "LDCollision NO_COLLISION czm1, tpm1 %f> %f\n", fabs(tpm1), tel.rear_length) ;
	  if( state_collision != COLLIDING) {
	    state_collision= NO_COLLISION ;
	  }
	} else {
	  fprintf(stderr, "Collision 1 on the negative side\n" ) ;
	  state_collision= COLLIDING ;
	}
      }
      //else  if((czm1 > PierN[3].value) && ( czm1 <= PierN[2].value))
    } else  if((czm1 > pr.danger_zone_below) && ( czm1 <= pr.floor)){
      fprintf(stderr, "Within danger zone below 1 on the negative side\n") ;
      state_danger= WITHIN_DANGER_ZONE_BELOW ;
      //else  if((czm1 < PierN[4].value) && ( czm1 >= PierN[1].value))
    } else  if((czm1 < pr.danger_zone_above) && ( czm1 >= pr.wedge)){
      fprintf(stderr, "Within danger zone above 1 on the negative side\n") ;
      state_danger= WITHIN_DANGER_ZONE_ABOVE ;
    }
  } else {
    //fprintf( stderr, "LDCollision czm1==nan\n") ;
  }
  if(  !std::isnan(czp3)) {
    //if((czp3 > PierN[2].value) && ( czp3 < PierN[1].value))
    if((czp3 > pr.floor) && ( czp3 < pr.wedge)){
		if( !std::isnan(tpp3)){
	if( fabs(tpp3) > tel.rear_length){
	  fprintf( stderr, "LDCollision NO_COLLISION czp3, tpp3 %f> %f\n", fabs(tpp3), tel.rear_length) ;
	  if( state_collision != COLLIDING) {
	    state_collision= NO_COLLISION ;
	  }
	} else {
	  fprintf(stderr, "Collision 3 on the positive side\n" ) ;
	  state_collision= COLLIDING ;
	}
      }
      //else  if((czp3 > PierN[3].value) && ( czp3 <= PierN[2].value))
    } else  if((czp3 > pr.danger_zone_below) && ( czp3 <= pr.floor)) {
      fprintf(stderr, "Within danger zone below 3 on the positive side\n") ;
      state_danger= WITHIN_DANGER_ZONE_BELOW ;
      //else  if((czp3 < PierN[4].value) && ( czp3 >= PierN[1].value))
    } else  if((czp3 < pr.danger_zone_above) && ( czp3 >= pr.wedge)){
      fprintf(stderr, "Within danger zone above 3 on the positive side\n") ;
      state_danger= WITHIN_DANGER_ZONE_ABOVE ;
    }
  } else {
    //fprintf( stderr, "LDCollision czp3==nan\n") ;
  }
  if(  !std::isnan(czm3)) {
    //if((czm3 > PierN[2].value) && ( czm3 < PierN[1].value))
    if((czm3 > pr.floor) && ( czm3 < pr.wedge)){
		if( !std::isnan(tpm3)){
	if( fabs(tpm3) > tel.rear_length){
	  fprintf( stderr, "LDCollision NO_COLLISION czm3, tpm3 %f> %f\n", fabs(tpm3), tel.rear_length) ;
	  if( state_collision != COLLIDING) {
	    state_collision= NO_COLLISION ;
	  }
	} else {
	  fprintf(stderr, "Collision 3 on the negative side\n" ) ;
	  state_collision= COLLIDING ;
	}
      }
      //!!if((czm3 > PierN[3].value) && ( czm3 <= PierN[2].value))
    } else if((czm3 > pr.danger_zone_below) && ( czm3 <= pr.floor)){
      fprintf(stderr, "Within danger zone below 3 on the negative side\n") ;
      state_danger= WITHIN_DANGER_ZONE_BELOW ;
      //else  if((czm3 < PierN[4].value) && ( czm3 >= PierN[1].value))
    } else  if((czm3 < pr.danger_zone_above) && ( czm3 >= pr.wedge)){
      fprintf(stderr, "Within danger zone above 3 on the negative side\n") ;
      state_danger= WITHIN_DANGER_ZONE_ABOVE ;
    }
  } else {
    //fprintf( stderr, "LDCollision czm3==nan\n") ;
  }
  if( state_collision != NO_COLLISION) {
    return COLLIDING ;
  } else if (state_danger != NO_DANGER) {
    return WITHIN_DANGER_ZONE ;
  }
  return NO_COLLISION ;
}
double LDTangentPlaneLineP(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double tp)
{
  double val= NAN;
  val = zd - sqrt(pow(Rdec,2))*cos(phi)*sin(HA) + (Rdec*Rtel*cos(phi)*sin(HA))/sqrt(pow(Rdec,2)) +
    (Rdec*Rtel*(cos(HA)*cos(phi)*sin(dec) - cos(dec)*sin(phi)))/sqrt(pow(Rdec,2)) + tp*(cos(HA)*cos(dec)*cos(phi) + sin(dec)*sin(phi)) ;

  //  fprintf( stderr, "====LDTangentPlaneLineP: WEST %+010.5f\n", val) ;

  return val ;
}

double LDTangentPlaneLineM(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double tp)
{
  double val= NAN;
  val=   zd - sqrt(pow(Rdec,2))*cos(phi)*sin(HA) + (Rdec*Rtel*cos(phi)*sin(HA))/sqrt(pow(Rdec,2)) +
	   (Rdec*Rtel*(-(cos(HA)*cos(phi)*sin(dec)) + cos(dec)*sin(phi)))/sqrt(pow(Rdec,2)) + tp*(cos(HA)*cos(dec)*cos(phi) + sin(dec)*sin(phi));

  //  fprintf( stderr, "----LDTangentPlaneLineM: WEST %+010.5f\n\n", val ) ;
    return val ;
}


double LDCutPierLineP1(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier)
{
  double val= NAN;

  val=  (Rdec*Rpier*xd*cos(phi)*sin(dec) + sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) -
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

  //  fprintf( stderr, "____LDCutPierLineP1: WEST %+010.5f\n", val) ;
  return val ;
}
double LDCutPierLineM1(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier)
{
  double val= NAN;

  val= (Rdec*Rpier*xd*cos(phi)*sin(dec) - sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) +
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


  //  fprintf( stderr, "xxxxLDCutPierLineM1: WEST %+010.5f\n", val) ;
  return val ;
}
double LDCutPierLineP3(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier)
{
  double val= NAN;
  val=  (Rdec*Rpier*xd*cos(phi)*sin(dec) + sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) -
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

  //  fprintf( stderr, "-x-xLDCutPierLineP3: WEST %+010.5f\n", val ) ;
  return val ;
}
double LDCutPierLineM3(double HA, double dec, double phi, double zd, double xd, double Rdec, double Rtel, double Rpier)
{
  double val= NAN;
  val=  (Rdec*Rpier*xd*cos(phi)*sin(dec) - sqrt(pow(Rdec,2))*Rpier*Rtel*cos(dec)*pow(cos(phi),2)*sin(dec) +
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

  //  fprintf( stderr, "X-X-LDCutPierLineM3: WEST %+010.5f\n", val ) ;
    return val ;
}
