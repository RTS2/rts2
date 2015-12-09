/* 
 * Irait telescope driver.
 * Copyright (C) 2014 Jean Marc Christille
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

#include "altaz.h"
#include "hoststring.h"

#include <libnova/libnova.h>
#include <stdio.h>
#include <unistd.h>

#include "irait/galil.h"
#include "irait/iraitenc.h"

#include <pthread.h>

#define OPT_CONN_DEBUG     OPT_LOCAL + 2000
#define OPT_GAL            OPT_LOCAL + 2001
#define OPT_ENC            OPT_LOCAL + 2002

namespace rts2teld

{

class Irait:public AltAz
{
	public:
		Irait (int argc, char **argv);
		virtual ~Irait ();

		// run tracking in a loop
		virtual void trackingThread ();

        protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual int isMoving ();
		virtual int startResync ();
		virtual int stopMove ();
		virtual int startPark ();
		virtual int endPark ();

	private:
                HostString *ip_galil;
		HostString *ip_iraitenc;

		rts2teld::Galil *connGalil;
		rts2teld::IraitEnc *connIraitEnc;

		pthread_t threadTracking;
};

}

using namespace rts2teld;

static void* thread_tracking (void *arg)
{
	Irait *tel = (Irait *) arg;
	tel->trackingThread ();
	return NULL;
}

Irait::Irait (int argc, char **argv):AltAz (argc, argv)
{
	ip_galil = NULL;
	ip_iraitenc = NULL;

	connGalil = NULL;
	connIraitEnc = NULL;

        //ip_galil="10.10.18.150"
	//ip_encoder="10.10.18.211"
	//connDebug = false;

	addOption (OPT_GAL, "gal", 1, "Galil IP address and hostname (default to 10.10.18.150:1000)");
	addOption (OPT_ENC, "enc", 1, "Irait Encoders IP address and hostname (default to 10.10.18.215:3490)");
}

Irait::~Irait (void)
{
	delete connGalil;
	delete connIraitEnc;
}

void Irait::trackingThread ()
{
	// if tracking then compute difference and set galil speed
  
        struct ln_equ_posn target;
	double deltat=0.4;   //in secondi
	double destPos = 0;
	double currPos = 0;
        struct ln_lnlat_posn observatory;
        observatory.lat = -75;
        observatory.lng = 123;
	getTarget (&target);
	
        if (currPos-destPos < 20)
        {
                double JD = ln_get_julian_from_sys ();
		//getTarget (&target);

                struct ln_hrz_posn hrz1, hrz2;

                ln_get_hrz_from_equ (&target, &observatory, JD, &hrz1);
                ln_get_hrz_from_equ (&target, &observatory, JD +deltat/86400.0, &hrz2);

                //printf ("Pos 1 [alt-az]: %f %f  Pos 2 [alt-az]: %f %f   DifferenceVector [alt-az]: %f %f\n", hrz1.alt,hrz1.az, hrz2.alt, hrz2.az, (hrz1.alt - hrz2.alt)*3600, (hrz1.az - hrz2.az)*3600);
		connGalil->setXMotVel ((hrz1.az - hrz2.az)*3600/deltat);
		connGalil->setYMotVel ((hrz1.alt - hrz2.alt)*3600/deltat);
		// inseguire con la velocità ovvero delta coordinate diviso il lasso di tempo del refresh
                usleep (deltat*1000000);
        }

	if (currPos-destPos > 20)
        {
                double JD = ln_get_julian_from_sys ();
		//getTarget (&target);

                struct ln_hrz_posn hrz1, hrz2;

                ln_get_hrz_from_equ (&target, &observatory, JD, &hrz1);
                ln_get_hrz_from_equ (&target, &observatory, JD +deltat/86400.0, &hrz2);

		connGalil->setXMotVel((hrz1.az - hrz2.az)*3600/deltat);  // da cambiare con goto arcsec del galil
		connGalil->setYMotVel((hrz1.alt - hrz2.alt)*3600/deltat);
		// inseguire con la velocità ovvero delta coordinate diviso il lasso di tempo del refresh
                usleep (deltat*1000000);
        }


}

int Irait::processOption (int in_opt)
{
	switch (in_opt)
	{
	        case OPT_GAL:
			ip_galil = new HostString (optarg, "1000");
			break;
	        case OPT_ENC:
		        ip_iraitenc = new HostString (optarg,"3490");
			break;
		default:
			return AltAz::processOption (in_opt);
	}
	return 0;
}

int Irait::initHardware ()
{
	connGalil = new Galil (this, ip_galil->getHostname (), ip_galil->getPort ());
	connGalil->setDebug (getDebug ());

	connIraitEnc = new IraitEnc (this, ip_iraitenc->getHostname (), ip_iraitenc->getPort ());
	connIraitEnc->setDebug (getDebug ());

	int ret = connGalil->init ();
	if (ret)
		return ret;

	ret = connIraitEnc->init();
	if (ret)
		return ret;

	pthread_create (&threadTracking, NULL, &thread_tracking, this);

	return 0;
}

int Irait::isMoving ()
{
	return 0;
}

int Irait::startResync ()
{
        return 0;
}

int Irait::stopMove ()
{
        return 0;
}

int Irait::startPark ()
{
        return 0;
}

int Irait::endPark ()
{
        return 0;
}

int main (int argc, char **argv)
{
	Irait device (argc, argv);
	return device.run ();
}
