#include "asio.hpp"
#include "RtspServer.hpp"

void rtspStart()
{
    asio::io_context context;
    RtspServer server(context);
    context.run();
}