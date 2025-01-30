#ifndef OS_H
#define OS_H

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  #define OS_WINDOWS 1
#elif __linux__
  #define OS_LINUX 1
#else
  #error "OS not supported"
#endif

#include "core/os_core.h"

#ifdef OS_INCLUDE_GFX
#include "gfx/os_gfx.h"
#endif

#endif // OS_H
