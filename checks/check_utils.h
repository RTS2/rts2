#ifndef __CHECK_UTILS__
#define __CHECK_UTILS__

#include <math.h>

#include <check.h>
#include <check_utils.h>

inline void ck_assert_dbl_eq(long double v1, long double v2, long double alow)
{
	ck_assert_msg(fabs(v1-v2) < alow, "difference between %Lf and %Lf > %Lf", v1, v2, alow);
}

#endif //!__CHECK_UTILS__
