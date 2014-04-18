////////////////////////////////////////////////////////////////////////////
// GC.cpp -- MZC3 GC
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifndef MZC_NO_GC

#include "GC_unwrap.h"

//////////////////////////////////////////////////////////////////////////////
// MzcTraceA --- Output a message for debugging

#ifndef MzcTraceA
    #ifdef _DEBUG
        void MzcTraceA(const char *fmt, ...)
        {
            using namespace std;
            assert(fmt);
            #ifdef MZC_DEBUG_OUTPUT_IS_STDERR
                va_list va;
                va_start(va, fmt);
                vfprintf(stderr, fmt, va);
                va_end(va);
            #elif defined(MZC_DEBUG_OUTPUT_IS_STDOUT)
                va_list va;
                va_start(va, fmt);
                vfprintf(stdout, fmt, va);
                va_end(va);
            #elif defined(_WIN32)
                static char buf[512];
                va_list va;
                va_start(va, fmt);
                wvsprintfA(buf, fmt, va);
                OutputDebugStringA(buf);
                va_end(va);
            #else
                va_list va;
                va_start(va, fmt);
                vfprintf(stderr, fmt, va);
                va_end(va);
            #endif
        }
        #define MzcTraceA MzcTraceA
    #else
        inline void MzcTraceA(const char *, ...)
        {
        }
        #define MzcTraceA 1 ? (void)0 : (void)MzcTraceA
    #endif
#endif  // ndef MzcTraceA

//////////////////////////////////////////////////////////////////////////////
// MZC3_GC_ENTRY --- GC entry

struct MZC3_GC_ENTRY
{
    void *      m_ptr;
    std::size_t m_size;
    std::size_t m_depth;

    MZC3_GC_ENTRY()
    {
    }

    #ifdef _DEBUG
        const char *m_file;
        int         m_line;
        MZC3_GC_ENTRY(void *ptr, std::size_t size, std::size_t depth,
                      const char *file, int line)
        : m_ptr(ptr), m_size(size), m_depth(depth),
          m_file(file), m_line(line)
        {
            assert(ptr);
            assert(file);
        }
    #else   // ndef DEBUG
        MZC3_GC_ENTRY(void *ptr, std::size_t size, std::size_t depth)
        : m_ptr(ptr), m_size(size), m_depth(depth)
        {
        }
    #endif  // ndef DEBUG
};

//////////////////////////////////////////////////////////////////////////////
// MZC3_GC_STATE

struct MZC3_GC_STATE
{
    MZC3_GC_STATE *next;
    int gc_enabled;
};

//////////////////////////////////////////////////////////////////////////////
// TODO: Use Win32 TLS

struct MZC3_GC_THREAD_ENTRY
{
    #ifdef MZC3_GC_MT
        #ifdef _WIN32
            DWORD tid;
        #else
            pid_t tid;
        #endif
    #endif
    std::size_t    depth;
    MZC3_GC_STATE *state_stack;
};

#ifdef MZC3_GC_MT
    static MZC3_GC_THREAD_ENTRY *s_gc_thread_entries = NULL;
    static std::size_t s_gc_thread_entry_count = 0;
    static std::size_t s_gc_thread_entry_capacity = 0;
#else
    static MZC3_GC_THREAD_ENTRY s_only_one_gc_thread_entry = {0, NULL};
#endif

//////////////////////////////////////////////////////////////////////////////

#ifndef MZC3_GC_MT
    inline
#endif
MZC3_GC_THREAD_ENTRY *MZC3_GC_GetThreadEntry(void)
{
    #ifdef MZC3_GC_MT
        #ifdef _WIN32
            const DWORD tid = GetCurrentThreadId();
        #else
            const pid_t tid = gettid();
        #endif

        for (std::size_t i = 0; i < s_gc_thread_entry_count; i++)
        {
            if (s_gc_thread_entries[i].tid == tid)
                return &s_gc_thread_entries[i];
        }

        if (s_gc_thread_entry_count + 1 < s_gc_thread_entry_capacity)
        {
            s_gc_thread_entries[s_gc_thread_entry_count].tid = tid;
            s_gc_thread_entries[s_gc_thread_entry_count].depth = 0;
            s_gc_thread_entries[s_gc_thread_entry_count].state_stack = NULL;
            return &s_gc_thread_entries[s_gc_thread_entry_count++];
        }

        std::size_t newcapacity;
        if (s_gc_thread_entry_capacity == 0)
            newcapacity = 10;
        else
            newcapacity = s_gc_thread_entry_capacity * 2;
        const std::size_t newsize = newcapacity * sizeof(MZC3_GC_THREAD_ENTRY);

        MZC3_GC_THREAD_ENTRY *newentries;
        newentries = reinterpret_cast<MZC3_GC_THREAD_ENTRY *>(
            realloc(s_gc_thread_entries, newsize));
        if (newentries)
        {
            s_gc_thread_entries = newentries;
            s_gc_thread_entries[s_gc_thread_entry_count].tid = tid;
            s_gc_thread_entries[s_gc_thread_entry_count].depth = 0;
            s_gc_thread_entries[s_gc_thread_entry_count].state_stack = NULL;
            return &s_gc_thread_entries[s_gc_thread_entry_count++];
        }

        MzcTraceA("MZC3_GC_GetThreadEntry: failed\n");
        return NULL;
    #else
        return &s_only_one_gc_thread_entry;
    #endif
}

inline std::size_t& MZC3_GC_GetDepth(void)
{
    return MZC3_GC_GetThreadEntry()->depth;
}

inline MZC3_GC_STATE*& MZC3_GC_GetStateStack(void)
{
    return MZC3_GC_GetThreadEntry()->state_stack;
}

inline bool MZC3_GC_IsEnabled(void)
{
    MZC3_GC_THREAD_ENTRY *entry = MZC3_GC_GetThreadEntry();
    return (entry && entry->state_stack && entry->state_stack->gc_enabled);
}

//////////////////////////////////////////////////////////////////////////////
// synchronization

#ifdef MZC3_GC_MT
    #ifdef _WIN32
        static CRITICAL_SECTION s_gc_cs;
    #else
        static pthread_mutex_t s_gc_cs;
    #endif
#endif

inline void InitializeLock(void)
{
    #ifdef MZC3_GC_MT
        #ifdef _WIN32
            InitializeCriticalSection(&s_gc_cs);
        #else
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
            pthread_mutex_init(&s_gc_cs, &attr);
            pthread_mutexattr_destroy(&attr);
        #endif
    #endif
}

inline void EnterLock(void)
{
    #ifdef MZC3_GC_MT
        #ifdef _WIN32
            EnterCriticalSection(&s_gc_cs);
        #else
            pthread_mutex_lock(&s_gc_cs);
        #endif
    #endif
}

inline void LeaveLock(void)
{
    #ifdef MZC3_GC_MT
        #ifdef _WIN32
            LeaveCriticalSection(&s_gc_cs);
        #else
            pthread_mutex_unlock(&s_gc_cs);
        #endif
    #endif
}

inline void DeleteLock(void)
{
    #ifdef MZC3_GC_MT
        #ifdef _WIN32
            DeleteCriticalSection(&s_gc_cs);
        #else
            pthread_mutex_destroy(&s_gc_cs);
        #endif
    #endif
}

//////////////////////////////////////////////////////////////////////////////
// MZC3_GC

static MZC3_GC_ENTRY *s_gc_entries = NULL;
static std::size_t    s_gc_count = 0;
static std::size_t    s_gc_capacity = 0;
static bool           s_gc_constructed = false;

class MZC3_GC_MGR
{
public:
    MZC3_GC_MGR()
    {
        InitializeLock();
        s_gc_constructed = true;
    }

    ~MZC3_GC_MGR();
};

MZC3_GC_MGR::~MZC3_GC_MGR()
{
    assert(s_gc_entries == NULL || s_gc_capacity);

    EnterLock();
    s_gc_constructed = false;

    std::size_t gc_count = s_gc_count;
    s_gc_count = 0;
    s_gc_capacity = 0;

    MZC3_GC_ENTRY *gc_entries = s_gc_entries;
    s_gc_entries = NULL;
    for (std::size_t i = 0; i < gc_count; i++)
        free(gc_entries[i].m_ptr);
    free(gc_entries);

    #ifdef MZC3_GC_MT
        MZC3_GC_THREAD_ENTRY *gc_thread_entries = s_gc_thread_entries;
        s_gc_thread_entries = NULL;

        std::size_t gc_thread_entry_count = s_gc_thread_entry_count;
        s_gc_thread_entry_count = 0;
        s_gc_thread_entry_capacity = 0;

        for (std::size_t i = 0; i < gc_thread_entry_count; i++)
        {
            MZC3_GC_STATE *state = gc_thread_entries[i].state_stack;
            while (state)
            {
                MZC3_GC_STATE *next = state->next;
                free(state);
                state = next;
            }
        }
        free(gc_thread_entries);
    #else
        MZC3_GC_STATE *state = s_only_one_gc_thread_entry.state_stack;
        while (state)
        {
            MZC3_GC_STATE *next = state->next;
            free(state);
            state = next;
        }
    #endif
    LeaveLock();

    DeleteLock();
}

MZC3_GC_MGR mzc_gc_mgr;

static MZC3_GC_ENTRY *MZC3_GC_Find(void *ptr)
{
    assert(s_gc_entries == NULL || s_gc_capacity);
    if (ptr == NULL || !s_gc_constructed)
        return NULL;

    for (std::size_t i = 0; i < s_gc_count; i++)
    {
        if (s_gc_entries[i].m_ptr == ptr)
            return &s_gc_entries[i];
    }
    return NULL;
}

static void MZC3_GC_EraseEntry(MZC3_GC_ENTRY *entry)
{
    if (entry == NULL || !s_gc_constructed)
        return;

    assert(s_gc_entries == NULL || s_gc_capacity);
    MZC3_GC_ENTRY *end = s_gc_entries + --s_gc_count;
    for (MZC3_GC_ENTRY *e = entry; e != end; e++)
        *e = *(e + 1);
}

static void MZC3_GC_ErasePtr(void *ptr)
{
    MZC3_GC_ENTRY *entry = MZC3_GC_Find(ptr);
    if (entry == NULL)
        return;

    assert(s_gc_count);
    for (MZC3_GC_ENTRY *e = entry; e < s_gc_entries + s_gc_count; e++)
        *e = *(e + 1);

    s_gc_count--;
}

static void MZC3_GC_GarbageCollect(void)
{
    assert(s_gc_entries == NULL || s_gc_capacity);
    for (std::size_t i = s_gc_count - 1; i < s_gc_count; i--)
    {
        if (s_gc_entries[i].m_depth >= MZC3_GC_GetDepth())
        {
            void *ptr = s_gc_entries[i].m_ptr;
            MZC3_GC_EraseEntry(&s_gc_entries[i]);
            free(ptr);
        }
    }
}

#ifdef _DEBUG
    static void MZC3_GC_AddPtr(void *ptr, std::size_t size, const char *file, int line)
    {
        assert(ptr);
        assert(file);
        if (!s_gc_constructed)
            return;

        MZC3_GC_ENTRY entry(ptr, size, MZC3_GC_GetDepth(), file, line);
        if (s_gc_count + 1 > s_gc_capacity)
        {
            std::size_t newcapacity;
            if (!s_gc_capacity)
                newcapacity = 50;
            else
                newcapacity = s_gc_capacity * 2;

            const std::size_t newsize = newcapacity * sizeof(MZC3_GC_ENTRY);
            MZC3_GC_ENTRY *newentries =
                reinterpret_cast<MZC3_GC_ENTRY *>(realloc(s_gc_entries, newsize));
            if (newentries)
            {
                s_gc_entries = newentries;
                s_gc_capacity = newcapacity;
            }
            else
            {
                MzcTraceA("%s (%d): MZC3_GC: ERROR: MZC3_GC_AddPtr failed\n",
                          file, line);
                return;
            }
        }
        memcpy(&s_gc_entries[s_gc_count], &entry, sizeof(entry));
        s_gc_count++;
    }
#else   // ndef _DEBUG
    static void MZC3_GC_AddPtr(void *ptr, std::size_t size)
    {
        assert(ptr);
        if (!s_gc_constructed)
            return;

        MZC3_GC_ENTRY entry(ptr, size, MZC3_GC_GetDepth());
        if (s_gc_count + 1 > s_gc_capacity)
        {
            std::size_t newcapacity;
            if (!s_gc_capacity)
                newcapacity = 50;
            else
                newcapacity = s_gc_capacity * 2;

            const std::size_t newsize = newcapacity * sizeof(MZC3_GC_ENTRY);
            MZC3_GC_ENTRY *newentries =
                reinterpret_cast<MZC3_GC_ENTRY *>(realloc(s_gc_entries, newsize));
            if (newentries)
            {
                s_gc_entries = newentries;
                s_gc_capacity = newcapacity;
            }
            else
            {
                return;
            }
        }
        memcpy(&s_gc_entries[s_gc_count], &entry, sizeof(entry));
        s_gc_count++;
    }
#endif  // ndef _DEBUG

//////////////////////////////////////////////////////////////////////////////
// misc functions

extern "C" void MzcGC_Enter(int enable_gc)
{
    EnterLock();

    MZC3_GC_THREAD_ENTRY *entry = MZC3_GC_GetThreadEntry();
    if (entry)
    {
        MZC3_GC_STATE *state;
        state = reinterpret_cast<MZC3_GC_STATE *>(malloc(sizeof(MZC3_GC_STATE)));
        if (state)
        {
            state->gc_enabled = enable_gc;
            state->next = entry->state_stack;
            entry->state_stack = state;
            entry->depth++;
            assert(entry->depth > 0);
        }
        else
            MzcTraceA("ERROR: MzcGC_Enter: malloc failed\n");
    }
    else
        MzcTraceA("ERROR: MzcGC_Enter: MZC3_GC_GetThreadEntry failed\n");

    LeaveLock();
}

extern "C" void MzcGC_Leave(void)
{
    EnterLock();

    MZC3_GC_THREAD_ENTRY *entry = MZC3_GC_GetThreadEntry();
    if (entry)
    {
        assert(entry->depth > 0);
        MZC3_GC_STATE *state = entry->state_stack;
        if (state)
        {
            MZC3_GC_STATE *next = state->next;
            if (state->gc_enabled)
            {
                MZC3_GC_GarbageCollect();
            }
            free(state);
            entry->state_stack = next;
            entry->depth--;
        }
        else
            MzcTraceA("ERROR: MzcGC_Enter and MzcGC_Leave mismatched\n");
    }
    else
        MzcTraceA("ERROR: MzcGC_Leave: MZC3_GC_GetThreadEntry failed\n");

    LeaveLock();
}

#ifdef _DEBUG
    extern "C" void MzcGC_Report(void)
    {
        EnterLock();

        assert(s_gc_entries == NULL || s_gc_capacity);
        MZC3_GC_THREAD_ENTRY *entry = MZC3_GC_GetThreadEntry();
        if (entry)
        {
            for (std::size_t i = 0; i < s_gc_count; i++)
            {
                if (s_gc_entries[i].m_depth >= entry->depth)
                {
                    #ifdef _WIN64
                        MzcTraceA("%s (%d): MZC3_GC: leaked object 0x%p (size: %I64u)\n",
                            s_gc_entries[i].m_file, s_gc_entries[i].m_line,
                            s_gc_entries[i].m_ptr, s_gc_entries[i].m_size);
                    #else
                        MzcTraceA("%s (%d): MZC3_GC: leaked object 0x%p (size: %u)\n",
                            s_gc_entries[i].m_file, s_gc_entries[i].m_line,
                            s_gc_entries[i].m_ptr, s_gc_entries[i].m_size);
                    #endif
                }
            }
        }
        else
            MzcTraceA("ERROR: MzcGC_Report: MZC3_GC_GetThreadEntry failed\n");

        LeaveLock();
    }
#endif

extern "C" void MzcGC_GarbageCollect(void)
{
    EnterLock();

    MZC3_GC_GarbageCollect();

    LeaveLock();
}

//////////////////////////////////////////////////////////////////////////////
// mzcmalloc, mzccalloc, mzcrealloc, mzcfree, mzcstrdup, mzcwcsdup

#ifdef _DEBUG
    extern "C" void *mzcmalloc(std::size_t size, const char *file, int line)
    {
        using namespace std;
        void *ptr = malloc(size);
        if (ptr)
        {
            EnterLock();

            if (MZC3_GC_IsEnabled())
                MZC3_GC_AddPtr(ptr, size, file, line);

            LeaveLock();
        }
        else if (size > 0)
        {
            #ifdef _WIN64
                MzcTraceA("%s (%d): MZC3_GC ERROR: malloc(%I64u) failed\n",
                    file, line, size);
            #else
                MzcTraceA("%s (%d): MZC3_GC ERROR: malloc(%u) failed\n",
                    file, line, size);
            #endif
        }
        return ptr;
    }

    extern "C" void *mzccalloc(std::size_t num, std::size_t size, const char *file, int line)
    {
        using namespace std;
        void *ptr = calloc(num, size);
        if (ptr)
        {
            EnterLock();

            if (MZC3_GC_IsEnabled())
                MZC3_GC_AddPtr(ptr, num * size, file, line);

            LeaveLock();
        }
        else if (num && size)
        {
            #ifdef _WIN64
                MzcTraceA(
                    "%s (%d): MZC3_GC ERROR: calloc(%I64u, %I64u) failed\n",
                    file, line, num, size);
            #else
                MzcTraceA(
                    "%s (%d): MZC3_GC ERROR: calloc(%u, %u) failed\n",
                    file, line, num, size);
            #endif
        }
        return ptr;
    }

    extern "C" void *mzcrealloc(void *ptr, std::size_t size, const char *file, int line)
    {
        using namespace std;
        void *newptr;

        EnterLock();

        MZC3_GC_ENTRY *entry = MZC3_GC_Find(ptr);
        if (entry)
        {
            newptr = realloc(ptr, size);
            if (newptr)
            {
                entry->m_ptr = newptr;
                entry->m_size = size;
                entry->m_file = file;
                entry->m_line = line;
            }
            else if (size)
            {
                #ifdef _WIN64
                    MzcTraceA(
                        "%s (%d): MZC3_GC ERROR: realloc(%p, %I64u) failed\n",
                        file, line, ptr, size);
                #else
                    MzcTraceA(
                        "%s (%d): MZC3_GC ERROR: realloc(%p, %u) failed\n",
                        file, line, ptr, size);
                #endif
            }
        }
        else if (ptr == NULL)
        {
            newptr = realloc(ptr, size);
            if (newptr)
            {
                if (MZC3_GC_IsEnabled())
                    MZC3_GC_AddPtr(newptr, size, file, line);
            }
            else if (size)
            {
                #ifdef _WIN64
                    MzcTraceA(
                        "%s (%d): MZC3_GC ERROR: realloc(%p, %I64u) failed\n",
                        file, line, ptr, size);
                #else
                    MzcTraceA(
                        "%s (%d): MZC3_GC ERROR: realloc(%p, %u) failed\n",
                        file, line, ptr, size);
                #endif
            }
        }
        else
        {
            #ifdef _WIN64
                MzcTraceA(
                    "%s (%d): MZC3_GC ERROR: realloc got bad pointer 0x%p\n",
                    file, line, ptr, size);
            #else
                MzcTraceA(
                    "%s (%d): MZC3_GC ERROR: realloc got bad pointer 0x%p\n",
                    file, line, ptr, size);
            #endif
        }

        LeaveLock();

        return newptr;
    }

    extern "C" void mzcfree(void *ptr)
    {
        using namespace std;
        if (ptr == NULL)
            return;

        EnterLock();

        MZC3_GC_ErasePtr(ptr);

        LeaveLock();

        free(ptr);
    }

    extern "C" char *mzcstrdup(const char *str, const char *file, int line)
    {
        using namespace std;
        const std::size_t len = strlen(str);
        const std::size_t size = (len + 1) * sizeof(char);
        char *p = reinterpret_cast<char *>(mzcmalloc(size, file, line));
        if (p)
            memcpy(p, str, size);
        return p;
    }

    extern "C" wchar_t *mzcwcsdup(const wchar_t *str, const char *file, int line)
    {
        using namespace std;
        const std::size_t len = wcslen(str);
        const std::size_t size = (len + 1) * sizeof(wchar_t);
        wchar_t *p = reinterpret_cast<wchar_t *>(mzcmalloc(size, file, line));
        if (p)
            memcpy(p, str, size);
        return p;
    }
#else   // ndef _DEBUG
    extern "C" void *mzcmalloc(std::size_t size)
    {
        using namespace std;
        void *ptr = malloc(size);
        if (ptr)
        {
            EnterLock();

            if (MZC3_GC_IsEnabled())
                MZC3_GC_AddPtr(ptr, size);

            LeaveLock();
        }
        return ptr;
    }

    extern "C" void *mzccalloc(std::size_t num, std::size_t size)
    {
        using namespace std;
        void *ptr = calloc(num, size);
        if (ptr)
        {
            EnterLock();

            if (MZC3_GC_IsEnabled())
                MZC3_GC_AddPtr(ptr, num * size);

            LeaveLock();
        }
        return ptr;
    }

    extern "C" void *mzcrealloc(void *ptr, std::size_t size)
    {
        using namespace std;
        void *newptr;

        EnterLock();

        MZC3_GC_ENTRY *entry = MZC3_GC_Find(ptr);
        if (entry)
        {
            newptr = realloc(ptr, size);
            if (newptr)
            {
                entry->m_ptr = newptr;
                entry->m_size = size;
            }
        }
        else if (ptr == NULL)
        {
            newptr = realloc(ptr, size);
            if (newptr)
            {
                if (MZC3_GC_IsEnabled())
                    MZC3_GC_AddPtr(newptr, size);
            }
        }

        LeaveLock();

        return newptr;
    }

    extern "C" void mzcfree(void *ptr)
    {
        using namespace std;
        if (ptr == NULL)
            return;

        EnterLock();

        MZC3_GC_ErasePtr(ptr);

        LeaveLock();

        free(ptr);
    }

    extern "C" char *mzcstrdup(const char *str)
    {
        using namespace std;
        const std::size_t len = strlen(str);
        const std::size_t size = (len + 1) * sizeof(char);
        char *p = reinterpret_cast<char *>(mzcmalloc(size));
        if (p)
            memcpy(p, str, size);
        return p;
    }

    extern "C" wchar_t *mzcwcsdup(const wchar_t *str)
    {
        using namespace std;
        const std::size_t len = wcslen(str);
        const std::size_t size = (len + 1) * sizeof(wchar_t);
        wchar_t *p = reinterpret_cast<wchar_t *>(mzcmalloc(size));
        if (p)
            memcpy(p, str, size);
        return p;
    }
#endif  // ndef _DEBUG

//////////////////////////////////////////////////////////////////////////////
// new, delete

void* operator new(std::size_t size) throw(std::bad_alloc)
{
    #ifdef _DEBUG
        void *ptr = mzcmalloc(size ? size : 1, __FILE__, __LINE__);
    #else
        void *ptr = mzcmalloc(size ? size : 1);
    #endif
    if (ptr == NULL)
        throw std::bad_alloc();
    return ptr;
}

void* operator new[](std::size_t size) throw(std::bad_alloc)
{
    #ifdef _DEBUG
        void *ptr = mzcmalloc(size ? size : 1, __FILE__, __LINE__);
    #else
        void *ptr = mzcmalloc(size ? size : 1);
    #endif
    if (ptr == NULL)
        throw std::bad_alloc();
    return ptr;
}

#ifndef __BORLANDC__    // avoid E2171
    void* operator new(std::size_t size, const std::nothrow_t&) throw()
    {
        #ifdef _DEBUG
            void *ptr = mzcmalloc(size ? size : 1, __FILE__, __LINE__);
        #else
            void *ptr = mzcmalloc(size ? size : 1);
        #endif
        return ptr;
    }

    void* operator new[](std::size_t size, const std::nothrow_t&) throw()
    {
        #ifdef _DEBUG
            void *ptr = mzcmalloc(size ? size : 1, __FILE__, __LINE__);
        #else
            void *ptr = mzcmalloc(size ? size : 1);
        #endif
        return ptr;
    }
#endif

void operator delete(void* ptr) throw()
{
    mzcfree(ptr);
}

void operator delete[](void* ptr) throw()
{
    mzcfree(ptr);
}

#ifdef _DEBUG
    void* operator new(std::size_t size, const char *file, int line)
          throw(std::bad_alloc)
    {
        void *ptr = mzcmalloc((size ? size : 1), file, line);
        if (ptr == NULL)
            throw std::bad_alloc();
        return ptr;
    }

    void* operator new[](std::size_t size, const char *file, int line)
          throw(std::bad_alloc)
    {
        void *ptr = mzcmalloc((size ? size : 1), file, line);
        if (ptr == NULL)
            throw std::bad_alloc();
        return ptr;
    }

    void* operator new(std::size_t size, const std::nothrow_t&,
                       const char *file, int line) throw()
    {
        return mzcmalloc((size ? size : 1), file, line);
    }

    void* operator new[](std::size_t size, const std::nothrow_t&,
                         const char *file, int line) throw()
    {
        return mzcmalloc((size ? size : 1), file, line);
    }
#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef UNITTEST
    // unit test and example
    #include "GC_wrap.h"
    int main(void)
    {
        using namespace std;
        void *p1;
        void *p2;
        void *p3;
        char *p4;
        void *p5;
        char *p6;
        MzcGC_Enter(1); // GC-enabled section
        {
            p1 = malloc(1);
            printf("p1: %p\n", p1);
            MzcGC_Enter(1); // GC-enabled section
            {
                p2 = realloc(NULL, 2);
                printf("p2: %p\n", p2);
                p3 = new char[3];
                printf("p3: %p\n", p3);
                MzcGC_Enter(0); // GC-disabled section
                {
                    p4 = new char[4];
                    printf("p4: %p\n", p4);
                    MzcGC_Report();
                }
                MzcGC_Leave();
                MzcGC_Report();
            }
            MzcGC_Leave();
            p5 = new char[5];
            printf("p5: %p\n", p5);
            p6 = strdup("test6");
            printf("p6: %p\n", p6);
            char *p7 = new char[7];
            printf("p7: %p\n", p7);
            MzcGC_Report();
        }
        MzcGC_Leave();
        delete[] p4;
        return 0;
    }
#endif  // def UNITTEST

#endif  // ndef MZC_NO_GC
