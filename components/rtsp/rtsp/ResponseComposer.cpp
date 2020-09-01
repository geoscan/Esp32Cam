#include "ResponseComposer.hpp"
#include <sstream>

using namespace Rtsp;
using namespace std;

const map<StatusCode, const char *> ResponseComposer::statusCodeString = {
	{StatusCode::Ok, "OK"},
	{StatusCode::StreamNotFound, "Stream Not Found"},
	{StatusCode::UnsupportedTransport, "Unsupported Transport"},
	{StatusCode::NotImplemented, "Not Implemented"},
	{StatusCode::MethodNotAllowed, "Method Not Allowed"},
	{StatusCode::BadRequest, "Bad Request"} };

string ResponseComposer::responseCode(const Request &req, StatusCode code)
{
	std::string responseString;
	if (statusCodeString.find(code) != statusCodeString.end()) {
		responseString = std::string(statusCodeString.at(code));
	}
	return composeDel(kCrlf,
		composeDel(kS, kRtspVer, (size_t)code, responseString),
		composeDel(kS, kCseq, req.cseq.val()));
}

string ResponseComposer::toString(char c, size_t &len)
{
	len++;
	return {1, c};
}

string ResponseComposer::toString(const char *str, size_t &len)
{
	string ret{str};
	len += ret.length();
	return ret;
}

string ResponseComposer::toString(const string &str, size_t &len)
{
	len += str.length();
	return str;
}