#ifndef COMPONENTS_VERSION_PRIVATE_INCLUDE_VERSIONPARSER_HPP
#define COMPONENTS_VERSION_PRIVATE_INCLUDE_VERSIONPARSER_HPP

#include <array>
#include <vector>

// Parses STM32's version acquired via UART
// Seeks for packages of format <prefix><payloadsize: 1 byte><payload: payloadsize bytes><postfix>
class VersionParser {
public:
	VersionParser();
	bool parsed() const;
	void parse(const void *, const size_t);
	const std::vector<uint8_t> &payload() const;
private:
	static const std::array<uint8_t, 3> prefix;
	static const std::array<uint8_t, 3> postfix;

	void reset();
	void iter(const uint8_t byte);

	struct State {
		enum {
			Start,
			Prefix,
			Size,
			Payload,
			Postfix,
			End,
		} id;
		size_t info;  // Current position or number of bytes left
	} state;

	std::vector<uint8_t> pl;
};

#endif  // COMPONENTS_VERSION_PRIVATE_INCLUDE_VERSIONPARSER_HPP
