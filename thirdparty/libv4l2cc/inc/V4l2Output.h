#ifndef V4L2_OUTPUT_H
#define V4L2_OUTPUT_H

#include "V4l2Access.h"
#include "V4l2Device.h"
#include <cstddef>

class V4l2Output :public V4l2Access {
protected:
	V4l2Output(V4l2Device *device);
public:
	static V4l2Output *create(const V4L2DeviceParameters &param);
	virtual ~V4l2Output();

	size_t write(char *buffer, size_t bufferSize);
	bool isWriteable(timeval *tv);
	bool startPartialWrite();
	size_t writePartial(char *buffer, size_t bufferSize);
	bool endPartialWrite();
};

#endif
