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

    int Init();
    int Inference(uint8_t *inbuf, uint8_t *outbuf, int width, int height);

private:
    rknn_context m_rknn_ctx;
    rknn_input_output_num m_io_num;
    rknn_tensor_attr *m_input_attrs;
    rknn_tensor_attr *m_output_attrs;
    rknn_sdk_version m_version;
    rknn_input m_input;
    uint8_t *m_model_data;

    Config_t m_config;
    
    void Destroy();
};

#endif