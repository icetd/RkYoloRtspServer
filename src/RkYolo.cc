#include "RkYolo.h"
#include "INIReader.h"
#include "postprocess.h"
#include "log.h"
#include <rga.h>
#include <im2d.h>
#include <RgaUtils.h>
#include <string.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

RkYolo::RkYolo()
{
}

RkYolo::~RkYolo()
{
    Destroy();
}

int RkYolo::Init(rknn_core_mask mask)
{   
    int ret = -1;
    FILE *fp;

    INIReader ini("configs/config.ini");
    m_config.model_path = ini.Get("rknn", "model_path", "model/yolov5.rknn");

    fp = fopen(m_config.model_path.c_str(), "rb");
    
    fseek(fp, 0 , SEEK_END);
    int size = ftell(fp);

    m_model_data = (uint8_t *)malloc(size);
    if (m_model_data == nullptr) {
        return -1;
    }

    ret = fseek(fp, 0, SEEK_SET);
    if (ret < 0) {
        LOG(ERROR, "blob seek failure.");
        return -1;
    }

    memset(m_model_data, 0, sizeof(m_model_data));
    ret = fread(m_model_data, 1, size, fp);
    if (ret < 0) {
        LOG(ERROR, "laod model failed");
        return -1;
    }

    ret = rknn_init(&m_rknn_ctx, m_model_data, size, 0, NULL);
    if (ret < 0) {
        LOG(ERROR, "rknn_init error ret=%d", ret);
        return -1;
    }
    
    m_core_mask = mask;
    ret = rknn_set_core_mask(m_rknn_ctx, m_core_mask);
    if (ret < 0) {
        LOG(ERROR, "rknn_init core error ret=%d\n", ret);
        return -1;
    }

    // get rknn version
    ret = rknn_query(m_rknn_ctx, RKNN_QUERY_SDK_VERSION, &m_version, sizeof(rknn_sdk_version));
    if (ret < 0) {
        LOG(ERROR, "rknn query version failed.");
        return -1;
    }

    // get rknn params
    ret = rknn_query(m_rknn_ctx, RKNN_QUERY_IN_OUT_NUM, &m_io_num, sizeof(m_io_num));
    if (ret < 0) {
        LOG(ERROR, "rknn query io_num failed.");
        return -1;
    }

    // set rknn input arr
    m_input_attrs = new rknn_tensor_attr[m_io_num.n_input];
    memset(m_input_attrs, 0, sizeof(m_input_attrs));
    for (int i = 0; i < m_io_num.n_input; i++) {
        m_input_attrs[i].index = i;
        ret = rknn_query(m_rknn_ctx, RKNN_QUERY_INPUT_ATTR, &(m_input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret < 0) {
            LOG(ERROR, "rknn_init error ret=%d", ret);
            return -1;
        }
    }

    // set rknn output arr
    m_output_attrs = new rknn_tensor_attr[m_io_num.n_output];
    memset(m_output_attrs, 0, sizeof(m_output_attrs));
    for (int i = 0; i < m_io_num.n_output; i++) {
        m_output_attrs[i].index = i;
        ret = rknn_query(m_rknn_ctx, RKNN_QUERY_OUTPUT_ATTR, &(m_output_attrs[i]), sizeof(rknn_tensor_attr));
    }

    // set rknn input params
    if (m_input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        LOG(NOTICE, "model is NCHW input fmt npu_core:%d", m_core_mask >> 1);
        m_config.model_channel = m_input_attrs[0].dims[1];
        m_config.model_height = m_input_attrs[0].dims[2];
        m_config.model_width = m_input_attrs[0].dims[3];
    } else {
        LOG(NOTICE, "model is NHWC input fmt npu_core:%d", m_core_mask >> 1);
        m_config.model_height = m_input_attrs[0].dims[1];
        m_config.model_width = m_input_attrs[0].dims[2];
        m_config.model_channel = m_input_attrs[0].dims[3];
    }

    m_input.index = 0;
    m_input.type = RKNN_TENSOR_UINT8;
    m_input.size = m_config.model_width * m_config.model_height * m_config.model_channel;
    m_input.fmt = RKNN_TENSOR_NHWC;
    m_input.pass_through = 0;

    return ret;
}


void drawTextWithBackground(cv::Mat &image, const std::string &text, cv::Point org, int fontFace, double fontScale, cv::Scalar textColor, cv::Scalar bgColor, int thickness) 
{
	int baseline = 0;
	cv::Size textSize = cv::getTextSize(text, fontFace, fontScale, thickness, &baseline);
	baseline += thickness;
	cv::Rect textBgRect(org.x, org.y - textSize.height, textSize.width, textSize.height + baseline);
	cv::rectangle(image, textBgRect, bgColor, cv::FILLED);
	cv::putText(image, text, org, fontFace, fontScale, textColor, thickness);
}

int RkYolo::Inference(int in_width, int in_height)
{
    int ret = -1;
    cv::Mat yuvImage(in_height, in_width, CV_8UC2, (void*)m_inbuf);
    cv::Mat ori_img;
    cvtColor(yuvImage, ori_img, cv::COLOR_YUV2RGB_YUY2);  // Use COLOR_YUV2BGR_YUY2 for YUYV format
    
    // init rga context
    rga_buffer_t src;
    rga_buffer_t dst;
    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));
    im_rect src_rect;
    im_rect dst_rect;
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    // You may not need resize when src resulotion equals to dst resulotion
    void *resize_buf = nullptr;
    if (in_width != m_config.model_width || in_height != m_config.model_height) {
        resize_buf = malloc(m_config.model_height * m_config.model_width * m_config.model_channel);
        memset(resize_buf, 0x00, m_config.model_width * m_config.model_height * m_config.model_channel);

        src = wrapbuffer_virtualaddr((void *)ori_img.data, in_width, in_height, RK_FORMAT_RGB_888);
        dst = wrapbuffer_virtualaddr((void *)resize_buf, m_config.model_width, m_config.model_height, RK_FORMAT_RGB_888);
        ret = imcheck(src, dst, src_rect, dst_rect);
        if (IM_STATUS_NOERROR != ret) {
            LOG(ERROR, "%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
            exit(-1);
        }
        IM_STATUS STATUS = imresize(src, dst);

        cv::Mat resize_img(cv::Size(m_config.model_width, m_config.model_height), CV_8UC3, resize_buf);
        m_input.buf = resize_buf;
    } else {
        m_input.buf = (void *)ori_img.data;
    }

    // set rknn input
    rknn_inputs_set(m_rknn_ctx, m_io_num.n_input, &m_input);

    rknn_output outputs[m_io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < m_io_num.n_output; i++)
        outputs[i].want_float = 0;
    ret = rknn_run(m_rknn_ctx, NULL);
    ret = rknn_outputs_get(m_rknn_ctx, m_io_num.n_output, outputs, NULL);

    // post process
    const float nms_threshold = NMS_THRESH;
    const float box_conf_threshold = BOX_THRESH;
    BOX_RECT pads;
    memset(&pads, 0, sizeof(BOX_RECT));
    float scale_w = (float)m_config.model_width / in_width;
    float scale_h = (float)m_config.model_height / in_height;

    detect_result_group_t detect_result_group;
    std::vector<float> out_scales;
    std::vector<int32_t> out_zps;
    for (int i = 0; i < m_io_num.n_output; ++i) {
        out_scales.push_back(m_output_attrs[i].scale);
        out_zps.push_back(m_output_attrs[i].zp);
    }
    post_process((int8_t *)outputs[0].buf, (int8_t *)outputs[1].buf, (int8_t *)outputs[2].buf, m_config.model_height, m_config.model_width,
                 box_conf_threshold, nms_threshold, pads, scale_w, scale_h, out_zps, out_scales, &detect_result_group);

    // Draw Objects
	char text[256];
	for (int i = 0; i < detect_result_group.count; i++) {
		detect_result_t *det_result = &(detect_result_group.results[i]);
		sprintf(text, "%s %.1f%%", det_result->name, det_result->prop * 100);
		int x1 = det_result->box.left;
		int y1 = det_result->box.top;
		int x2 = det_result->box.right;
		int y2 = det_result->box.bottom;
		cv::Rect rect(x1, y1, x2 - x1, y2 - y1);
		cv::Scalar color = cv::Scalar(0, 55, 218);
		cv::rectangle(ori_img, rect, color, 2);
		drawTextWithBackground(ori_img, text, cv::Point(x1 - 1, y1 - 6), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), cv::Scalar(0, 55, 218, 0.5), 2);
	}

    cv::Mat resized_img;
    resize(ori_img, resized_img, cv::Size(in_width, in_height));

    cv::Mat yuv_img;
    cv::cvtColor(resized_img, yuv_img, cv::COLOR_RGB2YUV_I420);

    memcpy(m_outbuf, yuv_img.data, in_width * in_height * 3 / 2);

    ret = rknn_outputs_release(m_rknn_ctx, m_io_num.n_output, outputs);

    if (resize_buf) {
        free(resize_buf);
    }
    return ret;
}

void RkYolo::Destroy()
{
    rknn_destroy(m_rknn_ctx);
    delete m_input_attrs;
    delete m_output_attrs;
    if (m_model_data)
        free(m_model_data);
}
