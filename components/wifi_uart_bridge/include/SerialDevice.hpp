//
// SerialDevice.hpp
//
//  Created on: Mar 27, 2013
//      Author: MAvkhimenia
//

#ifndef CORE_HAL_SERIALDEVICE_HPP_
#define CORE_HAL_SERIALDEVICE_HPP_

#include <cstddef>
#include <cstdint>

class SerialDevice {
public:
	enum class Parity {
		SERIAL_PARITY_NONE,
		SERIAL_PARITY_ODD,
		SERIAL_PARITY_EVEN
	};

	enum class StopBits {
		SERIAL_ONE_STOP,
		SERIAL_TWO_STOP
	};

	virtual ~SerialDevice() = default;

	virtual size_t read(void *data, size_t size) = 0;
	virtual size_t write(const void *data, size_t size) = 0;
	virtual size_t bytesToRead() = 0;
	virtual size_t bytesToWrite() = 0;

	virtual void setBaudRate(uint32_t baudrate) = 0;
	virtual uint32_t getBaudRate() = 0;
	virtual void setParity(Parity parity) = 0;
	virtual Parity getParity() = 0;
	virtual void setStopBits(StopBits stopBits) = 0;
	virtual StopBits getStopBits() = 0;

	virtual bool waitForReadyRead(unsigned int ms = 0) = 0;
	virtual bool waitForBytesWritten(unsigned int ms = 0) = 0;
};

#endif // CORE_HAL_SERIALDEVICE_HPP_
