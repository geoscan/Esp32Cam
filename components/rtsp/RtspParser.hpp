//
// RtspParser.hpp
//
// Created on: Aug. 20, 2020
// Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef RTSPPARSER_HPP
#define RTSPPARSER_HPP

#include "Types.hpp"

class RtspParser {
public:
	bool parse(Rtsp::Data, Rtsp::Request &);
};

#endif // RTSPPARSER_HPP
