#ifndef COMPONENTS_CAMERASTREAMERCAMERASTREAMCONTROL_HPP
#define COMPONENTS_CAMERASTREAMERCAMERASTREAMCONTROL_HPP

#include <memory>
#include <asio.hpp>

#include "CameraStream.hpp"

// Wrapper around TCP socket which
// 0. Accepts client's TCP connection
// 1. Instantiates UDP transmission to the same client's IP/PORT
// 2. Destructs transmission session when a client disconnects

class CameraStreamControl final : public std::enable_shared_from_this<CameraStreamControl>{
public:
	CameraStreamControl(asio::io_context &context, unsigned port, CameraStream &);
	// async run
	void asyncRun();
	~CameraStreamControl();
private:
	CameraStream            &cameraStream;
	asio::ip::tcp::acceptor acceptor;
};

#endif // CAMERASTREAMCONTROL_HPP
