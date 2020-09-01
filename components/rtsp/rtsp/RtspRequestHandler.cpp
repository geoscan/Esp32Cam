//
// RtspRequestHandler.cpp
//
// Created on:  Aug 24, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include <algorithm>
#include "RtspRequestHandler.hpp"
#include "ResponseComposer.hpp"
#include <sstream>
#include "rtsp.h"
#include "esp_timer.h"

using namespace std;
using namespace Rtsp;

using Rc = ResponseComposer;

extern void parse(Rtsp::Request &, const void *, const size_t); // Parser generated by gnu flex/bison


std::string RtspRequestHandler::handle(asio::const_buffer buffer, asio::ip::address addr)
{
	Rtsp::Request request;
	parse(request, buffer.data(), buffer.size());
	request.clientAddress = addr;

	switch (request.requestType.val()) {

		case Rtsp::RequestType::Play:
			return handlePlay(request);

		case Rtsp::RequestType::Pause:
			return handlePause(request);

		case Rtsp::RequestType::Describe:
			return handleDescribe(request);

		case Rtsp::RequestType::Setup:
			return handleSetup(request);

		case Rtsp::RequestType::Teardown:
			return handleTeardown(request);

		default:
			return handleNotStated(request);
	}
}


std::string RtspRequestHandler::handlePlay(const Rtsp::Request &req)
{
	// using Rc = ResponseComposer;

	if (!req.hostaddr.isVal() || !req.hostResource.isVal()) {  // Client didn't provide us with an URL
		return Rc::responseCode(req, StatusCode::StreamNotFound);
	}

	if (!(req.session.isVal() && rtpServer.setSessionStreaming(req.session.val(), true))) {
		// Client didn't specify Session header, or RTP server is unable to fullfill the request
		return Rc::responseCode(req, StatusCode::SessionNotFound);
	}

	// Went smooth, prepare 200 OK.
	string port((req.hostport.isVal()) ? Rc::compose(Rc::kColon, req.hostport.val()) : "");
	auto url(Rc::compose(Rc::kRtsp, req.hostaddr.val(), port, req.hostResource.val()));

	return Rc::composeDel(Rc::kCrlf,
		Rc::responseCode(req, StatusCode::Ok),
		Rc::compose(Rc::kRange,   Rc::kS, Rc::kNptEq, "0.000-"),
		Rc::compose(Rc::kSession, Rc::kS, req.session.val()),
		Rc::compose(Rc::kRtpInfo, Rc::kS, Rc::kUrlEq, url),
		Rc::kCrlf);

}

std::string RtspRequestHandler::handlePause(const Rtsp::Request &req)
{
	// See ::handlePlay to get an insight in what's going on.
	// Those methods are almost identical.

	if (!req.hostaddr.isVal() || !req.hostResource.isVal()) {  // Client didn't provide us with an URL
		return Rc::responseCode(req, StatusCode::StreamNotFound);
	}

	if (!(req.session.isVal() && rtpServer.setSessionStreaming(req.session.val(), false))) {
		return Rc::responseCode(req, StatusCode::SessionNotFound);
	}

	return Rc::composeDel(Rc::kCrlf,
		Rc::responseCode(req, StatusCode::Ok),
		Rc::compose(Rc::kSession, Rc::kS, req.session.val()));
}

std::string RtspRequestHandler::handleDescribe(const Rtsp::Request &req)
{
	// using Rc = ResponseComposer;

	if (!media.canCreateStreams(req)) {
		return ResponseComposer::responseCode(req, StatusCode::StreamNotFound);
	}

	string mediaDescription(media.getSdp(req));

	auto url(Rc::compose("rtsp://", req.hostaddr.val(), ':', req.hostport.val(), '/', req.hostResource.val()));

	return Rc::composeDel(Rc::kCrlf,
		Rc::responseCode(req, StatusCode::Ok),
		Rc::composeDel(Rc::kS, Rc::kCseq, req.cseq.val()),
		Rc::composeDel(Rc::kS, Rc::kContentBase, url),
		Rc::composeDel(Rc::kS, Rc::kContentType, Rc::kApplicationSdp),
		Rc::composeDel(Rc::kS, Rc::kContentLength, mediaDescription.length()),
		mediaDescription);
}

std::string RtspRequestHandler::handleSetup(const Rtsp::Request &req)
{
	// using Rc = ResponseComposer;

	if (!req.clientPort.isVal()) {  // Client hasn't specified its port
		return Rc::responseCode(req, StatusCode::BadRequest);
	}

	if (!media.canCreateStreams(req)) {  // Server couldn't find an appropriate stream source
		return Rc::responseCode(req, StatusCode::StreamNotFound);
	}

	if (!req.udp.isValEq(true)) {  // Server only supports RTP over UDP
		return Rc::responseCode(req, StatusCode::UnsupportedTransport);
	}

	auto transportInfo(Rc::composeDel(Rc::kSemicolon,
		Rc::kUdp,
		Rc::kUnicast,
		Rc::compose(Rc::kClientPort, Rc::kEq, req.clientPort.val())));

	auto streams = media.createStreams(req);

	if (all_of(streams.begin(), streams.end(), [](const Media::Stream &s) {return s.valid();})) { // All streams are valid
		Rtsp::SessionId sid = esp_timer_get_time();

		auto response = Rc::composeDel(Rc::kCrlf,
			Rc::responseCode(req, StatusCode::Ok),
			Rc::compose(Rc::kTransport, Rc::kS, transportInfo),
			Rc::compose(Rc::kSession, Rc::kS, sid));

		for (auto &stream : streams) {
			if (stream.source) {
				rtpServer.addSession(sid, move(stream.source), stream.sink);
			}
			rtpServer.addSession(sid, stream.sourceId, stream.sink);
		}

		return response;
	} else {
		return Rc::responseCode(req, StatusCode::StreamNotFound);
	}
}

std::string RtspRequestHandler::handleTeardown(const Rtsp::Request &)
{
	return "";
}

std::string RtspRequestHandler::handleNotStated(const Rtsp::Request &req)
{
	return ResponseComposer::responseCode(req, StatusCode::MethodNotAllowed);
}