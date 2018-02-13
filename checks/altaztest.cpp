#include "altaztest.h"

AltAzTest::AltAzTest (int argc, char **argv):rts2teld::AltAz (argc, argv)
{
}

AltAzTest::~AltAzTest ()
{
}

void AltAzTest::setTelescope (double _lat, double _long, double _alt, long _az_ticks, long _alt_ticks, double _azZero, double _zdZero, double _azCpd, double _altCpd, long _azMin, long _azMax, long _altMin, long _altMax)
{
	setDebug (1);
	setNotDaemonize ();

	setTelLongLat (_long, _lat);
	setTelAltitude (_alt);

	az_ticks->setValueLong (_az_ticks);
	alt_ticks->setValueLong (_alt_ticks);

	azZero->setValueDouble (_azZero);
	zdZero->setValueDouble (_zdZero);

	azCpd->setValueDouble (_azCpd);
	altCpd->setValueDouble (_altCpd);

	azMin->setValueLong (_azMin);
	azMax->setValueLong (_azMax);
	altMin->setValueLong (_altMin);
	altMax->setValueLong (_altMax);

#ifndef RTS2_LIBERFA
	setCorrections (false, false, false, false);
#endif

	hardHorizon = new ObjectCheck ("../conf/horizon_flat_19_flip.txt");
}

int AltAzTest::test_sky2counts (const double utc1, const double utc2, struct ln_equ_posn *pos, int32_t &azc, int32_t &altc)
{
	struct ln_hrz_posn hrz;
	return sky2counts (utc1, utc2, pos, &hrz, azc, altc, false, 0, false);
}

int AltAzTest::test_hrz2counts (struct ln_hrz_posn *hrz, int32_t &azc, int32_t &altc)
{
	bool use_flipped = false;
	return hrz2counts (hrz, azc, altc, false, use_flipped, false, 0, 0);
}

void AltAzTest::test_counts2sky (double JD, int32_t azc, int32_t altc, double &ra, double &dec)
{
	counts2sky (JD, azc, altc, ra, dec);
}

void AltAzTest::test_counts2hrz (int32_t azc, int32_t altc, struct ln_hrz_posn *hrz)
{
	double un_az, un_zd;
	counts2hrz (azc, altc, hrz->az, hrz->alt, un_az, un_zd);
}

float AltAzTest::test_move (double JD, struct ln_equ_posn *pos, int32_t &azc, int32_t &altc, float speed, float max_time)
{
	int32_t t_azc = azc;
	int32_t t_altc = altc;

	// calculates elapsed time
	float elapsed = 0;

	int ret = test_sky2counts (JD, 0, pos, t_azc, t_altc);
	if (ret)
		return NAN;

	const int32_t tt_azc = t_azc;
	const int32_t tt_altc = t_altc;

	int pm = 0;

	struct ln_hrz_posn hrz;

	double un_az, un_alt;

	counts2hrz (azc, altc, hrz.az, hrz.alt, un_az, un_alt);

	std::cout << "moving from alt az " << hrz.alt << " " << hrz.az << std::endl;

	while (elapsed < max_time)
	{
		pm = calculateMove (JD, azc, altc, t_azc, t_altc);
		if (pm == -1)
			return NAN;

		int32_t d_ac = labs (azc - t_azc);
		int32_t d_dc = labs (altc - t_altc);

		float e = 1;

		if (d_ac > d_dc)
			e = d_ac / azCpd->getValueDouble () / speed;
		else
			e = d_dc / altCpd->getValueDouble () / speed;
		elapsed += (e > 1) ? e : 1;

		counts2hrz (t_azc, t_altc, hrz.az, hrz.alt, un_az, un_alt);

		std::cout << "move to alt az " << hrz.alt << " " << hrz.az << " pm " << pm << std::endl;

		if (pm == 0)
		{
			azc = t_azc;
			altc = t_altc;
			break;
		}

		// 1 deg margin for check..
		if (azc < t_azc)
			azc = t_azc - azCpd->getValueDouble ();
		else if (azc > t_azc)
			azc = t_azc + azCpd->getValueDouble ();

		if (altc < t_altc)
			altc = t_altc - altCpd->getValueDouble ();
		else if (azc > t_azc)
			altc = t_altc + altCpd->getValueDouble ();

		t_azc = tt_azc;
		t_altc = tt_altc;

		JD += elapsed / 86400.0;
	}
	
	return elapsed;
}
