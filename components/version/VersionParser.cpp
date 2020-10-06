#include "VersionParser.hpp"

using namespace std;

const array<uint8_t, 3> VersionParser::prefix  = {'f', 'w', 'v'};
const array<uint8_t, 3> VersionParser::postfix = {'F', 'W', 'V'};

VersionParser::VersionParser()
{
	reset();
}

void VersionParser::reset()
{
	state.id = State::Start;
	state.info = 0u;
	pl.clear();
}

bool VersionParser::parsed() const
{
	return (state.id == State::End);
}

void VersionParser::parse(const void *data, const size_t size)
{
	size_t pos = 0;
	while (!parsed() && pos < size) {
		iter(static_cast<const uint8_t *>(data)[pos++]);
	}
}

const vector<uint8_t> &VersionParser::payload() const
{
	return pl;
}

void VersionParser::iter(const uint8_t byte)
{
	switch (state.id) {

		case State::Start:
			if (byte == prefix[0]) {
				state.id = State::Prefix;
				state.info = 1;  // position
			}
			break;

		case State::Prefix:
			if (state.info < prefix.size()) {
				if (prefix[state.info] == byte) {
					++state.info;
				} else {
					reset();
					iter(byte);
				}
				break;
			} else {
				state.id = State::Size;
				iter(byte);
			}
			break;

		case State::Size:
			state.info = byte;  // n bytes left
			state.id = State::Payload;
			break;

		case State::Payload:
			if (state.info > 0) {
				--state.info;
				pl.push_back(byte);
				break;
			} else if (byte == postfix[0]) {
				state.id = State::Postfix;
				state.info = 1;  // position
			} else {
				reset();
				iter(byte);
			}
			break;

		case State::Postfix:
			if (state.info < postfix.size()) {
				if (byte == postfix[state.info]) {
					++state.info;
				} else {
					reset();
					iter(byte);
				}
			} else {
				state.id = State::End;
			}
			break;

		case State::End:
			break;
	}
}