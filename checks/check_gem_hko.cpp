#include "gemtest.h"

#include <time.h>
#include <stdlib.h>
#include <check.h>
#include <libnova/libnova.h>

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

#define ck_assert_dbl_eq(v1,v2,alow)  ck_assert_msg(fabs(v1-v2) < alow, "difference %f and %f > %f", v1, v2, alow)

START_TEST(test_gem_hko_1)
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

	// target
	pos.ra = 62.95859;
	pos.dec = -3.51601;

	float e = gemTest->test_move (JD, &pos, ac, dc, 2.0, 200);
	ck_assert_msg (!isnan (e), "position %f %f not reached", pos.ra, pos.dec);

	struct ln_equ_posn curr;
	curr.ra = curr.dec = 0;

	gemTest->test_counts2sky (JD, ac, dc, curr.ra, curr.dec);

	ck_assert_dbl_eq (pos.ra, curr.ra, 10e-5);
	ck_assert_dbl_eq (pos.dec, curr.dec, 10e-5);

	struct ln_hrz_posn hrz;

	gemTest->test_counts2hrz (JD, -70194687, -48165219, &hrz);
	ck_assert_dbl_eq (hrz.alt, 17.664712, 10e-2);
	ck_assert_dbl_eq (hrz.az, 185.232715, 10e-2);

	gemTest->test_counts2hrz (JD, -68591258, -68591258, &hrz);
	ck_assert_dbl_eq (hrz.alt, 14.962282, 10e-2);
	ck_assert_dbl_eq (hrz.az, 68.627175, 10e-2);
}
END_TEST

START_TEST(test_gem_hko_2)
{
	// 2016-02-05T02:45:03.641 HST T0 4 moving from 11:14:56.562 -32:10:53.92 to 12:41:07.548 -17:34:03.94 (altaz from +37 05 39.22 001 28 09.42)
	// counts ra -80983622 -51447273 counts dec -72023258 -29171948 
	struct ln_date test_t;
	test_t.years = 2016;
	test_t.months = 2;
	test_t.days = 5;
	test_t.hours = 12;
	test_t.minutes = 45;
	test_t.seconds = 3;

	double JD = ln_get_julian_day (&test_t);
	ck_assert_dbl_eq (JD, 2457424.031285, 10e-5);

	struct ln_equ_posn pos;

	// origin
	pos.ra = 168.7333;
	pos.dec = -32.1813;

	int32_t ac = -70000000;
	int32_t dc = -68000000;

	int ret = gemTest->test_sky2counts (JD, &pos, ac, dc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (ac, -80983656);
	ck_assert_int_eq (dc, -72023192);

	// target
	pos.ra = 190.2791;
	pos.dec = -17.5675;

	float e = gemTest->test_move (JD, &pos, ac, dc, 2.0, 200);
	ck_assert_msg (!isnan (e), "position %f %f not reached", pos.ra, pos.dec);

	ck_assert_int_eq (ac, -51445652);
	ck_assert_int_eq (dc, -29194913);

	struct ln_equ_posn curr;
	curr.ra = curr.dec = 0;

	gemTest->test_counts2sky (JD, ac, dc, curr.ra, curr.dec);

	ck_assert_dbl_eq (pos.ra, curr.ra, 10e-5);
	ck_assert_dbl_eq (pos.dec, curr.dec, 10e-5);
}
END_TEST

Suite * gem_suite (void)
{
	Suite *s;
	TCase *tc_gem_hko_pointings;

	s = suite_create ("GEM");
	tc_gem_hko_pointings = tcase_create ("HKO Pointings");

	tcase_add_checked_fixture (tc_gem_hko_pointings, setup_hko, teardown_hko);
	tcase_add_test (tc_gem_hko_pointings, test_gem_hko_1);
	tcase_add_test (tc_gem_hko_pointings, test_gem_hko_2);
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
