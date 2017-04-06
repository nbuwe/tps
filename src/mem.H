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
#ifndef TPS_MEM_HH
#define TPS_MEM_HH 1

/**************************************************/

EXTERNC char* Tps_strdup(const char*);

/**************************************************/

#ifdef sun
#  ifdef DEBUGMALLOC
EXTERNC int malloc_verify();
EXTERNC int malloc_debug(const int level);
#  endif
#endif

/**************************************************/
/* Allow for overriding use of malloc */
#ifdef MALLOCREPLACE
EXTERNC char* Tps_malloc(const long len);
EXTERNC void Tps_free(char* obj);

#ifdef __cplusplus
EXTERN void* operator new(const size_t);
EXTERN void operator delete(void*);
#endif

#else /*!MALLOCREPLACE*/

#define Tps_malloc(l) (char*)malloc(l)
#define Tps_free(p) (void)free(p)

#endif /*MALLOCREPLACE*/

#endif /*TPS_MEM_HH*/
