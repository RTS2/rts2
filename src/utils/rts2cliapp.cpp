#include "rts2cliapp.h"

#include <iostream>

Rts2CliApp::Rts2CliApp (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
}


void
Rts2CliApp::afterProcessing ()
{
}


int
Rts2CliApp::run ()
{
	int ret;
	ret = init ();
	if (ret)
		return ret;
	ret = doProcessing ();
	afterProcessing ();
	return ret;
}


int
Rts2CliApp::parseDate (const char *in_date, struct ln_date *out_time)
{
	int ret;
	int ret2;
	out_time->hours = out_time->minutes = 0;
	out_time->seconds = 0;
	ret =
		sscanf (in_date, "%i-%i-%i%n", &out_time->years, &out_time->months,
		&out_time->days, &ret2);
	if (ret == 3)
	{
		in_date += ret2;
		// we end with is T, let's check if it contains time..
		if (*in_date == 'T' || isspace (*in_date))
		{
			in_date++;
			ret2 =
				sscanf (in_date, "%u:%u:%lf", &out_time->hours,
				&out_time->minutes, &out_time->seconds);
			if (ret2 == 3)
				return 0;
			ret2 =
				sscanf (in_date, "%u:%u", &out_time->hours, &out_time->minutes);
			if (ret2 == 2)
				return 0;
			ret2 = sscanf (in_date, "%i", &out_time->hours);
			if (ret2 == 1)
				return 0;
			std::cerr << "Cannot parse time of the date: " << in_date << std::
				endl;
			return -1;
		}
		// only year..
		return 0;
	}
	std::cerr << "Cannot parse date: " << in_date << std::endl;
	return -1;
}
