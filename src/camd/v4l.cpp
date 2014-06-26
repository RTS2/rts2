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

#ifndef V4L2_PIX_FMT_Y16
#define V4L2_PIX_FMT_Y16   v4l2_fourcc('Y', '1', '6', ' ')
#endif

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
		virtual int initHardware ();
		virtual void initDataTypes ();	

		virtual void setExposure (float in_exp);
		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);
		virtual int startExposure ();
		virtual int stopExposure ();
		virtual int doReadout ();
	private:
		const char *videodev;
		int fd;
		bufinfo_t *buffers;
		size_t bufsize;

		struct v4l2_requestbuffers reqbuf;
		// native CCD format
		__u32 format;

		rts2core::ValueSelection *greyMode;

		void everyEvenByte (char *bytes);
};

}

using namespace rts2camd;

V4L::V4L (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
	videodev = "/dev/video0";
	fd = 0;
	buffers = NULL;

	createValue (greyMode, "grey", "grey modes", true, RTS2_VALUE_WRITABLE);

	addOption ('f', NULL, 1, "videodevice. Defaults to /dev/video0");
}

V4L::~V4L ()
{
	ioctl (fd, VIDIOC_STREAMOFF, &(reqbuf.type));

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

int V4L::initHardware ()
{
	// try to open video file..
	fd = open (videodev, O_RDWR);
	if (fd < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot open " << videodev << ":" << strerror (errno) << sendLog;
		return -1;
	}
	struct v4l2_capability cap;
	int ret = ioctl (fd, VIDIOC_QUERYCAP, &cap);
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

	memset (&reqbuf, 0, sizeof (reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = 5;

	// get and set format..
	struct v4l2_format fmt;
	memset (&fmt, 0, sizeof (fmt));

	fmt.type = reqbuf.type;

	if (ioctl (fd, VIDIOC_G_FMT, &fmt) == -1)
	{
		logStream (MESSAGE_ERROR) << "cannot get current format: " << strerror (errno) << sendLog;
		return -1;
	}

	// formats in order of preference..
	__u32 formats[3] = {V4L2_PIX_FMT_Y16, V4L2_PIX_FMT_GREY, V4L2_PIX_FMT_YUYV};
	int i;
	for (i = 0; i < 3; i++)
	{
		fmt.fmt.pix.pixelformat = formats[i];
		if (ioctl (fd, VIDIOC_S_FMT, &fmt) == 0)
		{
			char f[5];
			memcpy (f, &fmt.fmt.pix.pixelformat, 4);
			f[4] = '\0';
			logStream (MESSAGE_INFO) << "camera support " << f << " format" << sendLog;
			format = fmt.fmt.pix.pixelformat;
			break;
		}
	}

	if (i == 3)
	{
		logStream (MESSAGE_ERROR) << "cannot set grey format: " << strerror (errno) << sendLog;
		return -1;
	}

	setSize (fmt.fmt.pix.width, fmt.fmt.pix.height, 0, 0);

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
		buffers[bufsize].start = mmap (NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);

		if (buffers[bufsize].start == MAP_FAILED)
		{
			logStream (MESSAGE_ERROR) << "cannot map buffer # " << (bufsize + 1) << ":" << strerror (errno) << sendLog;
			return -1;
		}
	}

	struct v4l2_queryctrl queryctrl;
	memset (&queryctrl, 0, sizeof (queryctrl));
	queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
	while (0 == ioctl (fd, VIDIOC_QUERYCTRL, &queryctrl))
	{
		if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
			continue;
#if defined(VL42_CID_EXPOSURE_AUTO) && defined (V4L2_CID_EXPOSURE_ABSOLUTE)
		switch (queryctrl.id)
		{
			case V4L2_CID_EXPOSURE_AUTO:
				break;
			case V4L2_CID_EXPOSURE_ABSOLUTE:
				break;
			default:
				logStream (MESSAGE_DEBUG) << "unhandled option: " << queryctrl.name << " " << queryctrl.id << sendLog;
		}
#endif
		queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
	}

	return 0;
}

void V4L::initDataTypes ()
{
	switch (format)
	{
		case V4L2_PIX_FMT_YUYV:
			addDataType (RTS2_DATA_USHORT);
			greyMode->addSelVal ("yuyv");
		case V4L2_PIX_FMT_GREY:
			addDataType (RTS2_DATA_BYTE);
			greyMode->addSelVal ("grey_8bit");
			break;
		case V4L2_PIX_FMT_Y16:
			addDataType (RTS2_DATA_USHORT);
			greyMode->addSelVal ("grey_16bit");
			break;
		default:
			logStream (MESSAGE_ERROR) << "data type " << format << " is not supported" << sendLog;
			exit (EXIT_FAILURE);
	}
}

void V4L::setExposure (float in_exp)
{
	// set exposure time
#if defined(V4L2_CID_EXPOSURE_AUTO) && defined(V4L2_EXPOSURE_MANUAL) && defined(V4L2_CID_EXPOSURE_ABSOLUTE)
	struct v4l2_ext_control extc[2];
	extc[0].id = V4L2_CID_EXPOSURE_AUTO;
	extc[0].value = V4L2_EXPOSURE_MANUAL;

	extc[1].id = V4L2_CID_EXPOSURE_ABSOLUTE;
	extc[1].value64 = (int64_t) (getExposure () * 10000.0);

	struct v4l2_ext_controls extcs;
	extcs.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
	extcs.count = 2;
	extcs.controls = extc;

	if (ioctl (fd, VIDIOC_S_EXT_CTRLS, &extcs) == -1)
	{
		logStream (MESSAGE_ERROR) << "cannot set exposure time" << sendLog;
	}
#endif
}

int V4L::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == greyMode)
	{
		switch (format)
		{
			case V4L2_PIX_FMT_YUYV:
				setDataType (newValue->getValueInteger ());
				return 0;
			default:
				return -2;
		}
	}
	return Camera::setValue (oldValue, newValue);
}

int V4L::startExposure ()
{

	struct v4l2_buffer buffer;
	buffer.type = reqbuf.type;
	buffer.memory = V4L2_MEMORY_MMAP;
	buffer.index = 0;

	if (ioctl (fd, VIDIOC_QBUF, &buffer) == -1)
	{
		logStream (MESSAGE_ERROR) << "cannot enque buffer: " << strerror (errno) << sendLog;
		return -1;
	}
	if (ioctl (fd, VIDIOC_STREAMON, &(reqbuf.type)) == -1)
	{
		logStream (MESSAGE_ERROR) << "cannot start stream: " << strerror (errno) << sendLog;
		return -1;
	}
	return 0;
}

int V4L::stopExposure ()
{
	if (ioctl (fd, VIDIOC_STREAMOFF, &(reqbuf.type)) == -1)
	{
		logStream (MESSAGE_ERROR) << "cannot stop exposure: " << strerror (errno) << sendLog;
		return -1;
	}
	return Camera::stopExposure ();
}

int V4L::doReadout ()
{
	switch (format)
	{
		case V4L2_PIX_FMT_YUYV:
			{
				switch (greyMode->getValueInteger ())
				{
					case 0:
						sendReadoutData ((char *) buffers[0].start, chipByteSize ());
						break;
					case 1:
						memcpy (getDataBuffer (0), buffers[0].start, chipByteSize () * 2);
						everyEvenByte (getDataBuffer (0));
						sendReadoutData (getDataBuffer (0), chipByteSize ());
						break;
				}
			}
			break;
		case V4L2_PIX_FMT_GREY:
		case V4L2_PIX_FMT_Y16:
			sendReadoutData ((char *) buffers[0].start, chipByteSize ());
			break;
	}
	if (ioctl (fd, VIDIOC_STREAMOFF, &(reqbuf.type)) == -1)
	{
		logStream (MESSAGE_ERROR) << "cannot end stream" << sendLog;
		return -1;
	}
	return -2;
}

void V4L::everyEvenByte (char *bytes)
{
	char *s;
	char *d;
	for (s = bytes, d = bytes; d < bytes + chipByteSize (); s+=2, d++)
		*d = *s;
}

int main (int argc, char **argv)
{
	V4L device (argc, argv);
	return device.run ();
}
