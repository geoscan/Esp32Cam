char requestBody[] = 
	"rtsp://example.com/media.mp4 RTSP/1.0\r\n"
	"CSeq: 1\r\n"
	"Session:2\r\n"
	"Require: implicit-play\r\n"
	"Proxy-Require: gzipped-messages\r\n"
	"Transport: RTP/AVP/UDP;unicast;client_port=3058-3059";