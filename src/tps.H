/*
Copyright (c) 1993 Dennis Heimbigner
All rights reserved.

This software was developed by Dennis Heimbigner
as part of the Arcadia project at
the University of Colorado, Boulder.

Redistribution and use in source and binary forms are permitted
provided that the above copyright notice and this paragraph are
duplicated in all such forms and that any documentation,
and other materials related to such
distribution and use acknowledge that the software was developed
by Dennis Heimbigner.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/


#ifndef TPS_HH
#define TPS_HH  1

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "Sysdep.H"
#include "tps_platform.H"

/**************************************************/
/* Define the various compilation conditionals */

#ifndef CONFIGTEST /* test that various compile/run assumptions are true */
#  define CONFIGTEST 1
#endif
#ifndef PSMIMIC /* mimic postscript as closely as possible; default is not */
#  define PSMIMIC 0
#endif
#ifndef HASFLOAT
#  define HASFLOAT 1
#endif
#ifndef HASRAND
#  define HASRAND 1
#endif
# ifndef OO /* OO features default is on */
#  define OO 1
#endif

/**************************************************/
/* Define the Error numbers */
typedef enum Tps_Status {
	TPSSTAT_OK			= 0,
	TPSSTAT_FAIL			= 1, // generic failure
	TPSSTAT_DICTFULL		= (TPSSTAT_FAIL+1),
	TPSSTAT_DICTSTACKOVERFLOW	= (TPSSTAT_DICTFULL+1),
	TPSSTAT_DICTSTACKUNDERFLOW	= (TPSSTAT_DICTSTACKOVERFLOW+1),
	TPSSTAT_EXECSTACKOVERFLOW	= (TPSSTAT_DICTSTACKUNDERFLOW+1),
	TPSSTAT_EXECSTACKUNDERFLOW	= (TPSSTAT_EXECSTACKOVERFLOW+1),
	TPSSTAT_INTERRUPT		= (TPSSTAT_EXECSTACKUNDERFLOW+1),
	TPSSTAT_INVALIDACCESS		= (TPSSTAT_INTERRUPT+1),
	TPSSTAT_INVALIDEXIT		= (TPSSTAT_INVALIDACCESS+1),
	TPSSTAT_INVALIDSTREAMACCESS	= (TPSSTAT_INVALIDEXIT+1),
	TPSSTAT_IOERROR			= (TPSSTAT_INVALIDSTREAMACCESS+1),
	TPSSTAT_LIMITCHECK		= (TPSSTAT_IOERROR+1),
	TPSSTAT_RANGECHECK		= (TPSSTAT_LIMITCHECK+1),
	TPSSTAT_STACKOVERFLOW		= (TPSSTAT_RANGECHECK+1),
	TPSSTAT_STACKUNDERFLOW		= (TPSSTAT_STACKOVERFLOW+1),
	TPSSTAT_SYNTAXERROR		= (TPSSTAT_STACKUNDERFLOW+1),
	TPSSTAT_TIMEOUT			= (TPSSTAT_SYNTAXERROR+1),
	TPSSTAT_TYPECHECK		= (TPSSTAT_TIMEOUT+1),
	TPSSTAT_UNDEFINED		= (TPSSTAT_TYPECHECK+1),
	TPSSTAT_UNDEFINEDFILENAME	= (TPSSTAT_UNDEFINED+1),
	TPSSTAT_UNDEFINEDRESULT		= (TPSSTAT_UNDEFINEDFILENAME+1),
	TPSSTAT_UNMATCHEDMARK		= (TPSSTAT_UNDEFINEDRESULT+1),
	TPSSTAT_UNREGISTERED		= (TPSSTAT_UNMATCHEDMARK+1),
	TPSSTAT_VMERROR			= (TPSSTAT_UNREGISTERED+1),
	/* New conditions (not necessarily errors as such) */
	TPSSTAT_SYSTEMERROR		= (TPSSTAT_VMERROR+1),
	TPSSTAT_ARITYMISMATCH		= (TPSSTAT_SYSTEMERROR+1),
	TPSSTAT_UNCAUGHTTHROW		= (TPSSTAT_ARITYMISMATCH+1),
	TPSSTAT_EOF			= (TPSSTAT_UNCAUGHTTHROW+1),
	TPSSTAT_QUIT			= (TPSSTAT_EOF+1),
	TPSSTAT_RETRYFRAME		= (TPSSTAT_QUIT+1),
	TPSSTAT_POPFRAME		= (TPSSTAT_RETRYFRAME+1),
	TPSSTAT_TAILFRAME		= (TPSSTAT_POPFRAME+1),
	TPSSTAT_STOP			= (TPSSTAT_TAILFRAME+1),
	TPSSTAT_UNSAFE			= (TPSSTAT_STOP+1)
	/* if you add to this list, then fix TPS_ERROR_COUNT, and init.C*/
} Tps_Status;

#define TPS_ERROR_COUNT (1+(int)TPSSTAT_UNSAFE)

/**************************************************/
/* Misc other defs */
/**************************************************/
typedef unsigned long Tps_Userdata;

#define TPS_USERDATA_NULL ((Tps_Userdata)0)

/* Thread Related Flags and definitions*/

#define GLOBAL_LOCK()
#define GLOBAL_UNLOCK()

#define TESTERR(cmd) { Tps_Status err = cmd; if(err != TPSSTAT_OK) return err; }

struct Tps_Interp; /* forward */
struct Tps_Value; /* forward */

typedef Tps_Status (*Tpsstatfcn)(struct Tps_Interp*, struct Tps_Value*, long);

/**************************************************/

#include "value.H"
#include "array.H"
#include "dict.H"
#include "name.H"
#include "operator.H"
#include "str.H"
#include "stream.H"
#include "lex.H"
#include "interp.H"
#include "init.H"
#include "exec.H"
#include "isolate.H"

#endif /*TPS_HH*/
