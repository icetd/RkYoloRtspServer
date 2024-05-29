#include <list>
#include <string.h>
#include <vector>
#include <functional>
#include "transcoder.h"
#include "INIReader.h"
#include "log.h"

TransCoder::TransCoder()
{
    init();
}

TransCoder::~TransCoder()
{
}

void TransCoder::init()
{
    std::list<uint32_t> formatList;
    v4l2IoType ioTypeIn = IOTYPE_MMAP;
    MppFrameFormat mpp_fmt;

    INIReader configs("./configs/config.ini");
    if (configs.ParseError() < 0) {
        LOG(ERROR, "read video config failed.");
        return;
    } else {
        config.width = configs.GetInteger("video", "width", 640);
        config.height = configs.GetInteger("video", "height", 480);
        config.fps = configs.GetInteger("video", "fps", 30);
        config.fix_qp = configs.GetInteger("video", "fix_qp", 23);
        config.device_name = configs.Get("video", "device", "/dev/video0");
        config.rknn_thread = configs.GetInteger("rknn", "rknn_thread", 3);

        config.rknn_thread = (config.rknn_thread > 6) ? 6 : config.rknn_thread;

        formatList.push_back(V4L2_PIX_FMT_YUYV);
        mpp_fmt = MPP_FMT_YUV420P;
    }

    V4L2DeviceParameters param(config.device_name.c_str(),
                               formatList,
                               config.width,
                               config.height,
                               config.fps,
                               ioTypeIn,
                               DEBUG);

    capture = V4l2Capture::create(param);
    encodeData = (uint8_t *)malloc(capture->getBufferSize());

    Encoder_Param_t encoder_param{
        mpp_fmt,
        config.width,
        config.height,
        0,
        0,
        0,
        config.fps,
        config.fix_qp};

    rk_encoder = new RkEncoder(encoder_param);
    rk_encoder->init();
 
    m_pool = new ThreadPool(config.rknn_thread);


    for (int i = 0; i < config.rknn_thread; i++) {
    RkYolo *rkyolo = new RkYolo();
        rkyolo->Init((rknn_core_mask)(1 << (i % 3)));
        m_rkyolo_list.push_back(rkyolo);
    }
    m_cur_yolo = 0;
}

void TransCoder::run()
{
    timeval tv;
    m_out_buffer_list.resize(config.rknn_thread, std::vector<uint8_t>(capture->getBufferSize()));
    for (;;) {
        if (m_cur_yolo >= config.rknn_thread) 
            m_cur_yolo = 0;
        
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int startCode = 0;
        int ret = capture->isReadable(&tv);
        if (ret == 1) {
            std::vector<uint8_t> data(capture->getBufferSize());
            m_in_buffer_list.push_back(data);
            LOG(INFO, "%ld \n", m_in_buffer_list.size());

            int resize = capture->read((char *)m_in_buffer_list.at(m_cur_yolo).data(), data.size());

            RkYolo *cur_yolo = m_rkyolo_list.at(m_cur_yolo);
            cur_yolo->SetBuffers(m_in_buffer_list.at(m_cur_yolo).data(), m_out_buffer_list.at(m_cur_yolo).data());
            
            m_cur_yolo++;
            frameSize = 0;
            
            if (m_in_buffer_list.size() >= config.rknn_thread) {
                m_pool->enqueue(&RkYolo::Inference, cur_yolo, config.width, config.height);
                /*Use the first output data to ensure that yolo is executed*/
                frameSize = rk_encoder->encode(m_out_buffer_list.at(m_cur_yolo % config.rknn_thread).data(), resize, encodeData); 
                LOG(INFO, "encodeData size %d", frameSize);
                if (rk_encoder->startCode3(encodeData))
                    startCode = 3;
                else
                    startCode = 4;

                if (onEncodedDataCallback && frameSize) {
                    onEncodedDataCallback(std::vector<uint8_t>(encodeData + startCode, encodeData + frameSize));
                }
                m_in_buffer_list.erase(m_in_buffer_list.begin());
            }
        } else if (ret == -1) {
            LOG(ERROR, "stop %s", strerror(errno));
        }
    }
}

TransCoder::Config_t const &TransCoder::getConfig() const
{
    return config;
}

void TransCoder::setOnEncoderDataCallback(std::function<void(std::vector<uint8_t> &&)> callback)
{
    onEncodedDataCallback = std::move(callback);
}
