#include "gemtest.h"

#include <time.h>
#include <stdlib.h>
#include <check.h>
#include <libnova/libnova.h>

// global telescope object
GemTest* gemTest;

void setup_tel (void)
{
	static const char *argv[] = {"testapp"};
	gemTest = new GemTest(0, (char **) argv);

	gemTest->setTelescope (20.70752, -156.257, 3039, 67108864, 67108864, 0, 75.81458333, -5.8187805555, 186413.511111, 186413.511111, -81949557, -47392062, -76983817, -21692458);
}

void teardown_tel (void)
{
	delete gemTest;
	gemTest = NULL;
}

#define ck_assert_dbl_eq(v1,v2,alow)  ck_assert_msg(fabs(v1-v2) < alow, "difference %f and %f > %f", v1, v2, alow)

START_TEST(test_refraction)
{
	struct ln_hrz_posn tpos;
	struct ln_equ_posn teq;
	double JD = 2452134;

	ck_assert_dbl_eq (gemTest->test_getAltitudePressure (0, 1010), 1010, 10e-1);
	ck_assert_dbl_eq (gemTest->test_getAltitudePressure (1000, 1010), 895.86, 10e-1);
	ck_assert_dbl_eq (gemTest->test_getAltitudePressure (2345, 1010), 759.04, 10e-1);
	ck_assert_dbl_eq (gemTest->test_getAltitudePressure (4000, 1010), 614.43, 10e-1);

	tpos.alt = 20;
	tpos.az = 300;
	gemTest->test_getEquFromHrz (&tpos, JD, &teq);

	ck_assert_dbl_eq (teq.ra, 43.8965, 10e-4);
	ck_assert_dbl_eq (teq.dec, -18.5755, 10e-4);

	gemTest->test_applyRefraction (&teq, JD, false);

	ck_assert_dbl_eq (teq.ra, 43.8642, 10e-4);
	ck_assert_dbl_eq (teq.dec, -18.5595, 10e-4);

	gemTest->test_getHrzFromEqu (&teq, JD, &tpos);

	ck_assert_dbl_eq (tpos.az, 300.00475, 10e-4);
	ck_assert_dbl_eq (tpos.alt, 20.0381, 10e-4);

	tpos.alt = 89;
	tpos.az = 231;
	gemTest->test_getEquFromHrz (&tpos, JD, &teq);

	ck_assert_dbl_eq (teq.ra, 43.8965, 10e-4);
	ck_assert_dbl_eq (teq.dec, -18.5755, 10e-4);

	gemTest->test_applyRefraction (&teq, JD, false);

	ck_assert_dbl_eq (teq.ra, 43.8642, 10e-4);
	ck_assert_dbl_eq (teq.dec, -18.5595, 10e-4);
}
END_TEST

Suite * tel_suite (void)
{
	Suite *s;
	TCase *tc_tel_corrs;

	s = suite_create ("Corrections");
	tc_tel_corrs = tcase_create ("Basic");

	tcase_add_checked_fixture (tc_tel_corrs, setup_tel, teardown_tel);
	tcase_add_test (tc_tel_corrs, test_refraction);
	suite_add_tcase (s, tc_tel_corrs);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = tel_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
