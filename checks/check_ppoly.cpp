#include "gtp/ppoly.h"
#include "utilsfunc.h"

#include <check.h>
#include <check_utils.h>
#include <stdlib.h>

void setup_ppoly (void)
{
}

void teardown_ppoly (void)
{
}

START_TEST(poly1)
{
	std::vector<Point> p = parsePoly("0:0 0:10 1:10 1:4 8:4 8:10 9:10 9:0 0:0");
	ck_assert_int_eq(p.size(), 9);

	ck_assert_int_eq(p[0].x, 0);
	ck_assert_int_eq(p[0].y, 0);

	ck_assert_int_eq(p[1].x, 0);
	ck_assert_int_eq(p[1].y, 10);

	ck_assert_int_eq(p[2].x, 1);
	ck_assert_int_eq(p[2].y, 10);

	ck_assert_int_eq(p[3].x, 1);
	ck_assert_int_eq(p[3].y, 4);

	ck_assert_int_eq(cn_PnPoly(Point(-1,0), p), 0);
	ck_assert_int_eq(cn_PnPoly(Point(0.5,0.1), p), 1);

	ck_assert_int_eq(wn_PnPoly(Point(-1,0), p), 0);
	ck_assert_int_eq(wn_PnPoly(Point(0.5,0.1), p), -1);
}
END_TEST

Suite * poly_suite (void)
{
	Suite *s;
	TCase *tc_poly;

	s = suite_create ("PPOLY");
	tc_poly = tcase_create ("Point in Polygon");

	tcase_add_checked_fixture (tc_poly, setup_ppoly, teardown_ppoly);
	tcase_add_test (tc_poly, poly1);
	suite_add_tcase (s, tc_poly);

	return s;
}

int main (void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = poly_suite ();
	sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
