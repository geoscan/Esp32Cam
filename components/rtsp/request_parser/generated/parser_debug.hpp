#ifndef COMPONENTS_RTSP_REQUEST_PARSER_GENERATED_PARSER_DEBUG_HPP
#define COMPONENTS_RTSP_REQUEST_PARSER_GENERATED_PARSER_DEBUG_HPP

#define PARSER_DEBUG 1
#define DEBUG_FLEX 0

#if PARSER_DEBUG != 1
# define debug(...)
#else // PARSER_DEBUG != 1

# include <iostream>
template<typename Arg>
void debug(Arg arg)
{
	std::cout << "DEBUG:" << arg << std::endl;
}

#endif // PARSER_DEBUG


#endif // COMPONENTS_RTSP_REQUEST_PARSER_GENERATED_PARSER_DEBUG_HPP