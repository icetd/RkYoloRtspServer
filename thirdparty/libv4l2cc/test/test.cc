#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <log.h>
#include <V4l2Device.h>
#include <V4l2Capture.h>
#include <signal.h>
#include <unistd.h>

int LogLevel;
int stop = 0;

void sighandler(int)
{
	printf("SIGINT\n");
	stop = 1;
}

int main()
{	
	const char *in_devname = "/dev/video0";	
	v4l2IoType ioTypeIn  = IOTYPE_MMAP;
	int format = 0;
	int width = 1920;
	int height = 1080;
	int fps = 30;
	
	initLogger(INFO);
	V4L2DeviceParameters param(in_devname, V4L2_PIX_FMT_MJPEG, width, height, fps, ioTypeIn, DEBUG);
	V4l2Capture *videoCapture = V4l2Capture::create(param);
	
	if (videoCapture == nullptr) {
		LOG(WARN, "Cannot reading from V4l2 capture interface for device: %s", in_devname);
	} else {
		timeval tv;

		LOG(NOTICE, "Start reading from %s", in_devname);
		signal(SIGINT, sighandler);
		while(!stop) {
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			int ret = videoCapture->isReadable(&tv);

			if(ret == 1) {
				char buffer[videoCapture->getBufferSize()];
				int resize = videoCapture->read(buffer, sizeof(buffer));
				if (resize == -1) {
					LOG(NOTICE, "stop %s", strerror(errno));
					stop = 1;
				} else {
					LOG(NOTICE, "size: %d", resize);
				}
			}else if (ret == -1) {
				LOG(NOTICE, "stop %s", strerror(errno));
				stop = 1;
			}
		}

		delete videoCapture;
	}

	LOG(INFO, "=============test=============");
	return 0;
}
