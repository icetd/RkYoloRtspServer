#include "CamUnicastServerMediaSubsession.h"
#include "log.h"

CamUbicastServerMediaSubsession *
CamUbicastServerMediaSubsession::createNew(UsageEnvironment &env,
                                           StreamReplicator *replicator,
                                           size_t estBitrate, size_t udpDatagramSize)
{
    return new CamUbicastServerMediaSubsession(env, replicator, estBitrate, udpDatagramSize);
}

CamUbicastServerMediaSubsession::CamUbicastServerMediaSubsession(UsageEnvironment &env,
                                                                 StreamReplicator *replicator,
                                                                 size_t estBitrate,
                                                                 size_t udpDatagramSize) :
    OnDemandServerMediaSubsession(env, False),
    replicator_(replicator),
    estBitrate_(estBitrate),
    udpDatagramSize_(udpDatagramSize)
{
    LOG(INFO, "Unicast media subsession with UDP datagram size of %d\n"
              "\tand estimated bitrate of %d (kbps) is created",
        udpDatagramSize, estBitrate);
}

FramedSource *
CamUbicastServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate)
{
    estBitrate = static_cast<unsigned int>(this->estBitrate_);

    auto source = replicator_->createStreamReplica();

    // only discrete frames are being sent (w/o start code bytes)
    return H264VideoStreamDiscreteFramer::createNew(envir(), source);
}

RTPSink *
CamUbicastServerMediaSubsession::createNewRTPSink(Groupsock *rtpGroupsock,
                                                  unsigned char rtpPayloadTypeIfDynamic,
                                                  FramedSource *inputSource)
{
    auto sink = H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
    sink->setPacketSizes(static_cast<unsigned int>(udpDatagramSize_), static_cast<unsigned int>(udpDatagramSize_));
    return sink;
}

void CamUbicastServerMediaSubsession::getStreamParameters(unsigned clientSessionId,
                                                          struct sockaddr_storage const &clientAddress,
                                                          Port const &clientRTPPort,
                                                          Port const &clientRTCPPort,
                                                          int tcpSocketNum,
                                                          unsigned char rtpChannelId,
                                                          unsigned char rtcpChannelId,
                                                          TLSState *tlsState,
                                                          struct sockaddr_storage &destinationAddress,
                                                          u_int8_t &destinationTTL,
                                                          Boolean &isMulticast,
                                                          Port &serverRTPPort,
                                                          Port &serverRTCPPort,
                                                          void *&streamToken)
{
    char ipStr[INET6_ADDRSTRLEN] = {0};

    if (clientAddress.ss_family == AF_INET) {
        // IPv4
        struct sockaddr_in *addr_in = (struct sockaddr_in *)&clientAddress;
        inet_ntop(AF_INET, &(addr_in->sin_addr), ipStr, sizeof(ipStr));
    } else if (clientAddress.ss_family == AF_INET6) {
        // IPv6
        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&clientAddress;
        inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ipStr, sizeof(ipStr));
    } else {
        strcpy(ipStr, "Unknown AF");
    }

    LOG(NOTICE, "Client connected, sessionId=%u, IP=%s", clientSessionId, ipStr);

    {
        std::lock_guard<std::mutex> lock(clientIpMutex_);
        clientIpMap_[clientSessionId] = ipStr;
    }

    OnDemandServerMediaSubsession::getStreamParameters(clientSessionId,
                                                       clientAddress,
                                                       clientRTPPort,
                                                       clientRTCPPort,
                                                       tcpSocketNum,
                                                       rtpChannelId,
                                                       rtcpChannelId,
                                                       tlsState,
                                                       destinationAddress,
                                                       destinationTTL,
                                                       isMulticast,
                                                       serverRTPPort,
                                                       serverRTCPPort,
                                                       streamToken);
}

void CamUbicastServerMediaSubsession::deleteStream(unsigned clientSessionId, void *&streamToken)
{
    std::string ip;
    {
        std::lock_guard<std::mutex> lock(clientIpMutex_);
        auto it = clientIpMap_.find(clientSessionId);
        if (it != clientIpMap_.end()) {
            ip = it->second;
            clientIpMap_.erase(it);
        }
    }

    if (!ip.empty()) {
        LOG(NOTICE, "Client disconnected, sessionId=%u, IP=%s", clientSessionId, ip.c_str());
    } else {
        LOG(NOTICE, "Client disconnected, sessionId=%u, IP unknown", clientSessionId);
    }

    OnDemandServerMediaSubsession::deleteStream(clientSessionId, streamToken);
}