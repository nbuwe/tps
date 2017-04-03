/*
Copyright (c) 1993 Dennis Heimbigner.
All rights reserved.

This software was developed by the Dennis Heimbignbger
as part of the Arcadia project at the University of Colorado, Boulder.

Redistribution and use in source and binary forms are permitted
provided that the above copyright notice and this paragraph are
duplicated in all such forms.
THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef TPS_MEM_HH
#define TPS_MEM_HH 1

/**************************************************/

EXTERNC char* AC(Tps_strdup,(const char*));

/**************************************************/

#ifdef sun
#  ifdef DEBUGMALLOC
EXTERNC int AC(malloc_verify,());
EXTERNC int AC(malloc_debug,(const int level));
#  endif
#endif

/**************************************************/
/* Allow for overriding use of malloc */
#ifdef MALLOCREPLACE
EXTERNC char* AC(Tps_malloc,(const long len));
EXTERNC void AC(Tps_free,(char* obj));

#ifdef __cplusplus
EXTERN void* operator new(const size_t);
EXTERN void operator delete(void*);
#endif

#else /*!MALLOCREPLACE*/

#define Tps_malloc(l) (char*)malloc(l)
#define Tps_free(p) (void)free(p)

#endif /*MALLOCREPLACE*/

#endif /*TPS_MEM_HH*/
