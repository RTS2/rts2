#include "gpointmodel.h"

#include <check.h>
#include <check_utils.h>

class TestGPoint:public rts2telmodel::GPointModel
{
	public:
		TestGPoint
};

void setup_altcaz (void)
{
}

void teardown_altcaz (void)
{
}

START_TEST(model_1)
{
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
