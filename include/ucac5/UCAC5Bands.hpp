#ifndef __UCAC5BANDS__
#define __UCAC5BANDS__

#include <stdint.h>
#include <sys/types.h>

class UCAC5Bands
{
	public:
		UCAC5Bands();
		~UCAC5Bands();
		int openBand(const char *bfn);

		/**
		 * Returns index of next RA/Dec band.
		 *
		 * @param  ra      queried RA (radians)
		 * @param  dec     queried Dec (radians)
		 * @param  radius  radius (radians)
		 */
		int nextBand(double ra, double dec, double radius, uint16_t &dec_b, uint16_t &ra_b, uint32_t &ra_start, int32_t &len);
	private:
		int fd;
		uint32_t *data;
		size_t dataSize;

		// total size
		int total_dec;
		int total_ra;
};

#endif // !__UCAC5BANDS__
