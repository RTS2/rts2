#include <stdlib.h>
#include <check.h>
#include <check_utils.h>

#include <libnova/libnova.h>

#include "sgp4.h"

void setup_sgp4 (void)
{
}

void teardown_sgp4 (void)
{
}

/**
 * Run test on TLEs.
 */
void test_tle (const char *tle1, const char *tle2, double JD, struct ln_rect_posn *sat_pos)
{
	rts2sgp4::elsetrec satrec;
	struct ln_rect_posn v;

	int ret = rts2sgp4::init (tle1, tle2, &satrec);
	ck_assert_int_eq (ret, 0);

	ret = rts2sgp4::propagate (&satrec, JD, sat_pos, &v);
	ck_assert_int_eq (ret, 0);
}


void test_tle_ra_dec (const char *tle1, const char *tle2, double JD, struct ln_lnlat_posn *observer, double altitude, double ra, double dec, double distance)
{
	struct ln_rect_posn sat_pos;

	test_tle (tle1, tle2, JD, &sat_pos);

	struct ln_rect_posn observer_loc;

	struct ln_equ_posn radec;
	double dist_to_satellite;

	double rho_cos;
	double rho_sin;

	rts2sgp4::ln_lat_alt_to_parallax (observer, altitude, &rho_cos, &rho_sin);
	rts2sgp4::ln_observer_cartesian_coords (observer, rho_cos, rho_sin, JD, &observer_loc);

	rts2sgp4::ln_get_satellite_ra_dec_delta (&observer_loc, &sat_pos, &radec, &dist_to_satellite);

	ln_get_equ_prec (&radec, JD, &radec);

	ck_assert_dbl_eq (radec.ra, ra, 0.5);
	ck_assert_dbl_eq (radec.dec, dec, 0.5);
	ck_assert_dbl_eq (dist_to_satellite, distance, 0.5);
}

void test_tle_alt_az (const char *tle1, const char *tle2, double JD, struct ln_lnlat_posn *observer, double altitude, double alt, double az, double distance)
{
	struct ln_rect_posn sat_pos;

	test_tle (tle1, tle2, JD, &sat_pos);

	struct ln_rect_posn observer_loc;

	struct ln_equ_posn radec;
	double dist_to_satellite;

	double rho_cos;
	double rho_sin;

	rts2sgp4::ln_lat_alt_to_parallax (observer, altitude, &rho_cos, &rho_sin);
	rts2sgp4::ln_observer_cartesian_coords (observer, rho_cos, rho_sin, JD, &observer_loc);

	rts2sgp4::ln_get_satellite_ra_dec_delta (&observer_loc, &sat_pos, &radec, &dist_to_satellite);

	struct ln_hrz_posn hrz;

	ln_get_hrz_from_equ (&radec, observer, JD, &hrz);

	ck_assert_dbl_eq (hrz.alt, alt, 0.5);
	ck_assert_dbl_eq (hrz.az, az, 0.5);
	ck_assert_dbl_eq (dist_to_satellite, distance, 0.5);
}

START_TEST(PLUTO)
{
	const char *tle1 = "1 25544U 98067A   02256.70033192  .00045618  00000-0  57184-3 0  1499";
	const char *tle2 = "2 25544  51.6396 328.6851 0018421 253.2171 244.7656 15.59086742217834";

	double JD = 2452541.5;         /* 24 Sep 2002 0h UT */

	struct ln_lnlat_posn observer;
	observer.lng = -69.9;
	observer.lat = 44.01;

	test_tle_ra_dec (tle1, tle2, JD, &observer, 100, 350.1615, -24.0241, 1867.97542);
}
END_TEST

START_TEST(ISS)
{
	const char *tle1 = "1 25544U 98067A   16128.85424799  .00005564  00000-0  90091-4 0  9999";
	const char *tle2 = "2 25544  51.6438 259.2325 0002021  92.7504  10.7493 15.54477273998701";

	struct ln_date test_t;
	test_t.years = 2016;
	test_t.months = 5;
	test_t.days = 10;
	test_t.hours = 3;
	test_t.minutes = 47;
	test_t.seconds = 26;

	double JD = ln_get_julian_day (&test_t);

	struct ln_lnlat_posn observer;
	observer.lng = -4.4643;
	observer.lat = 40.4610;

	test_tle_alt_az (tle1, tle2, JD, &observer, 791, 21, 14.7, 963);

	test_t.minutes = 49;
	test_t.seconds = 8;
	JD = ln_get_julian_day (&test_t);

	test_tle_alt_az (tle1, tle2, JD, &observer, 791, 39, 318, 621);

	test_t.minutes = 52;
	test_t.seconds = 12;
	JD = ln_get_julian_day (&test_t);

	test_tle_alt_az (tle1, tle2, JD, &observer, 791, 10, 247, 1448);

	test_t.minutes = 54;
	test_t.seconds = 22;
	JD = ln_get_julian_day (&test_t);

	test_tle_alt_az (tle1, tle2, JD, &observer, 791, 0, 239.6, 2315);
}
END_TEST

START_TEST(XMM)
{
	const char *tle1 = "1 25989U 99066A   16126.72024749 -.00000083  00000-0  00000+0 0  9995";
	const char *tle2 = "2 25989  67.4812  25.2476 8203967  94.8547 359.5975  0.50170988 18843";

	// test 1, 2016-05-07T19:09:47 CEST = 2016-05-07U17:09:47
	struct ln_date test_t;
	test_t.years = 2016;
	test_t.months = 5;
	test_t.days = 7;
	test_t.hours = 17;
	test_t.minutes = 9;
	test_t.seconds = 47;

	double JD = ln_get_julian_day (&test_t);
	ck_assert_dbl_eq (JD, 2457516.215127, 10e-5);

	struct ln_lnlat_posn observer;
	observer.lng = -4.4643;
	observer.lat = 40.4610;

	test_tle_alt_az (tle1, tle2, JD, &observer, 791, 37, 184, 6974);
/*
Vychází	18:18:40	0°	271° (Z)	18 309	11,6	32,8°
Dosahuje výšku nad obzorem 10°	18:38:41	10°	286° (ZSZ)	12 988	11,1	29,0°
Nejvyšší poloha	19:09:47	38°	4° (S)	6 974	8,6	23,1°
Klesne pod výšku nad obzorem 10°	19:39:02	10°	81° (V)	12 332	9,1	17,6°
Zapadá	19:55:46	0°	95° (V)	16 852	9,7	14,5° */
}
END_TEST

Suite * tle_suite (void)
{
	Suite *s;
	TCase *tc_sgp4;

	s = suite_create ("SGP4");
	tc_sgp4 = tcase_create ("SGP4 orbit calculations");

	tcase_add_checked_fixture (tc_sgp4, setup_sgp4, teardown_sgp4);
	tcase_add_test (tc_sgp4, PLUTO);
	tcase_add_test (tc_sgp4, ISS);
//	tcase_add_test (tc_sgp4, XMM);
	suite_add_tcase (s, tc_sgp4);

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
