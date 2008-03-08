#include "rts2option.h"

#include <iostream>
#include <iomanip>

void
Rts2Option::getOptionChar (char **end_opt)
{
	if (!isalnum (short_option))
		return;
	**end_opt = short_option;
	(*end_opt)++;
	if (has_arg)
	{
		**end_opt = ':';
		(*end_opt)++;
		// optional text after option..
		if (has_arg == 2)
		{
			**end_opt = ':';
			(*end_opt)++;
		}
	}
}


void
Rts2Option::help ()
{
	if (short_option < 900)
	{
		if (long_option)
		{
			std::cout << "  -" << ((char) short_option) << "|--" << std::
				left << std::setw (15) << long_option;
		}
		else
		{
			std::cout << "  -" << ((char) short_option) << std::
				setw (18) << " ";
		}
	}
	else
	{
		std::cout << "  --" << std::left << std::setw (18) << long_option;
	}
	std::cout << " " << help_msg << std::endl;
}
