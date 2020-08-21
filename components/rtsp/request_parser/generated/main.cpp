#include "parser_debug.hpp"

#if DEBUG_FLEX != 1

#include <iostream>
#include "Types.hpp"

using namespace std;

extern void parse(Rtsp::Request &, Rtsp::Data);

// char *requestBody = 
//     "rtsp://example.com/media.mp4 RTSP/1.0"
//     "CSeq: 1"
//     "Session:2"
//     "Require: implicit-play"
//     "Proxy-Require: gzipped-messages";

int main(void)
{
    char requestBody[] = 
        "rtsp://example.com/media.mp4 RTSP/1.0\r\n"
        "CSeq: 1\r\n"
        "Session:2\r\n"
        "Require: implicit-play\r\n"
        "Proxy-Require: gzipped-messages";

    debug(requestBody);
    debug(sizeof(requestBody));

    Rtsp::Request request;
    Rtsp::Data    data{requestBody, sizeof(requestBody)};

    parse(request, data);
    
    cout << request.cseq << endl;
    cout << request.session << endl;

    return 0;
}

#endif //DEBUG_FLEX != 1