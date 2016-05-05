#include <stdlib.h>
#include <check.h>
#include <check_utils.h>

void setup_tle (void)
{
}

void teardown_tle (void)
{
}

START_TEST(XMM)
{

}
END_TEST

Suite * tle_suite (void)
{
	Suite *s;
	TCase *tc_tle;

	s = suite_create ("TLE");
	tc_tle = tcase_create ("TLE orbit calculations");

	tcase_add_checked_fixture (tc_tle, setup_tle, teardown_tle);
	tcase_add_test (tc_tle, XMM);
	suite_add_tcase (s, tc_tle);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = tle_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
