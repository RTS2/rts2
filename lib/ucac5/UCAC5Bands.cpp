#include "UCAC5Bands.hpp"

#include <unistd.h>

UCAC5Bands::UCAC5Bands(): fd(-1), data(NULL)
{

}

UCAC5Bands::~UCAC5Bands()
{
}

int UCAC5Bands::openBand(const char *bfn)
{
	return 0;
}
