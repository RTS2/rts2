#include "gpointmodel.h"

#include <check.h>
#include <check_utils.h>

rts2telmodel::GPointModel testGPoint_34 (34);
rts2telmodel::GPointModel testGPoint_altaz_34 (34);
rts2telmodel::GPointModel testGPoint_n39 (-39);

void setup_gpoint (void)
{
	std::istringstream iss1 ("# test #1\nRTS2_MODEL 0.01 0 0 0 -0.02 0 0 0 0");
	testGPoint_34.load (iss1);

	std::istringstream iss2 ("# test #2\nRTS2_ALTAZ 1.0d 0 0 0 0 -7200\" 0\n# extras..\nAZ 0.05235987755982989 SiN el 2\nEL 0.06981317007977318 cos az 2");
	testGPoint_altaz_34.load (iss2);

	std::istringstream iss3 ("# test #2\nRTS2_ALTAZ 1.0d 0 0 0 0 -7200\" 0\n# extras..\nAZ 0.05235987755982989 SiN el 2\nAZ 1d offset az 1\nEL 2d offset el 2\nEL 0.06981317007977318 cos az 2");
	testGPoint_n39.load (iss3);

}

void teardown_gpoint (void)
{
}

START_TEST(model_altaz_34)
{
	struct ln_hrz_posn test_hrz;
	struct ln_hrz_posn test_err;

	testGPoint_altaz_34.print (std::cout);

	test_hrz.az = 0;
	test_hrz.alt = 45;
	testGPoint_altaz_34.getErrAltAz (&test_hrz, &test_err);

	ck_assert_dbl_eq (test_hrz.az, 2, 10e-4);
	ck_assert_dbl_eq (test_hrz.alt, 51, 10e-4);

	ck_assert_dbl_eq (test_err.az, 2, 10e-4);
	ck_assert_dbl_eq (test_err.alt, 6, 10e-4);

	test_hrz.az = 90;
	test_hrz.alt = 45;
	testGPoint_altaz_34.getErrAltAz (&test_hrz, &test_err);

	ck_assert_dbl_eq (test_hrz.az, 92, 10e-4);
	ck_assert_dbl_eq (test_hrz.alt, 43, 10e-4);

	ck_assert_dbl_eq (test_err.az, 2, 10e-4);
	ck_assert_dbl_eq (test_err.alt, -2, 10e-4);

	test_hrz.az = 30;
	test_hrz.alt = 15;
	testGPoint_altaz_34.getErrAltAz (&test_hrz, &test_err);

	ck_assert_dbl_eq (test_hrz.az, 30.5, 10e-4);
	ck_assert_dbl_eq (test_hrz.alt, 19, 10e-4);

	ck_assert_dbl_eq (test_err.az, 0.5, 10e-4);
	ck_assert_dbl_eq (test_err.alt, 4, 10e-4);
}
END_TEST

START_TEST(model_n39)
{
	struct ln_hrz_posn test_hrz;
	struct ln_hrz_posn test_err;

	test_hrz.az = 0;
	test_hrz.alt = 45;
	testGPoint_n39.getErrAltAz (&test_hrz, &test_err);

	ck_assert_dbl_eq (test_hrz.az, 2, 10e-4);
	ck_assert_dbl_eq (test_hrz.alt, 51, 10e-4);

	ck_assert_dbl_eq (test_err.az, 2, 10e-4);
	ck_assert_dbl_eq (test_err.alt, 6, 10e-4);

	test_hrz.az = 90;
	test_hrz.alt = 45;
	testGPoint_n39.getErrAltAz (&test_hrz, &test_err);

	ck_assert_dbl_eq (test_hrz.az, 92, 10e-4);
	ck_assert_dbl_eq (test_hrz.alt, 43, 10e-4);

	ck_assert_dbl_eq (test_err.az, 2, 10e-4);
	ck_assert_dbl_eq (test_err.alt, -2, 10e-4);

	test_hrz.az = 30;
	test_hrz.alt = 15;
	testGPoint_n39.getErrAltAz (&test_hrz, &test_err);

	ck_assert_dbl_eq (test_hrz.az, 30.5, 10e-4);
	ck_assert_dbl_eq (test_hrz.alt, 19, 10e-4);

	ck_assert_dbl_eq (test_err.az, 0.5, 10e-4);
	ck_assert_dbl_eq (test_err.alt, 4, 10e-4);
}
END_TEST

Suite * gpoint_suite (void)
{
	Suite *s;
	TCase *tc_gpoint;

	s = suite_create ("ALT AZ");
	tc_gpoint = tcase_create ("GPoint Model");

	tcase_add_checked_fixture (tc_gpoint, setup_gpoint, teardown_gpoint);
	tcase_add_test (tc_gpoint, model_altaz_34);
	tcase_add_test (tc_gpoint, model_n39);
	suite_add_tcase (s, tc_gpoint);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = gpoint_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
