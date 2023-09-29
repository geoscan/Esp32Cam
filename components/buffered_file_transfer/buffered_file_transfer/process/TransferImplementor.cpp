//
// TransferImplementor.cpp
//
// Created on: Sep 22, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#include "buffered_file_transfer/buffered_file_transfer.hpp"
#include "system/os/Assert.hpp"
#include "system/os/Logger.hpp"
#include "system/os/Mutex.hpp"
#include "utility/comm/InstanceRegistry.hpp"
#include "utility/thr/LockGuard.hpp"
#include <vector>

#include "TransferImplementor.hpp"

namespace Bft {

struct Instances
{
	Ut::Comm::InstanceRegistry<TransferImplementor, std::vector> instanceRegistry;
	Sys::Mutex mutex;
};

static Instances sInstances{};
static constexpr const char *kLogPreamble = "TransferImplementor";

void TransferImplementor::notifyAllOnFileBufferingFinished(std::shared_ptr<File> aFile, bool aIsLastChunk)
{
	volatile Ut::LockGuard<Sys::Mutex> lock{sInstances.mutex};

	for (auto *instance : sInstances.instanceRegistry.instances) {
		if (instance == nullptr) {
			Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s::%s got `nullptr` instance, fatal!", kLogPreamble,
				__func__);
			Sys::panic();
		} else {
			instance->onFileBufferingFinished(aFile, aIsLastChunk);
		}
	}
}

void TransferImplementor::subscribeInstanceForBufferingUpdates(TransferImplementor *aInstance)
{
	volatile Ut::LockGuard<Sys::Mutex> lock{sInstances.mutex};

	if (aInstance == nullptr) {
		Sys::Logger::write(Sys::LogLevel::Error, debugTag(), "%s:%s attempting to register nullptr instance",
			kLogPreamble, __func__);
		Sys::panic();
	}

	sInstances.instanceRegistry.add(*aInstance);
}

}  // Bft
