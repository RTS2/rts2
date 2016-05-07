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

START_TEST(XMM)
{
	const char *tle1 = "1 25989U 99066A   16126.72024749 -.00000083  00000-0  00000+0 0  9995";
	const char *tle2 = "2 25989  67.4812  25.2476 8203967  94.8547 359.5975  0.50170988 18843";
	tle_t tle;

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

	double longitude = -4.4643;
	double latitude = 40.4610;
	double rho_cos;
	double rho_sin;
	double observer_loc[3];

	lat_alt_to_parallax (ln_deg_to_rad (latitude), 791, &rho_cos, &rho_sin);

	int ret = parse_elements (tle1, tle2, &tle);
	ck_assert_int_eq (ret, 0);

	int ephem = 1;
	int is_deep = select_ephemeris (&tle);
	if (is_deep && (ephem == 1 || ephem == 2))
		ephem += 2;	/* switch to an SDx */
	if (!is_deep && (ephem == 3 || ephem == 4))
		ephem -= 2;	/* switch to an SGx */

	double sat_pos[3]; /* Satellite position vector */
	double sat_params[N_SAT_PARAMS];

	observer_cartesian_coords (JD, ln_deg_to_rad (longitude), rho_cos, rho_sin, observer_loc);
	double t_since = (JD - tle.epoch) * 1440.;
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
	tcase_add_test (tc_tle, XMM);
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
