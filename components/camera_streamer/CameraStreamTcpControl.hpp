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

#include "CameraStream.hpp"
#include "SinkUdp.hpp"

class CameraStreamTcpControl {
public:
	void operator()();
	CameraStreamTcpControl(CameraStream &, asio::ip::tcp::acceptor &, asio::ip::tcp::socket &, asio::ip::udp::socket &);
	~CameraStreamTcpControl();
private:
	void addUdpSink(asio::ip::address, unsigned short port);
	void removeUdpSink(asio::ip::address);
	void handleApClientDisconnected(asio::ip::address);  // Handle end of client's Wi-Fi connection
	static void handleApClientDisconnected(void *arg, esp_event_base_t, int32_t, void *data);  // Handle end of client's Wi-Fi connection

	CameraStream            &cameraStream;

	struct {
		asio::ip::tcp::socket   &socket;
		asio::ip::tcp::acceptor &acceptor;
	} tcp;

	struct {
		std::map<asio::ip::address, SinkUdp> map;
		std::mutex                           mutex;
		asio::ip::udp::socket                &udpSocket;
	} sinks;

};

#endif  // CAMERA_STREAMER_CAMERASTREAMTCPCONTROL_HPP
