//
// DisconnectHandler.hpp
//
// Created on: Aug 08, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#ifndef WIFI_WIFI_DISCONNECTHANDLER_HPP
#define WIFI_WIFI_DISCONNECTHANDLER_HPP

#include "utility/MakeSingleton.hpp"
#include <esp_event.h>

namespace Wifi {

// Notifies subscribers when Wi-Fi client disconnected from the AP
class DisconnectHandler : public Ut::MakeSingleton<DisconnectHandler> {
public:
	DisconnectHandler();
	~DisconnectHandler();
	void onDisconnected(void *aData);

private:
	static void dispatchDisconnectedEvent(void */*nullptr*/, esp_event_base_t, int32_t, void *aData);
};

}  // Wifi

#endif // WIFI_WIFI_DISCONNECTHANDLER_HPP
