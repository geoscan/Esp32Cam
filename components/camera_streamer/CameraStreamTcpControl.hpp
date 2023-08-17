//
// CameraStreamTcpControl.hpp
//
// Created on: Mar 03, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//
// Listens to incoming client's connection on a designated TCP port, subscribes
// client on CameraStream, handles client's disconnect.
//

#ifndef CAMERA_STREAMER_CAMERASTREAMTCPCONTROL_HPP
#define CAMERA_STREAMER_CAMERASTREAMTCPCONTROL_HPP

#include <esp_event.h>
#include <map>
#include "wifi/Sub.hpp"
#include "sub/Subscription.hpp"

namespace CameraStreamer {

class CameraStreamTcpControl {
public:
	void operator()();
	CameraStreamTcpControl(asio::ip::tcp::acceptor &, asio::ip::tcp::socket &);
	~CameraStreamTcpControl();
private:
	static size_t instances;

	void handleApClientDisconnected(asio::ip::address);  // Handle end of client's Wi-Fi connection

	struct {
		asio::ip::tcp::socket   &socket;
		asio::ip::tcp::acceptor &acceptor;
	} tcp;

	struct {
		Wifi::Sub::Disconnected wifiDisconnected;
	} key;

};

}  // namespace CameraStreamer

#endif  // CAMERA_STREAMER_CAMERASTREAMTCPCONTROL_HPP
