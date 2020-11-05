#include "CameraStreamControl.hpp"

using asio::ip::tcp;

	// TODO: consider deinit. after obj. destruction
//	esp_event_loop_args_t loopArgs {1024, "CamStreamer", 5, 1024, 0};
//	esp_event_loop_handle_t loopHandle;
//	esp_event_loop_create(&loopArgs, &loopHandle);
//	ESP_ERROR_CHECK(esp_event_handler_register_with(loopHandle, WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, CameraStreamControl::handleApDisconnected, this));


CameraStreamControl::CameraStreamControl(asio::io_context &context, unsigned port, CameraStream &cs) :
	cameraStream(cs),
	context(context),
	acceptor(context, tcp::endpoint(tcp::v4(), port))
{
	esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, CameraStreamControl::handleApDisconnected, this);
//	assert(a == ESP_OK);
}

CameraStreamControl::~CameraStreamControl()
{
	cameraStream.removeSinks();
}

void CameraStreamControl::run()
{
	while (true) {
		socket = std::make_shared<asio::ip::tcp::socket>(context);
		asio::ip::tcp::endpoint clientEndpoint;
		asio::error_code        err;

		acceptor.accept(*socket, clientEndpoint, err);
		if (!err) {
			asio::ip::udp::endpoint clientEndpoint(socket->remote_endpoint().address(), socket->remote_endpoint().port());
			cameraStream.addSink(clientEndpoint);

			char stubBuffer[1];
			std::error_code err;

			while (err != asio::error::connection_reset && err != asio::error::eof && err != asio::error::bad_descriptor) {
				socket->receive(asio::buffer(stubBuffer, 1), 0, err);
			}
			cameraStream.removeSinks();
		}

		socket->close();
	}
}

void CameraStreamControl::handleApDisconnected(void *arg, esp_event_base_t, int32_t, void *)
{
	auto &instance = *reinterpret_cast<CameraStreamControl *>(arg);
	if (instance.socket) {
		instance.socket->close();
	}
	instance.cameraStream.removeSinks();
}