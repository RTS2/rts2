#include "altaztest.h"

#include <time.h>
#include <stdlib.h>
#include <check.h>
#include <check_utils.h>
#include <libnova/libnova.h>

// global telescope object
AltAzTest* altAzTest;

void setup_altcaz (void)
{
	static const char *argv[] = {"testapp"};
	altAzTest = new AltAzTest(0, (char **) argv);

	altAzTest->setTelescope (-40, -5, 200, 67108864, 67108864, 90, -7, 186413.511111, 186413.511111, -80000000, 80000000, -40000000, 40000000);
}

void teardown_altcaz (void)
{
	delete altAzTest;
	altAzTest = NULL;
}

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

	struct ln_hrz_posn hrz, res_hrz;
	hrz.alt = 80;
	hrz.az = 20;

	struct ln_equ_posn pos;
	pos.ra = 20;
	pos.dec = 80;

	int32_t azc = -20000000;
	int32_t altc = 1000;

	int ret = altAzTest->test_hrz2counts (&hrz, azc, altc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (azc, -13048946);
	ck_assert_int_eq (altc, 3169029);

	altAzTest->test_counts2hrz (azc, altc, &res_hrz);
	ck_assert_dbl_eq (res_hrz.az, hrz.az, 10e-5);
	ck_assert_dbl_eq (res_hrz.alt, hrz.alt, 10e-5);

	ret = altAzTest->test_sky2counts (JD, &pos, azc, altc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (azc, 16147947);
	ck_assert_int_eq (altc, 27349159);

	altAzTest->test_counts2sky (JD, azc, altc, pos.ra, pos.dec);
	ck_assert_dbl_eq (pos.ra, 20, 10e-4);
	ck_assert_dbl_eq (pos.dec, 80, 10e-4);

	// origin
	pos.ra = 344.16613;
	pos.dec = 2.3703305;

	ret = altAzTest->test_sky2counts (JD, &pos, azc, altc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (azc, 43783567);
	ck_assert_int_eq (altc, 26826255);

	// target
	pos.ra = 62.95859;
	pos.dec = -3.51601;

	float e = altAzTest->test_move (JD, &pos, azc, altc, 2.0, 200);
	ck_assert_msg (!isnan (e), "position %f %f not reached", pos.ra, pos.dec);

	struct ln_equ_posn curr;
	curr.ra = curr.dec = 0;

	altAzTest->test_counts2sky (JD, azc, altc, curr.ra, curr.dec);

	ck_assert_dbl_eq (pos.ra, curr.ra, 10e-3);
	ck_assert_dbl_eq (pos.dec, curr.dec, 10e-3);

	altAzTest->test_counts2hrz (-70194687, -48165219, &hrz);
	ck_assert_dbl_eq (hrz.alt, -4.621631, 10e-3);
	ck_assert_dbl_eq (hrz.az, 73.446355, 10e-3);

	ret = altAzTest->test_hrz2counts (&hrz, azc, altc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (azc, 64023041);
	ck_assert_int_eq (altc, 18943645);

	altAzTest->test_counts2hrz (-68591258, -68591258, &hrz);
	ck_assert_dbl_eq (hrz.alt, 75.047819, 10e-2);
	ck_assert_dbl_eq (hrz.az, 262.047819, 10e-2);

	ret = altAzTest->test_hrz2counts (&hrz, azc, altc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (azc, 32072038);
	ck_assert_int_eq (altc, 4092184);
}
END_TEST

Suite * altaz_suite (void)
{
	Suite *s;
	TCase *tc_altaz_pointings;

	s = suite_create ("ALT AZ");
	tc_altaz_pointings = tcase_create ("AltAz Pointings");

	tcase_add_checked_fixture (tc_altaz_pointings, setup_altcaz, teardown_altcaz);
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
