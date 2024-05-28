#include "CamRTSPServer.h"
#include "INIReader.h"
#include "log.h"
CamRTSPServer::CamRTSPServer() :
    watcher_(0),
    scheduler_(nullptr),
    env_(nullptr)
{
    INIReader configs("./configs/config.ini");
    if (configs.ParseError() < 0) {
        LOG(ERROR, "read server config failed.");
        return;
    } else {
        config.rtspPort = configs.GetInteger("server", "rtsp_port", 8554);
        config.streamName = configs.Get("server", "stream_name", "unicast");
		config.maxBufSize = configs.GetInteger("server", "max_buf_size", 2000000);
		config.maxPacketSize = configs.GetInteger("server", "max_packet_size", 1500);
		config.isHttpEnable = configs.GetBoolean("server", "http_ebable", false);
        config.httpPort = configs.GetInteger("server", "http_port", 8000);
        config.bitrate = configs.GetInteger("server", "bitrate", 2000);
    }

    OutPacketBuffer::maxSize = config.maxBufSize;
    LOG(NOTICE, "setting OutPacketbuffer max size to %d (bytes)", OutPacketBuffer::maxSize);

    scheduler_ = BasicTaskScheduler::createNew();
    env_ = BasicUsageEnvironment::createNew(*scheduler_);
}

CamRTSPServer::~CamRTSPServer()
{
    Medium::close(server_);

    for (auto &src : allocatedVideoSources) {
        if(src) 
            Medium::close(src);
    }

    env_->reclaim();
    if (scheduler_)
        delete scheduler_;
    
    transcoders_.clear();
    allocatedVideoSources.clear();
    watcher_ = 0;

    LOG(NOTICE, "RTSP server has been destructed!");
}

void CamRTSPServer::stopServer()
{
    LOG(NOTICE, "Stop server is involed!");
    watcher_ = 's';
}

void CamRTSPServer::run()
{
    // create server listening on the specified RTSP port
    server_ = RTSPServer::createNew(*env_, config.rtspPort);
    if (!server_) {
        LOG(ERROR, "Failed to create RTSP server: %s", env_->getResultMsg());
        exit(1);
    }

    LOG(NOTICE, "Server has been created on port %d", config.rtspPort);

    if (config.isHttpEnable) {
        auto ret = server_->setUpTunnelingOverHTTP(config.httpPort);
        if (ret) {
            LOG(NOTICE, "Enable HTTP tunneling over:%d", config.httpPort);
        }
    }

    LOG(INFO, "Creating media session for each transcoder");
    for (auto &transcoder : transcoders_) {
        addMediaSession(transcoder, config.streamName.c_str(), "stream description");
    }

    env_->taskScheduler().doEventLoop(&watcher_); // do not return;
}

void CamRTSPServer::announceStream(ServerMediaSession *sms, const std::string &deviceName) 
{
    auto url = server_->rtspURL(sms);
    LOG(NOTICE, "Play the stream of the %s, url: %s", deviceName.c_str(), url);   
    delete[] url;
}

void CamRTSPServer::addMediaSession(std::shared_ptr<TransCoder> transoder, const std::string &streamName, 
                                    const std::string &streamDesc)
{
    LOG(INFO, "Adding media session for camera: %s", transoder->getConfig().device_name.c_str());

    // create framed source for camera
    auto framedSource = CamFramedSource::createNew(*env_, *transoder);

    // store it in order to cleanup after
    allocatedVideoSources.push_back(framedSource);

    // store it in order to replicator for the framed source **
    auto replicator = StreamReplicator::createNew(*env_, framedSource, False);

    // create media session with the specified topic and descripton
    auto sms = ServerMediaSession::createNew(*env_, streamName.c_str(), "stream information",
                                            streamDesc.c_str(), False, "a=fmtp:96\n");

    // add unicast subsession 
    sms->addSubsession(CamUbicastServerMediaSubsession::createNew(*env_, replicator, config.bitrate, config.maxPacketSize));

    server_->addServerMediaSession(sms);

    announceStream(sms, transoder->getConfig().device_name);
}

void CamRTSPServer::addTranscoder(std::shared_ptr<TransCoder> transcoder)
{
    transcoders_.emplace_back(transcoder);
}