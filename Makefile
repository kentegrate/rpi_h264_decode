decode: video_decode.c
	gcc decode_video.c `pkg-config --cflags --libs libavformat libavutil`  -L/opt/vc/
