/* 
 * UCAC5 indexing application
 * Copyright (C) 2018 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "app.h"
#include "ucac5/UCAC5Record.hpp"

#include <errno.h>
#include <string>
#include <vector>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

class Ucac5Idx:public rts2core::App
{
	public:
		Ucac5Idx (int argc, char **argv);

		virtual int run();
	
	protected:
		virtual int processOption (int opt);
		virtual int processArgs (const char *arg);
	
	private:
		std::vector <const char *> files;
		bool dump;
};

Ucac5Idx::Ucac5Idx (int argc, char **argv):App (argc, argv), dump(false)
{
	addOption('d', NULL, 0, "prints processed stars");
}

int Ucac5Idx::run()
{
	int ret = init();
	if (ret)
		return ret;

	for (std::vector <const char *>::iterator iter = files.begin(); iter != files.end(); iter++)
	{
		int fd = open(*iter, O_RDONLY);
		if (fd < 0)
		{
			std::cerr << "Cannot open " << *iter << ": " << strerror(errno) << std::endl;
			continue;
		}

		std::string fon = std::string(*iter) + ".xyz";

		int fo = open(fon.c_str(), O_CREAT | O_TRUNC | O_WRONLY);
		if (fo < 0)
		{
			std::cerr << "Cannot create " << fon << ": " << strerror(errno) << std::endl;
			close(fd);
			continue;
		}

		ret = fchmod(fo, 0644);
		if (ret < 0)
		{
			std::cerr << "Cannot change mode of " << fon << ": " << strerror(errno) << std::endl;
		}

		std::cout << "Processing " << *iter;
		struct stat s;
		ret = fstat(fd, &s);
		if (ret < 0)
		{
			std::cerr << "Cannot get length of " << *iter << ": " << strerror(errno) << std::endl;
			close(fd);
			continue;
		}

		struct ucac5 *data = (struct ucac5*) mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
		if (data == MAP_FAILED)
		{
			std::cerr << "Cannot map memory file of size " << s.st_size << " for file " << *iter << ": " << strerror(errno) << std::endl;
			close(fd);
			return -1;
		}
		struct ucac5 *dp = data;
		size_t rn = 0;
		while ((char*) dp < ((char *) data + s.st_size))
		{
			UCAC5Record rec(dp);
			if (dump)
				std::cout << rec.getString() << std::endl;
			double c[3];
			rec.getXYZ(c);
			write(fo, c, sizeof(c));
			dp++;
			rn++;
		}

		close(fd);
		close(fo);

		std::cout << " " << s.st_size << " bytes, " << rn << " records" << std::endl;

		munmap(data, s.st_size);
	}
	return 0;
}

int Ucac5Idx::processOption (int opt)
{
	switch (opt)
	{
		case 'd':
			dump = true;
			break;
		default:
			return App::processOption(opt);
	}
	return 0;
}

int Ucac5Idx::processArgs (const char *arg)
{
	files.push_back(arg);
	return 0;
}

int main (int argc, char **argv)
{
	Ucac5Idx app(argc, argv);
	return app.run();
}
