/* 
 * Video for Linux webcam and other cameras driver.
 * Copyright (C) 2010 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "camd.h"

#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

namespace rts2camd
{

typedef struct
{
	void *start;
	size_t length;
} bufinfo_t;

/**
 * Class for V4L webcams.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class V4L:public Camera
{
	public:
		V4L (int in_argc, char **in_argv);
		virtual ~V4L (void);

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int startExposure ();
		virtual int doReadout ();
	private:
		const char *videodev;
		int fd;
		bufinfo_t *buffers;
		size_t bufsize;

};

}

using namespace rts2camd;

V4L::V4L (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
	videodev = "/dev/video0";
	fd = 0;
	buffers = NULL;

	addOption ('f', NULL, 1, "videodevice. Defaults to /dev/video0");
}

V4L::~V4L ()
{
	while (bufsize > 0)
	{
		munmap (buffers[bufsize].start, buffers[bufsize].length);
		bufsize--;
	}
	delete[] buffers;
	close (fd);
}

int V4L::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			videodev = optarg;
			break;
		default:
			return Camera::processOption (opt);
	}
	return 0;
}

int V4L::init ()
{
	int ret;
	ret = Camera::init ();
	if (ret)
		return ret;
	// try to open video file..
	fd = open (videodev, O_RDONLY);
	if (fd < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot open " << videodev << ":" << strerror (errno) << sendLog;
		return -1;
	}
	struct v4l2_capability cap;
	ret = ioctl (fd, VIDIOC_QUERYCAP, &cap);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot determine device capabilities: " << strerror (errno) << sendLog;
		return -1;
	}
	if (!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		logStream (MESSAGE_ERROR) << "device does not support streaming" << sendLog;
		return -1;
	}

	struct v4l2_requestbuffers reqbuf;
	memset (&reqbuf, 0, sizeof (reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = 5;

	if (ioctl (fd, VIDIOC_REQBUFS, &reqbuf) == -1)
	{
		logStream (MESSAGE_ERROR) << "cannot request buffers" << sendLog;
		return -1;
	}
	
	buffers = new bufinfo_t[reqbuf.count];

	for (bufsize = 0; bufsize < reqbuf.count; bufsize++)
	{
		struct v4l2_buffer buffer;
		memset (&buffer, 0, sizeof (buffer));
		buffer.type = reqbuf.type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = bufsize;
	
		if (ioctl (fd, VIDIOC_QUERYBUF, &buffer) == -1)
		{
			logStream (MESSAGE_ERROR) << "cannot get buffer information for buffer #" << (bufsize + 1) << ":" << strerror (errno) << sendLog;
			return -1;
		}
		buffers[bufsize].length = buffer.length;
		buffers[bufsize].start = mmap (NULL, buffer.length, PROT_READ, MAP_SHARED, fd, buffer.m.offset);

		if (buffers[bufsize].start == MAP_FAILED)
		{
			logStream (MESSAGE_ERROR) << "cannot map buffer # " << (bufsize + 1) << ":" << strerror (errno) << sendLog;
			return -1;
		}
	}

	setSize (10, 5, 0, 0);

	return 0;
}

int V4L::startExposure ()
{
	return 0;
}

int V4L::doReadout ()
{
	sendReadoutData ((char *) buffers[0].start, 100);
	return -2;
}

int main (int argc, char **argv)
{
	V4L device = V4L (argc, argv);
	return device.run ();
}
