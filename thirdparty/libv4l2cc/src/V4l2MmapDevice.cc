#include <linux/videodev2.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "V4l2Device.h"
#include "log.h"
#include "V4l2MmapDevice.h"


V4l2MmapDevice::V4l2MmapDevice(const V4L2DeviceParameters &params, v4l2_buf_type deviceType) : 
	V4l2Device(params, deviceType),
	n_buffers(0)
{
	memset(&m_buffer, 0, sizeof(m_buffer));
}

bool V4l2MmapDevice::init(uint32_t mandatoryCapabilities)
{
	bool ret = V4l2Device::init(mandatoryCapabilities);
	if (ret) {
		ret = this->start();
	}
	return ret;
}

V4l2MmapDevice::~V4l2MmapDevice()
{
	this->stop();
}

bool V4l2MmapDevice::start()
{
	LOG(NOTICE, "Device %s Mmap Start", m_params.m_devName.c_str());

	bool success = true;
	
	/* applay for kernel space */
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(req));
	req.count	= V4L2MMAP_NBBUFFER;
	req.type	= m_deviceType;
	req.memory	= V4L2_MEMORY_MMAP;
	
	/* get 10 QBUFS */
	if (ioctl(m_fd, VIDIOC_REQBUFS, &req) < 0) {
		if (EINVAL == errno) {
			LOG(ERROR, "Device %s does not support memory mapping", 
					m_params.m_devName.c_str());
			success = false;
		} else {
			perror("VIDIOC_REQBUFS");
			success = false;
		}
	} else {
		/* mmap get the qbuf address */
		LOG(NOTICE, "Device %s nb buffer:%d", m_params.m_devName.c_str(), req.count);
		
		memset(&m_buffer, 0, sizeof(m_buffer));
		for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
			struct v4l2_buffer buf;
			memset(&buf, 0, sizeof(buf));
			buf.type	= m_deviceType;
			buf.memory	= V4L2_MEMORY_MMAP;
			buf.index	= n_buffers;

			if ( ioctl(m_fd, VIDIOC_QUERYBUF, &buf) < 0 ) {
				perror("VIDIOC_QUERYBUF");
				success = false;
			} else {
				LOG(INFO, "Device %s buffer idx:%d, size:%d offset:%d", 
						m_params.m_devName.c_str(), n_buffers, buf.length, buf.m.offset);
				m_buffer[n_buffers].length = buf.length;
				if (!m_buffer[n_buffers].length) {
					m_buffer[n_buffers].length = buf.bytesused;
				}
				//Save the first address of user space after mapping
				m_buffer[n_buffers].start = mmap(NULL,
						m_buffer[n_buffers].length, 
						PROT_READ | PROT_WRITE,
						MAP_SHARED,
						m_fd,
						buf.m.offset);
				
				if (MAP_FAILED == m_buffer[n_buffers].start) {
					perror("mmap");
					success =false;
				}
			}
		}
		/* queue buffers */
		for(uint32_t i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;
			memset(&buf, 0, sizeof(buf));
			buf.type	= m_deviceType;
			buf.memory	= V4L2_MEMORY_MMAP;
			buf.index	= i;
			if (ioctl(m_fd, VIDIOC_QBUF, &buf) < 0) {
				perror("VIDIOC_QBUF");
				success = false;
			}
		}
		
		/* start collect stream */
		int type = m_deviceType;
		if (ioctl(m_fd, VIDIOC_STREAMON, &type) < 0) {
			perror("VIDIOC_STREAMON");
			success = false;
		}
	}

	return success;
}

bool V4l2MmapDevice::stop()
{
	LOG(NOTICE, "Device %s Mmap Stop", m_params.m_devName.c_str());
	
	bool success = true;

	int type = m_deviceType;
	if (ioctl(m_fd, VIDIOC_STREAMOFF, &type) < 0) {
		perror("VIDIOC_STREAMOFF");
		success = false;
	}

	for (uint32_t i = 0; i < n_buffers; ++i) {
		if (munmap(m_buffer[i].start, m_buffer[i].length) < 0) {
			perror("munmap");
			success = false;
		}
	}

	/* free buffers */
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(req));
	req.count		= 0;
	req.type		= m_deviceType;
	req.memory		= V4L2_MEMORY_MMAP;
	if (ioctl(m_fd, VIDIOC_REQBUFS, &req) < 0) {
		perror("VIDIOC_REQBUFS");
		success = false;
	}
	n_buffers = 0;
	return success;
}

size_t V4l2MmapDevice::readInternal(char *buffer, size_t bufferSize)
{
	size_t size = 0;
	if (n_buffers > 0) {
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = m_deviceType;
		buf.memory = V4L2_MEMORY_MMAP;
		/* 将已经捕获好视频的内存拉出队列 */
		if (ioctl(m_fd, VIDIOC_DQBUF, &buf) == -1){
			perror("VIDIOC_DQBUF");
			size = -1;
		} else if (buf.index < n_buffers){
			size = buf.bytesused;
			if (size > bufferSize) {
				size = bufferSize;
				LOG(WARN, "Device %s buffer truncated available:%d needed:%d", m_params.m_devName.c_str(),
					bufferSize, buf.bytesused);
			}
			/*get video stream*/
			memcpy(buffer, m_buffer[buf.index].start, size);

			/* 将空闲的内存加入可捕获视频的队列 */
			if (ioctl(m_fd, VIDIOC_QBUF, &buf)) {
				perror("VIDIOC_QBUF");
				size = -1;
			}
		}
	}
	return size;
}

size_t V4l2MmapDevice::writeInternal(char *buffer, size_t bufferSize)
{
	size_t size = 0;
	if (n_buffers > 0) {
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = m_deviceType;
		buf.memory = V4L2_MEMORY_MMAP;
		
		if (ioctl(m_fd, VIDIOC_DQBUF, &buf) < 0){
			perror("VIDIOC_DQBUF");
			size = -1;
		} else if (buf.index < n_buffers){
			size = bufferSize;
			if (size > buf.length) {
				size = buf.length;
				LOG(WARN, "Device %s buffer truncated available:%d needed:%d", m_params.m_devName.c_str(),
					buf.length, size);
			}
			
			memcpy(m_buffer[buf.index].start, buffer, size);
			buf.bytesused = size;

			if (ioctl(m_fd, VIDIOC_QBUF, &buf)) {
				perror("VIDIOC_QBUF");
				size = -1;
			}
		}
	}
	return size;
}

bool V4l2MmapDevice::startPartialWrite()
{
	if (n_buffers <= 0) {
		return false;
	}

	if (m_partialWriteInProgress)
		return false;
	memset(&m_partialWriteBuf, 0, sizeof(m_partialWriteBuf));
	m_partialWriteBuf.type = m_deviceType;
	m_partialWriteBuf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(m_fd, VIDIOC_DQBUF, &m_partialWriteBuf)) {
		perror("VIDIOC_QBUF");
		return false;
	}

	m_partialWriteBuf.bytesused = 0;
	m_partialWriteInProgress = true;
	return true;
}

size_t V4l2MmapDevice::writePartialInternal(char *buffer, size_t bufferSize)
{
	size_t new_size = 0;
	size_t size = 0;
	if ((n_buffers > 0) && m_partialWriteInProgress) {
		if (m_partialWriteBuf.index < n_buffers) {
			new_size = m_partialWriteBuf.bytesused + bufferSize;
			if (new_size > m_partialWriteBuf.length) {
				LOG(WARN, "Device %s buffer truncated available %d needed: %d", m_params.m_devName.c_str(),
						m_partialWriteBuf.length, new_size);
				new_size = m_partialWriteBuf.length;
			}
			size = new_size - m_partialWriteBuf.bytesused;
			memcpy(&((char *)m_buffer[m_partialWriteBuf.index].start)[m_partialWriteBuf.bytesused], buffer, size);

			m_partialWriteBuf.bytesused += size;
		}
	}

	return size;
}

bool V4l2MmapDevice::endPartWrite()
{
	if (!m_partialWriteInProgress)
		return false;
	if (n_buffers <= 0) {
		m_partialWriteInProgress = false;
		return true;
	}

	if(ioctl(m_fd, VIDIOC_QBUF, &m_partialWriteBuf) < 0) {
		perror("VIDIOC_QBUF");
		m_partialWriteInProgress = false;
		return true;
	}
	m_partialWriteInProgress = false;
	return true;
}


