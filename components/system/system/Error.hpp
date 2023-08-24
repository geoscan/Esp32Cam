//
// Error.hpp
//
// Created on: Aug 18, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#ifndef COMPONENTS_SYSTEM_SYSTEM_ERROR_HPP
#define COMPONENTS_SYSTEM_SYSTEM_ERROR_HPP

namespace Sys {

enum class ErrorCode : unsigned {
	// Generic error codes
	None = 0,
	Fail = 1,
	Nullptr = 2,  // Unexpected nullptr, be that arguments, or return value

	// Flash memory error codes
	FlashMemory = 100,
};

struct Error {
	ErrorCode errorCode;
	const char *description;

	constexpr Error(ErrorCode aErrorCode = ErrorCode::None, const char *aDescription = ""):
		errorCode{aErrorCode},
		description{aDescription}
	{
	}

	constexpr Error(const char *aDescription):
		Error{ErrorCode::Fail, aDescription}
	{
	}
};

}  // Sys

#endif // COMPONENTS_SYSTEM_SYSTEM_ERROR_HPP
