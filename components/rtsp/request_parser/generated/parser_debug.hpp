#ifndef COMPONENTS_RTSP_REQUEST_PARSER_GENERATED_PARSER_DEBUG_HPP
#define COMPONENTS_RTSP_REQUEST_PARSER_GENERATED_PARSER_DEBUG_HPP

#define PARSER_DEBUG 1
#define DEBUG_FLEX 0

#if PARSER_DEBUG != 1
# define debug(...)
#else // PARSER_DEBUG != 1

# include <iostream>
template<typename Arg>
void debug(Arg arg)
{
	std::cout << "DEBUG:" << arg << std::endl;
}

# define DEBUG_TEXT \
	"rtsp://example.com/media.mp4 RTSP/1.0\r\n"\
    "CSeq: 1\r\n"\
    "Session:2\r\n"\
    "Require: implicit-play\r\n"\
    "Proxy-Require: gzipped-messages"\
    "Transport: RTP/AVP/UDP;unicast;client_port=3058-3059"


#endif // PARSER_DEBUG


#endif // COMPONENTS_RTSP_REQUEST_PARSER_GENERATED_PARSER_DEBUG_HPP