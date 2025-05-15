#ifndef OS_CORE_WIN32_H
#define OS_CORE_WIN32_H

#define NOMINMAX

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#undef OS_WINDOWS
#include <Shlwapi.h>
#include <timeapi.h>

#undef OS_WINDOWS
#define OS_WINDOWS 1

#endif // OS_CORE_WIN32_H
