/* 
 * Obs. Vermes cupola driver.
 * Copyright (C) 2010 Markus Wildi <markus.wildi@one-arcsec.org>
 * based on Petr Kubanek's dummy_cup.cpp
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "cupola.h"
#include "../utils/rts2config.h" 

#ifdef __cplusplus
extern "C"
{
#endif
// wildi ToDo: go to dome-target-az.h
double dome_target_az( struct ln_equ_posn tel_eq, int angle, struct ln_lnlat_posn *obs) ;
#ifdef __cplusplus
}
#endif

using namespace rts2dome;

namespace rts2dome
{
/**
 * Obs. Vermes cupola driver.
 *
 * @author Markus Wildi <markus.wildi@one-arcsec.org>
 */
  class Vermes:public Cupola
  {
  private:
    const char *device_ssd650v;
    const char *device_bcr_lft; 
    const char *device_bcr_rgt; 
    struct ln_lnlat_posn *obs ;
    struct ln_equ_posn tel_eq ;
    Rts2Config *config ;

  protected:
    virtual int moveStart () ;
    virtual int moveEnd () ;
    virtual long isMoving () ;
    // there is no dome door to open 
    virtual int startOpen (){return 0;}
    virtual long isOpened (){return -2;}
    virtual int endOpen (){return 0;}
    virtual int startClose (){return 0;}
    virtual long isClosed (){return -2;}
    virtual int endClose (){return 0;}

  public:
    Vermes (int argc, char **argv) ;
    virtual int initValues () ;
    virtual double getSplitWidth (double alt) ;
  };
}

int Vermes::moveEnd ()
{
  //	logStream (MESSAGE_ERROR) << "Vermes::moveEnd set Az "<< hrz.az << sendLog ;
  logStream (MESSAGE_ERROR) << "Vermes::moveEnd did nothing "<< sendLog ;
  return Cupola::moveEnd ();
}
long Vermes::isMoving ()
{
  logStream (MESSAGE_DEBUG) << "Vermes::isMoving"<< sendLog ;
  if ( 1) // if there, return -2
    return -2;
  return USEC_SEC;
}
int Vermes::moveStart ()
{
  tel_eq.ra= getTargetRa() ;
  tel_eq.dec= getTargetDec() ;

  logStream (MESSAGE_ERROR) << "Vermes::moveStart RA " << tel_eq.ra  << " Dec " << tel_eq.dec << sendLog ;

  double target_az= -1. ;
  target_az= dome_target_az( tel_eq, -1,  obs) ;
  
  logStream (MESSAGE_ERROR) << "Vermes::moveStart idome target az" << target_az << sendLog ;
  setCurrentAz (target_az);

  return Cupola::moveStart ();
}
int Vermes::initValues ()
{
  int ret ;
  config = Rts2Config::instance ();

  ret = config->loadFile ();
  if (ret)
    return -1;
  obs= Cupola::getObserver() ;

  return Cupola::initValues ();
}
Vermes::Vermes (int in_argc, char **in_argv):Cupola (in_argc, in_argv) 
{
  // since this driver is Obs. Vermes specific no options are really required
  device_ssd650v = "/dev/ssd650v";
  device_bcr_lft = "/dev/bcreader_lft"; 
  device_bcr_rgt = "/dev/bcreader_rgt"; 
}
double Vermes::getSplitWidth (double alt)
{
  return 1;
}

int main (int argc, char **argv)
{
	Vermes device (argc, argv);
	return device.run ();
}
