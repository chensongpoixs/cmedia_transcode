# 媒体服务转码版本GPU（cuda） 支持H264与H265转码



```

./ffmpeg -rtsp_transport tcp  -c:v h264 -i "rtsp://admin:hik12345@192.168.1.4:554/H264/ch1/main/av_stream" -c:v libx264  -b:v 2M -maxrate 1M -bufsize 4M -s 1920x1080 -c:a aac -b:a 128k -f flv   rtmp://192.168.31.113/live/test1
 
./ffmpeg -rtsp_transport tcp  -c:v h264 -i "rtsp://admin:hik12345@192.168.1.4:554/H264/ch1/main/av_stream"   -vcodec copy  -f flv   rtmp://192.168.31.113:11935/live/test1


```