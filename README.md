## build
```
git submodule update --init
./scripts/build_x264.sh
mkdir build && cd build
cmake .. && make -j8
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
