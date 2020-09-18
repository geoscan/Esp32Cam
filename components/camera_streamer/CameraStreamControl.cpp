#include "CameraStreamControl.hpp"

using asio::ip::tcp;

CameraStreamControl::CameraStreamControl(asio::io_context &context, unsigned port, CameraStream &cs) :
	cameraStream(cs),
	context(context),
	acceptor(context, tcp::endpoint(tcp::v4(), port))
{
}

void CameraStreamControl::asyncRun()
{
	auto self(shared_from_this());
	acceptor.async_accept([this, self](std::error_code err, tcp::socket socket) {
		asio::ip::udp::endpoint sink(socket.remote_endpoint().address(), socket.remote_endpoint().port());
		cameraStream.addSink(sink);
		asyncRun();
		if (!err) {
//			connected();
			char stubBuffer[1];
			std::error_code recvCode;
			socket.receive(asio::buffer(stubBuffer, 1), asio::socket_base::message_flags{}, recvCode);
			cameraStream.removeSink(sink);
		}
	});
}

void CameraStreamControl::run()
{
	while (true) {
		asio::ip::tcp::socket client(context);
		acceptor.accept(client);
		asio::ip::udp::endpoint clientEndpoint(client.remote_endpoint().address(), client.remote_endpoint().port());
		cameraStream.addSink(clientEndpoint);

		char stubBuffer[1];
		std::error_code err;

		while (err != asio::error::connection_reset && err != asio::error::eof) {
			client.receive(asio::buffer(stubBuffer, 1), 0, err);
//			client.send(asio::buffer("hello", 5));
		}
	}
}

CameraStreamControl::~CameraStreamControl()
{
	cameraStream.removeSinks();
}