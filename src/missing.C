#include "tps.H"
#include "mem.H"

/*
Define some procedures that are commonly missing in
various flavors of Unix.
*/

/* define a local version of strdup */
char*
Tps_strdup(char* s)
{
    register long len;
    register char* s2;

    len = strlen(s);
    s2 = Tps_malloc(len+1);
    MEMCPY(s2,s,len+1);
    return s2;
}
