#ifdef _WIN32
    #ifndef _INC_WINDOWS
        #include <windows.h>
    #endif
#else
    #include <sys/types.h>  // gettid
    #include <pthread.h>
#endif

#include <map>      // std::map

#include <cstdlib>  // malloc, calloc, realloc, free
#include <cstdio>   // std::fprintf, std::vfprintf
#include <cstring>  // std::strcpy, std::memcpy
#include <cwchar>   // std::wcscpy
#include <cassert>  // assert

// No GC
//#define MZC_NO_GC

// Multithread
//#define MZC3_GC_MT

// Debugging output to stderr
//#define MZC_DEBUG_OUTPUT_IS_STDERR

// Debugging output to stdout
//#define MZC_DEBUG_OUTPUT_IS_STDOUT

#include "GC.h"
