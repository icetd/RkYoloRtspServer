#ifndef RK_ENCODER_H
#define RK_ENCODER_H

#include <bits/stdint-uintn.h>
#include <rockchip/rk_mpi.h>
#include <rockchip/rk_mpi_cmd.h>
#include <rockchip/rk_type.h>
#include <rockchip/mpp_buffer.h>
#include <rockchip/mpp_frame.h>

typedef struct
{
    MppFrameFormat fmt;
    int width;
    int height;
    int hor_stride;
    int ver_stride;
    int frame_size;
    int fps;
    int fix_qp;
} Encoder_Param_t;

class RkEncoder
{
public:
    RkEncoder(Encoder_Param_t param);
    ~RkEncoder();

    int init();
    int encode(void *inbuf, int insize, uint8_t *outbuf);

    int startCode3(uint8_t *buf);
    int startCode4(uint8_t *buf);

private:
    Encoder_Param_t m_param;
    MppCtx m_contex;
    MppApi *m_mpi;
    MppPacket m_packet = nullptr;
    MppFrame m_frame = nullptr;
    MppBuffer m_buffer = nullptr;
};

#endif
