This is MZC3 C/C++ garbage collector (MZC3_GC) by Katayama Hirofumi MZ.
This software is public domain software.


**USAGE**

The malloc, calloc, realloc and free functions will be wrapped by 
#including "GC.h".

The new and delete operators will be also wrapped if you are using C++.

A ``GC section'' is the code between MzcGC_Enter(enable_gc); and MzcGC_Leave();.
If paramter enable_gc is non-zero, then the GC section will become GC-enabled.
Otherwise the section will be GC-disabled.
Leaving a GC-enabled GC section causes garbage collection immediately.

Please enclose the code partition by MzcGC_Enter(1); and MzcGC_Leave(); to 
enable GC in it.

Please enclose the code partition by MzcGC_Enter(0); and MzcGC_Leave(); to 
disable GC in it.

You can nest the pairs of MzcGC_Enter(enable_gc); and MzcGC_Leave();.

MzcGC_GarbageCollect() immediately causes garbage collection in the current 
GC section.

MzcGC_Report() reports memory leaks in the current GC section if debugging.

You can disable MZC3_GC by #defining MZC_NO_GC before #including "GC.h".


**WARNING**

 * Enabling GC makes your program slower.
 * Don't use non-POD for operator new in a GC-enabled section.
   Otherwise target may be freed incorrectly.
 * Use new_nothrow rather than ::new(std::nothrow) to enable GC.


**SWITCHING MACROS**

 * #define MZC_NO_GC to disable GC,
 * #define NDEBUG for non-debugging,
 * #define MZC_GC_MT for multithread,
 * #define MZC_DEBUG_OUTPUT_IS_STDERR to output report to stderr (Windows only),
 * #define _WIN32 for Windows.


---
Katayama Hirofumi MZ
katayama.hirofumi.mz@gmail.com
