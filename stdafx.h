/* MZC3_GC -- MZC3 C/C++ garbage collector
   by Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
   This file is public domain software. */

#ifdef _WIN32
    #ifndef _INC_WINDOWS
        #include <windows.h>
    #endif
#else
    #include <sys/types.h>  // gettid
    #include <pthread.h>
#endif

#include <map>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include "GC.h"

// No GC
//#define MZC_NO_GC

// Multithread
//#define MZC_GC_MT

// Debugging output to stderr (for Windows only)
//#define MZC_DEBUG_OUTPUT_IS_STDERR
