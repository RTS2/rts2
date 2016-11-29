#include "gem.h"

class GemTest:public rts2teld::GEM
{
	public:
		GemTest (int argc, char **argv);
		~GemTest ();

		void setTelescope (double _lat, double _long, double _alt, long _ra_ticks, long _dec_ticks, int _haZeroPos, double _haZero, double _decZero, double _haCpd, double _decCpd, long _acMin, long _acMax, long _dcMin, long _dcMax);
		int test_sky2counts (const double utc1, const double utc2, struct ln_equ_posn *pos, int32_t &ac, int32_t &dc);
		int test_counts2sky (const double utc1, const double utc2, int32_t ac, int32_t dc, double &ra, double &dec);
		int test_counts2hrz (const double utc1, const double utc2, int32_t ac, int32_t dc, struct ln_hrz_posn *hrz);

		double test_getAltitudePressure (double alt, double see_press) { return getAltitudePressure (alt, see_press); };

		void test_getHrzFromEqu (struct ln_equ_posn *pos, double JD, struct ln_hrz_posn *hrz) { return getHrzFromEqu (pos, JD, hrz); };
		void test_getEquFromHrz (struct ln_hrz_posn *hrz, double JD, struct ln_equ_posn *pos) { return getEquFromHrz (hrz, JD, pos); };

		void test_applyRefraction (struct ln_equ_posn *pos, double JD, bool writeValue) { return applyRefraction (pos, JD, writeValue); };
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
