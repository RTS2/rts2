#include "UCAC5Idx.hpp"

#include <erfa.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

UCAC5Idx::UCAC5Idx ():fd(-1), data(NULL), dataSize(0), current(NULL), currentEnd(NULL)
{
}

UCAC5Idx::~UCAC5Idx ()
{
	if (data)
		munmap(data, dataSize);
	if (fd >= 0)
		close(fd);
}

int UCAC5Idx::openIdx (const char *idx)
{
	fd = open(idx, O_RDONLY);
	if (fd == -1)
		return -1;
	struct stat sb;
	int ret = fstat(fd, &sb);
	if (ret)
		return -1;
	dataSize = sb.st_size;
	data = (Vector*) mmap(NULL, dataSize, PROT_READ, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED)
		return -1;
	return 0;
}

int UCAC5Idx::select (size_t offset, size_t length)
{
	if (offset + length > (dataSize / sizeof(Vector)))
		return -1;
	current = data + offset;
	currentEnd = current + length;
	return 0;
}

int UCAC5Idx::nextMatched (Vector *fc, double minRad, double maxRad)
{
	while (current < currentEnd)
	{
		double d = eraSepp(fc->data, current->data);
		if (d >= minRad && d <= maxRad)
			return current - data;
		current++;
	}
	// no more entry found
	return -1;
}
