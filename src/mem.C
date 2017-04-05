#include <malloc.h>
#include "tps.H"
#include "mem.H"

/*
Define some procedures that are commonly missing in
various flavors of Unix.
*/

/* define a local version of strdup */
char*
Tps_strdup(const char* s)
{
    long len;
    char* s2;

    len = strlen(s);
    s2 = Tps_malloc(len+1);
    memcpy(s2,s,len+1);
    return s2;
}

#ifdef MALLOCREPLACE

void* operator new(size_t size) {return (void*)Tps_malloc(size);}
void operator delete(void* p) {(void)Tps_free((char*)p);}

char*
Tps_malloc(long len)
{
    char* p;
    p = (char*)malloc((unsigned int)len);
    return p;
}

void
Tps_free(char* obj)
{
    (void)free(obj);
}

#endif /*MALLOCREPLACE*/

