#include "rts2spiral.h"

Rts2Spiral::Rts2Spiral ()
{
	step = 0;
	step_size_x = 1;
	step_size_y = 1;
	step_size = step_size_x * 2 + step_size_y;
	up_d = 1;
}


void
Rts2Spiral::getNextStep (short &x, short &y)
{
	if (step == step_size)
	{
		up_d *= -1;
		if (up_d == 1)
		{
			step_size_x++;
		}
		step_size_y++;
		step_size = step_size_x * 2 + step_size_y;
		step = 0;
	}
	y = 0;
	x = 0;
	if (step < step_size_x)
		y = 1;
	else if (step < step_size_x + step_size_y)
		x = 1;
	else
		y = -1;
	y *= up_d;
	x *= up_d;
	step++;
}
