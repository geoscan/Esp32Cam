//
// HttpFetchTest.cpp
//
// Created on: Sep 29, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "buffered_file_transfer/buffered_file_transfer.hpp"
#include "buffered_file_transfer/process/TransferImplementor.hpp"
#include "http/client/file.h"
#include "system/os/Assert.hpp"
#include "system/os/Logger.hpp"
#include <esp_err.h>

#include "HttpFetchTest.hpp"

namespace Bft {

static constexpr const char *kLogPreamble = "HttpFetchTest";

HttpFetchTest::HttpFetchTest(const char *aFileHttpUrl, const char *aBufferedFileTransferFileName):
	fileHttpUrl{aFileHttpUrl},
	bufferedFileTransferFileName{aBufferedFileTransferFileName}
{
}

void HttpFetchTest::runTest()
{
	Sys::Logger::write(Sys::LogLevel::Info, debugTag(),
		"%s:%s initializing file transfer name=\"%s\" BFT file name=\"%s\"", kLogPreamble, __func__);
	fetchFile(fileHttpUrl, bufferedFileTransferFileName);
}

}  // Bft
