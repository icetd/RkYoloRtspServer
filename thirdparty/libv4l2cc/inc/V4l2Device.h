#ifndef V4L2_DEVICE
#define V4L2_DEVICE

#include <bits/stdint-uintn.h>
#include <cstddef>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <string>
#include <list>

#ifndef V4L2_PIX_FMT_VP8
#define V4L2_PIX_FMT_VP8 v4l2_fourcc('V', 'P', '8', '0')
#endif
#ifndef V4L2_PIX_FMT_VP9
#define V4L2_PIX_FMT_VP9 v4l2_fourcc('V', 'P','9', '0')
#endif
#ifndef V4L2_PIX_FMT_HEVC
#define V4L2_PIX_FMT_HEVC v4l2_fourcc('H', 'E', 'V', 'C')
#endif

enum v4l2IoType {
	IOTYPE_READWRITE,
	IOTYPE_MMAP
};

struct V4L2DeviceParameters {
	/** @brief device paramters */
	V4L2DeviceParameters(const char *devname, const std::list<uint32_t> &formatList, 
			uint32_t width, uint32_t height,int fps, v4l2IoType ioType, 
			int level = 0, int openFlags = O_RDWR | O_NONBLOCK) : 
		m_devName (devname),
		m_formatList(formatList),
		m_width(width),
		m_height(height),
		m_IoType(ioType),
		m_fps(fps),
		m_level(level),
		m_openFlags(openFlags) {}
	
	V4L2DeviceParameters(const char *devname, uint32_t format, 
			uint32_t width, uint32_t height, int fps, v4l2IoType ioType = IOTYPE_MMAP, 
			int level = 0, int openFlags = O_RDWR | O_NONBLOCK) : 
		m_devName (devname),
		m_width(width),
		m_height(height),
		m_IoType(ioType),
		m_fps(fps),
		m_level(level),
		m_openFlags(openFlags) {
		if (format)
			m_formatList.push_back(format);
	}

	std::string m_devName;
	std::list<uint32_t> m_formatList;
	uint16_t m_width;
	uint16_t m_height;
	int m_fps;
	v4l2IoType m_IoType;
	int m_level;
	int m_openFlags;
};

class V4l2Device {
protected:
	void destroy();
	int initdevice(const char *dev_name, uint32_t mandatoryCapabilities);
	int checkCapAbilities(int fd, uint32_t mandatoryCapabilities);
	int configureFormat(int fd);
	int configureFormat(int fd, uint32_t format, uint32_t width, uint32_t height);
	int configureParam(int fd, int fps);

	virtual bool	init(uint32_t mandatoryCapabilities);
	virtual size_t	writeInternal(char *, size_t)		{ return -1;	 }
	virtual bool	startPartialWrite()					{ return -false; }
	virtual size_t	writePartialInternal(char*, size_t)	{ return -1;	 }
	virtual bool	endPartialWrite()					{ return -1;	 }
	virtual size_t	readInternal(char*, size_t)			{ return -1;	 }

public:
	friend class V4l2Capture;
	friend class V4l2Output;

public:
	V4l2Device(const V4L2DeviceParameters &params, v4l2_buf_type deviceType);
	virtual ~V4l2Device();
	
	virtual bool isReady()	{ return (m_fd != -1); }
	virtual bool start()	{ return true; }
	virtual bool stop()		{ return true; }

	uint32_t getBufferSize()	{ return m_bufferSize;	}
	uint32_t getFormat()		{ return m_format;		}
	uint32_t getWidth()			{ return m_width;		}
	uint32_t getHeight()		{ return m_height;		}
	uint32_t getFd()			{ return m_fd;			}
	void queryFormat();	
	
	int setFormat(uint32_t format, uint32_t width, uint32_t height) {
		return this->configureFormat(m_fd, format, width, height);
	}
	int setFps(int fps) {
		return this->configureParam(m_fd, fps);
	}

	static std::string fourcc(uint32_t format);
	static uint32_t fourcc(const char *format);

protected:
	V4L2DeviceParameters m_params;
	int m_fd;
	v4l2_buf_type m_deviceType;	

	uint32_t m_bufferSize;
	uint32_t m_format;
	uint32_t m_width;
	uint32_t m_height;	

	struct v4l2_buffer m_partialWriteBuf;
	bool m_partialWriteInProgress;
};

#endif
