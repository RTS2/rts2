#include "altaztest.h"

#include <time.h>
#include <stdlib.h>
#include <check.h>
#include <libnova/libnova.h>

// global telescope object
AltAzTest* altAzTest;

void setup_altaz (void)
{
	static const char *argv[] = {"testapp"};
	altAzTest = new AltAzTest(0, (char **) argv);

	altAzTest->setTelescope (-40, -5, 200, 67108864, 67108864, 90, -7, 186413.511111, 186413.511111, -80000000, 80000000, -40000000, 40000000);
}

void teardown_altaz (void)
{
	delete altAzTest;
	altAzTest = NULL;
}

#define ck_assert_dbl_eq(v1,v2,alow)  ck_assert_msg(fabs(v1-v2) < alow, "difference %f and %f > %f", v1, v2, alow)

START_TEST(test_altaz_1)
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

	int ret = altAzTest->test_sky2counts (JD, &pos, ac, dc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (ac, -78244743);
	ck_assert_int_eq (dc, -51111083);

	// origin
	pos.ra = 344.16613;
	pos.dec = 2.3703305;

	ret = altAzTest->test_sky2counts (JD, &pos, ac, dc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (ac, -71564825);
	ck_assert_int_eq (dc, -65582303);

	// target
	pos.ra = 62.95859;
	pos.dec = -3.51601;

	float e = altAzTest->test_move (JD, &pos, ac, dc, 2.0, 200);
	ck_assert_msg (!isnan (e), "position %f %f not reached", pos.ra, pos.dec);

	struct ln_equ_posn curr;
	curr.ra = curr.dec = 0;

	altAzTest->test_counts2sky (JD, ac, dc, curr.ra, curr.dec);

	ck_assert_dbl_eq (pos.ra, curr.ra, 10e-5);
	ck_assert_dbl_eq (pos.dec, curr.dec, 10e-5);

	struct ln_hrz_posn hrz;

	altAzTest->test_counts2hrz (JD, -70194687, -48165219, &hrz);
	ck_assert_dbl_eq (hrz.alt, 17.664712, 10e-2);
	ck_assert_dbl_eq (hrz.az, 185.232715, 10e-2);

	altAzTest->test_counts2hrz (JD, -68591258, -68591258, &hrz);
	ck_assert_dbl_eq (hrz.alt, 14.962282, 10e-2);
	ck_assert_dbl_eq (hrz.az, 68.627175, 10e-2);
}
END_TEST

Suite * altaz_suite (void)
{
	Suite *s;
	TCase *tc_altaz_pointings;

	s = suite_create ("ALT AZ");
	tc_altaz_pointings = tcase_create ("AltAz Pointings");

	tcase_add_checked_fixture (tc_altaz_pointings, setup_altaz, teardown_altaz);
	tcase_add_test (tc_altaz_pointings, test_altaz_1);
	suite_add_tcase (s, tc_altaz_pointings);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = altaz_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
