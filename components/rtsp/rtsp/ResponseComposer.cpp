#include "ResponseComposer.hpp"
#include <sstream>

using namespace Rtsp;
using namespace std;

const map<StatusCode, const char *> ResponseComposer::statusCodeString = {
	{StatusCode::Ok, "OK"},
	{StatusCode::StreamNotFound, "Stream not found"} };

string ResponseComposer::responseCode(const Request &req, StatusCode code)
{
	return "";
//	return compose(kRtspVer, ' ', (size_t)code, ' ', statusCodeString[code], kCrlf,
//		kCseq, req.cseq.val(), kCrlf,
//		dateHeader(), kCrlf);
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