#include "../writers/rts2image.h"

#include <unistd.h>
#include <iostream>

int
main (int argc, char **argv)
{
	if (!argv[1])
	{
		std::cout << "Don't get image!" << std::endl;
		return 1;
	}
	Rts2Image *image;
	long naxes[2];
	image = new Rts2Image (argv[1]);
	image->openImage ();
	std::cout << "average: " << image->getAverage () << std::endl;
	image->getValues ("NAXIS", naxes, 2);
	std::cout << "NAXIS: " << naxes[0] << "x" << naxes[1] << std::endl;
	sleep (10);
	std::cout << "change 132 10" << std::endl;
	return 0;
}
