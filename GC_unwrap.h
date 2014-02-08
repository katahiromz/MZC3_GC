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

#define mzcnew new
#define mzcnew_nothrow new(std::nothrow)
#define mzcdelete delete
