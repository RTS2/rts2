#include "gemtest.h"

#include <time.h>
#include <stdlib.h>
#include <check.h>
#include <check_utils.h>
#include <libnova/libnova.h>

// global telescope object
GemTest* gemTest;

void setup_mlo (void)
{
	static const char *argv[] = {"testapp"};
	gemTest = new GemTest(0, (char **) argv);

	gemTest->setTelescope (19.5362, -155.5763, 3000, 67108864, 67108864, 0, -70.3428840636, -72.6120328902, 186413.511111, 186413.511111, -55000000, -19000000, -60000000, -10000000);
}

void teardown_mlo (void)
{
	delete gemTest;
	gemTest = NULL;
}

#define ck_assert_dbl_eq(v1,v2,alow)  ck_assert_msg(fabs(v1-v2) < alow, "difference %f and %f > %f", v1, v2, alow)

START_TEST(test_gem_mlo)
{
	// test 1, 2016-01-12T19:20:47 HST = 2016-01-13U05:20:47
	struct ln_date test_t;
	test_t.years = 2016;
	test_t.months = 1;
	test_t.days = 31;
	test_t.hours = 5;
	test_t.minutes = 20;
	test_t.seconds = 47;

	double JD = ln_get_julian_day (&test_t);
	ck_assert_dbl_eq (JD, 2457418.722766, 10e-5);

	struct ln_equ_posn pos;
	pos.ra = 20;
	pos.dec = 80;

	int32_t ac = -70000000;
	int32_t dc = -68000000;

	int ret = gemTest->test_sky2counts (JD, &pos, ac, dc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (ac, -47564851);
	ck_assert_int_eq (dc, -38659919);

	// origin
	pos.ra = 344.16613;
	pos.dec = 2.3703305;

	ret = gemTest->test_sky2counts (JD, &pos, ac, dc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (ac, -40884933);
	ck_assert_int_eq (dc, -53131138);

	pos.ra = 62.95859;
	pos.dec = -3.51601;

	float e = gemTest->test_move (JD, &pos, ac, dc, 2.0, 200);
	ck_assert_msg (!isnan (e), "position %f %f not reached", pos.ra, pos.dec);

	struct ln_hrz_posn hrz;

	gemTest->test_counts2hrz (JD, -70194687, -48165219, &hrz);
	ck_assert_dbl_eq (hrz.alt, 11.934541, 10e-2);
	ck_assert_dbl_eq (hrz.az, 243.195274, 10e-2);

	gemTest->test_counts2hrz (JD, -68591258, -68591258, &hrz);
	ck_assert_dbl_eq (hrz.alt, -17.369510, 10e-2);
	ck_assert_dbl_eq (hrz.az, 350.316641, 10e-2);
}
END_TEST

Suite * gem_suite (void)
{
	Suite *s;
	TCase *tc_gem_mlo_pointings;

	s = suite_create ("GEM");
	tc_gem_mlo_pointings = tcase_create ("MLO Pointings");

	tcase_add_checked_fixture (tc_gem_mlo_pointings, setup_mlo, teardown_mlo);
	tcase_add_test (tc_gem_mlo_pointings, test_gem_mlo);
	suite_add_tcase (s, tc_gem_mlo_pointings);

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
