#include "log.h"
#include <cerrno>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <V4l2Device.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>    
#include <sys/stat.h>

std::string V4l2Device::fourcc(uint32_t format) 
{
	char formatArray[] = {	(char)(format & 0xff), 
		(char)((format >> 8 ) & 0xff), 
		(char)((format >> 16) & 0xff), 
		(char)((format >> 24) & 0xff), 
		0 };
	return std::string (formatArray, strlen(formatArray));
}

uint32_t V4l2Device::fourcc(const char *format) 
{
	char fourcc[4];
	memset(&fourcc, 0, sizeof(fourcc));
	if (format != NULL) {
		strncpy(fourcc, format, 4);
	}
	return v4l2_fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
}

V4l2Device::V4l2Device(const V4L2DeviceParameters &params, v4l2_buf_type deviceType) :
	m_params(params),
	m_fd(-1),
	m_deviceType(deviceType),
	m_bufferSize(0),
	m_format(0) 
{
}

V4l2Device::~V4l2Device()
{
	this->destroy();
}

void V4l2Device::destroy()
{
	if (m_fd != -1) {
		close(m_fd);
	}
	m_fd = -1;
}

/** @brief query camera pixelformat 
 * 
 * */
void V4l2Device::queryFormat()
{
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = m_deviceType;
	if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) == 0) {
		m_format		= fmt.fmt.pix.pixelformat;
		m_width			= fmt.fmt.pix.width;
		m_height		= fmt.fmt.pix.height;
		m_bufferSize	= fmt.fmt.pix.sizeimage;
		LOG(INFO, "%s:%s size:%dx%d bufferSize:%d", m_params.m_devName.c_str(), 
				fourcc(m_format).c_str(), m_width, m_height, m_bufferSize);
	}
}

/** @brief open device and initialzie 
 *	transfer initdevice
 *  @param mandatoryCapabilities:
 *	@return success: 1 | false: 0
 * */
bool V4l2Device::init(uint32_t mandatoryCapabilities)
{
	struct stat st;
	if ((stat(m_params.m_devName.c_str(), &st) == 0) && S_ISCHR(st.st_mode)) {
		if (initdevice(m_params.m_devName.c_str(), mandatoryCapabilities) == -1) {
			LOG(ERROR, "Cannot init device:%s", m_params.m_devName.c_str());
		}
	} else { // if dev not exist creat a file
		m_fd = open(m_params.m_devName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	}
	return (m_fd != -1);
}
/** @brief 
 *  init device: 
 *		check cap abilities
 *		config video format
 *		config fps paramters
 *	@param dev_name:		video device name
 *	@mandatoryCapabilities: 
 *	@return success: fd | false: -1
 **/
int V4l2Device::initdevice(const char *dev_name, uint32_t mandatoryCapabilities)
{
	m_fd = open(dev_name, m_params.m_openFlags);
	if (m_fd < 0) {
		LOG(ERROR, "Connot open device:%s %s", m_params.m_devName.c_str(), 
			strerror(errno));
		this->destroy();
		return -1;
	}
	if (checkCapAbilities(m_fd, mandatoryCapabilities) != 0) {
		this->destroy();
		return -1;
	}
	if (configureFormat(m_fd) != 0) {
		this->destroy();
		return -1;
	}
	if (configureParam(m_fd, m_params.m_fps) != 0) {
		this->destroy();
		return -1;
	}
	return m_fd;
}
/** @brief check Cap Abilities
 *	@param fd:	File descriptor
 *	@param mandatoryCapabilities:	
 *	@return success: 0 | false: -1
 * */
int V4l2Device::checkCapAbilities(int fd, uint32_t mandatoryCapabilities)
{	
	struct v4l2_capability capability;
	memset(&(capability), 0, sizeof(capability));
	if (ioctl(m_fd, VIDIOC_QUERYCAP, &capability) < 0) {
		LOG(ERROR, "Cannot get capabilities for device:%s %s", 
					m_params.m_devName.c_str(), strerror(errno));
		return -1;
	}
	LOG(NOTICE, "driver:%s capabilities:%x mandatory:%x" ,
					capability.driver,
					capability.capabilities,
					mandatoryCapabilities);
	
	if ((capability.capabilities & V4L2_CAP_VIDEO_OUTPUT) 
			== V4L2_CAP_VIDEO_OUTPUT) {
		LOG(NOTICE, "%s support output" , m_params.m_devName.c_str());
	}
	if ((capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) 
			== V4L2_CAP_VIDEO_CAPTURE) {
		LOG(NOTICE, "%s support capture" , m_params.m_devName.c_str());
	}
	if ((capability.capabilities & V4L2_CAP_READWRITE) 
			== V4L2_CAP_READWRITE) {
		LOG(NOTICE, "%s support read/write" , m_params.m_devName.c_str());
	}
	if ((capability.capabilities & V4L2_CAP_STREAMING) 
			== V4L2_CAP_STREAMING) {
		LOG(NOTICE, "%s support streaming" , m_params.m_devName.c_str());
	}
	if ((capability.capabilities & V4L2_CAP_TIMEPERFRAME) 
			== V4L2_CAP_TIMEPERFRAME) {
		LOG(NOTICE, "%s support timeperframe" , m_params.m_devName.c_str());
	}
	if ((capability.capabilities & mandatoryCapabilities) 
			!= mandatoryCapabilities) {
		LOG(ERROR, "Mandatory capability not avaiable for device:%s" , 
				m_params.m_devName.c_str());
		return -1;
	}
	return 0;
}

/** @brief config device capture format 
 *	@param fd: File descriptor
 *	@return success: 0 | false: -1
 * */
int V4l2Device::configureFormat(int fd)
{
	/* get current configuration */
	this->queryFormat();

	uint32_t width = m_width;
	uint32_t height = m_height;
	
	if(m_params.m_width != 0)
		width = m_params.m_width;
	if(m_params.m_height != 0)
		height = m_params.m_height;
	if( (m_params.m_formatList.size() == 0) && (m_format != 0)) {
		m_params.m_formatList.push_back(m_format);
	}

	std::list<uint32_t>::iterator it;
	for (it = m_params.m_formatList.begin(); 
			it != m_params.m_formatList.end(); ++it) {
		uint32_t format = *it;
		if (this->configureFormat(m_fd, format, width, height) == 0) {
			// format has been set
			// get the format again 
			// because calling SET-FMT return a bad buffersize 
			// using v4l2loopback
			this->queryFormat();		
			return 0;
		}
	}
	return -1;
}

/** @brief config camera video capture format 
 *	@param fd:		File descriptor
 *	@param width:	video frame width
 *	@brief height:	video frame height
 *	
 * */
int V4l2Device::configureFormat(int fd, uint32_t format, 
		uint32_t width, uint32_t height)
{
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = m_deviceType;
	if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
		LOG(ERROR, "%s: Cannot get format", m_params.m_devName.c_str());
		return -1;
	}
	if (width != 0)
		fmt.fmt.pix.width = width;
	if (height != 0)
		fmt.fmt.pix.height = height;
	if (format != 0)
		fmt.fmt.pix.pixelformat = format;

	if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
		LOG(ERROR, "%s: Connot set format:%s %s", 
				m_params.m_devName.c_str(),  strerror(errno));
		return -1;
	}
	if (fmt.fmt.pix.pixelformat != format) {
		LOG(ERROR, "%s: Cannot set pixelformat to: %s, format is: %s", 
			m_params.m_devName.c_str(), 
		fourcc(format).c_str(), fourcc(fmt.fmt.pix.pixelformat).c_str());
		return -1;
	}
	if (fmt.fmt.pix.width != width || fmt.fmt.pix.height != height) {
		LOG(ERROR, "%s: Connot set size to: %dx%d, Size is: %dx%d", 
			m_params.m_devName.c_str(), 
			fmt.fmt.pix.width, fmt.fmt.pix.height);
	}
	m_format		= fmt.fmt.pix.pixelformat;
	m_width			= fmt.fmt.pix.width;
	m_height		= fmt.fmt.pix.height;
	m_bufferSize	= fmt.fmt.pix.sizeimage;

	LOG(NOTICE, "%s: %s size:%dx%d buffersize:%d", 
			m_params.m_devName.c_str(), fourcc(m_format).c_str(), m_width, m_height, m_bufferSize);

	return 0;
}

/** @brief config video stream fps 
 *	@param fd:		File descriptor
 *	@param fps:		video fps
 * */
int V4l2Device::configureParam(int fd, int fps)
{
	if (fps != 0) {
		struct v4l2_streamparm params;
		memset(&params, 0, sizeof(params));
		params.type = m_deviceType;
		params.parm.capture.timeperframe.numerator = 1;
		params.parm.capture.timeperframe.denominator = fps;

		if (ioctl(fd, VIDIOC_S_PARM, &params) == -1) {
			LOG(ERROR, "Cannot set parm for device:%s %s", 
					m_params.m_devName.c_str(), strerror(errno));
		}
		LOG(NOTICE, "fps:%d/%d", 
			params.parm.capture.timeperframe.numerator,			
			params.parm.capture.timeperframe.denominator);
		LOG(NOTICE, "nbBuffer:%d", params.parm.capture.readbuffers);
	}
	return 0;
}
