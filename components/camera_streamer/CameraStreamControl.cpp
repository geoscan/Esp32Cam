#include "CameraStreamControl.hpp"

using asio::ip::tcp;

CameraStreamControl::CameraStreamControl(asio::io_context &context, unsigned port, CameraStream &cs) :
	cameraStream(cs),
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
			char stubBuffer[1];
			std::error_code recvCode;
			// XXX: will asio allow us to create an empty buffer?
			socket.receive(asio::buffer(stubBuffer, sizeof(stubBuffer)), asio::socket_base::message_flags{}, recvCode);
			cameraStream.removeSink(sink);
		}
	});
}

CameraStreamControl::~CameraStreamControl()
{
	cameraStream.removeSinks();
}