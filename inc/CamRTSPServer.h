#ifndef CAM_RTSP_SERVER_H
#define CAM_RTSP_SERVER_H

#include <UsageEnvironment.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <liveMedia.hh>
#include <string>

#include "CamFramedSource.h"
#include "CamUnicastServerMediaSubsession.h"

class CamRTSPServer 
{
public:
    typedef struct {
        int rtspPort;
        std::string streamName;
        int maxPacketSize;
        int maxBufSize;
        bool isHttpEnable;
        int httpPort;
        int bitrate;
    } Config_t;

    CamRTSPServer();
    ~CamRTSPServer();

    void stopServer();

    void addTranscoder(std::shared_ptr<TransCoder> transcoder);

    void run();

private:
    char watcher_;
    TaskScheduler *scheduler_;
    UsageEnvironment *env_;
    RTSPServer *server_;
    Config_t config;
    /**
     * @brief Video sources.
     */
    std::vector<std::shared_ptr<TransCoder>> transcoders_;

    /**
     * @brief Framed sources.
     */
    std::vector<FramedSource *> allocatedVideoSources;

    /**
     * @brief Announce new create media session.
     * 
     * @param sms - create server media session.
     * @param deviceName - the name of video source device.
     */
    void announceStream(ServerMediaSession *sms, const std::string &deviceName);

    /**
     * @brief Add new server media session using transcoder as a source to the server
     * 
     * @param transoder - video source.
     * @param streamName - the name of stream (part of the URL), e.g. rtsp://<ip>/camera/1.
     * @param streamDesc - description of stream.
     */
    void addMediaSession(std::shared_ptr<TransCoder> transoder, const std::string &streamName, const std::string &streamDesc);

};

#endif