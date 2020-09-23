/*
 * TcpStream.hpp
 *
 *  Created on: Jul 15, 2014
 *      Author: Alexander
 */

#ifndef PLATFORM_LINUX_DRIVERS_TCPSTREAM_HPP_
#define PLATFORM_LINUX_DRIVERS_TCPSTREAM_HPP_

#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <lwip/sockets.h>

class TcpStream {
public:
	TcpStream(uint16_t port);
	virtual ~TcpStream();

	size_t read(void *data, size_t size);
	size_t write(const void *data, size_t size);
	size_t bytesToRead();
	size_t bytesToWrite();

	bool waitForReadyRead(unsigned int ms = 0);
	bool waitForBytesWritten(unsigned int ms = 0);

	uint16_t port()
	{
		return actualPort;
	}

	static void run(void *ignore);
	void start();
	bool accept();
	int getServerSocket()
	{
		return serverSocket;
	}

	bool anyClientSockets();

	SemaphoreHandle_t xSemaphore;

private:
	enum {
		RETRY_INTERVAL = 1000
	};
	enum {
		MAX_CLIENTS = 8
	};

	static char tag[];

	int serverSocket;
	struct sockaddr_in serverAddress;
	std::vector<int> clientSockets;

	uint16_t actualPort;

//	SemaphoreHandle_t xMutex;
};

#endif // PLATFORM_LINUX_DRIVERS_TCPSTREAM_HPP_
