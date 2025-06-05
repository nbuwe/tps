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
#ifndef SYSDEP_HH
#define SYSDEP_HH

/* Prepare for c++ and ansi c use */

#if(defined(__cplusplus) || defined(c_plusplus))
#  ifndef CPLUSPLUS
#    define CPLUSPLUS
#  endif
#  ifndef __STDC__
#    define __STDC__
#  endif
#endif

#ifdef CPLUSPLUS

#  ifndef DOTS
#    define DOTS ...
#  endif

#  ifndef NOARGS
#    define NOARGS
#  endif

#  ifndef ANYARGS
#    define ANYARGS DOTS
#  endif

#  ifndef EXTERNC
#    define EXTERNC extern "C"
#  endif


#else /*!CPLUSPLUS*/
#ifdef __STDC__

#  ifndef DOTS
#    define DOTS ...
#  endif

#  ifndef NOARGS
#    define NOARGS void
#  endif

#  ifndef ANYARGS
#    define ANYARGS
#  endif

#  ifndef EXTERNC
#    define EXTERNC extern
#  endif

#  ifndef inline
#    define inline static
#  endif

#endif /*__STDC__*/
#endif /*_CPLUSPLUS*/

#ifndef EXTERN
#define EXTERN extern
#endif

#ifndef MAX_INTEGER
#define MAX_INTEGER 0x7fffffff
#endif

#ifndef FALLTHRU
#define FALLTHRU
#endif

#ifdef __alpha
#  ifndef WORD64
#  define WORD64 1
#  endif
#endif

#ifndef __GNUC__
#define volatile
#endif

#ifndef __STDC__
#define const
#endif

/**************************************************/

typedef unsigned char byte;
typedef unsigned short uint_16;
typedef unsigned int uint_32;
typedef int int_32;

#if WORD64
typedef unsigned int ulong_32;
typedef int long_32;
#else
typedef unsigned long ulong_32;
typedef long long_32;
#endif

typedef int boolean;

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#ifndef EOF
#define EOF (-1)
#endif

/* Definitions that tend to wander from system to system */
#define SLEEP(n) sleep((int)(n))
#define SRAND(n) srand((int)(n))
#define STRTOL(s,p,r) strtol(s,p,(int)r)
#define STRDUP(s) Tps_strdup(s)

#endif /*SYSDEP_HH*/
