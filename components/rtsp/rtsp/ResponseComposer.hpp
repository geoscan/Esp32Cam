#ifndef COMPONENTS_RTSP_RTSP_RESPONSECOMPOSER_HPP
#define COMPONENTS_RTSP_RTSP_RESPONSECOMPOSER_HPP

#include <string>
#include <map>
#include <array>
#include <algorithm>
#include "Types.hpp"

namespace Rtsp {

struct ResponseComposer final {
	static constexpr const char *kCseq           = "CSeq:";
	static constexpr const char *kRtspVer        = "RTSP/1.0";
	static constexpr const char *kSession        = "Session:";
	static constexpr const char *kTransport      = "Transport:";
	static constexpr const char *kUdp            = "RTP/AVP/UDP";
	static constexpr const char *kUnicast        = "unicast";
	static constexpr const char *kClientPort     = "client_port";
	static constexpr const char *kApplicationSdp = "application/sdp";

	static constexpr const char *kContentBase   = "Content-Base:";
	static constexpr const char *kContentType   = "Content-Type:";
	static constexpr const char *kContentLength = "Content-Length:";


	static constexpr const char *kCrlf       = "\r\n";
	static constexpr const char *kS          = " ";
	static constexpr const char *kSemicolon  = ";";
	static constexpr const char *kEq         = "=";

	static const std::map<StatusCode, const char *> statusCodeString;

//	static std::string dateHeader();
	static std::string responseCode(const Request &, StatusCode);

	template <typename ... Args>
	static std::string compose(const Args&...);

	template <typename Arg, typename ... Args>
	static std::string composeDel(const char *, const Arg &arg, const Args&...); ///< Compose with delimeter
private:
	template <typename Numeric>
	static std::string toString(const Numeric &val, size_t &len);

	static std::string toString(char, size_t &);
	static std::string toString(const std::string &, size_t &);
	static std::string toString(const char *, size_t &);
};

template <typename ... Args>
inline std::string ResponseComposer::compose(const Args&... args)
{
	size_t len = 0;
	std::array<std::string, sizeof...(args)> converted = {toString(args, len)...};

	std::string ret;
	ret.reserve(len);

	auto it = ret.begin();
	std::for_each(converted.begin(), converted.end(),
		[&it](const std::string &str) {
			it = std::copy(str.begin(), str.end(), it);
		});

	return ret;
}

template <typename Arg, typename ... Args>
inline std::string ResponseComposer::composeDel(const char *delimeter, const Arg &arg, const Args&...args)
{
	return compose(arg, compose(delimeter, args)...);
}

template <typename Numeric>
inline std::string ResponseComposer::toString(const Numeric &val, size_t &len)
{
	std::string ret{std::to_string(val)};
	len += ret.length();
	return ret;
}

} // Rtsp

#endif // COMPONENTS_RTSP_RTSP_RESPONSECOMPOSER_HPP
