/* MZC3_GC -- MZC3 C/C++ garbage collector
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

#ifdef MZC_NO_GC
    // no effect if defined(MZC_NO_GC)
    #define MzcGC_Enter(enable_gc)
    #define MzcGC_Leave()
    #define MzcGC_GarbageCollect()
    #define MzcGC_Report()
    #define mzcnew new
    #define mzcnew_nothrow new(std::nothrow)
    #define mzcdelete delete
    #undef new
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
        // No effect on release
        #define MzcGC_Report()  /*empty*/
    #endif

    // mzcmalloc, mzccalloc, mzcrealloc, mzcfree
    #ifdef _DEBUG
        #ifdef __cplusplus
            void *mzcmalloc(std::size_t size, const char *file, int line);
            void *mzccalloc(std::size_t num, std::size_t size, const char *file, int line);
            void *mzcrealloc(void *ptr, std::size_t size, const char *file, int line);
            void mzcfree(void *ptr);
            char *mzcstrdup(const char *str, const char *file, int line);
            wchar_t *mzcwcsdup(const wchar_t *str, const char *file, int line);
        #else  // def __cplusplus
            void *mzcmalloc(size_t size, const char *file, int line);
            void *mzccalloc(size_t num, size_t size, const char *file, int line);
            void *mzcrealloc(void *ptr, size_t size, const char *file, int line);
            void mzcfree(void *ptr, const char *file, int line);
            char *mzcstrdup(const char *str, const char *file, int line);
            wchar_t *mzcwcsdup(const wchar_t *str, const char *file, int line);
        #endif // def __cplusplus
    #else
        #ifdef __cplusplus
            void *mzcmalloc(std::size_t size);
            void *mzccalloc(std::size_t num, std::size_t size);
            void *mzcrealloc(void *ptr, std::size_t size);
            void mzcfree(void *ptr);
            char *mzcstrdup(const char *str);
            wchar_t *mzcwcsdup(const wchar_t *str);
        #else  // def __cplusplus
            void *mzcmalloc(size_t size);
            void *mzccalloc(size_t num, size_t size);
            void *mzcrealloc(void *ptr, size_t size);
            void mzcfree(void *ptr);
            char *mzcstrdup(const char *str);
            wchar_t *mzcwcsdup(const wchar_t *str);
        #endif // def __cplusplus
    #endif

    #ifdef __cplusplus
    } // extern "C"
    #endif

    #ifdef __cplusplus
        // new and delete
        void* operator new(std::size_t size) throw(std::bad_alloc);
        void* operator new[](std::size_t size) throw(std::bad_alloc);

        #ifndef __BORLANDC__    // avoid E2171
            void* operator new(std::size_t size, const std::nothrow_t&) throw();
            void* operator new[](std::size_t size, const std::nothrow_t&) throw();
        #endif

        void operator delete(void* ptr) throw();
        void operator delete[](void* ptr) throw();

        #ifdef _DEBUG
            void* operator new(std::size_t size, const char *file, int line)
                throw(std::bad_alloc);
            void* operator new[](std::size_t size, const char *file, int line)
                throw(std::bad_alloc);
            void* operator new(std::size_t size, const std::nothrow_t&,
                                         const char *file, int line) throw();
            void* operator new[](std::size_t size, const std::nothrow_t&,
                                         const char *file, int line) throw();
        #endif
    #endif  // __cplusplus

    // wrapping
    #include "GC_wrap.h"
#endif  // ndef MZC_NO_GC

//////////////////////////////////////////////////////////////////////////////

#endif  /* ndef __MZC3_GC_H__ */
