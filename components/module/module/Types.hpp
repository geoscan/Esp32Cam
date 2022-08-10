//
// Types.hpp
//
// Created on: Jun 17, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_MOD_TYPES_HPP_)
#define UTILITY_UTILITY_MOD_TYPES_HPP_

#include "utility/Algorithm.hpp"
#include <asio.hpp>
#include <cstdint>
#include <mapbox/variant.hpp>
#include <Rr/Trait/StoreType.hpp>
#include <utility>
#include <functional>

namespace Mod {

enum class Module : std::uint8_t {
	Camera,
	Autopilot,  ///< The main (autopilot) board
	WifiStaConnection,  ///< Connection info (the ESP32 is connected to some other Access Point, AP)
	All,  ///< The request is addressed to every module
};

struct None {};

namespace Fld {

enum class Field : std::uint8_t {
	FrameSize,
	VendorName,
	ModelName,
	Initialized,
	CaptureCount,  ///< Number of frames captured by a camera
	Recording,  ///< Whether a recording process is ongoing
	VersionSoftwareMajor,  ///< Software version, major revision
	VersionSoftwareMinor,  ///< Software version, minor revision
	VersionSoftwarePatch,  ///< Software version, patch revision, or the number of commits from the latest revision,
	VersionCommitHash,  ///< Git commit hash
	Ip,  ///< Depends on context. Module=WifiStaConnection - host ip
};

template <class T>
using StoreType = typename Rr::Trait::StoreType<T>;

/// \brief Compile-time selector of respective field types
///
template <Field, Module=Module::All> struct GetType : StoreType<None> {};  ///< Type selector
template <> struct GetType<Field::FrameSize, Module::Camera> : StoreType<std::pair<int, int>> {};
template <Module I> struct GetType<Field::Initialized, I> : StoreType<bool> {};
template <Module I> struct GetType<Field::VendorName, I> : StoreType<const char *> {};
template <Module I> struct GetType<Field::ModelName, I> : StoreType<const char *> {};
template <> struct GetType<Field::CaptureCount, Module::Camera> : StoreType<unsigned> {};
template <> struct GetType<Field::Recording, Module::Camera> : StoreType<bool> {};
template <Module I> struct GetType<Field::VersionSoftwareMajor, I> : StoreType<unsigned> {};
template <Module I> struct GetType<Field::VersionSoftwareMinor, I> : StoreType<unsigned> {};
template <Module I> struct GetType<Field::VersionSoftwarePatch, I> : StoreType<unsigned> {};
template <Module I> struct GetType<Field::VersionCommitHash, I> : StoreType<unsigned> {};
template <Module I> struct GetType<Field::Ip, I> : StoreType<mapbox::util::variant<asio::ip::address_v4>> {};

/// \brief Encapsulates responses produced by a module.
///
struct Resp {
	/// \brief Response variant type.
	///
	using Variant = mapbox::util::variant<
		None,
		unsigned,
		std::pair<int, int>,
		bool,
		const char *,
		mapbox::util::variant<asio::ip::address_v4>
	>;

	Variant variant;  ///< The actual response
};

struct Req {
	Field field;  ///< Requested field
};

using OnResponseCallback = typename std::function<void(Resp)>;

struct WriteReq {
	Field field;  ///< Requested field
};

struct WriteResp {
};

using OnWriteResponseCallback = typename std::function<void(WriteResp)>;

}  // namespace Fld
}  // namespace Mod

#endif // UTILITY_UTILITY_MOD_TYPES_HPP_
