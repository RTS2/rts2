#include "altaztest.h"

#include <time.h>
#include <stdlib.h>
#include <check.h>
#include <check_utils.h>
#include <libnova/libnova.h>

// global telescope object
AltAzTest* altAzTest;

void setup_altcaz (void)
{
	static const char *argv[] = {"testapp"};
	altAzTest = new AltAzTest(0, (char **) argv);

	altAzTest->setTelescope (-40, -5, 200, 67108864, 67108864, 90, -7, 186413.511111, 186413.511111, -80000000, 80000000, -40000000, 40000000);
}

void teardown_altcaz (void)
{
	delete altAzTest;
	altAzTest = NULL;
}

void test_pa (double ha, double dec, double pa, double parate)
{
	double c_pa, c_parate;
	altAzTest->test_parallactic_angle (ha, dec, c_pa, c_parate);
	ck_assert_dbl_eq (c_pa, pa, 10e-4);
	ck_assert_dbl_eq (c_parate, parate, 10e-4);
}

START_TEST(derotator_1)
{
	test_pa (0, -70, 0, 22.9813);
	test_pa (1, -70, 1.5319, 22.9765);
	test_pa (15, -70, 22.6356, 21.9735);
	test_pa (-15, -70, -22.6356, 21.9735);
	test_pa (30, -70, 43.5044, 19.6526);
	test_pa (45, -70, 61.9054, 17.2009);
	test_pa (60, -70, 78.0773, 15.2443);
	test_pa (90, -70, 106.0128, 13.0227);
	test_pa (120, -70, 131.1507, 12.2828);
	test_pa (150, -70, 155.5714, 12.1934);
	test_pa (179, -70, 179.1847, 12.2281);
	test_pa (180, -70, 180.0, 12.2281);
	test_pa (181, -70, -179.1847, 12.2281);
	test_pa (210, -70, -155.5714, 12.1934);
	test_pa (240, -70, -131.1507, 12.2828);
	test_pa (270, -70, -106.0128, 13.0227);
	test_pa (300, -70, -78.0773, 15.2443);
	test_pa (315, -70, -61.9054, 17.2009);
	test_pa (330, -70, -43.5044, 19.6526);
	test_pa (359, -70, -1.5319, 22.9765);
	test_pa (360, -70, 0, 22.9813);
}
END_TEST

START_TEST(derotator_2)
{
	test_pa (1, -40, 0, 4.8211);
	test_pa (-1, -40, 0, 4.8211);
	test_pa (0, -39.99, 180, -65836.6706);
	test_pa (0, -40.01, 0, 65836.6705);
	test_pa (0.01, -40.01, 37.4549, 41493.6137);
	test_pa (-0.01, -40.01, -37.4549, 41493.6137);
	test_pa (0.01, -39.99, 142.5474, -41485.5540);
	test_pa (-0.01, -39.99, -142.5474, -41485.5540);
}
END_TEST

START_TEST(derotator_3)
{
	test_pa (15, -40, 94.8371, 4.8695);
	test_pa (30, -40, 99.7724, 5.0181);
	test_pa (60, -40, 110.3605, 5.6497);
	test_pa (90, -40, 122.7324, 6.8227);
	test_pa (120, -40, 138.0698, 8.6105);
	test_pa (150, -40, 157.3709, 10.6542);
	test_pa (179, -40, 179.2221, 11.6666);
	test_pa (180, -40, 180, 11.6679);
	test_pa (181, -40, -179.2221, 11.6666);
	test_pa (210, -40, -157.3709, 10.6542);
	test_pa (240, -40, -138.0698, 8.6105);
	test_pa (270, -40, -122.7324, 6.8227);
	test_pa (300, -40, -110.3605, 5.6497);
	test_pa (330, -40, -99.7724, 5.0181);
	test_pa (345, -40, -94.8371, 4.8695);
}
END_TEST

START_TEST(derotator_4)
{
	test_pa (0, -80, 0, 17.8763);
	test_pa (1, -80, 1.1917, 17.8754);
	test_pa (-1, -80, -1.1917, 17.8754);
	test_pa (15, -80, 17.8120, 17.6857);
	test_pa (30, -80, 35.2623, 17.1706);
	test_pa (60, -80, 68.1823, 15.7197);
	test_pa (90, -80, 98.2901, 14.4650);
	test_pa (120, -80, 126.3838, 13.7099);
	test_pa (150, -80, 153.4022, 13.3623);
	test_pa (179, -80, 179.1154, 13.2682);
	test_pa (180, -80, 180, 13.2682);
	test_pa (181, -80, -179.1154, 13.2682);
	test_pa (210, -80, -153.4022, 13.3623);
	test_pa (240, -80, -126.3838, 13.7099);
	test_pa (270, -80, -98.2901, 14.4650);
	test_pa (300, -80, -68.1823, 15.7197);
	test_pa (330, -80, -35.2623, 17.1706);
	test_pa (345, -80, -17.8120, 17.6857);
}
END_TEST

START_TEST(zenith_pa)
{
	test_pa (0, -39.5, 180, -1316.750118);
	test_pa (0, -39.5 + 2 / 60.0, 180, -1234.455394);
	test_pa (0, -39.5 - 2 / 60.0, 180, -1410.801390);
	test_pa (0 + 2 / 60.0, -39.5, 177.076456, -1313.287352);
	test_pa (0 - 2 / 60.0, -39.5, -177.076456, -1313.287352);

	test_pa (15, -39.5, 97.280406, 2.350959);
	test_pa (15, -39.5 - 2 / 60.0, 97.118261, 2.517749);
	test_pa (15, -39.5 + 2 / 60.0, 97.442431, 2.184371);
	test_pa (15 + 2 / 60.0, -39.5, 97.285642, 2.362140);
	test_pa (15 - 2 / 60.0, -39.5, 97.275194, 2.339706);
}
END_TEST

START_TEST(test_altaz_1)
{
	// test 1, 2016-01-12T19:20:47 HST = 2016-01-13U05:20:47
	struct ln_date test_t;
	test_t.years = 2016;
	test_t.months = 1;
	test_t.days = 13;
	test_t.hours = 5;
	test_t.minutes = 20;
	test_t.seconds = 47;

	double JD = ln_get_julian_day (&test_t);
	ck_assert_dbl_eq (JD, 2457400.7227662038, 10e-10);

	struct ln_hrz_posn hrz, res_hrz;
	hrz.alt = 80;
	hrz.az = 20;

	struct ln_equ_posn pos;
	pos.ra = 20;
	pos.dec = 80;

	int32_t azc = -20000000;
	int32_t altc = 1000;

	int ret = altAzTest->test_hrz2counts (&hrz, azc, altc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (azc, -13048946);
	ck_assert_int_eq (altc, 3169029);

	altAzTest->test_counts2hrz (azc, altc, &res_hrz);
	ck_assert_dbl_eq (res_hrz.az, hrz.az, 10e-5);
	ck_assert_dbl_eq (res_hrz.alt, hrz.alt, 10e-5);

	ck_assert_dbl_eq (ln_get_mean_sidereal_time (JD), ln_get_apparent_sidereal_time (JD) + 7.914799999397815e-06, 10e-10);

	altAzTest->modelOff ();

	ret = altAzTest->test_sky2counts (JD, 0, &pos, azc, altc);
	ck_assert_int_eq (ret, 0);
#ifdef USE_ERFA
	ck_assert_int_eq (azc, 16135692);
	ck_assert_int_eq (altc, 27308043);
#else
	ck_assert_int_eq (azc, 16147947);
	ck_assert_int_eq (altc, 27349159);
#endif

	altAzTest->test_counts2sky (JD, azc, altc, pos.ra, pos.dec);

#ifdef USE_ERFA
	ck_assert_dbl_eq (pos.ra, 20, 10e-1);
	ck_assert_dbl_eq (pos.dec, 80, 10e-1);
#else
	ck_assert_dbl_eq (pos.ra, 20, 10e-4);
	ck_assert_dbl_eq (pos.dec, 80, 10e-4);
#endif

	altAzTest->modelOn ();

	// origin
	pos.ra = 344.16613;
	pos.dec = -80.3703305;

	ret = altAzTest->test_sky2counts (JD, 0, &pos, azc, altc);
	ck_assert_int_eq (ret, 0);
#ifdef USE_ERFA
	ck_assert_int_eq (azc, 49514704);
	ck_assert_int_eq (altc, 12300262);
#else
	ck_assert_int_eq (azc, 49510274);
	ck_assert_int_eq (altc, 12292285);
#endif

	// target
	pos.ra = 62.95859;
	pos.dec = -80.51601;

	float e = altAzTest->test_move (JD, &pos, azc, altc, 2.0, 200);
	ck_assert_msg (!isnan (e), "position %f %f not reached", pos.ra, pos.dec);

	struct ln_equ_posn curr;
	curr.ra = curr.dec = 0;

	altAzTest->test_counts2sky (JD, azc, altc, curr.ra, curr.dec);

#ifdef USE_ERFA
	ck_assert_dbl_eq (pos.ra, curr.ra, 10e-1);
	ck_assert_dbl_eq (pos.dec, curr.dec, 10e-1);
#else
	ck_assert_dbl_eq (pos.ra, curr.ra, 10e-3);
	ck_assert_dbl_eq (pos.dec, curr.dec, 10e-3);
#endif

	altAzTest->test_counts2hrz (-70194687, -48165219, &hrz);
	ck_assert_dbl_eq (hrz.alt, -4.621631, 10e-3);
	ck_assert_dbl_eq (hrz.az, 73.446355, 10e-3);

	ret = altAzTest->test_hrz2counts (&hrz, azc, altc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (azc, 64023041);
	ck_assert_int_eq (altc, 18943644);

	altAzTest->test_counts2hrz (-68591258, -68591258, &hrz);
	ck_assert_dbl_eq (hrz.alt, 75.047819, 10e-2);
	ck_assert_dbl_eq (hrz.az, 262.047819, 10e-2);

	ret = altAzTest->test_hrz2counts (&hrz, azc, altc);
	ck_assert_int_eq (ret, 0);
	ck_assert_int_eq (azc, 32072038);
	ck_assert_int_eq (altc, 4092184);
}
END_TEST

START_TEST(test_altaz_2)
{
	// test 2, 2016-04-12T19:20:47 HST = 2016-4-13U20:23:47
	struct ln_date test_t;
	test_t.years = 2016;
	test_t.months = 4;
	test_t.days = 13;
	test_t.hours = 20;
	test_t.minutes = 23;
	test_t.seconds = 47;

	double JD = ln_get_julian_day (&test_t);
	ck_assert_dbl_eq (JD, 2457492.34985, 10e-5);

	ck_assert_dbl_eq (ln_get_mean_sidereal_time (JD), ln_get_apparent_sidereal_time (JD) + 6.842610000035165e-05, 10e-10);
}
END_TEST

Suite * altaz_suite (void)
{
	Suite *s;
	TCase *tc_altaz_pointings;

	s = suite_create ("ALT AZ");
	tc_altaz_pointings = tcase_create ("AltAz Pointings");

	tcase_add_checked_fixture (tc_altaz_pointings, setup_altcaz, teardown_altcaz);
	tcase_add_test (tc_altaz_pointings, derotator_1);
	tcase_add_test (tc_altaz_pointings, derotator_2);
	tcase_add_test (tc_altaz_pointings, derotator_3);
	tcase_add_test (tc_altaz_pointings, derotator_4);
	tcase_add_test (tc_altaz_pointings, zenith_pa);
	tcase_add_test (tc_altaz_pointings, test_altaz_1);
	tcase_add_test (tc_altaz_pointings, test_altaz_2);
	suite_add_tcase (s, tc_altaz_pointings);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = altaz_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
