#include "parser_debug.hpp"

#if DEBUG_FLEX != 1

#include <iostream>
#include "Types.hpp"

using namespace std;

extern void parse(Rtsp::Request &, const void *, size_t);


#if PARSER_DEBUG == 1

int main(void)
{
    char requestBody[] = DEBUG_TEXT;

    debug(requestBody);
    debug(sizeof(requestBody));

    Rtsp::Request request;
    // Rtsp::Data    data{requestBody, sizeof(requestBody)};

    parse(request, requestBody, sizeof(requestBody));
    
    cout << request.cseq.val() << endl;
    cout << request.session.val() << endl;
    cout << request.clientPort.val() << endl;
    cout << boolalpha << request.udp.val() << endl;
    cout << boolalpha << (request.requestType.val() == Rtsp::RequestType::Setup) << endl;
    // cout << request.hostaddr.val() << endl;
    // cout << (request.hostport.isVal() ? request.hostport.val() : "") << endl;

    return 0;
}

#endif // PARSER_DEBUG == 1

#endif // DEBUG_FLEX != 1