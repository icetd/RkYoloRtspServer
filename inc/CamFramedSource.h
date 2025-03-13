#ifndef CAM_FRAME_SOURCE_H
#define CAM_FRAME_SOURCE_H

#include <FramedSource.hh>
#include <UsageEnvironment.hh>
#include <mutex>
#include "transcoder.h"

class CamFramedSource : public FramedSource
{
public:
    static CamFramedSource *createNew(UsageEnvironment &env, TransCoder &transcoder);

protected:
    CamFramedSource(UsageEnvironment &env, TransCoder &transcoder);
    ~CamFramedSource();

    void doGetNextFrame() override;
    void doStopGettingFrames() override;

private:
    TransCoder &transcoder_;
    EventTriggerId eventTriggerId;
    std::mutex encodedDataMutex;

    size_t max_nalu_size_bytes;
    std::vector<uint8_t> encodedData;
    std::vector<std::vector<uint8_t>> encodedDataBuffer;

    void onEncodedData(std::vector<uint8_t> &&data);

    static void deliverFrame0(void *);
    void deliverData();
};

#endif