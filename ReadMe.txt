--------------------------------------------------------------------------------
                                  MZC3_GC
                         by Katayama Hirofumi MZ
                    2014.02.06  Public Domain Software
--------------------------------------------------------------------------------

This is MZC3 C/C++ garbage collector (MZC3_GC) by Katayama Hirofumi MZ.
This software is public domain software.


**USAGE**

At first, do #include "GC.h" in your program.  The malloc, calloc, realloc,
free, strdup and wcsdup functions will be wrapped by function macros of 
"GC.h".  new will also be wrapped by the mzcnew macro.

The `GC section' is code between MzcGC_Enter(enable_gc); and MzcGC_Leave();.
If paramter enable_gc is non-zero, then the section will become GC-enabled.
Otherwise the section will be GC-disabled.

Leaving a GC-enabled section by MzcGC_Leave call causes garbage collection 
immediately.

Please enclose the code partition by MzcGC_Enter(1); and MzcGC_Leave(); to 
enable GC in the section.

Please enclose the code partition by MzcGC_Enter(0); and MzcGC_Leave(); to 
disable GC in the section.

You can nest the balanced pairs of MzcGC_Enter(enable_gc); and MzcGC_Leave();.

MzcGC_GarbageCollect() immediately causes garbage collection in the current 
GC section.

MzcGC_Report() reports memory leaks in the current GC section if debugging.
You can use it for check of memory leaks.


**WARNING**

 * Enabling GC makes your program slower.
 * Don't use non-POD for new and/or mzcnew in a GC-enabled section.
   Otherwise target may be freed incorrectly.


**SWITCHING MACROS**

 * #define MZC_NO_GC to disable GC at all,
 * #define NDEBUG for non-debugging,
 * #define MZC3_GC_MT for multithread,
 * #define MZC_DEBUG_OUTPUT_IS_STDERR to output report to stderr,
 * #define MZC_DEBUG_OUTPUT_IS_STDOUT to output report to stdout,
 * #define _WIN32 for Windows.

---
Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
