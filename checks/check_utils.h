#ifndef __CHECK_UTILS__
#define __CHECK_UTILS__

#include <math.h>

#define ck_assert_dbl_eq(v1,v2,alow)  ck_assert_msg(fabs(v1-v2) < alow, "difference %f and %f > %f", v1, v2, alow)

#endif //!__CHECK_UTILS__
