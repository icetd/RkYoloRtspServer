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
