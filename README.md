## dependent
```
libx264:
https://code.videolan.org/videolan/x264.git
```

## build
```
amd64:
git submodule update --init
mkdir build && cd build
cmake .. && make -j8

arm64:
Download libx264 source code compilation.
Put the library in the corresponding position of thirdparty.
```

## config

You can change configs/config.ini 
```
[video]
width = 1280
height = 720
fps = 30
format = YUY2 
;[or format = MJPEG]
```

## use
```
./RtspServer

use ffplay to play:
ffplay -rtsp_transport tcp -i rtsp://127.0.0.1:8554/video0
```
