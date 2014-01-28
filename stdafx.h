/* MZC3_GC -- MZC3 C/C++ garbage collector library
   by Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
   This file is public domain software. */

#ifdef _WIN32
    #ifndef _INC_WINDOWS
        #include <windows.h>
    #endif
#else
    #include <sys/types.h>  // gettid
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
