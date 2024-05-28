# RkCamRtspServer
simple yolov5 rtspserver for rk3588

## test env
- rk3588s
- ubuntu 20.04
- USB camera

## build && run
```
./scripts/build_mpp.sh
mkdir build && cd build
cmake ..
make -j4
sudo ./RkYoloRtspServer
```

## config
config with file configs/config.ini
```
[rknn]
model_path = model/yolov5.rknn

[log]
level = NOTICE

[video]
width = 640
height = 480
fps = 30
fix_qp = 23     
device = /dev/video0

[server]
rtsp_port = 8554
stream_name = unicast
max_buf_size = 200000
max_packet_size  = 1500
http_enable = false
http_port = 
bitrate = 1440
```

## function
- support yuy2 format usb camera.
- support hardware h264 encode.
- yolov5 target detection

## demo

![](pic/demo.png)