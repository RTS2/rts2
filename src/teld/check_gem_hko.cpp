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
}

// global telescope object
GemTest* gemTest;

void setup_hko (void)
{
	gemTest = new GemTest(0, NULL);

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

	int32_t ac = 0, dc = 0;

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
