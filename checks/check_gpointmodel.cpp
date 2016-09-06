#include "gpointmodel.h"

#include <check.h>
#include <check_utils.h>

rts2telmodel::GPointModel testGPoint_34 (34);
rts2telmodel::GPointModel testGPoint_n39 (-39);

void setup_gpoint (void)
{
	std::istringstream iss1 ("# test #1\nRTS2_MODEL 0.01 0 0 0 -0.02 0 0 0 0");
	testGPoint_34.load (iss1);


	std::istringstream iss2 ("# test #2\nRTS2_ALTAZ 0.01 0 0 0 -0.02 0 0 0 0\n# extras..\nAZ 0.02 SiN el 2\nEL 0.02 cos az 2");
	testGPoint_n39.load (iss2);

}

void teardown_gpoint (void)
{
}

START_TEST(model_1)
{
	struct ln_hrz_posn test_hrz;
	test_hrz.alt = 25;
	test_hrz.az = 0;
	struct ln_hrz_posn test_err;
	testGPoint_n39.getErrAltAz (&test_hrz, &test_err);

	ck_assert_dbl_eq (test_hrz.az, -0.959513, 10e-4);
	ck_assert_dbl_eq (test_hrz.alt, 26.145916, 10e-4);
}
END_TEST

Suite * gpoint_suite (void)
{
	Suite *s;
	TCase *tc_gpoint;

	s = suite_create ("ALT AZ");
	tc_gpoint = tcase_create ("GPoint Model");

	tcase_add_checked_fixture (tc_gpoint, setup_gpoint, teardown_gpoint);
	tcase_add_test (tc_gpoint, model_1);
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
