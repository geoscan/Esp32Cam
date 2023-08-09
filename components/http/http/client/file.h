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
/// transfer
/// \param aUserData - optional argument, \sa `httpDownloadFileOverHttpGetByUrl`
typedef esp_err_t (*OnFileChunkCallable)(const char *aChunk, size_t aChunkSize, void *aUserData);

/// Downloads a binary file over HTTP.
///
/// \param aOnFileChunkCallable - callback. Gets invoked on each chunk
/// \param aUserData - will be passed to `aOnFileChunkCallable`
esp_err_t httpDownloadFileOverHttpGetByUrl(const char *aFileUrl, OnFileChunkCallable aOnFileChunkCallable, void *aUserData);

esp_err_t httpDownloadFileOverHttpGet(const char *aHost, int aPort, const char *aPath,
	OnFileChunkCallable aOnFileChunkCallable, void *aUserData);

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENTS_FILE_H
