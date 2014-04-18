////////////////////////////////////////////////////////////////////////////
// GC_wrap.h -- MZC3 GC wrapper
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#undef malloc
#undef calloc
#undef realloc
#undef free
#undef strdup
#undef wcsdup
#undef new
#undef mzcnew
#undef mzcnew_nothrow
#undef mzcdelete

#ifdef _DEBUG
    #define malloc(size) mzcmalloc((size), __FILE__, __LINE__)
    #define calloc(num,size) mzccalloc((num), (size), __FILE__, __LINE__)
    #define realloc(ptr,size) mzcrealloc((ptr), (size), __FILE__, __LINE__)
    #define free(ptr) mzcfree((ptr))
    #define strdup(p) mzcstrdup((p), __FILE__, __LINE__)
    #define wcsdup(p) mzcwcsdup((p), __FILE__, __LINE__)
    #define mzcnew new(__FILE__, __LINE__)
    #define mzcnew_nothrow new(std::nothrow, __FILE__, __LINE__)
#else
    #define malloc(size) mzcmalloc((size))
    #define calloc(num,size) mzccalloc((num), (size))
    #define realloc(ptr,size) mzcrealloc((ptr), (size))
    #define free(ptr) mzcfree((ptr))
    #define strdup(p) mzcstrdup((p))
    #define wcsdup(p) mzcwcsdup((p))
    #define mzcnew new
    #define mzcnew_nothrow new(std::nothrow)
#endif
#ifndef MZC3_USE_NOTHROW_NEW
    #define new mzcnew
#endif
#define mzcdelete delete
