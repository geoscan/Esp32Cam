#ifndef COMPONENTS_CAMERASTREAMERCAMERASTREAMCONTROL_HPP
#define COMPONENTS_CAMERASTREAMERCAMERASTREAMCONTROL_HPP

#include <memory>
#include <asio.hpp>
#include <atomic>
#include <esp_event.h>

#include "CameraStream.hpp"

// Wrapper around TCP socket which
// 0. Accepts client's TCP connection
// 1. Instantiates UDP transmission to the same client's IP/PORT
// 2. Destructs transmission session when a client disconnects

class CameraStreamControl final : public std::enable_shared_from_this<CameraStreamControl>{
public:
	CameraStreamControl(asio::io_context &context, unsigned port, CameraStream &);
	void run();
	~CameraStreamControl();
private:
	static void handleApDisconnected(void *arg, esp_event_base_t, int32_t eventId, void *eventData);

	CameraStream            &cameraStream;
	asio::io_context        &context;
	asio::ip::tcp::acceptor acceptor;
//	std::atomic_bool        fRunning;
	std::shared_ptr<asio::ip::tcp::socket> socket;
};

#endif // CAMERASTREAMCONTROL_HPP
