#ifndef RK_YOLO_H
#define TK_YOLO_H

#include "rknn_api.h"
#include <string>

class RkYolo {

typedef struct {
    int model_channel;
    int model_width;
    int model_height;
    std::string model_path;
} Config_t;

public:
    RkYolo();
    ~RkYolo();

    int Init(rknn_core_mask mask);
    int Inference(int width, int height);
    void SetBuffers(uint8_t* inbuf, uint8_t* out_buf) {
        m_inbuf = inbuf;
        m_outbuf = out_buf;
    }

private:
    rknn_context m_rknn_ctx;
    rknn_input_output_num m_io_num;
    rknn_tensor_attr *m_input_attrs;
    rknn_tensor_attr *m_output_attrs;
    rknn_sdk_version m_version;
    rknn_input m_input;
    uint8_t *m_model_data;
 
    uint8_t* m_inbuf;
    uint8_t* m_outbuf;
 
    Config_t m_config;
    rknn_core_mask m_core_mask;

    void Destroy();
};

#endif