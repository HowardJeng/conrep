// wrap the inclusion of boost/program_options.hpp and silence warnings from its inclusion
#ifndef CONREP_PROGRAM_OPTIONS_H
#define CONREP_PROGRAM_OPTIONS_H

#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4512)
#include <boost/program_options.hpp>
#pragma warning(pop)

#endif