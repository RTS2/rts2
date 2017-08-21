#include <stdlib.h>

#include "expander.h"

#include <check.h>
#include <check_utils.h>

rts2core::Expander *expander = NULL;

void setup_expander (void)
{
	//2.June 2017 21:50 UT
	struct timeval tv;
	tv.tv_sec = 1496440200;
	tv.tv_usec = 777;
	expander = new rts2core::Expander (&tv);
}

void teardown_expander (void)
{
}

START_TEST(expand)
{
	ck_assert_str_eq ("t2017_06_02.log", expander->expand ("t%y_%02m_%02d.log").c_str ());
	ck_assert_str_eq ("20170602T21:50", expander->expand ("%y%02m%02dT%02H:%02M").c_str ());
}
END_TEST

START_TEST(expandPath)
{
	ck_assert_str_eq ("t01.log", expander->expandPath ("t%02u.log").c_str ());
}
END_TEST

Suite * expander_suite (void)
{
	Suite *s;
	TCase *tc_expander;

	s = suite_create ("expander");
	tc_expander = tcase_create ("expander tests");

	tcase_add_checked_fixture (tc_expander, setup_expander, teardown_expander);
	tcase_add_test (tc_expander, expand);
	tcase_add_test (tc_expander, expandPath);
	suite_add_tcase (s, tc_expander);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = expander_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
