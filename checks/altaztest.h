#include "altaz.h"

class AltAzTest:public rts2teld::AltAz
{
	public:
		AltAzTest (int argc, char **argv);
		~AltAzTest ();

		void setTelescope (double _lat, double _long, double _alt, long _az_ticks, long _alt_ticks, double _azZero, double _altZero, double _azCpd, double _altCpd, long _azMin, long _azMax, long _altMin, long _altMax);
		int test_sky2counts (double JD, struct ln_equ_posn *pos, int32_t &azc, int32_t &altc);
		int test_hrz2counts (struct ln_hrz_posn *hrz, int32_t &azc, int32_t &altc);
		void test_counts2sky (double JD, int32_t azc, int32_t altc, double &ra, double &dec);
		void test_counts2hrz (int32_t azc, int32_t altc, struct ln_hrz_posn *hrz);

		void test_parallactic_angle (double ha, double dec, double &pa, double &parate) { parallactic_angle (ha, dec, pa, parate); };

		/**
		 * Test movement to given target position, from counts in ac dc parameters.
		 */
		float test_move (double JD, struct ln_equ_posn *pos, int32_t &azc, int32_t &altc, float speed, float max_time);

		double test_getAltitudePressure (double alt, double see_press) { return getAltitudePressure (alt, see_press); };

		void test_getHrzFromEqu (struct ln_equ_posn *pos, double JD, struct ln_hrz_posn *hrz) { return getHrzFromEqu (pos, JD, hrz); };
		void test_getHrzFromEquST (struct ln_equ_posn *pos, double ST, struct ln_hrz_posn *hrz) { return getHrzFromEquST (pos, ST, hrz); };
		void test_getEquFromHrz (struct ln_hrz_posn *hrz, double JD, struct ln_equ_posn *pos) { return getEquFromHrz (hrz, JD, pos); };
		void test_applyRefraction (struct ln_equ_posn *pos, double JD, bool writeValue) { return applyRefraction (pos, JD, writeValue); };
	protected:
		virtual int isMoving () { return 0; };
		virtual int startResync () { return 0; }
		virtual int stopMove () { return 0; }
		virtual int startPark () { return 0; }
		virtual int endPark () { return 0; }
		virtual int updateLimits() { return 0; };
};
