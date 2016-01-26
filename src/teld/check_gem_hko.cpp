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

int GemTest::test_sky2counts (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc)
{
	return sky2counts (JD, pos, ac, dc, false, 0);
}

START_TEST(test_gem_hko)
{
	// set to UTC, to make test independent of local timezone
	timezone = 0;
	daylight = 0;

	GemTest* gemTest = new GemTest(0, NULL);

	gemTest->setTelescope (20.70752, -156.257, 3039, 67108864, 67108864, 0, 75.81458333, -5.8187805555, 186413.511111, 186413.511111, -81949557, -47392062, -76983817, -21692458);

	// test 1, 2016-01-12T19:20:47 HST = 2016-01-13U05:20:47
	struct tm test_t;
	test_t.tm_year = 116;
	test_t.tm_mon = 0;
	test_t.tm_mday = 13;
	test_t.tm_hour = 5;
	test_t.tm_min = 20;
	test_t.tm_sec = 47;

	time_t t = mktime (&test_t);
	ck_assert_int_eq (t, 1452658847);

	double JD = ln_get_julian_from_timet (&t);

	struct ln_equ_posn pos;
	pos.ra = 20;
	pos.dec = 80;

	int32_t ac, dc;

	int ret = gemTest->test_sky2counts (JD, &pos, ac, dc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (ac, -47494169);
	ck_assert_int_eq (dc, -47382813);
	
	delete gemTest;
}
END_TEST

Suite * gem_suite (void)
{
	Suite *s;
	TCase *tc_gem_pointings;

	s = suite_create ("GEM");
	tc_gem_pointings = tcase_create ("Pointings");

	tcase_add_test (tc_gem_pointings, test_gem_hko);
	suite_add_tcase (s, tc_gem_pointings);

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
