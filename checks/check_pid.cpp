#include <check.h>
#include <check_utils.h>
#include <stdlib.h>

#include "pid.h"

rts2core::PID *pid_p = NULL;
rts2core::PID *pid_all = NULL;

void setup_pid (void)
{
	pid_p = new rts2core::PID();
	pid_p->setPID(0.5, 0, 0);

	pid_all = new rts2core::PID();
	pid_all->setPID(0.5, 0.4, 0.3);
}

void teardown_pid (void)
{
	delete pid_all;
	delete pid_p;
}

START_TEST(pid)
{
	double err = pid_p->loop(1, 1);
	ck_assert_dbl_eq(err, 0.5, 1e-4);

	err = pid_all->loop(1, 0.1);
	ck_assert_dbl_eq(err, 3.9, 1e-4);

	err = pid_all->loop(0, 0.1);
	ck_assert_dbl_eq(err, -2.6, 1e-4); 

	err = pid_all->loop(-1, 0.1);
	ck_assert_dbl_eq(err, -3.5, 1e-4); 

	err = pid_all->loop(0, 0.1);
	ck_assert_dbl_eq(err, 3, 1e-4); 

	err = pid_all->loop(0, 0.1);
	ck_assert_dbl_eq(err, 0, 1e-4); 
}
END_TEST

Suite * pid_suite (void)
{
	Suite *s;
	TCase *tc_pid;

	s = suite_create ("PID");
	tc_pid = tcase_create ("PID tests");

	tcase_add_checked_fixture (tc_pid, setup_pid, teardown_pid);
	tcase_add_test (tc_pid, pid);
	suite_add_tcase (s, tc_pid);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = pid_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
