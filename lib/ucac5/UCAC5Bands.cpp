#include "UCAC5Bands.hpp"

#include <erfa.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <utilsfunc.h>


UCAC5Bands::UCAC5Bands(): fd(-1), data(NULL), dataSize(0), total_dec(900), total_ra(1440)
{
}

UCAC5Bands::~UCAC5Bands()
{
	if (data)
		munmap(data, dataSize);
	if (fd > 0)
		close(fd);
}

int UCAC5Bands::openBand(const char *bfn)
{
	fd = open(bfn, O_RDONLY);
	if (fd == -1)
		return -1;
	struct stat sb;
	int ret = fstat(fd, &sb);
	if (ret)
		return -1;
	dataSize = sb.st_size;

	if (dataSize != (size_t) (8 * total_dec * total_ra))
		return -1;
	
	dataSize /= 2;

	data = (uint32_t*) mmap(NULL, dataSize, PROT_READ, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED)
		return -1;

	return 0;
}

int UCAC5Bands::nextBand(double ra, double dec, double radius, uint16_t &dec_b, uint16_t &ra_b, uint32_t &ra_start, int32_t &len)
{
	double cd = cos(dec);
	// return all..
	if (cd < 1e-5)
	{
		if (dec_b >= total_dec)
			return -1;
		// have to read full band..
		ra_start = data[total_ra * dec_b + ra_b];
		len = -1;
		dec_b++;
		return 0;
	}

	double r_cd = radius / cd;
	// has to change dec_b
	if (dec - (dec_b * M_PI / total_dec  - M_PI / 2.0) > radius + M_PI / total_dec)
	{
		dec_b = floor(total_dec * (dec + M_PI / 2.0 - radius) / M_PI);
		if (dec_b < 0)
			dec_b = 0;
		if (dec_b >= total_dec)
			return -1;

		ra_b = floor((total_ra / 2) * (ra - r_cd) / M_PI);
		if (ra_b < 0)
			ra_b = 0;
		else if (ra_b >= total_ra)
			return -1;
	}
	else
	{
		ra_b++;
		// next band...
		if (ra_b > floor((total_ra / 2) * (ra + r_cd) / M_PI))
		{
			dec_b++;
			if (dec_b > floor(total_dec * (dec + M_PI / 2.0 + radius) / M_PI))
				return -1;
			ra_b = floor((total_ra / 2) * (ra - r_cd) / M_PI);
			if (ra_b < 0)
				ra_b = 0;
			else if (ra_b >= total_ra)
				return -1;
		}
	}
	ra_start = data[ra_b * total_dec + dec_b];
	if (ra_b == total_ra)
		len = -1;
	else
		len = data[(ra_b + 1) * total_dec + dec_b] - ra_start;
	return 0;
}
