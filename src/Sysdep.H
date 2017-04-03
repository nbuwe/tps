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

#ifndef __STDC__
#  ifndef __KNR__
#    define __KNR__
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

/* use the following for defining external parameters */
#  ifndef AC
#    define AC(fcn,args) fcn args
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

/* use the following for defining external parameters */
#  ifndef AC
#    define AC(fcn,args) fcn args
#  endif

#endif /*__STDC__*/
#endif /*_CPLUSPLUS*/

#ifdef __STDC__
/* use the following for declaring function headers */
#define ACD1(t1,a1) (t1 a1)
#define ACD2(t1,a1,t2,a2) (t1 a1, t2 a2)
#define ACD3(t1,a1,t2,a2,t3,a3) (t1 a1, t2 a2, t3 a3)
#define ACD4(t1,a1,t2,a2,t3,a3,t4,a4) (t1 a1, t2 a2, t3 a3, t4 a4)
#define ACD5(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5) (t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)
#define ACD6(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6) (t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6)
#define ACD7(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7) (t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7)
#define ACD8(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8) (t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8)
#endif /*__STDC__*/

#ifdef __KNR__

#  ifndef DOTS
#    define DOTS
#  endif

#  ifndef NOARGS
#    define NOARGS
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

#  ifndef AC
#    define AC(fcn,args) fcn()
#  endif

/* use the following for declaring function headers */
#define ACD1(t1,a1) (a1) t1 a1;
#define ACD2(t1,a1,t2,a2) (a1,a2) t1 a1; t2 a2;
#define ACD3(t1,a1,t2,a2,t3,a3) (a1,a2,a3) t1 a1; t2 a2; t3 a3;
#define ACD4(t1,a1,t2,a2,t3,a3,t4,a4) (a1,a2,a3,a4) t1 a1; t2 a2; t3 a3; t4 a4;
#define ACD5(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5) (a1,a2,a3,a4,a5) t1 a1; t2 a2; t3 a3; t4 a4; t5 a5;
#define ACD6(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6) (a1,a2,a3,a4,a5,a6) t1 a1; t2 a2; t3 a3; t4 a4; t5 a5; t6 a6;
#define ACD7(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7) (a1,a2,a3,a4,a5,a6,a7) t1 a1; t2 a2; t3 a3; t4 a4; t5 a5; t6 a6; t7 a7;
#define ACD8(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8) (a1,a2,a3,a4,a5,a6,a7,a8) t1 a1; t2 a2; t3 a3; t4 a4; t5 a5; t6 a6; t7 a7; t8 a8;

#endif /*__KNR__*/

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
#define MEMSET(x,y,z) memset(x,y,(int)(z))
#define MEMCMP(x,y,z) memcmp(x,y,(int)(z))
#define MEMCPY(x,y,z) memcpy(x,y,(int)(z))
#define MEMCHR(x,y,z) memchr(x,y,(int)z)
#define STRNCMP(x,y,z) strncmp(x,y,(int)(z))
#define SLEEP(n) sleep((int)(n))
#define SRAND(n) srand((int)(n))
#define STRTOL(s,p,r) strtol(s,p,(int)r)
#define STRDUP(s) Tps_strdup(s)

/* Looks like virtual functions dont always work with g++ */
#ifdef __sgi
#define VIRTUAL
#else
#define VIRTUAL = 0
#endif

/*
It appears that under cxx compiler and osf1,
that __STDC__ has some value other than 1.
*/
#if defined(__osf__) && defined(__cplusplus)
#  if !defined(__GNUC__) && defined(__STDC__)
#    undef __STDC__
#    define __STDC__ 1
#  endif
#endif

#endif /*SYSDEP_HH*/
