#include "UCAC5Record.hpp"

#include <string.h>

UCAC5Record::UCAC5Record (struct ucac5 *_data)
{
	memcpy(&data, _data, sizeof (data));
}

void UCAC5Record::getXYZ (double c[3])
{
	eraS2c(data.rag, data.dcg, c);
}

