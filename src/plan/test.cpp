#include "target.h"
#include <stdio.h>
#include <libnova/libnova.h>

int main (int argc, char **argv)
{
        ParTarget *target;
        struct ln_par_orbit orbit;
        orbit.q = 0.16749;
        orbit.w = 333.298;
        orbit.omega = 222.992;
        orbit.i = 64.118;
        orbit.JD = 2453112.629;
        target = new ParTarget (&orbit);
        struct ln_equ_posn pos;
        target->getPosition (&pos, ln_get_julian_from_sys ());
        printf ("RA: %f DEC: %f\n", pos.ra, pos.dec);
        return 0;
}
