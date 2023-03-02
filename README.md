## dependent
```
libv4l2cc:
https://github.com/icetd/libv4l2cc.git

libx264:
https://code.videolan.org/videolan/x264.git
```

## build
```
amd64:
mkdir build && cd build
cmake .. && make -j8

others:
Download libv4l2cc and libx264 source code compilation.
Put the library in the corresponding position of thirdparty.
```

## use
```
./RtspServer

use ffplay to play:
ffplay -i https://127.0.0.1:8554/video0
```

size: 640x480 url: https://127.0.0.1:8554/video0

You can modify the parameters from the source code.



