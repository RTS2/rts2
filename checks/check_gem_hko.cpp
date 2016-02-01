#include "gem.h"

#include <time.h>
#include <stdlib.h>
#include <check.h>
#include <libnova/libnova.h>

class GemTest:public rts2teld::GEM
{
	public:
		GemTest (int argc, char **argv);
		~GemTest ();

		void setTelescope (double _lat, double _long, double _alt, long _ra_ticks, long _dec_ticks, int _haZeroPos, double _haZero, double _decZero, double _haCpd, double _decCpd, long _acMin, long _acMax, long _dcMin, long _dcMax);
		int test_sky2counts (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc);
		int test_counts2sky (double JD, int32_t ac, int32_t dc, double &ra, double &dec);
		int test_counts2hrz (double JD, int32_t ac, int32_t dc, struct ln_hrz_posn *hrz);

		/**
		 * Test movement to given target position, from counts in ac dc parameters.
		 */
		float test_move (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc, float speed, float max_time);

	protected:
		virtual int isMoving () { return 0; };
		virtual int startResync () { return 0; }
		virtual int stopMove () { return 0; }
		virtual int startPark () { return 0; }
		virtual int endPark () { return 0; }
		virtual int updateLimits() { return 0; };
};


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

// global telescope object
GemTest* gemTest;

void setup_hko (void)
{
	static const char *argv[] = {"testapp"};
	gemTest = new GemTest(0, (char **) argv);

	gemTest->setTelescope (20.70752, -156.257, 3039, 67108864, 67108864, 0, 75.81458333, -5.8187805555, 186413.511111, 186413.511111, -81949557, -47392062, -76983817, -21692458);
}

void teardown_hko (void)
{
	delete gemTest;
	gemTest = NULL;
}

int GemTest::test_sky2counts (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc)
{
	return sky2counts (JD, pos, ac, dc, false, 0);
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
			break;

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

#define ck_assert_dbl_eq(v1,v2,alow)  ck_assert_msg(fabs(v1-v2) < alow, "difference %f and %f > %f", v1, v2, alow)

START_TEST(test_gem_hko)
{
	// test 1, 2016-01-12T19:20:47 HST = 2016-01-13U05:20:47
	struct ln_date test_t;
	test_t.years = 2016;
	test_t.months = 1;
	test_t.days = 13;
	test_t.hours = 5;
	test_t.minutes = 20;
	test_t.seconds = 47;

	double JD = ln_get_julian_day (&test_t);
	ck_assert_dbl_eq (JD, 2457400.722766, 10e-5);

	struct ln_equ_posn pos;
	pos.ra = 20;
	pos.dec = 80;

	int32_t ac = -70000000;
	int32_t dc = -68000000;

	int ret = gemTest->test_sky2counts (JD, &pos, ac, dc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (ac, -78244743);
	ck_assert_int_eq (dc, -51111083);

	// origin
	pos.ra = 344.16613;
	pos.dec = 2.3703305;

	ret = gemTest->test_sky2counts (JD, &pos, ac, dc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (ac, -71564825);
	ck_assert_int_eq (dc, -65582303);

	pos.ra = 62.95859;
	pos.dec = -3.51601;

	float e = gemTest->test_move (JD, &pos, ac, dc, 2.0, 200);
	ck_assert_msg (!isnan (e), "position %f %f not reached", pos.ra, pos.dec);

	struct ln_hrz_posn hrz;

	gemTest->test_counts2hrz (JD, -70194687, -48165219, &hrz);
	ck_assert_dbl_eq (hrz.alt, 17.664712, 10e-2);
	ck_assert_dbl_eq (hrz.az, 185.232715, 10e-2);

	gemTest->test_counts2hrz (JD, -68591258, -68591258, &hrz);
	ck_assert_dbl_eq (hrz.alt, 14.962282, 10e-2);
	ck_assert_dbl_eq (hrz.az, 68.627175, 10e-2);
}
END_TEST

Suite * gem_suite (void)
{
	Suite *s;
	TCase *tc_gem_hko_pointings;

	s = suite_create ("GEM");
	tc_gem_hko_pointings = tcase_create ("HKO Pointings");

	tcase_add_checked_fixture (tc_gem_hko_pointings, setup_hko, teardown_hko);
	tcase_add_test (tc_gem_hko_pointings, test_gem_hko);
	suite_add_tcase (s, tc_gem_hko_pointings);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = gem_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
