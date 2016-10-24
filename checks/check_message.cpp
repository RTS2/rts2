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
	ck_assert_str_eq ("moving from 00:49:21.600 +056:06:00.00 (altaz +012:21:00.00 +085:13:48.00) to 00:41:19.200 -056:06:00.00 (altaz +270:21:36.00 -080:12:00.00)", m1.expandString ("moving from $H1 $D2 (altaz $D5 $D6) to $H3 $D4 (altaz $D7 $D8)").c_str ());

	ck_assert_str_eq ("moving from 00:49:21.600 +056:06:00.00 (altaz 12:21:00.00 85:13:48.00) to 00:41:19.200 -056:06:00.00 (altaz 270:21:36.00 -80:12:00.00)", m1.expandString ("moving from $H1 $D2 (altaz $A5 $A6) to $H3 $D4 (altaz $A7 $A8)").c_str ());

	rts2core::Message m2 (1700001, "T0", MESSAGE_INFO | INFO_MOUNT_SLEW_START, "12.34 +56.10 10.33 -56.10 0.35 0 270.36 -80.20");

	ck_assert_str_eq ("moving from 00:49:21.600 +056:06:00.00 (altaz 0:21:00.00 0:00:00.00) to 00:41:19.200 -056:06:00.00 (altaz 270:21:36.00 -80:12:00.00)", m2.expandString ("moving from $H1 $D2 (altaz $A5 $A6) to $H3 $D4 (altaz $A7 $A8)").c_str ());

	rts2core::Message m3 (1700002, "T0", MESSAGE_INFO | DEBUG_MOUNT_TRACKING_LOG, "270.319554 -49.995853 270.326394 -49.999734 270.323712 -49.997449 270.320508 -49.999833 0.004076 -0.159195 -0.076785 -0 -0 0.456 0.678");
	ck_assert_str_eq ("target 270.319554 -49.995853 precession 270.326394 -49.999734 nutation 270.323712 -49.997449 aberation 270.320508 -49.999833 refraction 0.004076 model -0.159195 -0.076785 etar -0 -0 htar 0.456 0.678", m3.expandString ("target $1 $2 precession $3 $4 nutation $5 $6 aberation $7 $8 refraction $9 model $10 $11 etar $12 $13 htar $14 $15").c_str ());
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
