/* MZC3_GC -- MZC3 C/C++ garbage collector library
   by Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
   This file is public domain software. */
#ifndef __MZC3_GC_H__
#define __MZC3_GC_H__

#if !defined(NDEBUG) && !defined(_DEBUG)
    #define _DEBUG
#endif

#ifdef __cplusplus
    #include <new>  // std::bad_alloc
#endif

//////////////////////////////////////////////////////////////////////////////
// NOTE: Use new_nothrow rather than ::new(std::nothrow) for this library.

#ifdef MZC_NO_GC
    // no effect
    #define MzcGC_Enter(enable_gc)
    #define MzcGC_Leave()
    #define MzcGC_GarbageCollect()
    #define MzcGC_Report()
    #define new_nothrow new(std::nothrow())
#else   // ndef MZC_NO_GC
    #ifdef __cplusplus
    extern "C" {
    #endif

    // Enter the GC section.
    void MzcGC_Enter(int enable_gc);
    // Leave the GC section.
    void MzcGC_Leave(void);
    // Do garbage collection in the current GC section.
    void MzcGC_GarbageCollect(void);

    #ifdef _DEBUG
        // Report the leaks in the current GC section.
        void MzcGC_Report(void);
    #else
        // no effect
        #define MzcGC_Report()
    #endif

    // mzcmalloc, mzccalloc, mzcrealloc, mzcfree
    #ifdef __cplusplus
        #ifdef _DEBUG
            void *mzcmalloc(std::size_t size, const char *file, int line);
            void *mzccalloc(std::size_t num, std::size_t size, const char *file, int line);
            void *mzcrealloc(void *ptr, std::size_t size, const char *file, int line);
            void mzcfree(void *ptr);
        #else // ndef _DEBUG
            void *mzcmalloc(std::size_t size);
            void *mzccalloc(std::size_t num, std::size_t size);
            void *mzcrealloc(void *ptr, std::size_t size);
            void mzcfree(void *ptr);
        #endif // ndef _DEBUG
    #else  // def __cplusplus
        #ifdef _DEBUG
            void *mzcmalloc(size_t size, const char *file, int line);
            void *mzccalloc(size_t num, size_t size, const char *file, int line);
            void *mzcrealloc(void *ptr, size_t size, const char *file, int line);
            void mzcfree(void *ptr, const char *file, int line);
        #else // ndef _DEBUG
            void *mzcmalloc(size_t size);
            void *mzccalloc(size_t num, size_t size);
            void *mzcrealloc(void *ptr, size_t size);
            void mzcfree(void *ptr);
        #endif // ndef _DEBUG
    #endif // def __cplusplus

    #ifdef __cplusplus
    } // extern "C"
    #endif

    // new and delete
    #ifdef _DEBUG
        void* operator new(std::size_t size) throw(std::bad_alloc);
        void* operator new(std::size_t size, const char *file, int line)
            throw(std::bad_alloc);
        void* operator new[](std::size_t size, const char *file, int line)
            throw(std::bad_alloc);

        void operator delete(void* ptr);
        void operator delete[](void* ptr);

        void* operator new(std::size_t size, const std::nothrow_t&,
                                     const char *file, int line) throw();
        void* operator new[](std::size_t size, const std::nothrow_t&,
                                     const char *file, int line) throw();
    #else
        void* operator new(std::size_t size) throw(std::bad_alloc);
        void* operator new[](std::size_t size) throw(std::bad_alloc);

        void operator delete(void* ptr) throw();
        void operator delete[](void* ptr) throw();

        void* operator new(std::size_t size, const std::nothrow_t&) throw();
        void* operator new[](std::size_t size, const std::nothrow_t&) throw();
    #endif

    // inline functions
    #ifndef MZC_NO_INLINING
        #undef MZC_INLINE
        #define MZC_INLINE inline
        #ifdef MZC3_INSTALLED
            #include <mzc3/GC_inl.h>
        #else
            #include "GC_inl.h"
        #endif
    #endif

    // wrapping
    #ifdef _DEBUG
        #define malloc(size) mzcmalloc((size), __FILE__, __LINE__)
        #define calloc(num,size) mzccalloc((num), (size), __FILE__, __LINE__)
        #define realloc(ptr,size) mzcrealloc((ptr), (size), __FILE__, __LINE__)
        #define free(ptr) mzcfree((ptr))
        #define new new(__FILE__, __LINE__)
        #define new_nothrow new(std::nothrow(), __FILE__, __LINE__)
    #else
        #define malloc(size) mzcmalloc((size))
        #define calloc(num,size) mzccalloc((num), (size))
        #define realloc(ptr,size) mzcrealloc(ptr), (size))
        #define free(ptr) mzcfree((ptr))
        #define new new
        #define new_nothrow new(std::nothrow())
    #endif
#endif  // ndef MZC_NO_GC

//////////////////////////////////////////////////////////////////////////////

#endif  /* ndef __MZC3_GC_H__ */
