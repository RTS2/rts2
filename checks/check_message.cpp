#include "message.h"

#include <iostream>

#include <check.h>
#include <check_utils.h>

void setup_messages (void)
{
}

void teardown_messages (void)
{
}

START_TEST(message_format)
{
	rts2core::Message m1 (1700000, "T0", MESSAGE_INFO | INFO_MOUNT_SLEW_START, "12.34 +56.10 10.33 -56.10 12.35 85.23 270.36 -80.20");
	ck_assert_str_eq ("moving from 00:49:21.600 +056 06 00.00 (altaz +012 21 00.00 +085 13 48.00) to 00:41:19.200 -056 06 00.00 (altaz +270 21 36.00 -080 12 00.00)", m1.expandString ("moving from $H1 $D2 (altaz $D5 $D6) to $H3 $D4 (altaz $D7 $D8)").c_str ());
}
END_TEST

Suite * message_suite (void)
{
	Suite *s;
	TCase *tc_message;

	s = suite_create ("Message");
	tc_message = tcase_create ("Message format");

	tcase_add_checked_fixture (tc_message, setup_messages, teardown_messages);
	tcase_add_test (tc_message, message_format);

	suite_add_tcase (s, tc_message);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = message_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
