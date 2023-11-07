#define OHDEBUG_PORT_ENABLE 1
#define OHDEBUG_TAGS_ENABLE "Trace"

#include <OhDebug.hpp>

OHDEBUG_TEST("Test name")
{
	OHDEBUG("Trace", "Traced");
}

int main(void)
{
	OHDEBUG("Trace", "EXECUTABLE_NAME_STUB");
	OHDEBUG_RUN_TESTS();

	return 0;
}
