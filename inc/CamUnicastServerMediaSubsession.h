#ifndef CAM_UNICAST_SERVER_MEDIA_SUBSESSION_H
#define CAM_UNICAST_SERVER_MEDIA_SUBSESSION_H

#include <OnDemandServerMediaSubsession.hh>
#include <StreamReplicator.hh>
#include <H264VideoRTPSink.hh>
#include <H264VideoStreamDiscreteFramer.hh>

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
};

#endif