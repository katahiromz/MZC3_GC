/* MZC3_GC -- MZC3 C/C++ garbage collector library
   by Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
   This file is public domain software. */
#ifndef _DEBUG
    MZC_INLINE void* operator new(std::size_t size) throw(std::bad_alloc)
    {
        void *ptr = malloc(size);
        if (ptr == NULL)
            throw std::bad_alloc();
        return ptr;
    }

    MZC_INLINE void* operator new(std::size_t size) throw(std::bad_alloc)
    {
        void *ptr = mzcmalloc(size);
        if (ptr == NULL)
            throw std::bad_alloc();
        return ptr;
    }

    MZC_INLINE void* operator new[](std::size_t size) throw(std::bad_alloc)
    {
        void *ptr = mzcmalloc(size);
        if (ptr == NULL)
            throw std::bad_alloc();
        return ptr;
    }

    MZC_INLINE void operator delete(void* ptr)
    {
        mzcfree(ptr);
    }

    MZC_INLINE void operator delete[](void* ptr)
    {
        mzcfree(ptr);
    }

    MZC_INLINE void* operator new(const nothrow_t&, std::size_t size) throw()
    {
        return mzcmalloc(size);
    }

    MZC_INLINE void* operator new[](const nothrow_t&, std::size_t size) throw()
    {
        return mzcmalloc(size);
    }
#endif
