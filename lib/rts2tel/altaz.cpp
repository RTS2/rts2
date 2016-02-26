/*
 * Abstract class for Alt-Az mounts.
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
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
#include "configuration.h"

#include "libnova_cpp.h"

using namespace rts2teld;

AltAz::AltAz (int in_argc, char **in_argv, bool diffTrack, bool hasTracking, bool hasUnTelCoordinates):Telescope (in_argc, in_argv, diffTrack, hasTracking, hasUnTelCoordinates)
{
	createValue (az_ticks, "_az_ticks", "[cnts] counts per full revolution on az axis", false);
	createValue (alt_ticks, "_alt_ticks", "[cnts] counts per full revolution on alt axis", false);

	createValue (azZero, "_az_zero", "[deg] azimuth zero offset", false);
	createValue (altZero, "_alt_zero", "[deg] altitude zero offset", false);

	createValue (azCpd, "_az_cpd", "[cnts] azimuth counts per azimth degree", false);
	createValue (altCpd, "_alt_cpd", "[cnts] altitude counts per azimth degree", false);

	createValue (azMin, "_az_min", "[cnts] minimal azimuth axis count", false);
	createValue (azMax, "_az_max", "[cnts] maximal azimuth axis count", false);
	createValue (altMin, "_alt_min", "[cnts] minimal altitude axis count", false);
	createValue (altMax, "_alt_max", "[cnts] maximal altitude axis count", false);
}

AltAz::~AltAz (void)
{
}

int AltAz::sky2counts (double JD, struct ln_equ_posn *pos, int32_t &altc, int32_t &azc, bool writeValue, double haMargin)
{
	struct ln_hrz_posn hrz;
	getHrzFromEqu (pos, JD, &hrz);

	int32_t d_alt = altc - (hrz.alt - altZero->getValueDouble ()) * altCpd->getValueDouble ();
	int32_t d_az = azc - (hrz.az - azZero->getValueDouble ()) * azCpd->getValueDouble ();

	d_alt %= alt_ticks->getValueLong ();
	d_az %= az_ticks->getValueLong ();

	if (d_alt > alt_ticks->getValueLong () / 2.0)
		d_alt -= alt_ticks->getValueLong ();

	if (d_az > az_ticks->getValueLong () / 2.0)
		d_az -= az_ticks->getValueLong ();

	int32_t t_alt = altc + d_alt;
	int32_t t_az = azc + d_az;

	while (t_alt < altMin->getValueLong ())
		t_alt += alt_ticks->getValueLong ();
	while (t_alt > altMax->getValueLong ())
		t_alt -= alt_ticks->getValueLong ();
	
	while (t_az < azMin->getValueLong ())
		t_az += az_ticks->getValueLong ();
	while (t_az > azMax->getValueLong ())
		t_az -= az_ticks->getValueLong ();

	if (t_alt > altMin->getValueLong () && t_alt < altMax->getValueLong () && t_az > azMin->getValueLong () && t_az < azMax->getValueLong ())
		return 0;

	return -1;
}

void AltAz::unlockPointing ()
{
	az_ticks->setWritable ();
	alt_ticks->setWritable ();

	azZero->setWritable ();
	altZero->setWritable ();
	azCpd->setWritable ();
	altCpd->setWritable ();

	azMin->setWritable ();
	azMax->setWritable ();
	altMin->setWritable ();
	altMax->setWritable ();

	updateMetaInformations (az_ticks);
	updateMetaInformations (alt_ticks);

	updateMetaInformations (azZero);
	updateMetaInformations (altZero);
	updateMetaInformations (azCpd);
	updateMetaInformations (altCpd);

	updateMetaInformations (azMin);
	updateMetaInformations (azMax);
	updateMetaInformations (altMin);
	updateMetaInformations (altMax);
}
