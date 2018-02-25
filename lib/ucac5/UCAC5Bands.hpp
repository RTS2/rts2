#ifndef __UCAC5BANDS__
#define __UCAC5BANDS__


#include <stdint.h>

struct band
{
	uint16_t x0;
	uint16_t xn;
	uint16_t zn;
	uint16_t j;
};

class UCAC5Bands
{
	public:
		UCAC5Bands();
		~UCAC5Bands();
		int openBand(const char *bfn);

	private:
		int fd;
		struct band *data;
};

#endif // !__UCAC5BANDS__
