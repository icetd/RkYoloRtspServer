#include <string.h>
#include "V4l2Access.h"
#include "V4l2Device.h"
#include "log.h"
#include "V4l2Output.h"
#include "V4l2MmapDevice.h"
#include "V4l2ReadWriteDevice.h"

V4l2Output::V4l2Output(V4l2Device * device) :
	V4l2Access(device)
{
}


V4l2Output::~V4l2Output()
{
}

bool V4l2Output::isWriteable(timeval *tv)
{
	int fd = m_device->getFd();
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	return (select(fd + 1, NULL, &fdset, NULL, tv) == 1);
}

size_t V4l2Output::write(char *buffer, size_t bufferSize)
{
	return m_device->writeInternal(buffer, bufferSize);
}

bool V4l2Output::startPartialWrite()
{
	return m_device->startPartialWrite();
}

size_t V4l2Output::writePartial(char *buffer, size_t bufferSize)
{
	return m_device->writePartialInternal(buffer, bufferSize);
}

bool V4l2Output::endPartialWrite()
{
	return m_device->endPartialWrite();
}
