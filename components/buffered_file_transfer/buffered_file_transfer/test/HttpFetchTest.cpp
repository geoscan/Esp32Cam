//
// HttpFetchTest.cpp
//
// Created on: Sep 29, 2023
//     Author: Dmitry Murashov (d <DOT> murashov <AT> geoscan <DOT> aero)
//

#include "http/client/file.h"

#include "HttpFetchTest.hpp"

namespace Bft {

/// \brief Callback. The signature is compliant with HTTP file API (see
/// `file.h`)
static void onFileChunkReceivedWrapper(const char *aChunk, size_t aChunkSize, void *aUserData);

static void onFileChunkReceivedWrapper(const char *aChunk, size_t aChunkSize, void *aUserData)
{
	static_cast<HttpFetchTest *>(aUserData)->onFileChunkReceived(aChunk, aChunkSize, aUserData);
}

HttpFetchTest::HttpFetchTest(const char *aFileHttpUrl):
	fileHttpUrl{aFileHttpUrl}
{
}

void HttpFetchTest::runTest()
{
	httpDownloadFileOverHttpGetByUrl(fileHttpUrl, onFileChunkReceivedWrapper, static_cast<void *>(this));
}

void HttpFetchTest::onFileChunkReceived(const char *aChunk, size_t aChunkSize, void *aUserData)
{
	(void)aChunk;
	(void)aChunkSize;
	(void)aUserData;
}

}  // Bft
