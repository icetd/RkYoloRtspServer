#ifndef TRANS_CODE_H
#define TRANS_CODE_H

#include <functional>
#include <atomic>
#include <string>
#include <vector>

#include "MThread.h"
#include "V4l2Device.h"
#include "V4l2Capture.h"
#include "RkEncoder.h"
#include "RkYolo.h"
#include "ThreadPool.h"

class TransCoder : public MThread
{
public:
    typedef struct {
        int width;
        int height;
        int fps;
        int fix_qp;
        int rknn_thread;
        std::string device_name;
    } Config_t;

    TransCoder();
    ~TransCoder();
    void init();

    void run() override;
    void setOnEncoderDataCallback(std::function<void(std::vector<uint8_t> &&)> callback);

    TransCoder::Config_t const &getConfig() const;
private:
    Config_t config;
    
    V4l2Capture *capture;
    uint8_t *yuv_buf;
    int yuv_size;
    uint8_t *encodeData;
    int frameSize;
    std::function<void(std::vector<uint8_t> &&)> onEncodedDataCallback;

	RkEncoder *rk_encoder;
    std::vector<RkYolo*> m_rkyolo_list;
    ThreadPool *m_pool;   
    int m_cur_yolo;

    std::vector<std::vector<uint8_t>> m_in_buffer_list;
    std::vector<std::vector<uint8_t>> m_out_buffer_list;
};


#endif
