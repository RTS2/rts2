#include "dut1.h"

#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <check_utils.h>
#include <math.h>
#include <unistd.h>

void setup_dut1 (void)
{
}

void teardown_dut1 (void)
{
}

START_TEST(GETDUT1)
{
	struct tm gmt;
	gmt.tm_year = 117;
	gmt.tm_mon = 1;
	gmt.tm_mday = 5;
	ck_assert_dbl_eq (getDUT1 ("data/finals2000A.daily", &gmt), 0.5484645, 10e-5);

	gmt.tm_year = 118;
	gmt.tm_mon = 1;
	gmt.tm_mday = 5;
	ck_assert_msg (std::isnan (getDUT1 ("data/finals2000A.daily", &gmt)), "data for 2018 found!");

	gmt.tm_year = 117;
	gmt.tm_mon = 6;
	gmt.tm_mday = 28;
	ck_assert_dbl_eq (getDUT1 ("data/finals2000A.daily", &gmt), 1.3652135, 10e-5);

	ck_assert_msg (std::isnan (getDUT1 ("data/final2000A.daily", &gmt)), "data for not existing file found!");
}
END_TEST

START_TEST(UPDATE)
{
	const char *fn = "dut1_download";
	unlink (fn);

	int ret = retrieveDUT1 (fn, NULL);
	if (ret < 0)
	{
		printf ("cannot download updated file, skipping update tests");
		return;
	}

	time_t now = time (NULL);
	struct tm *gmt;
	gmt = gmtime (&now);

	double dut1 = getDUT1 (fn, gmt);
	printf ("Current DUT1: %f\n", dut1);
	ck_assert_msg (!std::isnan (dut1), "current data not found in downloade time diff file!");

	gmt->tm_year += 3;
	ck_assert_msg (std::isnan (getDUT1 (fn, gmt)), "found data for DUT1 3 years from now!");

	unlink (fn);
}
END_TEST

Suite * dut1_suite (void)
{
	Suite *s;
	TCase *tc_dut1, *tc_update;

	s = suite_create ("DUT1");
	tc_dut1 = tcase_create ("DUT1 extraction");

	tcase_add_checked_fixture (tc_dut1, setup_dut1, teardown_dut1);
	tcase_add_test (tc_dut1, GETDUT1);
	suite_add_tcase (s, tc_dut1);

	tc_update = tcase_create ("DUT1 download");
	tcase_add_test (tc_update, UPDATE);
	suite_add_tcase (s, tc_update);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = dut1_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
