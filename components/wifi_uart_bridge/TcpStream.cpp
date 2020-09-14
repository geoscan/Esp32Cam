/*
 * TcpStream.cpp
 *
 *  Created on: Jul 15, 2014
 *      Author: Alexander
 */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <esp_log.h>

#include "TcpStream.hpp"

char TcpStream::tag[] = "TcpStream";

void TcpStream::run(void *param)
{
	auto tcp = reinterpret_cast<TcpStream *>(param);
	const auto sock = tcp->getServerSocket();
	if (!::listen(sock, 5)) {
		ESP_LOGE(tag, "listen = error: %d %s", errno, strerror(errno));
	}
	while (true) {
		tcp->accept();
	}
}

TcpStream::TcpStream(uint16_t port):
	xSemaphore{xSemaphoreCreateMutex()},
	serverSocket{-1},
	actualPort{port}
{
	// Create a socket that we will listen upon.
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	xSemaphoreGiveRecursive( xSemaphore );
	// Bind our server socket to a port.
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);
	int rc  = bind(serverSocket, reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress));
	if (rc < 0) {
		ESP_LOGE(tag, "bind = error: %d %s", errno, strerror(errno));
	}
}

TcpStream::~TcpStream()
{
	for (auto clientSocket : clientSockets) {
		closesocket(clientSocket);
	}

	closesocket(serverSocket);
}

void TcpStream::start()
{
	xTaskCreate(TcpStream::run, "tcp_server_runnig", 1024*2, this, 5, NULL);
}

size_t TcpStream::read(void *data, size_t size)
{
	if( xSemaphoreTakeRecursive( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
		for (auto iter = clientSockets.begin(); iter != clientSockets.end();) {
			const ssize_t count = ::recv(*iter, data, size, MSG_DONTWAIT);
			if (count < 0 && errno != EAGAIN) {
				::shutdown(*iter, SHUT_RDWR);
				::closesocket(*iter);
				iter = clientSockets.erase(iter);
			} else if (count > 0) {
				xSemaphoreGiveRecursive( xSemaphore );
				return static_cast<size_t>(count);
			} else {
				++iter;
			}
		}
		xSemaphoreGiveRecursive( xSemaphore );
	}

	return 0;
}

size_t TcpStream::write(const void *data, size_t size)
{
	if( xSemaphoreTakeRecursive( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
		for (auto iter = clientSockets.begin(); iter != clientSockets.end();) {
			const ssize_t count = ::send(*iter, data, size, MSG_NOSIGNAL);
			size = count;
			if (count < 0 && errno != EAGAIN) {
				::shutdown(*iter, SHUT_RDWR);
				::closesocket(*iter);
				iter = clientSockets.erase(iter);
			} else {
				++iter;
			}
		}
		xSemaphoreGiveRecursive( xSemaphore );
	}
	return size;
}

size_t TcpStream::bytesToRead()
{
	if( xSemaphoreTakeRecursive( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
		for (int &clientSocket : clientSockets) {
			unsigned long bytesAvailable;
			const int ret = ioctlsocket(clientSocket, FIONREAD, &bytesAvailable);

			if (ret >= 0 && bytesAvailable > 0) {
				xSemaphoreGiveRecursive( xSemaphore );
				return static_cast<size_t>(bytesAvailable);
			}
		}
		xSemaphoreGiveRecursive( xSemaphore );
	}
	return 0;
}

size_t TcpStream::bytesToWrite()
{
	return 0;
}

bool TcpStream::waitForReadyRead(unsigned int ms)
{
	int ret = 0;
	size_t clientCount = 0;
	struct pollfd descriptors[MAX_CLIENTS];

	if( xSemaphoreTakeRecursive( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
		clientCount = clientSockets.size();

		if (!clientCount) {
			usleep(ms * 1000);
			xSemaphoreGiveRecursive( xSemaphore );
			return false;
		}

		for (size_t i = 0; i < clientCount; ++i) {
			descriptors[i].fd = clientSockets[i];
			descriptors[i].events = POLLIN;
		}
		xSemaphoreGiveRecursive( xSemaphore );
	}

	ret = poll(descriptors, clientCount, ms);

	return ret > 0;
}

bool TcpStream::waitForBytesWritten(unsigned int)
{
	return true;
}

bool TcpStream::accept()
{
	struct sockaddr_in clientAddress;
	socklen_t receivedLength = sizeof(clientAddress);

	if( xSemaphoreTakeRecursive( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
		if (serverSocket == -1 || clientSockets.size() >= MAX_CLIENTS) {
			xSemaphoreGiveRecursive( xSemaphore );
			return false;
		}
		xSemaphoreGiveRecursive( xSemaphore );
	}

	int clientSocket = ::accept(serverSocket, (struct sockaddr *)&clientAddress, &receivedLength);
	if (clientSocket < 0) {
		return false;
	}
	fcntl(clientSocket, F_SETFL, O_NONBLOCK);

	int nodelay = 1;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char *>(&timeout), sizeof(timeout));
	setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&nodelay), sizeof(nodelay));

	if( xSemaphoreTakeRecursive( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
		clientSockets.push_back(clientSocket);
		xSemaphoreGiveRecursive( xSemaphore );
	}

	return true;
}

bool TcpStream::anyClientSockets()
{
	bool res = false;
	if( xSemaphoreTakeRecursive( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
		res = clientSockets.size() != 0;
		xSemaphoreGiveRecursive( xSemaphore );
	}
	return res;
}
