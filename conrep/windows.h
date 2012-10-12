// wrap the inclusion of windows.h to provide appropriate preprocessor definitions
#ifndef CONREP_WINDOWS_H
#define CONREP_WINDOWS_H

#pragma warning (push)
#pragma warning (disable: 4820)
#pragma warning (disable: 4668)
#pragma warning (disable: 4514)
#define NOMINMAX
#include <Windows.h>
#pragma warning (pop)

#endif