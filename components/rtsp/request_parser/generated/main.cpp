#include "parser_debug.hpp"

#if DEBUG_FLEX != 1

#include <iostream>
#include "Types.hpp"

using namespace std;

extern void parse(Rtsp::Request &, Rtsp::Data);


#if PARSER_DEBUG == 1

int main(void)
{
    char requestBody[] = DEBUG_TEXT;

    debug(requestBody);
    debug(sizeof(requestBody));

    Rtsp::Request request;
    Rtsp::Data    data{requestBody, sizeof(requestBody)};

    parse(request, data);
    
    cout << request.cseq.val() << endl;
    cout << request.session.val() << endl;
    cout << request.clientPort.val() << endl;
    cout << boolalpha << request.udp.val() << endl;
    cout << boolalpha << (request.requestType.val() == Rtsp::RequestType::Setup);

    return 0;
}

#endif // PARSER_DEBUG == 1

#endif // DEBUG_FLEX != 1