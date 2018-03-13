#ifndef __VECTOR__
#define __VECTOR__

struct XYZ
{
	double x;
	double y;
	double z;
};

struct Vector
{
	union
	{
		double data[3];
		struct XYZ xyz;
	};
};

#endif
