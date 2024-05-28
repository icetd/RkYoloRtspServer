#ifndef V4L2_ACCESS_H
#define V4L2_ACCESS_H

#include "V4l2Device.h"

class V4l2Access {
public:
	V4l2Access(V4l2Device *device);
	virtual ~V4l2Access();

	int getFd()              { return m_device->getFd();         }
	uint32_t getBufferSize() { return m_device->getBufferSize(); }
	uint32_t getFormat()     { return m_device->getFormat();     }
	uint32_t getWidth()      { return m_device->getWidth();      }
	uint32_t getHeight()     { return m_device->getHeight();     }
	
	void queryForamt() { m_device->queryFormat(); }
	int setFormat(uint32_t format, uint32_t width, uint32_t height) {
		return m_device->setFormat(format, width, height);
	}
	int setFps(int fps) {
		return m_device->setFps(fps);
	}

	int isReady()       { return m_device->isReady();       }
	int start()         { return m_device->start();         }
	int stop()          { return m_device->stop();          }

private:
	V4l2Access(const V4l2Access&);
	V4l2Access &operator = (const V4l2Access);

protected:
	V4l2Device *m_device = nullptr;
};

#endif
