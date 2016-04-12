#include "gemtest.h"

GemTest::GemTest (int argc, char **argv):rts2teld::GEM (argc, argv)
{
}

GemTest::~GemTest ()
{
}

void GemTest::setTelescope (double _lat, double _long, double _alt, long _ra_ticks, long _dec_ticks, int _haZeroPos, double _haZero, double _decZero, double _haCpd, double _decCpd, long _acMin, long _acMax, long _dcMin, long _dcMax)
{
	setDebug (1);
	setNotDaemonize ();

	telLatitude->setValueDouble (_lat);
	telLongitude->setValueDouble (_long);
	telAltitude->setValueDouble (_alt);

	ra_ticks->setValueLong (_ra_ticks);
	dec_ticks->setValueLong (_dec_ticks);

	haZeroPos->setValueInteger (_haZeroPos);
	haZero->setValueDouble (_haZero);
	decZero->setValueDouble (_decZero);

	haCpd->setValueDouble (_haCpd);
	decCpd->setValueDouble (_decCpd);

	acMin->setValueLong (_acMin);
	acMax->setValueLong (_acMax);
	dcMin->setValueLong (_dcMin);
	dcMax->setValueLong (_dcMax);

	hardHorizon = new ObjectCheck ("../conf/horizon_flat_19_flip.txt");
}

int GemTest::test_sky2counts (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc)
{
	return sky2counts (JD, pos, ac, dc, false, 0, false);
}

int GemTest::test_counts2sky (double JD, int32_t ac, int32_t dc, double &ra, double &dec)
{
	double un_ra, un_dec;
	int flip;
	return counts2sky (ac, dc, ra, dec, flip, un_ra, un_dec, JD);
}

int GemTest::test_counts2hrz (double JD, int32_t ac, int32_t dc, struct ln_hrz_posn *hrz)
{
	return counts2hrz (ac, dc, hrz, JD);
}

float GemTest::test_move (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc, float speed, float max_time)
{
	int32_t t_ac = ac;
	int32_t t_dc = dc;

	// calculates elapsed time
	float elapsed = 0;

	int ret = test_sky2counts (JD, pos, t_ac, t_dc);
	if (ret)
		return NAN;

	const int32_t tt_ac = t_ac;
	const int32_t tt_dc = t_dc;

	int pm = 0;

	struct ln_hrz_posn hrz;

	counts2hrz (ac, dc, &hrz, JD);

	std::cout << "moving from alt az " << hrz.alt << " " << hrz.az << std::endl;

	while (elapsed < max_time)
	{
		pm = calculateMove (JD, ac, dc, t_ac, t_dc, pm);
		if (pm == -1)
			return NAN;

		int32_t d_ac = labs (ac - t_ac);
		int32_t d_dc = labs (dc - t_dc);

		float e = 1;

		if (d_ac > d_dc)
			e = d_ac / haCpd->getValueDouble () / speed;
		else
			e = d_dc / decCpd->getValueDouble () / speed;
		elapsed += (e > 1) ? e : 1;

		counts2hrz (t_ac, t_dc, &hrz, JD);

		std::cout << "move to alt az " << hrz.alt << " " << hrz.az << " pm " << pm << std::endl;

		if (pm == 0)
		{
			ac = t_ac;
			dc = t_dc;
			break;
		}

		// 1 deg margin for check..
		if (ac < t_ac)
			ac = t_ac - haCpd->getValueDouble ();
		else if (ac > t_ac)
			ac = t_ac + haCpd->getValueDouble ();

		if (dc < t_dc)
			dc = t_dc - decCpd->getValueDouble ();
		else if (ac > t_ac)
			dc = t_dc + decCpd->getValueDouble ();

		t_ac = tt_ac;
		t_dc = tt_dc;

		JD += elapsed / 86400.0;
	}
	
	return elapsed;
}
