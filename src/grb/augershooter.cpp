/* 
 * Receive and reacts to Auger showers.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#include "augershooter.h"
#include "libnova_cpp.h"
#include "command.h"

using namespace rts2grbd;

#define OPT_TRIGERING     OPT_LOCAL + 707

DevAugerShooter::DevAugerShooter (int in_argc, char **in_argv):DeviceDb (in_argc, in_argv, DEVICE_TYPE_AUGERSH, "AUGRSH")
{
	shootercnn = NULL;
	port = 1240;
	testParsing = NULL;

	addOption ('s', "shooter_port", 1, "port on which to listen for auger connection");
	addOption (OPT_TRIGERING, "disable-triggering", 0, "only record triggers, do not pass them to executor");
	addOption ('t', NULL, 1, "test parsing of auger data");

	createValue (triggeringEnabled, "triggering_enabled", "if true, shooter will trigger executor", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	triggeringEnabled->setValueBool (true);

	createValue (lastAugerSeen, "last_rejected", "date of last rejected shower", false, RTS2_VALUE_AUTOSAVE);
	createValue (lastAugerDate, "last_date", "date of last accepted shower", false, RTS2_VALUE_AUTOSAVE);
	createValue (lastAugerMessage, "last_message", "date of last message", false, RTS2_VALUE_AUTOSAVE);
	createValue (lastAugerRa, "last_ra", "RA of last shower", false, RTS2_DT_RA | RTS2_VALUE_AUTOSAVE);
	createValue (lastAugerDec, "last_dec", "DEC of last shower", false, RTS2_DT_DEC | RTS2_VALUE_AUTOSAVE);


	createValue (EyeId1, "Eye_Id_cut1", "Eye Id, cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	EyeId1->setValueInteger (1);

	createValue (minEnergy, "min_energy_cut1", "minimal shower energy, cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minEnergy->setValueDouble (10);

	createValue (maxXmaxErr, "max_xmax_err_cut1", "Maximal XmaxErr value, cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxXmaxErr->setValueDouble (40);

	createValue (maxEnergyDiv, "max_energy_div_cut1", "Maximal energy div (EnergyErr / Energy), cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxEnergyDiv->setValueDouble (0.2);

	createValue (maxGHChiDiv, "max_GHChi_div_cut1", "Maximal GHChi div (GHChi2 / GHNdf), cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxGHChiDiv->setValueDouble (2.5);

	createValue (minLineFitDiff, "min_LineFit_diff_cut1", "Minimal line fit difference (LineFitChi2 - GHChi2), cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minLineFitDiff->setValueDouble (4);

	createValue (maxAxisDist, "min_Axis_dist_cut1", "Maximal axis distance, cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxAxisDist->setValueDouble (2000.0);

	createValue (minRp, "min_Rp_cut1", "Minimal Rp Value, cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minRp->setValueDouble (0);

	createValue (minChi0, "min_Chi0_cut1", "Minimal Chi0 value, cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minChi0->setValueDouble (0);

	createValue (maxSPDDiv, "max_SPDDiv_cut1", "Maximal SPD Div, cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxSPDDiv->setValueDouble (7);
	
	createValue (maxTimeDiv, "max_TimeDiv_cut1", "Maximal time difference, cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxTimeDiv->setValueDouble (8);

	createValue (maxTheta, "max_Theta_cut1", "Maximal Theta value, cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxTheta->setValueDouble (70);

	createValue (maxTime, "max_time_cut1", "maximal time between shower and its observations, cut1", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxTime->setValueInteger (600);

 /*       second set of cuts         */
	createValue (EyeId2, "Eye_Id_cut2", "Eye Id, cut2", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	EyeId2->setValueInteger (1);

	createValue (minEnergy2, "min_energy_cut2", "minimal shower energy, cut2", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minEnergy2->setValueDouble (10.);

	createValue (minPix2, "min_Pix_cut2", "Minimal number of pixels, cut2", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minPix2->setValueInteger (6);

	createValue (maxAxisDist2, "min_Axis_dist_cut2", "Maximal axis distance, cut2", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxAxisDist2->setValueDouble (1500.0);

	createValue (maxTimeDiff2, "max_Time_Diff_cut2", "Maximal FD/SD time difference, cut2", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxTimeDiff2->setValueDouble (300.0);

	createValue (maxGHChiDiv2, "max_GHChi_div_cut2", "Maximal GHChi div (GHChi2 / GHNdf), cut2", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxGHChiDiv2->setValueDouble (6.);

	createValue (maxLineFitDiv2, "max_LineFit_div_cut2", "Minimal line fit difference (GHChi2 / LineFitChi2), cut2", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxLineFitDiv2->setValueDouble (0.9);

	createValue (minViewAngle2, "min_View_Angle_cut2", "Minimal viewing angle, cut2", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minViewAngle2->setValueDouble (15.0);
 /*       second set of cuts - end   */

 /*       third set of cuts          */
	createValue (EyeId3, "Eye_Id_cut3", "Eye Id, cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	EyeId3->setValueInteger (1);

	createValue (minEnergy3, "min_energy_cut3", "minimal shower energy, cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minEnergy3->setValueDouble (10.);

	createValue (maxXmaxErr3, "max_xmax_err_cut3", "Maximal XmaxErr value, cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxXmaxErr3->setValueDouble (80.);

	createValue (maxEnergyDiv3, "max_energy_div_cut3", "Maximal energy div (EnergyErr / Energy), cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxEnergyDiv3->setValueDouble (0.4);

	createValue (maxGHChiDiv3, "max_GHChi_div_cut3", "Maximal GHChi div (GHChi2 / GHNdf), cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxGHChiDiv3->setValueDouble (10.);

	createValue (maxLineFitDiv3, "max_LineFit_div_cut3", "Minimal line fit difference (GHChi2 / LineFitChi2), cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxLineFitDiv3->setValueDouble (1.);

	createValue (minPix3, "min_Pix_cut3", "Minimal number of pixels, cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minPix3->setValueInteger (10);

	createValue (maxAxisDist3, "min_Axis_dist_cut3", "Maximal axis distance, cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxAxisDist3->setValueDouble (2000.0);

	createValue (minRp3, "min_Rp_cut3", "Minimal Rp Value, cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minRp3->setValueDouble (0.);

	createValue (minChi03, "min_Chi0_cut3", "Minimal Chi0 value, cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minChi03->setValueDouble (0.);

	createValue (maxSPDDiv3, "max_SPDDiv_cut3", "Maximal SPD Div, cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxSPDDiv3->setValueDouble (7.);
	
	createValue (maxTimeDiv3, "max_TimeDiv_cut3", "Maximal time difference, cut3", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxTimeDiv3->setValueDouble (8.);
 /*       third set of cuts - end    */

 /*       fourth set of cuts         */
	createValue (EyeId4, "Eye_Id_cut4", "Eye Id, cut4", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	EyeId4->setValueInteger (1);

	createValue (minNPix4, "min_npix_cut4", "minimal number of pixels, cut4", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	minNPix4->setValueInteger (10);

	createValue (maxDGHChi2Improv4, "max_dghchi2_improv4", "maximal DGHChi2 Improv, cut4", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	maxDGHChi2Improv4->setValueDouble (-0.2);

 /*       fourth set of cuts - end   */

 /*       fifth set of cuts         */
	createValue (EyeId5, "Eye_Id_cut5", "Eye Id, cut5", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	EyeId5->setValueInteger (1);

	createValue (XmaxEnergyShift5, "XMax_energy_shift_cut5", "XMax energy shift factor, cut5", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	XmaxEnergyShift5->setValueDouble (900);

	createValue (XmaxEnergyLin5, "XMax_energy_lin_cut5", "XMax energy linear factor, cut5", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	XmaxEnergyLin5->setValueDouble (50);

	createValue (XmaxErr5, "XMax_err_cut5", "XMax error, cut5", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	XmaxErr5->setValueDouble (50);

	createValue (Energy5, "Energy_cut5", "minimal energy, cut5", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	Energy5->setValueDouble (0.6);

	createValue (ChkovFrac5, "Chkov_Frac_cut5", "cherenkov fraction, cut5", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	ChkovFrac5->setValueDouble (50);
 /*       fifth set of cuts - end   */
}

DevAugerShooter::~DevAugerShooter (void)
{
}

int DevAugerShooter::processOption (int in_opt)
{
	switch (in_opt)
	{
	  	case 't':
			testParsing = optarg;
			setNotDaemonize ();
			break;
		case 's':
			port = atoi (optarg);
			break;
		case OPT_TRIGERING:
			triggeringEnabled->setValueBool (false);
			break;
		default:
			return DeviceDb::processOption (in_opt);
	}
	return 0;
}

int DevAugerShooter::init ()
{
	int ret;
	ret = DeviceDb::init ();
	if (ret)
		return ret;

	shootercnn = new ConnShooter (port, this);

	if (testParsing)
	{
		shootercnn->processBuffer (testParsing);
		return -1;
	}

	ret = shootercnn->init ();

	if (ret)
		return ret;

	addConnection (shootercnn);

	return ret;
}

void DevAugerShooter::augerMessage ()
{
	lastAugerMessage->setValueDouble (getNow ());
	sendValueAll (lastAugerMessage);
}

void DevAugerShooter::rejectedShower (double lastDate, __attribute__((unused)) double ra, __attribute__((unused)) double dec)
{
	lastAugerSeen->setValueDouble (lastDate);
	sendValueAll (lastAugerSeen);
}

void DevAugerShooter::newShower (double lastDate, double ra, double dec)
{
	if (triggeringEnabled->getValueBool () == false)
	{
		logStream (MESSAGE_WARNING) << "Triggering is disabled, trigger " << LibnovaDateDouble (lastDate) << "not executed." << sendLog;
	}
	rts2core::Connection *exec;
	lastAugerDate->setValueDouble (lastDate);
	lastAugerRa->setValueDouble (ra);
	lastAugerDec->setValueDouble (dec);

	sendValueAll (lastAugerDate);
	sendValueAll (lastAugerRa);
	sendValueAll (lastAugerDec);

	exec = getOpenConnection (DEVICE_TYPE_EXECUTOR);
	if (exec)
	{
		exec->queCommand (new rts2core::CommandExecShower (this));
	}
	else
	{
		logStream (MESSAGE_ERROR) << "FATAL! No executor running to post shower!" << sendLog;
	}
}

bool DevAugerShooter::wasSeen (double lastDate, double ra, double dec)
{
	return (fabs (lastDate - lastAugerDate->getValueDouble ()) < 5
		&& ra == lastAugerRa->getValueDouble ()
		&& dec == lastAugerDec->getValueDouble ());
}

int main (int argc, char **argv)
{
	DevAugerShooter device (argc, argv);
	return device.run ();
}
