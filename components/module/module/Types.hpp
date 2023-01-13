//
// Types.hpp
//
// Created on: Jun 17, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#if !defined(UTILITY_UTILITY_MOD_TYPES_HPP_)
#define UTILITY_UTILITY_MOD_TYPES_HPP_

#include "utility/al/Algorithm.hpp"
#include "module/Variant.hpp"
#include <asio.hpp>
#include <cstdint>
#include <mapbox/variant.hpp>
#include <Rr/Trait/StoreType.hpp>
#include <utility>
#include <functional>
#include <tuple>

namespace Mod {

/// \brief Enumeration for modules, i.e. semantically coherent pieces of functionality
enum class Module : std::uint8_t {
	Camera,
	Autopilot,  ///< The main (autopilot) board
	WifiStaConnection,  ///< Connection info (the ESP32 is connected to some other Access Point, AP)
	/// \brief Tracking algorithm fields
	///
	/// \details
	/// Fields, semantics:
	/// - Initialized - set FALSE to disable tracking.
	/// - Roi - bounding rectangle. Set to activate or reset tracking algorithm.
	Tracking,
	/// \brief Wi-Fi access point
	WifiAp,
	All,  ///< The request is addressed to every module
};

namespace Fld {

/// \brief Enumerations for modules' fields.
///
/// \details Different modules may share similar fields. A field, and the type associated with it are uniquely
/// identified by (Module, Field) pair.
enum class Field : std::uint8_t {
	FrameSize,
	FrameFormat,  ///< Frame format currently used by a camera (e.g. JPEG)
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
	Gateway,  ///< Gateway of a network.
	Netmask,  ///< Netmask of a network
	Roi,  ///< Region of Interest
	StringIdentifier,  ///< Generic string name
	Password,  ///< Password. Wi-Fi STA, or AP
	RestoreState,  ///< If set, the module will attempt to restore its state (which resides in some non-volatile storage)
	Mac,  ///< Mac address of Wi-Fi network
};

template <class T>
using StoreType = typename Rr::Trait::StoreType<T>;

/// \brief Compile-time selector of respective field types
template <Field, Module=Module::All> struct GetType : StoreType<None> {};  ///< Type selector
template <> struct GetType<Field::FrameSize, Module::Camera> : StoreType<std::pair<int, int>> {};
template <Module I> struct GetType<Field::Initialized, I> : StoreType<std::int32_t> {};
template <Module I> struct GetType<Field::VendorName, I> : StoreType<const char *> {};
template <Module I> struct GetType<Field::ModelName, I> : StoreType<const char *> {};
template <> struct GetType<Field::CaptureCount, Module::Camera> : StoreType<unsigned> {};
template <> struct GetType<Field::Recording, Module::Camera> : StoreType<bool> {};
template <Module I> struct GetType<Field::VersionSoftwareMajor, I> : StoreType<unsigned> {};
template <Module I> struct GetType<Field::VersionSoftwareMinor, I> : StoreType<unsigned> {};
template <Module I> struct GetType<Field::VersionSoftwarePatch, I> : StoreType<unsigned> {};
template <Module I> struct GetType<Field::VersionCommitHash, I> : StoreType<unsigned> {};
template <Module I> struct GetType<Field::Ip, I> : StoreType<std::uint32_t> {};
template <Module I> struct GetType<Field::Netmask, I> : StoreType<std::uint32_t> {};
template <Module I> struct GetType<Field::Gateway, I> : StoreType<std::uint32_t> {};
template <> struct GetType<Field::FrameFormat, Module::Camera> : StoreType<const char *> {};  ///< (identifier, human-readable name)
template <> struct GetType<Field::Roi, Module::Tracking> : StoreType<std::array<std::uint16_t, 4>> {};  ///< Rect: (x, y, width, height)
template <Module I> struct GetType<Field::StringIdentifier, I> : StoreType<std::string> {};
template <Module I> struct GetType<Field::Password, I> : StoreType<std::string> {};
template <Module I> struct GetType<Field::RestoreState, I> : StoreType<std::int32_t> {};
template <Module I> struct GetType<Field::Mac, I> : StoreType<std::array<std::uint8_t, 6> *> {};

struct Variant : public Mod::Variant {
	using Mod::Variant::Variant;

	template <Module M, Field F>
	const typename GetType<F, M>::Type &getUnchecked() const
	{
		return Mod::Variant::get_unchecked<typename GetType<F, M>::Type>();
	}

	/// \brief Infers the underlying type and constructs an instance
	template <Module M, Field F, class ...Ts>
	static inline Variant make(Ts &&...aArgs)
	{
		return Variant{typename GetType<F, M>::Type{std::forward<Ts>(aArgs)...}};
	}
};

/// \brief Encapsulates responses produced by a module.
///
struct Resp {
	Variant variant;  ///< The actual response
};

struct Req {
	Field field;  ///< Requested field
};

using OnResponseCallback = typename std::function<void(Resp)>;

struct WriteReq {
	Field field;  ///< Requested field
	Variant variant;
};

struct RequestResult {
	enum Result {
		Ok = 0,
		StorageError,
		OutOfRange,
		Other,

		N,
	};

	static const char *toCstr(Result);
};

struct WriteResp {
	RequestResult::Result result;
	const char *errorMessage;
	const char *resultAsCstr() const;

	inline bool isOk() const
	{
		return RequestResult::Ok == result;
	}
	constexpr WriteResp(RequestResult::Result aRes) : result{aRes}, errorMessage{""}
	{
	}
	constexpr WriteResp(RequestResult::Result aRes, const char *aErrorMessage) : result{aRes}, errorMessage{aErrorMessage}
	{
	}
};

/// \brief Stores the value of a field, and its unique composite identifier. \sa `Sub::Mod::OnModuleFieldSetUpdate`
struct ModuleField {
	// Unique identifier
	Module module;
	Field field;
	// Value storage
	Mod::Fld::Variant fieldVariant;

	template <Mod::Module M, Mod::Fld::Field F>
	const inline typename Mod::Fld::GetType<F, M>::Type &getUnchecked() const
	{
		return fieldVariant.template getUnchecked<M, F>();
	}

	/// \brief Infers the underlying type and constructs an instance
	template <Module M, Field F, class ...Ts>
	static inline Module make(Ts &&...aArgs)
	{
		return Module{M, F, typename GetType<F, M>::Type{std::forward<Ts>(aArgs)...}};
	}
};


using OnWriteResponseCallback = typename std::function<void(WriteResp)>;

}  // namespace Fld
}  // namespace Mod

#endif // UTILITY_UTILITY_MOD_TYPES_HPP_
