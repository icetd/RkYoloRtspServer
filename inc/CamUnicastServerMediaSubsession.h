#ifndef CAM_UNICAST_SERVER_MEDIA_SUBSESSION_H
#define CAM_UNICAST_SERVER_MEDIA_SUBSESSION_H

#include <OnDemandServerMediaSubsession.hh>
#include <StreamReplicator.hh>
#include <H264VideoRTPSink.hh>
#include <H264VideoStreamDiscreteFramer.hh>
#include <mutex>
#include <string>
#include <map>

class CamUbicastServerMediaSubsession : public OnDemandServerMediaSubsession
{
public:
    static CamUbicastServerMediaSubsession *
    createNew(UsageEnvironment &env, StreamReplicator *replicator, size_t estBitrate, size_t udpDatagramSize);

protected:
    StreamReplicator *replicator_;

    size_t estBitrate_;

    size_t udpDatagramSize_;

    CamUbicastServerMediaSubsession(UsageEnvironment &env,
                                    StreamReplicator *replicator,
                                    size_t estBitrate,
                                    size_t udpDatagramSize);

    FramedSource *createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate) override;

    RTPSink *createNewRTPSink(Groupsock *rtpGroupsock,
                              unsigned char rtpPayloadTypeIfDynamic,
                              FramedSource *inputSource) override;

    void getStreamParameters(unsigned clientSessionId,
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
                             void *&streamToken) override;

    void deleteStream(unsigned clientSessionId, void *&streamToken) override;

private:
    std::mutex clientIpMutex_;
    std::map<unsigned, std::string> clientIpMap_;
};

#endif