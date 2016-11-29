#ifndef __CHECK_UTILS__
#define __CHECK_UTILS__

#include <math.h>

#include <check.h>
#include <check_utils.h>

#define ck_assert_dbl_eq(v1,v2,alow)  ck_assert_msg(fabs((v1)-(v2)) < (alow), "difference between %.10f and %.10f > %.10f", (double)(v1), (double)(v2), (double)(alow));
//to find out errors..
//#define ck_assert_dbl_eq(v1,v2,alow)  if (fabs((v1)-(v2)) >= (alow)) { printf ("%s:%d: difference between %.4f and %.10f > %.10f\n", __FILE__, __LINE__, (double)(v1), (double)(v2), (double)(alow)); }
#define ck_assert_lng_dbl_eq(v1,v2,alow)  ck_assert_msg(fabs((v1)-(v2)) < (alow), "difference between %.10Lf and %.10Lf > %.10Lf", (long double)(v1), (long double)(v2), (long double)(alow));

#endif //!__CHECK_UTILS__
