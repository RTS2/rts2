#include "gem.h"

class GemTest:public rts2teld::GEM
{
	public:
		GemTest (int argc, char **argv);
		~GemTest ();

		void setTelescope (double _lat, double _long, double _alt, long _ra_ticks, long _dec_ticks, int _haZeroPos, double _haZero, double _decZero, double _haCpd, double _decCpd, long _acMin, long _acMax, long _dcMin, long _dcMax);
		int test_sky2counts (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc);
		int test_counts2sky (double JD, int32_t ac, int32_t dc, double &ra, double &dec);
		int test_counts2hrz (double JD, int32_t ac, int32_t dc, struct ln_hrz_posn *hrz);

		/**
		 * Test movement to given target position, from counts in ac dc parameters.
		 */
		float test_move (double JD, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc, float speed, float max_time);

	protected:
		virtual int isMoving () { return 0; };
		virtual int startResync () { return 0; }
		virtual int stopMove () { return 0; }
		virtual int startPark () { return 0; }
		virtual int endPark () { return 0; }
		virtual int updateLimits() { return 0; };
};
