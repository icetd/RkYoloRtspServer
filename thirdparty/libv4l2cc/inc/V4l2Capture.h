#ifndef V4L2_CAPTURE
#define V4L2_CAPTURE

#include "V4l2Access.h"
#include "V4l2Device.h"

class V4l2Capture : public V4l2Access {
protected:
	
	V4l2Capture(V4l2Device *device);

public:
	static V4l2Capture *create(const V4L2DeviceParameters &param);
	virtual ~V4l2Capture();

	size_t read(char *buffer, size_t bufferSize);
	bool isReadable(timeval *tv);
};

#endif
