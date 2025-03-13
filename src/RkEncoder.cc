#include "RkEncoder.h"
#include "log.h"
#include "rockchip/rk_mpi.h"
#include <linux/videodev2.h>
#include <unistd.h>
#include <memory.h>

#define MPP_ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

RkEncoder::RkEncoder(Encoder_Param_t param) :
    m_param(param)
{
    m_param.hor_stride = m_param.width;
    m_param.ver_stride = m_param.height;

    if (m_param.fmt <= MPP_FMT_YUV422SP_VU) {
        m_param.hor_stride;
        m_param.frame_size = m_param.hor_stride * m_param.ver_stride * 2;
    } else if (m_param.fmt <= MPP_FMT_YUV422_VYUY) {
        m_param.hor_stride *= 2;
        m_param.frame_size = m_param.hor_stride * m_param.ver_stride;
    } else {
        m_param.frame_size = m_param.hor_stride * m_param.ver_stride * 4;
    }
    LOG(INFO, "frame_size %d", m_param.frame_size);
}

RkEncoder::~RkEncoder()
{
    if (m_contex) {
        m_mpi->reset(m_contex);
        mpp_destroy(m_contex);
    }
}

int RkEncoder::init()
{
    MppEncCfg cfg;
    MppCodingType type = MPP_VIDEO_CodingAVC;
    int ret = 0;

    ret = mpp_create(&m_contex, &m_mpi);
    if (ret) {
        LOG(ERROR, "mpp create failed. error:%d", ret);
        return -1;
    }

    ret = mpp_init(m_contex, MPP_CTX_ENC, type);
    if (ret) {
        LOG(ERROR, "mpp init failed. error:%d", ret);
        return -1;
    }

    ret = mpp_enc_cfg_init(&cfg);
    if (ret || !cfg) {
        LOG(ERROR, "mpp_enc_cfg_init failed error:%d", ret);
        return -1;
    }

    mpp_buffer_get(NULL, &m_buffer, m_param.frame_size);

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", 0);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", m_param.fps);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denom", 1);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", 0);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", m_param.fps);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denom", 1);

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", 14400);

    mpp_enc_cfg_set_s32(cfg, "prep:width", m_param.width);
    mpp_enc_cfg_set_s32(cfg, "prep:height", m_param.height);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", m_param.hor_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", m_param.ver_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:format", m_param.fmt);

    mpp_enc_cfg_set_s32(cfg, "rc:mode", MPP_ENC_RC_MODE_FIXQP);
    mpp_enc_cfg_set_s32(cfg, "rc:qp_init", m_param.fix_qp);
    mpp_enc_cfg_set_s32(cfg, "rc:qp_max", m_param.fix_qp);
    mpp_enc_cfg_set_s32(cfg, "rc:qp_min", m_param.fix_qp);
    mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", m_param.fix_qp);
    mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", m_param.fix_qp);
    mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 0);
    mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_i", m_param.fix_qp);
    mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_i", m_param.fix_qp);
    mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_p", m_param.fix_qp);
    mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_p", m_param.fix_qp);

    mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
    /*
     * H.264 level_idc parameter
     * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
     * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
     * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
     * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
     * 50 / 51 / 52         - 4K@30fps
     */
    mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
    mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
    mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
    mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);

    ret = m_mpi->control(m_contex, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        LOG(ERROR, "mpi control MPP_ENC_SET_CFG failed error:%d", ret);
        mpp_enc_cfg_deinit(cfg);
        return -1;
    }

    mpp_dec_cfg_deinit(cfg);
    return ret;
}

int RkEncoder::encode(void *inbuf, int insize, uint8_t *outbuf)
{
    int ret = 0;

    ret = mpp_frame_init(&m_frame);
    if (ret) {
        LOG(ERROR, "mpp_frame_init failed\n");
        return -1;
    }

    mpp_frame_set_width(m_frame, m_param.width);
    mpp_frame_set_height(m_frame, m_param.height);
    mpp_frame_set_hor_stride(m_frame, m_param.hor_stride);
    mpp_frame_set_ver_stride(m_frame, m_param.ver_stride);
    mpp_frame_set_fmt(m_frame, m_param.fmt);
    mpp_frame_set_eos(m_frame, 1);
    mpp_frame_set_buffer(m_frame, m_buffer);

    LOG(INFO, "insize %d", insize);
    memcpy(mpp_buffer_get_ptr(m_buffer), inbuf, insize);

    ret = m_mpi->encode_put_frame(m_contex, m_frame);
    if (ret) {
        LOG(ERROR, "encoder put frame failed error:%d", ret);
        return -1;
    }

    ret = m_mpi->encode_get_packet(m_contex, &m_packet);
    if (ret) {
        LOG(ERROR, "encoder get packet failed error:%d", ret);
        return -1;
    }

    MppPacket m_extra_info;
    ret = m_mpi->control(m_contex, MPP_ENC_GET_EXTRA_INFO, &m_extra_info);
    if (ret) {
        LOG(ERROR, "mpi control enc get extra info failed");
        return -1;
    }

    void *ptr_extra_info = mpp_packet_get_data(m_extra_info);
    size_t len_extra_info = mpp_packet_get_length(m_extra_info);
    memcpy(outbuf, ptr_extra_info, len_extra_info);
    mpp_packet_deinit(&m_extra_info);

    void *ptr_video_data = mpp_packet_get_data(m_packet);
    size_t len_video_data = mpp_packet_get_length(m_packet);
    memcpy(outbuf + len_extra_info, ptr_video_data, len_video_data);
    mpp_packet_deinit(&m_packet);

    size_t len = len_extra_info + len_video_data;
    return len;
}

int RkEncoder::startCode3(uint8_t *buf)
{
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
        return 1;
    else
        return 0;
}

int RkEncoder::startCode4(uint8_t *buf)
{
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
        return 1;
    else
        return 0;
}
