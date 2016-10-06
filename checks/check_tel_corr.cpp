#include "gemtest.h"
#include "check_utils.h"

#include <time.h>
#include <stdlib.h>
#include <check.h>
#include <libnova/libnova.h>

// global telescope object
GemTest* gemTest;
GemTest* gemTest4000;  // 4 athmosphere pressure

void setup_tel (void)
{
	static const char *argv[] = {"testapp"};

	gemTest = new GemTest(0, (char **) argv);
	gemTest->setTelescope (20.70752, -156.257, 3039, 67108864, 67108864, 0, 75.81458333, -5.8187805555, 186413.511111, 186413.511111, -81949557, -47392062, -76983817, -21692458);

	// telescope at 4 atmosphere pressure - test for pressure and altitude missuse

	gemTest4000 = new GemTest(0, (char **) argv);
	gemTest4000->setTelescope (20.70752, -156.257, -13500, 67108864, 67108864, 0, 75.81458333, -5.8187805555, 186413.511111, 186413.511111, -81949557, -47392062, -76983817, -21692458);
}

void teardown_tel (void)
{
	delete gemTest4000;
	gemTest4000 = NULL;

	delete gemTest;
	gemTest = NULL;
}

START_TEST(test_pressure)
{
	ck_assert_dbl_eq (gemTest4000->test_getAltitudePressure (-13500, 1010), 4084.32, 10e-1);
	ck_assert_dbl_eq (gemTest->test_getAltitudePressure (0, 1010), 1010, 10e-1);
	ck_assert_dbl_eq (gemTest->test_getAltitudePressure (1000, 1010), 895.86, 10e-1);
	ck_assert_dbl_eq (gemTest->test_getAltitudePressure (2345, 1010), 759.04, 10e-1);
	ck_assert_dbl_eq (gemTest->test_getAltitudePressure (4000, 1010), 614.43, 10e-1);
	ck_assert_dbl_eq (gemTest->test_getAltitudePressure (4015, 998), 605.99, 10e-1);
	ck_assert_dbl_eq (gemTest->test_getAltitudePressure (6087, 998), 459.19, 10e-1);
}
END_TEST

START_TEST(test_refraction)
{
	struct ln_hrz_posn tpos;
	struct ln_equ_posn teq;
	double JD = 2452134;

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

	teq.ra += 1;

	gemTest->test_applyRefraction (&teq, JD, false);

	ck_assert_dbl_eq (teq.ra, 44.8305, 10e-4);
	ck_assert_dbl_eq (teq.dec, -18.5430, 10e-4);

	gemTest->test_getHrzFromEqu (&teq, JD, &tpos);

	ck_assert_dbl_eq (tpos.az, 299.4879, 10e-4);
	ck_assert_dbl_eq (tpos.alt, 19.2619, 10e-4);

	tpos.alt = 89;
	tpos.az = 231;
	gemTest->test_getEquFromHrz (&tpos, JD, &teq);

	ck_assert_dbl_eq (teq.ra, 345.5796, 10e-4);
	ck_assert_dbl_eq (teq.dec, 21.3348, 10e-4);

	gemTest->test_applyRefraction (&teq, JD, false);

	ck_assert_dbl_eq (teq.ra, 345.5751, 10e-4);
	ck_assert_dbl_eq (teq.dec, 21.3348, 10e-4);

	gemTest->test_getHrzFromEqu (&teq, JD, &tpos);

	ck_assert_dbl_eq (tpos.az, 230.6954, 10e-4);
	ck_assert_dbl_eq (tpos.alt, 89.0066, 10e-4);

	tpos.alt = 2.34;
	tpos.az = 130.558;
	gemTest->test_getEquFromHrz (&tpos, JD, &teq);

	ck_assert_dbl_eq (teq.ra, 240.5846, 10e-4);
	ck_assert_dbl_eq (teq.dec, 38.4727, 10e-4);

	gemTest->test_applyRefraction (&teq, JD, false);

	ck_assert_dbl_eq (teq.ra, 240.8026, 10e-4);
	ck_assert_dbl_eq (teq.dec, 38.5529, 10e-4);

	gemTest->test_getHrzFromEqu (&teq, JD, &tpos);

	ck_assert_dbl_eq (tpos.az, 130.5609, 10e-4);
	ck_assert_dbl_eq (tpos.alt, 2.5252, 10e-4);
}
END_TEST

START_TEST(test_refraction4000)
{
	struct ln_hrz_posn tpos;
	struct ln_equ_posn teq;
	double JD = 2452134;

	tpos.alt = 40;
	tpos.az = 300;
	gemTest4000->test_getEquFromHrz (&tpos, JD, &teq);

	ck_assert_dbl_eq (teq.ra, 26.7491, 10e-4);
	ck_assert_dbl_eq (teq.dec, -7.5267, 10e-4);

	gemTest4000->test_applyRefraction (&teq, JD, false);

	gemTest4000->test_getHrzFromEqu (&teq, JD, &tpos);

	ck_assert_dbl_eq (tpos.az, 300.0067, 10e-4);
	ck_assert_dbl_eq (tpos.alt, 40.0862, 10e-4);

	teq.ra = 27.7491;
	teq.dec = -7.5267;

	gemTest4000->test_applyRefraction (&teq, JD, false);

	ck_assert_dbl_eq (teq.ra, 27.6771, 10e-4);
	ck_assert_dbl_eq (teq.dec, -7.4805, 10e-4);

	teq.ra = 27.7491;
	teq.dec = -7.5267;

	gemTest->test_applyRefraction (&teq, JD, false);

	ck_assert_dbl_eq (teq.ra, 27.7330, 10e-4);
	ck_assert_dbl_eq (teq.dec, -7.5188, 10e-4);

	teq.ra = 30.7491;
	teq.dec = -7.5267;

	gemTest4000->test_applyRefraction (&teq, JD, false);

	ck_assert_dbl_eq (teq.ra, 30.6693, 10e-4);
	ck_assert_dbl_eq (teq.dec, -7.4784, 10e-4);

	teq.ra = 30.7491;
	teq.dec = -7.5267;

	gemTest->test_applyRefraction (&teq, JD, false);

	ck_assert_dbl_eq (teq.ra, 30.7317, 10e-4);
	ck_assert_dbl_eq (teq.dec, -7.5184, 10e-4);
}
END_TEST

Suite * tel_suite (void)
{
	Suite *s;
	TCase *tc_tel_corrs;

	s = suite_create ("Corrections");
	tc_tel_corrs = tcase_create ("Basic");

	tcase_add_checked_fixture (tc_tel_corrs, setup_tel, teardown_tel);
	tcase_add_test (tc_tel_corrs, test_pressure);
	tcase_add_test (tc_tel_corrs, test_refraction);
	tcase_add_test (tc_tel_corrs, test_refraction4000);
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
