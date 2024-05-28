#include <linux/videodev2.h>
#include <sys/select.h>
#include <sys/types.h>
#include "V4l2Device.h"
#include "log.h"
#include "V4l2Capture.h"
#include "V4l2MmapDevice.h"
#include "V4l2ReadWriteDevice.h"

V4l2Capture::V4l2Capture(V4l2Device *device) :
	V4l2Access(device) 
{
}

V4l2Capture::~V4l2Capture()
{
}

V4l2Capture *V4l2Capture::create(const V4L2DeviceParameters &param)
{
	V4l2Capture *videoCapture = nullptr;
	V4l2Device *videoDevice = nullptr;
	int caps = V4L2_CAP_VIDEO_CAPTURE;

	switch(param.m_IoType) {
	case IOTYPE_MMAP:
		videoDevice = new V4l2MmapDevice(param, V4L2_BUF_TYPE_VIDEO_CAPTURE);
		caps |= V4L2_CAP_STREAMING;
		break;
	case IOTYPE_READWRITE:
		videoDevice = new V4l2ReadWriteDevice(param, V4L2_BUF_TYPE_VIDEO_CAPTURE);
		caps |= V4L2_CAP_READWRITE;
		break;
	}

	if (videoDevice && !videoDevice->init(caps)) {
		delete videoDevice;
		videoDevice = nullptr;
	}

	if (videoDevice) {
		videoCapture = new V4l2Capture(videoDevice);
	}
	return videoCapture;
}

bool V4l2Capture::isReadable(timeval *tv)
{
	int fd = m_device->getFd();
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	return (select(fd+1, &fdset, NULL, NULL, tv) == 1);
}

size_t V4l2Capture::read(char *buffer, size_t bufferSize)
{
	return m_device->readInternal(buffer, bufferSize);
}
