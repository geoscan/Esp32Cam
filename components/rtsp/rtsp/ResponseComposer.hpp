#ifndef COMPONENTS_RTSP_RTSP_RESPONSECOMPOSER_HPP
#define COMPONENTS_RTSP_RTSP_RESPONSECOMPOSER_HPP

#include <string>
#include <map>
#include <array>
#include <algorithm>
#include "Types.hpp"

namespace Rtsp {

struct ResponseComposer final {
	static constexpr const char *kCrlf    {"\r\n"};
	static constexpr const char *kCseq    {"CSeq:"};
	static constexpr const char *kRtspVer {"RTSP/1.0"};
	static constexpr const char *kSession {"Session:"};

	static const std::map<StatusCode, const char *> statusCodeString;

	static std::string dateHeader();
	static std::string responseCode(const Request &, StatusCode);

	template <typename ... Args>
	static std::string compose(const Args&...);
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

template <typename Numeric>
inline std::string ResponseComposer::toString(const Numeric &val, size_t &len)
{
	std::string ret{std::to_string(val)};
	len += ret.length();
	return ret;
}

} // Rtsp

#endif // COMPONENTS_RTSP_RTSP_RESPONSECOMPOSER_HPP
