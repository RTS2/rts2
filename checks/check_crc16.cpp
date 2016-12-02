#include "utilsfunc.h"

#include <iostream>

#include <check.h>
#include <check_utils.h>

void setup_messages (void)
{
}

void teardown_messages (void)
{
}

START_TEST(crc16)
{
	// from Binder documentation
	ck_assert_int_eq (getMsgBufCRC16 ("\x14\x03\x00\x01\x00\x02", 6), 0x0e97);
	ck_assert_int_eq (getMsgBufCRC16 ("\x14\x03\x04\x03\xe8\x01\xf4", 7), 0x953e);

	ck_assert_int_eq (getMsgBufCRC16 ("\x14\x05\x00\x18\xff\x00", 6), 0xf80e);

	// Modbus wikipedia
	ck_assert_int_eq (getMsgBufCRC16 ("\x01\x04\x02\xFF\xFF", 5), 0x80b8);
}
END_TEST

Suite * message_suite (void)
{
	Suite *s;
	TCase *tc_crc16;

	s = suite_create ("CRC16");
	tc_crc16 = tcase_create ("CRC-16 calculation");

	tcase_add_checked_fixture (tc_crc16, setup_messages, teardown_messages);
	tcase_add_test (tc_crc16, crc16);

	suite_add_tcase (s, tc_crc16);

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
