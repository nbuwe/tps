/*
 * Copyright (c) 1993 Dennis Heimbigner
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

