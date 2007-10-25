#ifndef __RTS2_SPIRAL__
#define __RTS2_SPIRAL__
/**
 * Compute next spiral coordinates.
 *
 * Class which holds some internal variables to compute next point in spiral.
 * Is currently used only for HAM centering
 *
 * @author Petr Kubanek <pkubanek@asu.cas.cz>
 */

class Rts2Spiral
{
	private:
		int step;
		int step_size_x;
		int step_size_y;
		int step_size;
		int up_d;
	public:
		Rts2Spiral ();
		void getNextStep (short &x, short &y);
};
#endif							 /* !__RTS2_SPIRAL__ */
