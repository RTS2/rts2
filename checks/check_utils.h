#ifndef __CHECK_UTILS__
#define __CHECK_UTILS__

#include <math.h>

#include <check.h>
#include <check_utils.h>

#define ck_assert_dbl_eq(v1,v2,alow)  ck_assert_msg(fabs(v1-v2) < alow, "difference between %.10f and %.10f > %.10f", v1, v2, alow);
#define ck_assert_lng_dbl_eq(v1,v2,alow)  ck_assert_msg(fabs(v1-v2) < alow, "difference between %.10f and %.10f > %.10f", v1, v2, alow);

#endif //!__CHECK_UTILS__
