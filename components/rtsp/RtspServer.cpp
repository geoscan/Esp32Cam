#include "RtspServer.hpp"
#include <memory>

using namespace std;
using namespace asio::ip;

RtspServer::RtspServer(asio::io_context &context)
	: tcpAcceptor(context)
{
	acceptConnection();
}

void RtspServer::acceptConnection()
{
	tcpAcceptor.async_accept(
		[this](error_code err, tcp::socket socket) {
			if (!err) {
				make_shared<RtspConnection>(move(socket))->start();
			}
			acceptConnection();
		});
}