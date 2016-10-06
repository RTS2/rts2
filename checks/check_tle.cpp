#include <stdlib.h>
#include <check.h>
#include <check_utils.h>

#include "pluto/norad.h"
#include "pluto/observe.h"
#include <libnova/libnova.h>

void setup_tle (void)
{
}

void teardown_tle (void)
{
}

/**
 * Run test on TLEs.
 */
void test_tle (const char *tle1, const char *tle2, double JD, struct ln_lnlat_posn *observer, double altitude, double sat_pos[3])
{
	tle_t tle;

	int ret = parse_elements (tle1, tle2, &tle);
	ck_assert_int_eq (ret, 0);

	int ephem = 1;
	int is_deep = select_ephemeris (&tle);

	if (is_deep && (ephem == 1 || ephem == 2))
		ephem += 2;	/* switch to an SDx */
	if (!is_deep && (ephem == 3 || ephem == 4))
		ephem -= 2;	/* switch to an SGx */

	double sat_params[N_SAT_PARAMS];

	double t_since = (JD - tle.epoch) * 1440.;

	switch (ephem)
	{
		case 0:
			SGP_init (sat_params, &tle);
			SGP (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 1:
			SGP4_init (sat_params, &tle);
			SGP4 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 2:
			SGP8_init (sat_params, &tle);
			SGP8 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 3:
			SDP4_init (sat_params, &tle);
			SDP4 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 4:
			SDP8_init (sat_params, &tle);
			SDP8 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		default:
			ck_abort_msg (0, "invalid ephem value: %d", ephem);
	}
}


void test_tle_ra_dec (const char *tle1, const char *tle2, double JD, struct ln_lnlat_posn *observer, double altitude, double ra, double dec, double distance)
{
	double sat_pos[3];

	test_tle (tle1, tle2, JD, observer, altitude, sat_pos);

	double observer_loc[3];

	struct ln_equ_posn radec;
	double dist_to_satellite;

	double rho_cos;
	double rho_sin;

	lat_alt_to_parallax (ln_deg_to_rad (observer->lat), altitude, &rho_cos, &rho_sin);
	observer_cartesian_coords (JD, ln_deg_to_rad (observer->lng), rho_cos, rho_sin, observer_loc);

	get_satellite_ra_dec_delta (observer_loc, sat_pos, &radec.ra, &radec.dec, &dist_to_satellite);

	epoch_of_date_to_j2000 (JD, &radec.ra, &radec.dec);

	ck_assert_dbl_eq (ln_rad_to_deg (radec.ra), ra, 0.5);
	ck_assert_dbl_eq (ln_rad_to_deg (radec.dec), dec, 0.5);
	ck_assert_dbl_eq (dist_to_satellite, distance, 0.5);
}

void test_tle_alt_az (const char *tle1, const char *tle2, double JD, struct ln_lnlat_posn *observer, double altitude, double alt, double az, double distance)
{
	double sat_pos[3];

	test_tle (tle1, tle2, JD, observer, altitude, sat_pos);

	double observer_loc[3];

	struct ln_equ_posn radec;
	struct ln_hrz_posn hrz;
	double dist_to_satellite;

	double rho_cos;
	double rho_sin;

	lat_alt_to_parallax (ln_deg_to_rad (observer->lat), altitude, &rho_cos, &rho_sin);
	observer_cartesian_coords (JD, ln_deg_to_rad (observer->lng), rho_cos, rho_sin, observer_loc);

	get_satellite_ra_dec_delta (observer_loc, sat_pos, &radec.ra, &radec.dec, &dist_to_satellite);

	epoch_of_date_to_j2000 (JD, &radec.ra, &radec.dec);

	radec.ra = ln_rad_to_deg (radec.ra);
	radec.dec = ln_rad_to_deg (radec.dec);

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
	TCase *tc_tle;

	s = suite_create ("TLE");
	tc_tle = tcase_create ("TLE orbit calculations");

	tcase_add_checked_fixture (tc_tle, setup_tle, teardown_tle);
	tcase_add_test (tc_tle, PLUTO);
	tcase_add_test (tc_tle, ISS);
//	tcase_add_test (tc_tle, XMM);
	suite_add_tcase (s, tc_tle);

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
