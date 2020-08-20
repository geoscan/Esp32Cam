#include "RtspServer.hpp"
#include <memory>

using namespace std;
using asio::ip::tcp;

RtspServer::RtspServer(asio::io_context &context) :
	tcpAcceptor(context, tcp::endpoint(tcp::v4(), RtspServer::kPort))
{
	acceptConnection();
}

void RtspServer::acceptConnection()
{
	tcpAcceptor.async_accept(
		[this](error_code err, tcp::socket socket) {
			if (!err) {
				make_shared<RtspConnection>(std::move(socket))->start();
			}
			acceptConnection();
		});
}