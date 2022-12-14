//
// Sta.hpp
//
// Created on: Aug 09, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#include "module/ModuleBase.hpp"
#include "module/Parameter/Result.hpp"
#include <esp_netif.h>
#include <string>

namespace Wifi {

/// \brief `Sta` class provides module API for Wi-Fi STA-related functionality
class Sta : public Mod::ModuleBase {
private:
	/// \brief Buffered connection credentials
	struct Credentials {
		/// SSID of the AP the STA should connect to
		std::string ssid;
		/// Password of the AP the STA should connect to
		std::string password;
		bool autoconnect;
		std::uint32_t ip;
		std::uint32_t gateway;
		std::uint32_t netmask;
		/// \brief Makes an attempt to fetch the parameters from non-volatile
		/// parameters storage.
		Mod::Par::Result fetch();
		bool isValid() const;
		bool trySetPassword(const std::string &password);
		bool trySetSsid(const std::string &ssid);
		std::array<std::uint8_t, 4> ipAsBytes() const;
		std::array<std::uint8_t, 4> netmaskAsBytes() const;
		std::array<std::uint8_t, 4> gatewayAsBytes() const;
		inline void resetUseDhcp();
		inline bool useDhcp() const
		{
			return ip == 0;
		}
	};
public:
	Sta(esp_netif_t **);
	void getFieldValue(Mod::Fld::Req aReq, Mod::Fld::OnResponseCallback aOnResponse);
	void setFieldValue(Mod::Fld::WriteReq, Mod::Fld::OnWriteResponseCallback onResponse);
	/// \brief Attempt to load STA parameters from a non-volatile parameters
	/// storage, and connect to the AP those specify. Will only work if
	/// autoconnect parameter is set to true.
	bool tryFetchConnect();
	bool isConnected() const;
private:
	esp_netif_t **espNetif;
	Credentials credentials;
};

}  // namespace Wifi
