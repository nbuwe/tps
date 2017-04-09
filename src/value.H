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
#ifndef VALUE_HH
#define VALUE_HH 1

/*
Note: for Tps_Value, a two word sequence of zeros
should be interpretable as the TPSTYPE_NULL value.
A null value has a zero type and a zero value.
*/

typedef enum Tps_Typeid {
TPSTYPE_NULL=0,
TPSTYPE_MARK=(TPSTYPE_NULL+1),
TPSTYPE_BOOLEAN=(TPSTYPE_MARK+1),
TPSTYPE_INTEGER=(TPSTYPE_BOOLEAN+1),
TPSTYPE_REAL=(TPSTYPE_INTEGER+1),
TPSTYPE_NAME=(TPSTYPE_REAL+1),
TPSTYPE_STRING=(TPSTYPE_NAME+1),
TPSTYPE_ARRAY=(TPSTYPE_STRING+1),
TPSTYPE_DICT=(TPSTYPE_ARRAY+1),
TPSTYPE_STREAM=(TPSTYPE_DICT+1),
TPSTYPE_OPERATOR=(TPSTYPE_STREAM+1)
} Tps_Typeid;

#define TPSTYPE_COUNT	(1+(int)TPSTYPE_OPERATOR)

#define TPSTYPE_ANY		TPSTYPE_NULL /* alias */
#define TPSTYPE_CONTAINER	TPSTYPE_ARRAY /* alias */

/**************************************************/
/* Forward decls: */

struct Tps_String;
struct Tps_Array;
struct Tps_Dict;
struct Tps_Operator;
struct Tps_Stream;
struct Tps_Container;

/**************************************************/

#ifdef WORD64
typedef unsigned int Halfword;
/* Need a float value that is same size as a long */
typedef double Tps_Real;
#else
typedef unsigned short Halfword;
typedef float Tps_Real;
#endif

/**************************************************/

typedef const char* Tps_Nameid; /* this works because it points to the string
				   in the name table */

/**************************************************/
/*
Define the values (if any) associated with various value types.
*/

typedef union Tps_Valueset {
	boolean			_boolean;	/*TPSTYPE_BOOLEAN*/
	long			_integer;	/*TPSTYPE_INTEGER*/
	Tps_Real		_real;		/*TPSTYPE_REAL*/
	Tps_Nameid		_name;		/*TPSTYPE_NAME*/
	struct Tps_String*	_string;	/*TPSTYPE_STRING*/
	struct Tps_Array*	_array;		/*TPSTYPE_ARRAY*/
	struct Tps_Dict*	_dict;		/*TPSTYPE_DICT*/
	struct Tps_Stream*	_stream;	/*TPSTYPE_STREAM*/
	struct Tps_Operator*	_operator;	/*TPSTYPE_OPERATOR*/
	/* all others have no associated value */
	/* Following are for special access */
	uintptr_t		_any;		/*TPSTYPE_ANY*/
	struct Tps_Container*	_container;	/*TPSTYPE_CONTAINER*/
} Tps_Valueset;

/**************************************************/
/* WARNING: Access flags are actually kept in inverse
   so that the value 0 means all access
   Also note that execute bit is separate.  Postscript
   is wrong in merging them because it has more semantics
   than just access; they should have had a separate
   "code array" type instead of using executable arrays.
*/

typedef struct Tps_Flags {
	Halfword	_flags;
	Halfword	_typeid;
} Tps_Flags;

#define TPS_TYPEID_OF(p) ((Tps_Typeid)((p)._flags._typeid))
#define TPS_SET_TYPEID(p,tid) ((p)._flags._typeid = (tid))

/* Flag field shifts */
#define TPS_ACCESSSHIFT		0
#define TPS_EXECUTABLESHIFT	2
#define TPS_TRACEOFFSHIFT	3
#define TPS_UNSAFESHIFT		4
#define TPS_METHODSHIFT		5

/* Flag field masks */
#define TPS_ACCESSMASK		(11<<TPS_ACCESSSHIFT)
#define TPS_EXECUTABLEMASK	( 1<<TPS_EXECUTABLESHIFT)
#define TPS_TRACEOFFMASK	( 1<<TPS_TRACEOFFSHIFT)
#define TPS_UNSAFEMASK		( 1<<TPS_UNSAFESHIFT)
#define TPS_METHODMASK		( 1<<TPS_METHODSHIFT)

#define TPS_ALLFLAGS	(TPS_ACCESSMASK \
			| TPS_EXECUTABLEMASK \
			| TPS_TRACEOFFMASK \
			| TPS_UNSAFEMASK \
			| TPS_METHODMASK)

/* Access flags are kept in inverse so that zero represents all access */
#define Tps_access_read 0x1
#define Tps_access_write 0x2

/* Combinations */
#define Tps_access_noaccess 0
#define Tps_access_readonly Tps_access_read
#define Tps_access_rw (Tps_access_read|Tps_access_write)

#define TPS_TESTVALUEFLAG(p,mask) ((p)._flags._flags & (mask))
#define TPS_ISEXECUTABLE(p) ((boolean)TPS_TESTVALUEFLAG((p),TPS_EXECUTABLEMASK))
#define TPS_ISTRACEOFF(p) (TPS_TESTVALUEFLAG((p),TPS_TRACEOFFMASK))
#define TPS_ISREADABLE(p) (!TPS_TESTVALUEFLAG((p),Tps_access_read))
#define TPS_ISWRITEABLE(p) (!TPS_TESTVALUEFLAG((p),Tps_access_write))
#define TPS_ISUNSAFE(p) (TPS_TESTVALUEFLAG((p),TPS_UNSAFEMASK))
#define TPS_ISMETHOD(p) (TPS_TESTVALUEFLAG((p),TPS_METHODMASK))

#define TPS_ISEXARRAY(p) (TPS_ISTYPE((p),TPSTYPE_ARRAY) && TPS_ISEXECUTABLE(p))
#define TPS_ISEXNAME(p)  (TPS_ISTYPE((p),TPSTYPE_NAME)  && TPS_ISEXECUTABLE(p))
#define TPS_ISEXDICT(p) (TPS_ISTYPE((p),TPSTYPE_DICT) && TPS_ISEXECUTABLE(p))

#define TPS_SET_VFLAGS(p,i,mask,shift) \
	((p)._flags._flags = ((p)._flags._flags & (~(mask))) | ((i)<<(shift)))

#define TPS_SET_EXECUTABLE(p,i) TPS_SET_VFLAGS(p,i,TPS_EXECUTABLEMASK,TPS_EXECUTABLESHIFT)
#define TPS_SET_TRACEOFF(p,i) TPS_SET_VFLAGS(p,i,TPS_TRACEOFFMASK,TPS_TRACEOFFSHIFT)
#define TPS_SET_ACCESS(p,i) TPS_SET_VFLAGS(p,i,TPS_ACCESSMASK,TPS_ACCESSSHIFT)
#define TPS_SET_UNSAFE(p,i) TPS_SET_VFLAGS(p,i,TPS_UNSAFEMASK,TPS_UNSAFESHIFT)
/* Note, METHOD => EXECUTABLE; see Tps_op_cvmethod */
#define TPS_SET_METHOD(p,i) TPS_SET_VFLAGS(p,i,TPS_METHODMASK,TPS_METHODSHIFT)

#define TPS_CLEAR_FLAGS(p) ((p)._flags._flags = 0)

/**************************************************/

typedef struct Tps_Value {
	Tps_Flags	_flags;
	Tps_Valueset	_value;
} Tps_Value;

#define TPS_BOOLEAN_OF(p) (p)._value._boolean
#define TPS_INTEGER_OF(p) (p)._value._integer
#define TPS_REAL_OF(p) (p)._value._real
#define TPS_STRING_OF(p) (p)._value._string
#define TPS_NAME_OF(p) (p)._value._name
#define TPS_ARRAY_OF(p) (p)._value._array
#define TPS_DICT_OF(p) (p)._value._dict
#define TPS_MARK_OF(p) (p)._value._mark
#define TPS_STREAM_OF(p) (p)._value._stream
#define TPS_OPERATOR_OF(p) (p)._value._operator

#define TPS_ANY_OF(p) (p)._value._any
#define TPS_CONTAINER_OF(p) (p)._value._container

/**************************************************/
/*
Test for various types
*/

#define TPS_TYPE(val) TPS_TYPEID_OF(val)

#define TPS_ISTYPE(val,typ) (TPS_TYPE(val) == (typ))

/**************************************************/
/*
Fill in a value from scratch
*/

/* Make a value with the flags set to zero */
#define TPS_MAKEVALUE(pv,typ,val) (\
	TPS_CLEAR_FLAGS(pv), \
	TPS_SET_TYPEID(pv,typ), \
	TPS_ANY_OF(pv) = (uintptr_t)(val) \
	)

/* change value only */
#define TPS_CHANGEVALUE(pv,typ,val) { TPS_ANY_OF(pv) = (uintptr_t)(val); }

#if HASFLOAT
/* Make a real value */
#define TPS_MAKEREAL(pv,val) (\
	TPS_CLEAR_FLAGS(pv), \
	TPS_SET_TYPEID(pv,TPSTYPE_REAL), \
	TPS_REAL_OF(pv) = (Tps_Real)(val) \
	)

#define TPS_CHANGEREAL(pv,val) { TPS_REAL_OF(pv) = (Tps_Real)(val); }
#endif /*HASFLOAT*/

/* Misc. */

typedef Tps_Value (*Tpsvalfcn)(ANYARGS);

/* Define an equivalent form of the Tps_Value, but
   suitable for static initialization in build.C
   This may need work on 64 bit machines like Alpha.
*/

typedef struct Tps_Value_Simple {
	Tps_Flags	_flags;
	void*		_value;
} Tps_Value_Simple;

/* Provide a converter */
typedef struct Tps_Value_Union {
    union {
	Tps_Value_Simple _simple;
	Tps_Value	 _value;
    } u;
} Tps_Value_Union;

/**************************************************/
#endif /*VALUE_HH*/
