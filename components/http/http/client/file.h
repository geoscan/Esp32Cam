//
// file.h
//
// Created on:  Jul 20, 2023
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef HTTP_CLIENTS_FILE_H
#define HTTP_CLIENTS_FILE_H

#include <esp_err.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Gets invoked each time a chunk of a file is received.
/// \param `aChunkSize` - size of the received chunk, 0 denotes the end of
/// transfer (only if `aChunk == 0` too)
/// \param `aChunk` - pointer to data. NULL means the announcement of the
/// expected file size (when the appropriate file header is received). In that
/// case, `aChunkSize` must be read
/// \param `aUserData` - optional argument, \sa `httpDownloadFileOverHttpGetByUrl`
///
/// Parameters interpretation
/// - aChunk == 0, aChunkSize == 0  -- finalize write
/// - aChunk == 0, aChunkSize != 0  -- announce beginning of a file
/// - aChunk != 0, aChunkSize != 0  -- file chunk has arrived
typedef esp_err_t (*OnFileChunkCallable)(const char *aChunk, size_t aChunkSize, void *aUserData);

/// Downloads a binary file over HTTP.
///
/// \param `aOnFileChunkCallable` - callback. Gets invoked on each chunk
/// \param `aUserData` - will be passed to `aOnFileChunkCallable`
esp_err_t httpDownloadFileOverHttpGetByUrl(const char *aFileUrl, OnFileChunkCallable aOnFileChunkCallable, void *aUserData);

esp_err_t httpDownloadFileOverHttpGet(const char *aHost, int aPort, const char *aPath,
	OnFileChunkCallable aOnFileChunkCallable, void *aUserData);

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENTS_FILE_H
