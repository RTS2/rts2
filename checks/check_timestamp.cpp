#include "displayvalue.h"
#include "timestamp.h"

#include <stdlib.h>
#include <check.h>
#include <check_utils.h>

#include <libnova/libnova.h>
#include <sstream>

void setup_timestamp (void)
{
}

void teardown_timestamp (void)
{
}

START_TEST(TIMEDIFF1)
{
	std::ostringstream oss;
	oss << TimeDiff (0.5, true);

	ck_assert_str_eq (oss.str ().c_str (), "0.500s");
}
END_TEST

START_TEST(TIMEDIFF2)
{
	std::ostringstream oss;
	oss << TimeDiff (0.005, true);

	ck_assert_str_eq (oss.str ().c_str (), "5ms");
}
END_TEST

START_TEST(TIMEDIFF3)
{
	std::ostringstream oss;
	oss << TimeDiff (0.5, false);

	ck_assert_str_eq (oss.str ().c_str (), "0.500s");
}
END_TEST

START_TEST(TIMEDIFF4)
{
	std::ostringstream oss;
	oss << TimeDiff (0.005, false);

	ck_assert_str_eq (oss.str ().c_str (), "5ms");
}
END_TEST

Suite * timestamp_suite (void)
{
	Suite *s;
	TCase *tc_timestamp;

	s = suite_create ("TimeStamp");
	tc_timestamp = tcase_create ("TimeDiff calculations");

	tcase_add_checked_fixture (tc_timestamp, setup_timestamp, teardown_timestamp);
	tcase_add_test (tc_timestamp, TIMEDIFF1);
	tcase_add_test (tc_timestamp, TIMEDIFF2);
	tcase_add_test (tc_timestamp, TIMEDIFF3);
	tcase_add_test (tc_timestamp, TIMEDIFF4);
	suite_add_tcase (s, tc_timestamp);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = timestamp_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
