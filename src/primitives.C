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

#include <math.h>
#include "tps.H"
#include "primitives.H"
#include "util.H"
#include "mem.H"
#include "export.H"

#include <sys/resource.h>
#if 0
#ifdef solaris2
/* for solaris2 and perhaps other systems */
#include <sys/rusage.h>
#endif
#endif
#include <sys/time.h>
#if defined hpux || solaris2
#include <sys/times.h>
#endif

#if VERBOSE > 1
#include "debug.H"
#endif

#if defined sun && ! defined solaris2
EXTERNC int gettimeofday(struct timeval*,struct timezone*);
#endif

EXTERNC int getrusage(int,struct rusage*);

/**************************************************/

/* Define some frame types over and above what is in exec.H */

struct Tps_Frame_Catch : Tps_Frame {
	Tps_Value	_body;
};

struct Tps_Frame_Stopped : Tps_Frame {
	Tps_Value	_body;
};

struct Tps_Frame_Runstream : Tps_Frame {
	Tps_Stream*	_strm;
};

struct Tps_Frame_Loop : Tps_Frame {
	Tps_Value	_body;
};

struct Tps_Frame_While : Tps_Frame_Loop {
	Tps_Value	_cond;
};

struct Tps_Frame_Repeat : Tps_Frame_Loop {
	long		_current;
};

struct Tps_Frame_Forall : Tps_Frame_Repeat {
	Tps_Value	_iterate;
};

struct Tps_Frame_For : Tps_Frame_Loop {
	long		_dir;
	Tps_Value	_current;
	Tps_Value	_limit;
	Tps_Value	_incr;
};

EXTERNC Tps_Status Tps_create_catch(Tps_Interp*, Tps_Value);
EXTERNC Tps_Status Tps_create_stopped(Tps_Interp*, Tps_Value);
EXTERNC Tps_Status Tps_create_runstream(Tps_Interp*, Tps_Stream*);
EXTERNC Tps_Status Tps_create_loop(Tps_Interp*);
EXTERNC Tps_Status Tps_create_while(Tps_Interp*);
EXTERNC Tps_Status Tps_create_repeat(Tps_Interp*, int);
EXTERNC Tps_Status Tps_create_forall(Tps_Interp*, Tps_Value);
EXTERNC Tps_Status Tps_create_for(Tps_Interp*, Tps_Value, Tps_Value, Tps_Value);

/**************************************************/

static Tps_Status Tps_loop_mark(TPS_MARK_ARGS0);
static Tps_Status Tps_loop_export(TPS_EXPORT_ARGS0);
static Tps_Status Tps_loop_import(TPS_IMPORT_ARGS0);

static Tps_Status Tps_repeat_export(TPS_EXPORT_ARGS0);
static Tps_Status Tps_repeat_import(TPS_IMPORT_ARGS0);

/**************************************************/

Tps_Status
Tps_op_abs(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Typeid rtyp;

    rtyp = TPS_TYPE(args[0]);
    switch (rtyp) {
	case TPSTYPE_INTEGER:
	    {
		long i = TPS_INTEGER_OF(args[0]);
		if(i < 0) TPS_INTEGER_OF(args[0]) = - i;
	    }
	    break;
#if HASFLOAT
	case TPSTYPE_REAL:
	    {
		Tps_Real f = TPS_REAL_OF(args[0]);
		if(f < 0) TPS_REAL_OF(args[0]) = - f;
	    }
	    break;
#endif /*HASFLOAT*/
	default:
	    return(TPSSTAT_TYPECHECK);
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_add(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Value result;
    long ir = 0;
    long i;
#if HASFLOAT
    Tps_Real fr = 0.0;
    int isfloat = 0;
#endif

    /* this is defined to potentially add any number of args;
	currently restrict to two.
    */
    for(i=0;i<2;i++) {
	switch (TPS_TYPE(args[i])) {
	    case TPSTYPE_INTEGER: ir += TPS_INTEGER_OF(args[i]); break;
#if HASFLOAT
	    case TPSTYPE_REAL: fr += TPS_REAL_OF(args[i]); isfloat = 1; break;
#endif
	    default: return TPSSTAT_TYPECHECK;
	}
    }
#if HASFLOAT
    if(isfloat) {
	fr += (Tps_Real)ir;
	TPS_MAKEREAL(result,fr);
    } else
#endif
    {
	TPS_MAKEVALUE(result,TPSTYPE_INTEGER,ir);
    }
    TPS_RETURNVAL(intrp,result,2);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_aload(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Array* a;
    Tps_Value v;
    long i;
    Tps_Value* p;

    v = TPS_POP(intrp);
    args++; /* keep in synch */
    if(!TPS_ISTYPE(v,TPSTYPE_ARRAY)) return(TPSSTAT_TYPECHECK);
    a = TPS_ARRAY_OF(v);
    i = a->length();
    /* need space for all elements in array + array*/
    TPS_GUARANTEE(intrp,i);
    /* transfer the contents to the stack s.t. a[0] is pushed first */
    TPS_PUSHN(intrp,i);
    p = (Tps_Value*)a->contents();
    while(i-- > 0) *(--args) = *p++;
    /* leave the array as top element */
    TPS_PUSH(intrp,v);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_anchorsearch(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_String* s0;
    Tps_String* s1;

    if(!TPS_ISTYPE(args[0],TPSTYPE_STRING) || !TPS_ISTYPE(args[1],TPSTYPE_STRING))
	return(TPSSTAT_TYPECHECK);
    s0 = TPS_STRING_OF(args[0]);
    s1 = TPS_STRING_OF(args[1]);
    if(s0->length() > s1->length()) return(TPSSTAT_INVALIDACCESS);
    if(MEMCMP(s0->contents(),s1->contents(),s0->length()) == 0) {
	/* match */
	/* need to construct 2 substrings */
	Tps_Value mid;
	Tps_Value post;
	Tps_String* s;

	TPS_GUARANTEE(intrp,1);
	s = new Tps_String(s1->contents(),s0->length());
	TPS_MAKEVALUE(mid,TPSTYPE_STRING,s);
	s = new Tps_String(s1->contents()+s0->length(),
			   s1->length() - s0->length());
	TPS_MAKEVALUE(post,TPSTYPE_STRING,s);
	/* stick things on the stack in right order */
	args[1] = post;
	args[0] = mid;
	TPS_MAKEVALUE(mid,TPSTYPE_BOOLEAN,1); /* reuse mid*/
	TPS_PUSH(intrp,mid);
	return TPSSTAT_OK;
    }
    /* not found */
    TPS_MAKEVALUE(args[0],TPSTYPE_BOOLEAN,0);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_and(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    u_long bits = ~0; /* so initial and will be correct */
    long i;
    int isint = 0;
    Tps_Value result;

    for(i=0;i<2;i++) {
	switch (TPS_TYPE(args[i])) {
	    case TPSTYPE_INTEGER:
		isint = 1;
	    case TPSTYPE_BOOLEAN:
		bits &= TPS_ANY_OF(args[i]);
		break;
	    default: return TPSSTAT_TYPECHECK;
	}
    }
    if(isint)
	TPS_MAKEVALUE(result,TPSTYPE_INTEGER,bits);
    else
	TPS_MAKEVALUE(result,TPSTYPE_BOOLEAN,bits);
    TPS_RETURNVAL(intrp,result,2);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_append(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;
    Tps_Typeid typ0;

    if(TPS_DEPTH(intrp) < 2) return(TPSSTAT_STACKUNDERFLOW);
    typ0 = TPS_TYPE(args[1]);
    switch (typ0) {
	case TPSTYPE_STRING: {
	    Tps_String* s0;
	    if(TPS_ISTYPE(args[0],TPSTYPE_STRING)) {
		Tps_String* s1;
		s0 = TPS_STRING_OF(args[1]);
		s1 = TPS_STRING_OF(args[0]);
		ok = s0->append(s1);
	    } else if(TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) {
		char c;
		s0 = TPS_STRING_OF(args[1]);
		c = TPS_INTEGER_OF(args[0]);
		ok = s0->append(c);
	    }	
	    if(ok != TPSSTAT_OK) return ok;
	    TPS_POPN(intrp,1);
	    break;
	case TPSTYPE_ARRAY: {
	    Tps_Array* a0;
	    a0 = TPS_ARRAY_OF(args[1]);
	    ok = a0->append(args[0]);
	    if(ok != TPSSTAT_OK) return ok;
	    TPS_POPN(intrp,1);
	    break;
	}
	default:
	    return(TPSSTAT_TYPECHECK);
	}/*switch*/
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_array(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Array* a;
    long i;

    if(!TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return(TPSSTAT_TYPECHECK);
    i = TPS_INTEGER_OF(args[0]);
    if(i < 0) return(TPSSTAT_RANGECHECK);
    a = new Tps_Array(i);
    if(!a) return(TPSSTAT_RANGECHECK);
    /* null out array */
    MEMSET((char*)a->contents(),0,a->length()*sizeof(Tps_Value));
    /* leave the array as top element */
    TPS_MAKEVALUE(args[0],TPSTYPE_ARRAY,a);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_astore(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Array* a;
    long len;
    long i;
    Tps_Value* p;
    Tps_Value v;

    /* ostensibly arity == 1 */
    v = TPS_POP(intrp);
    args++; /* keep in synch */
    if(!TPS_ISTYPE(v,TPSTYPE_ARRAY)) return(TPSSTAT_TYPECHECK);
    a = TPS_ARRAY_OF(v);
    len = a->length();
    /* see if enough values are on the stack as a whole */
    if(TPS_DEPTH(intrp) < len) return(TPSSTAT_STACKUNDERFLOW);
    /* transfer the contents to the array s.t. TOS goes to a[len-1] */
    p = len + (Tps_Value*)a->contents();
    i = len;
    while(i-- > 0) *(--p) = *args++;
    /* leave the array as top element */
    TPS_RETURNVAL(intrp,v,len);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_begin(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    if(!TPS_ISTYPE(args[0],TPSTYPE_DICT)) return(TPSSTAT_TYPECHECK);
    TPS_DGUARANTEE(intrp,1);
    TPS_DPUSH(intrp,args[0]);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_bind(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Value* ds;
    long dlen;

    ds = TPS_DTOSP(intrp);
    dlen = TPS_DDEPTH(intrp);
    return Tps_bind1(ds,dlen,&args[0]);
}

Tps_Status
Tps_op_bitshift(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    u_long iu;
    long i;

    if(!TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return(TPSSTAT_TYPECHECK);
    if(!TPS_ISTYPE(args[1],TPSTYPE_INTEGER)) return(TPSSTAT_TYPECHECK);
    iu = TPS_ANY_OF(args[1]);
    i = TPS_INTEGER_OF(args[0]);
    if(i < 0) { iu >>= - i; } else {iu <<= i; };
    TPS_CHANGEVALUE(args[1],TPSTYPE_ANY,iu);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_bytesavailable(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    long i;
    if(!TPS_ISTYPE(args[0],TPSTYPE_STREAM)) return(TPSSTAT_TYPECHECK);
    i = TPS_STREAM_OF(args[0])->bytesavailable();
    TPS_MAKEVALUE(args[0],TPSTYPE_INTEGER,i);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_catch(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    Tps_Value proc;

    proc = TPS_POP(intrp);
    if(TPS_ISEXARRAY(proc)) {
	Tps_Frame_Catch* f;
	if(!(f = (Tps_Frame_Catch*)Tps_create_frame(intrp,&Tps_handler_catch,sizeof(Tps_Frame_Catch)))) return TPSSTAT_VMERROR;
	f->_body = proc;
	return Tps_create_source(intrp,proc);
    }
    TPS_PUSH(intrp,TPS__CONST(TPS__FALSE));
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_catch_unwind(TPS_UNWIND_ARGS)
{
    Tps_Frame_Catch* f = (Tps_Frame_Catch*)frame;
    Tps_Status ok = Tps_unwind_frame(intrp,f);
    if(ok != TPSSTAT_OK) return ok;  
    if(thrown) {
	/* thrown, push throw value and true */
	TPS_GUARANTEE(intrp,2);
	TPS_PUSH(intrp,intrp->_throwflag);
	TPS_PUSH(intrp,TPS__CONST(TPS__TRUE));
	return TPSSTAT_STOP;
    } else {
	/* normal exit, push false */
	TPS_GUARANTEE(intrp,1);
	TPS_PUSH(intrp,TPS__CONST(TPS__FALSE));
    }
    return ok;
}

static
Tps_Status
Tps_catch_reenter(TPS_REENTER_ARGS0)
{
    return TPSSTAT_POPFRAME;
}

static
Tps_Status
Tps_catch_trace(TPS_TRACE_ARGS)
{
    Tps_Frame_Catch* f = (Tps_Frame_Catch*)frame;
    Tps_trace0(intrp,strm,f);
    strm->write(" body=");
    (void)Tps_cvts1(*strm,f->_body,TRUE,-1);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_catch_mark(TPS_MARK_ARGS1)
{
    Tps_Frame_Catch* f = (Tps_Frame_Catch*)frame;
    Tps_mark(f->_body);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_catch_export(TPS_EXPORT_ARGS1)
{
    Tps_Frame_Catch* f = (Tps_Frame_Catch*)frame;

    /* guarantee enough room for 1 value */
    TPS_GUARANTEE(intrp,1);
    /* dump body */
    TPS_PUSH(intrp,f->_body);
    count++;
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_catch_import(TPS_IMPORT_ARGS1)
{
    Tps_Frame_Catch* f = (Tps_Frame_Catch*)fr;

    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt != 1) return TPSSTAT_RANGECHECK;
    cnt--;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_Catch*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_Catch));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    }
    f->_body = TPS_POP(intrp);
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_catch_parms = {"Catch"};

Tps_Handler Tps_handler_catch = {
	&Tps_catch_parms,
	Tps_catch_unwind,
	Tps_catch_reenter,
	Tps_catch_trace,
	Tps_catch_mark,
	Tps_catch_export,
	Tps_catch_import
};

Tps_Status
Tps_op_clear(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    TPS_POPN(intrp,TPS_DEPTH(intrp));
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_cleartomark(Tps_Interp* intrp, Tps_Value* args, long nargs)
{
    long i;
    Tps_Value* top = args;

    /* need to look for topmost mark */
    for(i=0;i<nargs;i++,top++) {
	if(TPS_ISTYPE(*top,TPSTYPE_MARK)) {
	    TPS_POPN(intrp,i+1); /* wipe out including mark */
	    return TPSSTAT_OK;
	}
    }
    return(TPSSTAT_UNMATCHEDMARK);
}

Tps_Status
Tps_op_closestream(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Stream* strm;
    if(!TPS_ISTYPE(args[0],TPSTYPE_STREAM)) return(TPSSTAT_TYPECHECK);
    if(intrp->safe() && TPS_ISUNSAFE(args[0])) return TPSSTAT_UNSAFE;
    strm = TPS_STREAM_OF(args[0]);
    TPS_POPN(intrp,1);
    strm->close();
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_copy(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;
    long i;
    Tps_Value* pv;
    Tps_Value v;

    /* arity is considered 1, but special cases apply */
    if(TPS_TYPE(args[0]) == TPSTYPE_INTEGER) {
	v = TPS_POP(intrp);
	i = TPS_INTEGER_OF(v);
	if(i < 0) return(TPSSTAT_UNDEFINED);
	TPS_GUARANTEE(intrp,i);
	args = TPS_TOSP(intrp); /* the base of the values to copy */
	TPS_PUSHN(intrp,i);
	pv = TPS_TOSP(intrp); /* base of copies */
	/* dup the elements */
	(void)MEMCPY((char*)pv,(char*)args,sizeof(Tps_Value)*i);
    } else {
	if(TPS_DEPTH(intrp) < 2) return(TPSSTAT_STACKUNDERFLOW);
	/* make args cover the two args */
	args = TPS_TOSP(intrp);
	switch (TPS_TYPE(args[1])) {
	case TPSTYPE_STRING: {
		Tps_String* s0;
		Tps_String* s1;
		s0 = TPS_STRING_OF(args[1]);
		if(TPS_ISTYPE(args[0],TPSTYPE_NULL)) {
		    s1 = new Tps_String(s0->length());
		    if(!s1) return TPSSTAT_VMERROR;
		    TPS_MAKEVALUE(args[0],TPSTYPE_STRING,s1);
		} else if(!TPS_ISTYPE(args[0],TPSTYPE_STRING))
		    return TPSSTAT_TYPECHECK;
		else if(!TPS_ISWRITEABLE(args[0]))
		    return TPSSTAT_INVALIDACCESS;
		else
		    s1 = TPS_STRING_OF(args[0]);
		if(s1->length() < s0->length()) return(TPSSTAT_RANGECHECK);
		/* correct length of s1 */
		s1->setlength(s0->length());
		(void)MEMCPY(s1->contents(),s0->contents(),s0->length());
		args[1] = args[0];
		TPS_POPN(intrp,1);
	    }
	    break;
	case TPSTYPE_ARRAY: {
		Tps_Array* a0;
		Tps_Array* a1;
		a0 = TPS_ARRAY_OF(args[1]);
		if(TPS_ISTYPE(args[0],TPSTYPE_NULL)) {
		    a1 = new Tps_Array(a0->length());
		    if(!a1) return TPSSTAT_VMERROR;
		    TPS_MAKEVALUE(args[0],TPSTYPE_ARRAY,a1);
		} else if(!TPS_ISTYPE(args[0],TPSTYPE_ARRAY))
		    return TPSSTAT_TYPECHECK;
		else if(!TPS_ISWRITEABLE(args[0]))
		    return TPSSTAT_INVALIDACCESS;
		else
		    a1 = TPS_ARRAY_OF(args[0]);
		if(a1->length() < a0->length()) return(TPSSTAT_RANGECHECK);
		/* correct length of s1 */
		a1->setlength(a0->length());
		(void)MEMCPY((char*)a1->contents(),(char*)a0->contents(),
			    sizeof(Tps_Value)*(a0->length()));
		args[1] = args[0];
		TPS_POPN(intrp,1);
	    }
	    break;
	case TPSTYPE_DICT: {
		Tps_Dict* d0;
		Tps_Dict* d1;
		d0 = TPS_DICT_OF(args[1]);
		if(TPS_ISTYPE(args[0],TPSTYPE_NULL)) {
		    d1 = new Tps_Dict_Tcl;
		    if(!d1) return TPSSTAT_VMERROR;
		    TPS_MAKEVALUE(args[0],TPSTYPE_DICT,d1);
		} else if(!TPS_ISTYPE(args[0],TPSTYPE_DICT))
		    return TPSSTAT_TYPECHECK;
		else if(!TPS_ISWRITEABLE(args[0]))
		    return TPSSTAT_INVALIDACCESS;
		else
		    d1 = TPS_DICT_OF(args[0]);
		d1->clear();
		if((ok = d1->copy(d0)) != TPSSTAT_OK) return ok;
		args[1] = args[0];
		TPS_POPN(intrp,1);
	    }
	    break;
	default:
	    return(TPSSTAT_TYPECHECK);
	}/*switch*/
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_count(Tps_Interp* intrp, Tps_Value* /*args*/, long nargs)
{
    Tps_Value v;

    TPS_GUARANTEE(intrp,1);
    TPS_MAKEVALUE(v,TPSTYPE_INTEGER,nargs);
    TPS_PUSH(intrp,v);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_countdictstack(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    long i;
    Tps_Value v;

    TPS_GUARANTEE(intrp,1);
    i = TPS_DDEPTH(intrp);
    TPS_MAKEVALUE(v,TPSTYPE_INTEGER,i);
    TPS_PUSH(intrp,v);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_countexecstack(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    long xlen = 0;
    Tps_Frame* eframe;
    char* enext;
    long count;
    Tps_Status ok;
    long flen;
    Tps_Value v;

    TPS_GUARANTEE(intrp,1);
    enext=TPS_ETOSP(intrp);
    for(count = TPS_EFRAMECOUNT(intrp); count > 0; count--) {
	eframe = (Tps_Frame*)enext;
	/* push contents of the frame */
	flen = 0;
	ok = TPS_FRAME_EXPORT(intrp,eframe,flen,Tps_pass_all);
	if(ok != TPSSTAT_OK) return TPSSTAT_FAIL;
	xlen += flen + 2;
	TPS_POPN(intrp,flen);
	enext += TPS_FRAME_LENGTH(intrp,eframe);
    }
    TPS_MAKEVALUE(v,TPSTYPE_INTEGER,xlen);
    TPS_PUSH(intrp,v);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_counttomark(Tps_Interp* intrp, Tps_Value* args, long nargs)
{
    long i;
    Tps_Value* top = args;

    TPS_GUARANTEE(intrp,1);
    /* need to look for topmost mark */
    for(i=0;i<nargs;i++,top++) {
	if(TPS_ISTYPE(*top,TPSTYPE_MARK)) {
	    Tps_Value v;
	    TPS_MAKEVALUE(v,TPSTYPE_INTEGER,i);
	    TPS_PUSH(intrp,v);
	    return TPSSTAT_OK;
	}
    }
    return(TPSSTAT_UNMATCHEDMARK);
}

Tps_Status
Tps_op_currentdict(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    TPS_GUARANTEE(intrp,1);
    if(TPS_DDEPTH(intrp) == 0) return(TPSSTAT_DICTSTACKUNDERFLOW);
    TPS_PUSH(intrp,TPS_DTOP(intrp));    
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_currentstream(Tps_Interp* /*intrp*/, Tps_Value* /*args*/, long /*nargs*/)
{
return TPSSTAT_SYSTEMERROR;
}

Tps_Status
Tps_op_cvi(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Typeid rtyp;
    rtyp = TPS_TYPE(args[0]);
    switch (rtyp) {
	case TPSTYPE_INTEGER:
	    break;
	case TPSTYPE_STRING:
	    {
		long ir;
		char* next;
		char* s;
#if HASFLOAT
		Tps_Real fr;
#endif
		Tps_String* ss = TPS_STRING_OF(args[0]);
		if(ss->length() == 0)
		    return(TPSSTAT_SYNTAXERROR);
		/* need a null terminated version of ss */
		ss->append((char)0);
		s = ss->contents();
		ss->setlength(ss->length()-1);
		ir = strtol(s,&next,0); /* use c conventions */
		if(*next) { /* didn't use all of it */
#if HASFLOAT
		    fr = strtod(s,&next); /*see if its a real */
		    if(*next)
			return(TPSSTAT_SYNTAXERROR);
		    ir = (int)fr;
#else
		    return TPSSTAT_SYNTAXERROR;
#endif
		}
		TPS_MAKEVALUE(args[0],TPSTYPE_INTEGER,ir);
	    }
	    break;
#if HASFLOAT
	case TPSTYPE_REAL:
	    {
		long i = (int)TPS_REAL_OF(args[0]);
		TPS_MAKEVALUE(args[0],TPSTYPE_INTEGER,i);
	    }
	    break;
#endif
	default:
	    return(TPSSTAT_TYPECHECK);
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_cvlit(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    TPS_SET_EXECUTABLE(args[0],0);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_cvn(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Nameid nm;
    Tps_String* s;
    char* x;
    long len;

    switch (TPS_TYPE(args[0])) {
	case TPSTYPE_NAME:
	    break;
	case TPSTYPE_STRING:
	    s = TPS_STRING_OF(args[0]);
	    s->nullterminate(); // guarantee it
	    len = s->length(); // will not necessarily include the null
	    x = s->contents();
	    x += len;
	    /* test for non-null characters, except trailing ones */
	    /* walk down past trailing nulls */
	    while(len > 0) {
		if(*(--x)) break;
	    }
	    while(len > 0) {
		if(!(*(--x))) {
		    /* whoops, embedded null */
		    return TPSSTAT_TYPECHECK;
		}
	    }
	    nm = TPS_NAMETABLE->newname(s->contents());
	    /* preserve the flags */
	    TPS_MAKEVALUE(args[0],TPSTYPE_NAME,nm);
	default:
	    return TPSSTAT_TYPECHECK;
    }
    return TPSSTAT_OK;
}

#if HASFLOAT
Tps_Status
Tps_op_cvr(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Typeid rtyp;
    rtyp = TPS_TYPE(args[0]);
    switch (rtyp) {
	case TPSTYPE_REAL:
	    break;
	case TPSTYPE_INTEGER:
	    {
		float f = TPS_INTEGER_OF(args[0]);
		TPS_MAKEREAL(args[0],f);
	    }
	    break;
	case TPSTYPE_STRING:
	    {
		double dn;
		float fn;
		char* next;
		Tps_String* s = TPS_STRING_OF(args[0]);
	
		dn = strtod(s->contents(),&next);
		fn = dn;
		if(next != &s->contents()[s->length()])
		    return(TPSSTAT_TYPECHECK);
		TPS_MAKEREAL(args[0],fn);
	    }
	    break;
	default:
	    return(TPSSTAT_TYPECHECK);
    }
    return TPSSTAT_OK;
}
#endif /*HASFLOAT*/

static
char digitchars[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* Isn't there a C function to do the following? */
Tps_Status
Tps_op_cvrs(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Typeid rtyp0;
    Tps_Typeid rtyp2;
    Tps_String* ss;
    long r;
    long n;
    long i;
    char digits[33];

    if(!TPS_ISTYPE(args[2],TPSTYPE_INTEGER)
	|| ((rtyp2 = TPS_TYPE(args[0])) != TPSTYPE_STRING && rtyp2 != TPSTYPE_NULL)
	|| ((rtyp0 = TPS_TYPE(args[2])) != TPSTYPE_INTEGER && rtyp0 != TPSTYPE_REAL))
	return(TPSSTAT_TYPECHECK);
    r = TPS_INTEGER_OF(args[1]);
    if(r < 2 || r > 36) return(TPSSTAT_RANGECHECK);    
    if(rtyp0 == TPSTYPE_INTEGER)
	n = TPS_INTEGER_OF(args[2]);
    else
	n = (int)TPS_REAL_OF(args[2]);
    /* write digits in reverse order */
    i = 0;
    do {
	digits[i++] = digitchars[(n < 0)? -(n % r):(n % r)];
	n = n / r;
    } while(n != 0);
    /* add sign */
    if(n < 0) digits[i++] = '-';
    /* create string result if necessary */
    if(rtyp2 == TPSTYPE_NULL) {
	ss = new Tps_String(i);
	if(!ss) return TPSSTAT_VMERROR;
    } else
	ss = TPS_STRING_OF(args[0]);
    if(ss->length() < i) return(TPSSTAT_RANGECHECK);
    for(r=i-1,i=0;r>=0;r--,i++) {
	ss->contents()[i] = digits[r];
    }
    if(ss->length() > i) {
	/* leave a substring as the result */
	ss = new Tps_String(ss->contents(),i);
	if(!ss) return TPSSTAT_VMERROR;
    }
    TPS_MAKEVALUE(args[2],TPSTYPE_STRING,ss);
    TPS_POPN(intrp,2);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_cvs(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Typeid typdst;
    Tps_String* ss;
    long len;
    const char* result;
    Tps_Value src;

    if((typdst = TPS_TYPE(args[0])) != TPSTYPE_STRING && typdst != TPSTYPE_NULL)
	return(TPSSTAT_TYPECHECK);
    src = args[1];
    intrp->_tokenbuf.rewind();
    switch (TPS_TYPE(src)) {
	case TPSTYPE_BOOLEAN: {
		result = TPS_BOOLEAN_OF(src)?"true":"false";
		len = strlen(result);
	    } break;
 	case TPSTYPE_INTEGER: {
		intrp->_tokenbuf.printf("%d",TPS_INTEGER_OF(src));
		result = intrp->_tokenbuf.contents();
		len = strlen(result);
	    } break;
 	case TPSTYPE_REAL: {
		intrp->_tokenbuf.printf("%f",TPS_REAL_OF(src));
		result = intrp->_tokenbuf.contents();
		len = strlen(result);
	    } break;
 	case TPSTYPE_NAME: {
		result = TPS_NAME_OF(src);
		len = strlen(result);
	    } break;
 	case TPSTYPE_OPERATOR: {
		Tps_Operator* op;
		op = TPS_OPERATOR_OF(src);
		result = op->name();
		len = strlen(result);
	    } break;
 	case TPSTYPE_STRING:
	    {
		Tps_String* s1;
		s1 = TPS_STRING_OF(src);
		result = s1->contents();
		len = s1->length();
	    }
	    break;
	default:
		result = "--nostringval--";
		len = strlen(result);
	    break;
    }    
    /* create string result if necessary */
    if(typdst == TPSTYPE_NULL) {
	ss = new Tps_String(len);
	if(!ss) return TPSSTAT_VMERROR;
    } else {
	if(!TPS_ISWRITEABLE(args[0])) return TPSSTAT_INVALIDACCESS;
	ss = TPS_STRING_OF(args[0]);
    }
    /* move result into the arg string, if room */
    if(ss->length() < len) return(TPSSTAT_RANGECHECK);
    (void)MEMCPY(ss->contents(),result,len);
    TESTERR(ss->setlength(len));
    TPS_MAKEVALUE(args[1],TPSTYPE_STRING,ss);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

/*
cvts (convert to token string): any string|null cvts substring
	Intrp routine does the eqitpvalent of ==, but stores the result
	into a string on the stack (analogous to cvs).
*/
Tps_Status
Tps_op_cvts(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Typeid rtyp0;
    Tps_Status ok;
    Tps_String* ss;
    long len;
    char* result;
    Tps_Value v;

    v = args[1];
    /* recursively print token */
    intrp->_tokenbuf.rewind();
    ok = Tps_cvts1(intrp->_tokenbuf,v,FALSE,-1);
    if(ok != TPSSTAT_OK) return(TPSSTAT_SYSTEMERROR);
    intrp->_tokenbuf.ends(); /* null terminate */
    /* extract the written string */
    /* avoid use of strdup */
    len = strlen(intrp->_tokenbuf.contents());
    result = Tps_malloc(len+1);
    MEMCPY(result,intrp->_tokenbuf.contents(),len+1);
    /* move result into the arg string, if room */
    rtyp0 = TPS_TYPE(args[0]);
    if(rtyp0 == TPSTYPE_STRING) {
	if(!TPS_ISWRITEABLE(args[0])) return TPSSTAT_INVALIDACCESS;
	ss = TPS_STRING_OF(args[0]);
	if(ss->length() < len) return(TPSSTAT_RANGECHECK);
    } else if(rtyp0 == TPSTYPE_NULL) {
	ss = new Tps_String(len);
        if(!ss) return TPSSTAT_VMERROR;
	TPS_MAKEVALUE(args[0],TPSTYPE_STRING,ss);
    } else
	return(TPSSTAT_TYPECHECK);
    (void)MEMCPY(ss->contents(),result,len);
    ss->setlength(len);
    args[1] = args[0];
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_cvx(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    /* add some protection */
    if(!intrp->safe() || !TPS_ISUNSAFE(args[0])) {
	TPS_SET_EXECUTABLE(args[0],1);
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_def(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;
    Tps_Nameid nm;
    Tps_Dictpair pair;

    if(TPS_DDEPTH(intrp) == 0) return TPSSTAT_DICTSTACKUNDERFLOW;
    pair._value = args[0];
    if(TPS_ISTYPE(args[1],TPSTYPE_NAME)
	&& TPS_ISEXARRAY(args[0])) {
	nm = TPS_NAME_OF(args[1]);
	TPS_SET_EXECUTABLE(pair._value,1);
    }
    pair._key = args[1];
    /* insert the key value pair into the table, possibly
       overwriting the existing value */
    ok = Tps_dictstack_define(TPS_DTOSP(intrp),TPS_DDEPTH(intrp),pair,(long*)0);
    if(ok == TPSSTAT_OK) {
	TPS_POPN(intrp,2);
    }
    return ok;
}

Tps_Status
Tps_op_deletefile(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_String* ss;
    if(!TPS_ISTYPE(args[0],TPSTYPE_STRING)) return(TPSSTAT_TYPECHECK);
    ss = TPS_STRING_OF(args[0]);
    TPS_POPN(intrp,1);
    if(ss->nullterminate() != TPSSTAT_OK) return TPSSTAT_FAIL;
    return (unlink(ss->contents()) == 0)?TPSSTAT_OK:TPSSTAT_FAIL;
}

Tps_Status
Tps_op_dict(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Dict* d;
    long len;

    if(TPS_TYPEID_OF(args[0]) != TPSTYPE_INTEGER) return TPSSTAT_TYPECHECK;
    len = TPS_INTEGER_OF(args[0]);
    if(len < 0) return(TPSSTAT_RANGECHECK);
    d = new Tps_Dict_Tcl(len);
    if(!d) return(TPSSTAT_SYSTEMERROR);
    TPS_MAKEVALUE(args[0],TPSTYPE_DICT,d);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_dictstack(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Array* a;
    Tps_Value* ds;
    Tps_Value* cnts;
    long ddepth;

    if(!TPS_ISTYPE(args[0],TPSTYPE_ARRAY)) return(TPSSTAT_TYPECHECK);
    a = TPS_ARRAY_OF(args[0]);
    ddepth = TPS_DDEPTH(intrp);
    if(a->length() < ddepth) return(TPSSTAT_RANGECHECK);
    ds = TPS_DTOSP(intrp);
    cnts = &a->contents()[ddepth];
    /* need to reverse the dict stack into the array */
    while(ddepth--) {
	--cnts;
	*cnts = *ds;
	ds++;
    }
    TESTERR(a->setlength(TPS_DDEPTH(intrp)));
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_div(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
#if HASFLOAT
    Tps_Typeid rtyp0;
    Tps_Typeid rtyp1;
    Tps_Real fr1;
    Tps_Real fr0;

    switch (rtyp1 = TPS_TYPE(args[0])) {
	case TPSTYPE_INTEGER: fr1 = (Tps_Real)TPS_INTEGER_OF(args[0]); break;
	case TPSTYPE_REAL: fr1 = TPS_REAL_OF(args[0]); break;
	default: return(TPSSTAT_TYPECHECK);
    }
    switch (rtyp0 = TPS_TYPE(args[1])) {
	case TPSTYPE_INTEGER: fr0 = (Tps_Real)TPS_INTEGER_OF(args[1]); break;
	case TPSTYPE_REAL: fr0 = TPS_REAL_OF(args[1]); break;
	default: return(TPSSTAT_TYPECHECK);
    }
    fr0 = fr0 / fr1;
    /* now, see if we can make it back to an integer */
    if(rtyp1 == TPSTYPE_INTEGER && rtyp0 == TPSTYPE_INTEGER) {
	long ir = (int)fr0;
	if(fr0 == ir)
	    TPS_MAKEVALUE(args[1],TPSTYPE_INTEGER,ir);
	else
	    TPS_MAKEREAL(args[1],fr0);
    } else {
	TPS_MAKEREAL(args[1],fr0);
    }
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
#else /*!HASFLOAT*/
    if(TPS_TYPE(args[0]) != TPSTYPE_INTEGER
       || TPS_TYPE(args[1]) != TPSTYPE_INTEGER)
	return TPSSTAT_TYPECHECK;
    long ir;
    ir = TPS_INTEGER_OF(args[1]) / TPS_INTEGER_OF(args[0]);
    TPS_MAKEVALUE(args[1],TPSTYPE_INTEGER,ir);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
#endif /*HASFLOAT*/
}

Tps_Status
Tps_op_dload(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Dict* d;
    Tps_Value v;
    long i;
    Tps_Dictpair* pairp;

    v = TPS_POP(intrp);
    args++; /* keep in synch */
    if(!TPS_ISTYPE(v,TPSTYPE_DICT)) return(TPSSTAT_TYPECHECK);
    d = TPS_DICT_OF(v);
    i = d->length();
    /* need space for all elements in dict + dict*/
    TPS_GUARANTEE(intrp,2*i);
    /* transfer the contents to the stack */
    long r = d->range();
    for(i=0;i<r;i++) {
	Tps_Status ok = d->ith(i,pairp);
	if(ok == TPSSTAT_UNDEFINED) continue; /* no entry at this index */
	if(ok != TPSSTAT_OK) return ok; /* some form of error */
	TPS_PUSH(intrp,pairp->_key);
	TPS_PUSH(intrp,pairp->_value);
    }
    /* leave the dict as top element */
    TPS_PUSH(intrp,v);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_dup(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    TPS_PUSH(intrp,args[0]);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_dupn(Tps_Interp* intrp, Tps_Value* args, long argcnt)
{
    /* a1...an n dupn a1..an a1..an ; uses copy */
    if(!TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return TPSSTAT_TYPECHECK;
    return Tps_op_copy(intrp,args,argcnt);
}

Tps_Status
Tps_op_end(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    if(TPS_DDEPTH(intrp) == 0) return(TPSSTAT_DICTSTACKUNDERFLOW);
    TPS_DPOPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_eq(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    int comparison = 0;
    Tps_Typeid t0;
    Tps_Typeid t1;

    if((t1 = TPS_TYPE(args[0])) != (t0 = TPS_TYPE(args[1]))) {
	/* need to do some special checking */
	if(t1 == TPSTYPE_INTEGER && t0 == TPSTYPE_REAL) goto ok;
	if(t1 == TPSTYPE_REAL && t0 == TPSTYPE_INTEGER) goto ok;
	if(t1 == TPSTYPE_STRING && t0 == TPSTYPE_NAME) goto ok;
	if(t1 == TPSTYPE_NAME && t0 == TPSTYPE_STRING) goto ok;
	goto done;
    }
ok:
    comparison = (Tps_compare(args[1],args[0]) == 0)?1:0;
done:
    TPS_MAKEVALUE(args[1],TPSTYPE_BOOLEAN,comparison);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_eqeqsign(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    char* result;
    Tps_Value v;
    long len;
    Tps_Stream* strm;

    intrp->_tokenbuf.rewind();
    v = args[0];
    /* recursively print token */
    TESTERR(Tps_cvts1(intrp->_tokenbuf,v,FALSE,-1));
    /* extract the written string */
    intrp->_tokenbuf.ends();
    /* avoid use of strdup */
    len = strlen(intrp->_tokenbuf.contents());
    result = Tps_malloc(len+1);
    MEMCPY(result,intrp->_tokenbuf.contents(),len+1);
    /* write the result to _stdout */
    (void)(intrp->stdstream(Tps_stdout))->write(result);
    Tps_free(result);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_exch(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Value temp;

    temp = args[0];
    args[0] = args[1];
    args[1] = temp;
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_exec(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Value v = args[0];
    TPS_POPN(intrp,1);
    return Tps_create_source(intrp,v,FALSE);
}

Tps_Status
Tps_op_execstack(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Value v;
    Tps_Status ok;
    long count;
    Tps_Array* a;
    Tps_Value* e;

    v = args[0];
    if(!TPS_ISTYPE(v,TPSTYPE_ARRAY)) return TPSSTAT_TYPECHECK;
    ok = Tps_export_exec(intrp,count,intrp->safe()?Tps_pass_state:Tps_pass_all);
    if(ok == TPSSTAT_OK) {
	a = TPS_ARRAY_OF(v);
	if(a->length() < count) return TPSSTAT_RANGECHECK;
	e = TPS_TOSP(intrp);
	MEMCPY((char*)a->contents(),e,a->length()*sizeof(Tps_Value));
	a->setlength(count);
	TPS_POPN(intrp,count);
    }
    return ok;
}

Tps_Status
Tps_op_executeonly(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    switch (TPS_TYPE(args[0])) {
	case TPSTYPE_STRING:
	case TPSTYPE_ARRAY:
	case TPSTYPE_DICT:
	case TPSTYPE_STREAM:
		TPS_SET_ACCESS(args[0],Tps_access_noaccess);
		TPS_SET_EXECUTABLE(args[0],1);
		break;
	case TPSTYPE_OPERATOR:
	case TPSTYPE_NULL:
	case TPSTYPE_MARK:
	case TPSTYPE_BOOLEAN:
	case TPSTYPE_INTEGER:
	case TPSTYPE_REAL:
	case TPSTYPE_NAME:
	default: break;
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_exit(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    /* do /exit throw */
    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,TPS__NM(TPS_NMEXIT));
    return TPS_CALL_PRIM(intrp,Tps_op_throw);
}

Tps_Status
Tps_op_for(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Value initial;
    Tps_Value incr;
    Tps_Value lim;
    Tps_Value proc;
    Tps_Frame_For* frame;
    int direction;
#if HASFLOAT
    Tps_Typeid tinit; /*= initial value */
    Tps_Typeid tinc;
    Tps_Typeid tlim;

    proc = args[0];
    lim = args[1];
    incr = args[2];
    initial = args[3];
    tlim = TPS_TYPE(lim);
    tinc = TPS_TYPE(incr);
    tinit = TPS_TYPE(initial);
    if(tlim != TPSTYPE_INTEGER && tlim != TPSTYPE_REAL
       && tinc != TPSTYPE_INTEGER && tinc != TPSTYPE_REAL
       && tinit != TPSTYPE_INTEGER && tinit != TPSTYPE_REAL)
	return(TPSSTAT_TYPECHECK);

    direction = 1; /* guess */
    /* special case the all integer case */
    if(tlim == TPSTYPE_INTEGER && tinc == TPSTYPE_INTEGER && tinit == TPSTYPE_INTEGER) {
	if(TPS_INTEGER_OF(incr) < 0) direction = 0;
    } else { /* coerce all values to real */
	Tps_Real f;
	if(tinit == TPSTYPE_INTEGER) {
	    f = TPS_INTEGER_OF(initial);
	} else {
	    f = TPS_REAL_OF(initial);
	}
	TPS_MAKEREAL(initial,f);
	if(tinc == TPSTYPE_INTEGER) {
	    f = TPS_INTEGER_OF(incr);
	} else {
	    f = TPS_REAL_OF(incr);
	}
	TPS_MAKEREAL(incr,f);
	/* figure out direction */
	if(f < 0.0) direction = 0;
	if(tlim == TPSTYPE_INTEGER) {
	    f = TPS_INTEGER_OF(lim);
	} else {
	    f = TPS_REAL_OF(lim);
	}
	TPS_MAKEREAL(lim,f);
    }
#else /*!HASFLOAT*/
    proc = args[0];
    lim = args[1];
    incr = args[2];
    initial = args[3];
    if(TPS_TYPE(lim) != TPSTYPE_INTEGER
       || TPS_TYPE(incr) != TPSTYPE_INTEGER
       || TPS_TYPE(initial) != TPSTYPE_INTEGER)
	return(TPSSTAT_TYPECHECK);

    direction = (TPS_INTEGER_OF(incr) < 0)?0:1;
#endif /*HASFLOAT*/
    /* now save all intrp info away in exec stack */
    if(!(frame = (Tps_Frame_For*)Tps_create_frame(intrp,&Tps_handler_for,sizeof(Tps_Frame_For)))) return TPSSTAT_VMERROR;
    frame->_body = proc;
    frame->_dir = direction;
    frame->_limit = lim;
    frame->_incr = incr;
    frame->_current = initial;
    TPS_POPN(intrp,4);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_for_unwind(TPS_UNWIND_ARGS)
{
    Tps_Frame_For* f = (Tps_Frame_For*)frame;
    Tps_Status ok = Tps_unwind_frame(intrp,f);
    if(ok != TPSSTAT_OK) return ok;  
    if(thrown
	&& TPS_ISTYPE(intrp->_throwflag,TPSTYPE_NAME)
	&& TPS_NAME_OF(intrp->_throwflag) == TPS_NM(TPS_NMEXIT)) {
	return TPSSTAT_STOP;
    }
    return ok;
}

static
Tps_Status
Tps_for_reenter(TPS_REENTER_ARGS)
{
    Tps_Frame_For* f = (Tps_Frame_For*)frame;
    long direction = f->_dir;
    Tps_Value curr = f->_current;
#if HASFLOAT
    switch (TPS_TYPE(curr)) {
	case TPSTYPE_INTEGER: {
	    long ci = TPS_INTEGER_OF(curr);
	    long li = TPS_INTEGER_OF(f->_limit);
	    long ii = TPS_INTEGER_OF(f->_incr);
	    if((direction && ci > li)
	       || (!direction && ci < li)) return TPSSTAT_POPFRAME;
	    /* increment _current */
	    ci = ci + ii;
	    TPS_MAKEVALUE(f->_current,TPSTYPE_INTEGER,ci);
	} break;
	case TPSTYPE_REAL:
	{
	    Tps_Real cf = TPS_REAL_OF(curr);
	    Tps_Real lf = TPS_REAL_OF(f->_limit);
	    Tps_Real iff = TPS_REAL_OF(f->_incr);
	    if((direction && cf > lf)
	       || (!direction && cf < lf)) return TPSSTAT_POPFRAME;
	    /* increment _current */
	    cf = cf + iff;
	    TPS_MAKEREAL(f->_current,cf);
	} break;
	default:
	    return TPSSTAT_SYSTEMERROR;
    }
#else /*!HASFLOAT*/
    long ci = TPS_INTEGER_OF(curr);
    long li = TPS_INTEGER_OF(f->_limit);
    long ii = TPS_INTEGER_OF(f->_incr);
    if((direction && ci > li)
       || (!direction && ci < li)) return TPSSTAT_POPFRAME;
    /* increment _current */
    ci = ci + ii;
    TPS_MAKEVALUE(f->_current,TPSTYPE_INTEGER,ci);
#endif /*HASFLOAT*/
    /* push original current index value */
    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,curr);
    Tps_create_source(intrp,f->_body);
    return TPSSTAT_RETRYFRAME;
}

static
Tps_Status
Tps_for_trace(TPS_TRACE_ARGS)
{
    Tps_Frame_For* f = (Tps_Frame_For*)frame;
    Tps_trace0(intrp,strm,f);
    if(TPS_ISTYPE(f->_current,TPSTYPE_INTEGER)) {
	strm->printf(" %d",TPS_INTEGER_OF(f->_current));
	strm->printf(" to %d",TPS_INTEGER_OF(f->_limit));
	strm->printf(" by %d:",TPS_INTEGER_OF(f->_incr));
    } else { /* Assume real valued iterate */
	strm->printf(" %f",TPS_REAL_OF(f->_current));
	strm->printf(" to %f",TPS_REAL_OF(f->_limit));
	strm->printf(" by %f:",TPS_REAL_OF(f->_incr));
    }
    (void)Tps_cvts1(*strm,f->_body,TRUE,-1);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_for_export(TPS_EXPORT_ARGS)
{
    Tps_Frame_For* f = (Tps_Frame_For*)frame;

    /* assume length and kind will be dumped by caller */
    TPS_GUARANTEE(intrp,3);
    TPS_PUSH(intrp,f->_current);
    TPS_PUSH(intrp,f->_limit);
    TPS_PUSH(intrp,f->_incr);
    count += 3;
    return Tps_loop_export(intrp,frame,count,flags);
}

static
Tps_Status
Tps_for_import(TPS_IMPORT_ARGS)
{
    Tps_Frame_For* f = (Tps_Frame_For*)fr;
    Tps_Status ok;
    Tps_Value v;

    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt < 3) return TPSSTAT_RANGECHECK;
    cnt -= 3;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_For*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_For));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    }
    if((ok = Tps_loop_import(intrp,h,cnt,f,flags)) != TPSSTAT_OK) return ok;
    f->_incr = TPS_POP(intrp);
    f->_limit = TPS_POP(intrp);
    f->_current = TPS_POP(intrp);
    v = TPS_POP(intrp);
    if(TPS_ISTYPE(f->_current,TPSTYPE_INTEGER)
	&& TPS_ISTYPE(f->_limit,TPSTYPE_INTEGER)
	&& TPS_ISTYPE(f->_incr,TPSTYPE_INTEGER)) {
	f->_dir = TPS_INTEGER_OF(f->_incr) >= 0;
    } else if(TPS_ISTYPE(f->_current,TPSTYPE_REAL)
	&& TPS_ISTYPE(f->_limit,TPSTYPE_REAL)
	&& TPS_ISTYPE(f->_incr,TPSTYPE_REAL)) {
	f->_dir = TPS_REAL_OF(f->_incr) >= 0;
    } else
	return TPSSTAT_TYPECHECK;
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_for_parms = {"For"};

Tps_Handler Tps_handler_for = {
	&Tps_for_parms,
	Tps_for_unwind,
	Tps_for_reenter,
	Tps_for_trace,
	Tps_loop_mark,
	Tps_loop_export,
	Tps_loop_import
};

Tps_Status
Tps_op_forall(Tps_Interp* intrp,  Tps_Value* args, long /*nargs*/)
{
    /* post form is:
	compound body forall -> -
    */
    Tps_Frame_Forall* f;

    switch (TPS_TYPE(args[1])) {
	case TPSTYPE_ARRAY:
	case TPSTYPE_STRING:
	case TPSTYPE_DICT:
	if(!(f = (Tps_Frame_Forall*)Tps_create_frame(intrp,&Tps_handler_forall,sizeof(Tps_Frame_Forall)))) return TPSSTAT_VMERROR;	
	    f->_body = args[0];
	    f->_iterate = args[1];
	    f->_current = 0;
	    TPS_POPN(intrp,2); /* clear stack */
	    return TPSSTAT_OK;
	default:
	    return(TPSSTAT_TYPECHECK);
    }
/*NOTREACHED*/
}

static
Tps_Status
Tps_forall_unwind(TPS_UNWIND_ARGS)
{
    Tps_Frame_Forall* f = (Tps_Frame_Forall*)frame;
    Tps_Status ok = Tps_unwind_frame(intrp,f);
    if(ok != TPSSTAT_OK) return ok;  
    if(thrown
	&& TPS_ISTYPE(intrp->_throwflag,TPSTYPE_NAME)
	&& TPS_NAME_OF(intrp->_throwflag) == TPS_NM(TPS_NMEXIT)) {
	return TPSSTAT_STOP;
    }
    return ok;
}

static
Tps_Status
Tps_forall_reenter(TPS_REENTER_ARGS)
{
    Tps_Frame_Forall* f = (Tps_Frame_Forall*)frame;
    long n = f->_current;
    Tps_Array* a;
    Tps_String* s;
    Tps_Dict* d;
    Tps_Value v;
    Tps_Dictpair* pairp;

    /* subvert and restart if compound is not exhausted */
    TPS_GUARANTEE(intrp,2); /* slight overkill */
    switch (TPS_TYPE(f->_iterate)) {
	case TPSTYPE_ARRAY:
	    a = TPS_ARRAY_OF(f->_iterate);
	    if(n < 0 || n >= a->length()) return TPSSTAT_POPFRAME;
	    TPS_PUSH(intrp,a->contents()[n]);
	    break;
	case TPSTYPE_STRING:
	    s = TPS_STRING_OF(f->_iterate);
	    if(n < 0 || n >= s->length()) return TPSSTAT_POPFRAME;
	    TPS_MAKEVALUE(v,TPSTYPE_INTEGER,s->contents()[n]);
	    TPS_PUSH(intrp,v);
	    break;
	case TPSTYPE_DICT:
	    d = TPS_DICT_OF(f->_iterate);
nexti:
	    intrp->_status = d->ith(n,pairp);
	    switch(intrp->_status) {
		case TPSSTAT_OK:
		    break;
		case TPSSTAT_UNDEFINED:
		    n++;
		    goto nexti;
		case TPSSTAT_RANGECHECK:
		    return TPSSTAT_POPFRAME;
		default:
		    return intrp->_status;
	    };
	    TPS_PUSH(intrp,pairp->_key);
	    TPS_PUSH(intrp,pairp->_value);
	    break;
	default:
	    return TPSSTAT_SYSTEMERROR;
    }
    // fix count for next time
    f->_current = n+1;
    /* rexecute body */
    Tps_create_source(intrp,f->_body);
    return TPSSTAT_RETRYFRAME;
}

static
Tps_Status
Tps_forall_trace(TPS_TRACE_ARGS)
{
    Tps_Frame_Forall* f = (Tps_Frame_Forall*)frame;
    Tps_trace0(intrp,strm,f);
    strm->printf(" index=%d",f->_current);
    strm->write("; iterate=");
    (void)Tps_cvts1(*strm,f->_iterate,TRUE,-1);
    strm->write("; body=");
    (void)Tps_cvts1(*strm,f->_body,TRUE,-1);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_forall_mark(TPS_MARK_ARGS)
{
    Tps_Frame_Forall* f = (Tps_Frame_Forall*)frame;
    (void)Tps_loop_mark(intrp,frame);
    Tps_mark(f->_iterate);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_forall_export(TPS_EXPORT_ARGS)
{
    Tps_Frame_Forall* f = (Tps_Frame_Forall*)frame;

    /* assume length and kind will be dumped by caller */
    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,f->_iterate);
    count++;
    return Tps_repeat_export(intrp,frame,count,flags);
}

static
Tps_Status
Tps_forall_import(TPS_IMPORT_ARGS)
{
    Tps_Frame_Forall* f = (Tps_Frame_Forall*)fr;
    Tps_Status ok;

    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt < 1) return TPSSTAT_RANGECHECK;
    cnt--;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_Forall*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_Forall));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    }
    if((ok = Tps_repeat_import(intrp,h,cnt,f,flags)) != TPSSTAT_OK) return ok;
    f->_iterate = TPS_POP(intrp);
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_forall_parms = {"Forall"};

Tps_Handler Tps_handler_forall = {
	&Tps_forall_parms,
	Tps_forall_unwind,
	Tps_forall_reenter,
	Tps_forall_trace,
	Tps_forall_mark,
	Tps_forall_export,
	Tps_forall_import
};

Tps_Status
Tps_op_flushstream(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Stream* strm;
    if(!TPS_ISTYPE(args[0],TPSTYPE_STREAM)) return(TPSSTAT_TYPECHECK);
    strm = TPS_STREAM_OF(args[0]);
    TPS_POPN(intrp,1);
    strm->flush();
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_ge(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    int comparison;
    Tps_Typeid t0;
    Tps_Typeid t1;

    if((t1 = TPS_TYPE(args[0])) != (t0 = TPS_TYPE(args[1]))) {
	/* need to do some special checking */
	if(t0 == TPSTYPE_INTEGER && t1 == TPSTYPE_REAL) goto ok;
	if(t0 == TPSTYPE_REAL && t1 == TPSTYPE_INTEGER) goto ok;
	if(t0 == TPSTYPE_STRING && t1 == TPSTYPE_NAME) goto ok;
	if(t0 == TPSTYPE_NAME && t1 == TPSTYPE_STRING) goto ok;
	goto badtype;
    }
ok:
    comparison = (Tps_compare(args[1],args[0]) >= 0)?1:0;
    TPS_MAKEVALUE(args[1],TPSTYPE_BOOLEAN,comparison);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
badtype:
    return(TPSSTAT_TYPECHECK);
}

Tps_Status
Tps_op_get(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Typeid t0;
    Tps_Typeid t1;
    long index;
    Tps_Status ok;

    t0 = TPS_TYPE(args[1]);
    t1 = TPS_TYPE(args[0]);
    switch (t0) {
	case TPSTYPE_ARRAY:
	    {
		Tps_Array* a = TPS_ARRAY_OF(args[1]);
		if(t1 != TPSTYPE_INTEGER) goto badtype;
		index = TPS_INTEGER_OF(args[0]);
		if(index < 0 || index >= a->length()) goto badrange;
		args[1] = a->contents()[index];
	    }
	    break;
	case TPSTYPE_DICT:
	    {
		Tps_Dict* d = TPS_DICT_OF(args[1]);
		Tps_Dictpair* pairp;
		ok = d->lookup(args[0],&pairp);
		if(ok != TPSSTAT_OK) return ok;
		args[1] = pairp->_value;
	    }
	    break;
	case TPSTYPE_STRING:
	    {
		Tps_String* s = TPS_STRING_OF(args[1]);
		if(t1 != TPSTYPE_INTEGER) goto badtype;
		index = TPS_INTEGER_OF(args[0]);
		if(index < 0 || index >= s->length()) goto badrange;
		TPS_MAKEVALUE(args[1],TPSTYPE_INTEGER,s->contents()[index]);
	    }
	    break;
	default:
	    goto badtype;
    }
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
badtype:
    return(TPSSTAT_TYPECHECK);
badrange:
    return(TPSSTAT_RANGECHECK);
}

Tps_Status
Tps_op_getinterval(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    long index;
    long count;

    if(!TPS_ISTYPE(args[0],TPSTYPE_INTEGER) || !TPS_ISTYPE(args[1],TPSTYPE_INTEGER))
	return(TPSSTAT_TYPECHECK);
    count = TPS_INTEGER_OF(args[0]);
    index = TPS_INTEGER_OF(args[1]);
    switch (TPS_TYPE(args[2])) {
	case TPSTYPE_ARRAY:
	    {
		Tps_Array* a = TPS_ARRAY_OF(args[2]);
		if(index < 0 || count < 0 || index+count > a->length())
		    return(TPSSTAT_RANGECHECK);
		Tps_Array* anew = new Tps_Array(0);
		anew->append(a->contents()+index,count);
		TPS_MAKEVALUE(args[2],TPSTYPE_ARRAY,anew);
	    }
	    break;
	case TPSTYPE_STRING:
	    {
		Tps_String* s = TPS_STRING_OF(args[2]);
		if(index < 0 || count < 0 || index+count > s->length())
		    return(TPSSTAT_RANGECHECK);
		Tps_String* snew = new Tps_String;
		snew->append(s->contents()+index,count);
		TPS_MAKEVALUE(args[2],TPSTYPE_STRING,snew);
	    }
	    break;
	default:
	    return(TPSSTAT_TYPECHECK);
    }
    TPS_POPN(intrp,2);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_gt(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    int comparison;
    Tps_Typeid t0;
    Tps_Typeid t1;

    if((t1 = TPS_TYPE(args[0])) != (t0 = TPS_TYPE(args[1]))) {
	/* need to do some special checking */
	if(t0 == TPSTYPE_INTEGER && t1 == TPSTYPE_REAL) goto ok;
	if(t0 == TPSTYPE_REAL && t1 == TPSTYPE_INTEGER) goto ok;
	if(t0 == TPSTYPE_STRING && t1 == TPSTYPE_NAME) goto ok;
	if(t0 == TPSTYPE_NAME && t1 == TPSTYPE_STRING) goto ok;
	goto badtype;
    }
ok:
    comparison = (Tps_compare(args[1],args[0]) > 0)?1:0;
    TPS_MAKEVALUE(args[1],TPSTYPE_BOOLEAN,comparison);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
badtype:
    return(TPSSTAT_TYPECHECK);
}

Tps_Status
Tps_op_idiv(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    if(!TPS_ISTYPE(args[1],TPSTYPE_INTEGER)
        || !TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return(TPSSTAT_TYPECHECK);
    TPS_INTEGER_OF(args[1]) /= TPS_INTEGER_OF(args[0]);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_if(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    int tf;
    Tps_Value proc;

    if(!TPS_ISTYPE(args[1],TPSTYPE_BOOLEAN)) return(TPSSTAT_TYPECHECK);
    tf = TPS_BOOLEAN_OF(args[1]);
    proc = args[0];
    TPS_POPN(intrp,2);
    if(tf) {
	Tps_Status ok = Tps_create_source(intrp,proc);
	if(ok != TPSSTAT_OK) return ok;
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_ifelse(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    int tf;
    Tps_Value proc;

    if(!TPS_ISTYPE(args[2],TPSTYPE_BOOLEAN)) return(TPSSTAT_TYPECHECK);
    tf = TPS_BOOLEAN_OF(args[2]);
    proc = tf?args[1]:args[0];
    TPS_POPN(intrp,3);
    return Tps_create_source(intrp,proc);
}

Tps_Status
Tps_op_index(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    long count;

    if(!TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return(TPSSTAT_TYPECHECK);
    count = TPS_INTEGER_OF(args[0]);
    if(TPS_DEPTH(intrp) <= 1+count) return(TPSSTAT_RANGECHECK);
    args[0] = args[count+1];
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_interrupt(Tps_Interp* /*intrp*/, Tps_Value* /*args*/, long /*nargs*/)
{
    return TPSSTAT_INTERRUPT;
}

Tps_Status
Tps_op_known(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Dict* d;
    Tps_Dictpair* pairp;
    Tps_Status found;

    if(!TPS_ISTYPE(args[1],TPSTYPE_DICT)) return(TPSSTAT_TYPECHECK);
    d = TPS_DICT_OF(args[1]);
    found = d->lookup(args[0],&pairp);
    switch (found) {
	case TPSSTAT_OK:
	    args[1] = TPS__CONST(TPS__TRUE);
	    break;
	case TPSSTAT_UNDEFINED:
	    args[1] = TPS__CONST(TPS__FALSE);
	    break;
	default:
	    return found;
    }
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_le(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    int comparison;
    Tps_Typeid t0;
    Tps_Typeid t1;

    if((t1 = TPS_TYPE(args[0])) != (t0 = TPS_TYPE(args[1]))) {
	/* need to do some special checking */
	if(t0 == TPSTYPE_INTEGER && t1 == TPSTYPE_REAL) goto ok;
	if(t0 == TPSTYPE_REAL && t1 == TPSTYPE_INTEGER) goto ok;
	if(t0 == TPSTYPE_STRING && t1 == TPSTYPE_NAME) goto ok;
	if(t0 == TPSTYPE_NAME && t1 == TPSTYPE_STRING) goto ok;
	goto badtype;
    }
ok:
    comparison = (Tps_compare(args[1],args[0]) <= 0)?1:0;
    TPS_MAKEVALUE(args[1],TPSTYPE_BOOLEAN,comparison);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
badtype:
    return(TPSSTAT_TYPECHECK);
}

Tps_Status
Tps_op_length(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    long len;

    switch (TPS_TYPE(args[0])) {
	case TPSTYPE_STRING:
	    len = TPS_STRING_OF(args[0])->length(); break;
	case TPSTYPE_ARRAY:
	    len = TPS_ARRAY_OF(args[0])->length(); break;
	case TPSTYPE_DICT:
	    len = TPS_DICT_OF(args[0])->length(); break;
	default:
	    return(TPSSTAT_TYPECHECK);
    }
    TPS_MAKEVALUE(args[0],TPSTYPE_INTEGER,len);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_load(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;
    Tps_Dictpair* pairp;

    ok = Tps_dictstack_lookup(TPS_DTOSP(intrp),TPS_DDEPTH(intrp),
			      args[0],NULL,&pairp);
    if(ok != TPSSTAT_OK) return ok;
    args[0] = pairp->_value;
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_loop(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    Tps_Frame_Loop* f;

    if(!(f = (Tps_Frame_Loop*)Tps_create_frame(intrp,&Tps_handler_loop,sizeof(Tps_Frame_Loop)))) return TPSSTAT_VMERROR;
    f->_body = TPS_POP(intrp);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_loop_unwind(TPS_UNWIND_ARGS)
{
    Tps_Frame_Loop* f = (Tps_Frame_Loop*)frame;
    Tps_Status ok = Tps_unwind_frame(intrp,f);
    if(ok != TPSSTAT_OK) return ok;  
    if(thrown
	&& TPS_ISTYPE(intrp->_throwflag,TPSTYPE_NAME)
	&& TPS_NAME_OF(intrp->_throwflag) == TPS_NM(TPS_NMEXIT)) {
	return TPSSTAT_STOP;
    }
    return ok;
}

static
Tps_Status
Tps_loop_reenter(TPS_REENTER_ARGS)
{
    Tps_Frame_Loop* f = (Tps_Frame_Loop*)frame;
    Tps_create_source(intrp,f->_body);
    return TPSSTAT_RETRYFRAME;
}

static
Tps_Status
Tps_loop_trace(TPS_TRACE_ARGS)
{
    Tps_Frame_Loop* f = (Tps_Frame_Loop*)frame;
    Tps_trace0(intrp,strm,f);
    strm->write(" body=");
    (void)Tps_cvts1(*strm,f->_body,TRUE,-1);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_loop_mark(TPS_MARK_ARGS1)
{
    Tps_Frame_Loop* f = (Tps_Frame_Loop*)frame;
    Tps_mark(f->_body);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_loop_export(TPS_EXPORT_ARGS)
{
    Tps_Frame_Loop* f = (Tps_Frame_Loop*)frame;

    /* assume length and kind will be dumped by caller */
    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,f->_body);
    count++;
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_loop_import(TPS_IMPORT_ARGS1)
{
    Tps_Frame_Loop* f = (Tps_Frame_Loop*)fr;

    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt < 1) return TPSSTAT_RANGECHECK;
    cnt--;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_Loop*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_Loop));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    }
    f->_body = TPS_POP(intrp);
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_loop_parms = {"Loop"};

Tps_Handler Tps_handler_loop = {
	&Tps_loop_parms,
	Tps_loop_unwind,
	Tps_loop_reenter,
	Tps_loop_trace,
	Tps_loop_mark,
	Tps_loop_export,
	Tps_loop_import
};

Tps_Status
Tps_op_lt(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    int comparison;
    Tps_Typeid t0;
    Tps_Typeid t1;

    if((t1 = TPS_TYPE(args[0])) != (t0 = TPS_TYPE(args[1]))) {
	/* need to do some special checking */
	if(t0 == TPSTYPE_INTEGER && t1 == TPSTYPE_REAL) goto ok;
	if(t0 == TPSTYPE_REAL && t1 == TPSTYPE_INTEGER) goto ok;
	if(t0 == TPSTYPE_STRING && t1 == TPSTYPE_NAME) goto ok;
	if(t0 == TPSTYPE_NAME && t1 == TPSTYPE_STRING) goto ok;
	goto badtype;
    }
ok:
    comparison = (Tps_compare(args[1],args[0]) < 0)?1:0;
    TPS_MAKEVALUE(args[1],TPSTYPE_BOOLEAN,comparison);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
badtype:
    return(TPSSTAT_TYPECHECK);
}

Tps_Status
Tps_op_maxlength(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    long maxlen;

    if(!TPS_ISTYPE(args[0],TPSTYPE_DICT)) return(TPSSTAT_TYPECHECK);
    maxlen = TPS_DICT_OF(args[0])->maxlength();
    TPS_MAKEVALUE(args[0],TPSTYPE_INTEGER,maxlen);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_mod(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
#if HASFLOAT
    Tps_Typeid rtyp0;
    Tps_Typeid rtyp1;
    long ir;
    Tps_Real fr;

    rtyp0 = TPS_TYPE(args[1]);
    rtyp1 = TPS_TYPE(args[0]);
    switch (rtyp1) {
	case TPSTYPE_INTEGER:
	    switch (rtyp0) {
		case TPSTYPE_INTEGER:
		    ir = TPS_INTEGER_OF(args[1]) % TPS_INTEGER_OF(args[0]);
		    goto int_result;
		case TPSTYPE_REAL:
		    fr = TPS_INTEGER_OF(args[0]);
		    fr = fmod(TPS_REAL_OF(args[1]),fr);
		    goto real_result;
		default:
		    goto wrongtype;
	    }
	    break;
	case TPSTYPE_REAL:
	    switch (rtyp0) {
		case TPSTYPE_INTEGER:
		    fr = TPS_INTEGER_OF(args[1]);
		    fr = fmod(fr,TPS_REAL_OF(args[0]));
		    goto real_result;
		case TPSTYPE_REAL:
		    fr = fmod(TPS_REAL_OF(args[1]),TPS_REAL_OF(args[0]));
		    goto real_result;
		default:
		    goto wrongtype;
	    }
	    break;
	default:
	    goto wrongtype;
    }
int_result:
    TPS_MAKEVALUE(args[1],TPSTYPE_INTEGER,ir);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
real_result:
    TPS_MAKEREAL(args[1],fr);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
wrongtype:
    return(TPSSTAT_TYPECHECK);
#else /*!HASFLOAT*/
    if(TPS_TYPE(args[1]) != TPSTYPE_INTEGER
       || TPS_TYPE(args[0]) != TPSTYPE_INTEGER)
	return TPSSTAT_TYPECHECK;
    long ir = TPS_INTEGER_OF(args[1]) % TPS_INTEGER_OF(args[0]);
    TPS_MAKEVALUE(args[1],TPSTYPE_INTEGER,ir);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
#endif /*HASFLOAT*/
}

Tps_Status
Tps_op_mul(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Value result;
    long ir = 1;
    long i;
#if HASFLOAT
    Tps_Real fr = 1.0;
    int isfloat = 0;
#endif

    /* this is defined to potentially add any number of args */
    for(i=0;i<2;i++) {
	switch (TPS_TYPE(args[i])) {
	    case TPSTYPE_INTEGER: ir *= TPS_INTEGER_OF(args[i]); break;
#if HASFLOAT
	    case TPSTYPE_REAL: fr *= TPS_REAL_OF(args[i]); isfloat = 1; break;
#endif
	    default: return TPSSTAT_TYPECHECK;
	}
    }
#if HASFLOAT
    if(isfloat) {
	fr *= (Tps_Real)ir;
	TPS_MAKEREAL(result,fr);
    } else
#endif
    {
	TPS_MAKEVALUE(result,TPSTYPE_INTEGER,ir);
    }
    TPS_RETURNVAL(intrp,result,2);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_ne(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok = Tps_op_eq(intrp,args,2);
    if(ok != TPSSTAT_OK) return ok;
    args++;
    TPS_CHANGEVALUE(args[0],TPSTYPE_BOOLEAN,!TPS_BOOLEAN_OF(args[0]));
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_neg(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
#if HASFLOAT
    Tps_Typeid rtyp;

    rtyp = TPS_TYPE(args[0]);
    switch (rtyp) {
	case TPSTYPE_INTEGER:
	    {
		long i = TPS_INTEGER_OF(args[0]);
		TPS_CHANGEVALUE(args[0],TPSTYPE_INTEGER,- i);
	    }
	    break;
	case TPSTYPE_REAL:
	    {
		Tps_Real f = TPS_REAL_OF(args[0]);
		TPS_CHANGEREAL(args[0],- f);
	    }
	    break;
	default:
	    return(TPSSTAT_TYPECHECK);
    }
    return TPSSTAT_OK;
#else /*!HASFLOAT*/
    if(TPS_TYPE(args[0]) != TPSTYPE_INTEGER)
	return TPSSTAT_TYPECHECK;
    long i = TPS_INTEGER_OF(args[0]);
    TPS_CHANGEVALUE(args[0],TPSTYPE_INTEGER,- i);
    return TPSSTAT_OK;
#endif /*HASFLOAT*/
}

Tps_Status
Tps_op_noaccess(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    switch (TPS_TYPE(args[0])) {
	case TPSTYPE_STRING:
	case TPSTYPE_ARRAY:
	case TPSTYPE_DICT:
	case TPSTYPE_STREAM:
		TPS_SET_ACCESS(args[0],Tps_access_noaccess);
		break;
	case TPSTYPE_OPERATOR:
	case TPSTYPE_NULL:
	case TPSTYPE_MARK:
	case TPSTYPE_BOOLEAN:
	case TPSTYPE_INTEGER:
	case TPSTYPE_REAL:
	case TPSTYPE_NAME:
	default: break;
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_noop(Tps_Interp* /*intrp*/, Tps_Value* /*args*/, long /*nargs*/)
{
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_not(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    switch (TPS_TYPE(args[0])) {
	case TPSTYPE_BOOLEAN:
	    TPS_CHANGEVALUE(args[0],TPSTYPE_BOOLEAN,TPS_BOOLEAN_OF(args[0]) ^ 0x1);
	    break;
	case TPSTYPE_INTEGER:
	    TPS_CHANGEVALUE(args[0],TPSTYPE_BOOLEAN,~TPS_BOOLEAN_OF(args[0]));
	    break;
	default:
	    return(TPSSTAT_TYPECHECK);    
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_operator(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;
    Tps_Nameid nm;
    long arity;
    Tpsstatfcn proc;

    /* name arity ptr operator => -- */

    if(intrp->safe()) return TPSSTAT_UNSAFE;
    if(!TPS_ISTYPE(args[2],TPSTYPE_NAME)) return TPSSTAT_TYPECHECK;
    if(!TPS_ISTYPE(args[1],TPSTYPE_INTEGER)) return TPSSTAT_TYPECHECK;
    nm = TPS_NAME_OF(args[2]);
    arity = TPS_INTEGER_OF(args[1]);
    if(arity < 0) return TPSSTAT_RANGECHECK;
    if(TPS_TYPE(args[0]) == TPSTYPE_INTEGER) {
	proc = (Tpsstatfcn)TPS_INTEGER_OF(args[0]);
	ok = intrp->newoperator(nm,arity,proc);
	if(ok == TPSSTAT_OK) {
	    TPS_POPN(intrp,3);
	}
    } else 
	ok = TPSSTAT_TYPECHECK;
    return ok;
}

Tps_Status
Tps_op_or(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    u_long bits = 0; /* so initial and will be correct */
    long i;
    int isint = 0;
    Tps_Value result;

    for(i=0;i<2;i++) {
	switch (TPS_TYPE(args[i])) {
	    case TPSTYPE_INTEGER:
		isint = 1;
	    case TPSTYPE_BOOLEAN:
		bits |= TPS_ANY_OF(args[i]);
		break;
	    default: return TPSSTAT_TYPECHECK;
	}
    }
    if(isint)
	TPS_MAKEVALUE(result,TPSTYPE_INTEGER,bits);
    else
	TPS_MAKEVALUE(result,TPSTYPE_BOOLEAN,bits);
    TPS_RETURNVAL(intrp,result,2);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_pop(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_put(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Typeid t0;
    Tps_Typeid t1;
    Tps_Typeid t2;
    long index;

    t0 = TPS_TYPE(args[2]);
    t1 = TPS_TYPE(args[1]);
    t2 = TPS_TYPE(args[0]);
    if(!TPS_ISWRITEABLE(args[2])) return TPSSTAT_INVALIDACCESS;
    switch (t0) {
	case TPSTYPE_ARRAY:
	    {
		Tps_Array* a1 = TPS_ARRAY_OF(args[2]);
		if(t1 != TPSTYPE_INTEGER) goto badtype;
		index = TPS_INTEGER_OF(args[1]);
		if(index < 0 || index >= a1->length()) goto badrange;
		a1->contents()[index] = args[0];
	    }
	    break;
	case TPSTYPE_DICT:
	    {
		Tps_Dict* d1 = TPS_DICT_OF(args[2]);
		Tps_Status ok;
		Tps_Value v;
		Tps_Dictpair pair;

 		pair._key = args[1];
 		pair._value = args[0];
		ok = d1->insert(pair,&v);
		if(ok != TPSSTAT_OK)
		    return(TPSSTAT_UNDEFINED);
	    }
	    break;
	case TPSTYPE_STRING:
	    {
		Tps_String* s1 = TPS_STRING_OF(args[2]);
		if(t2 != TPSTYPE_INTEGER || t1 != TPSTYPE_INTEGER) goto badtype;
		index = TPS_INTEGER_OF(args[1]);
		if(index < 0 || index >= s1->length()) goto badrange;
		s1->contents()[index] = (char)TPS_INTEGER_OF(args[0]);
	    }
	    break;
	default:
	    goto badtype;
    }
    TPS_POPN(intrp,3);
    return TPSSTAT_OK;
badtype:
    return(TPSSTAT_TYPECHECK);
badrange:
    return(TPSSTAT_RANGECHECK);
}

Tps_Status
Tps_op_putinterval(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Typeid t0;
    long index;

    if(!TPS_ISTYPE(args[1],TPSTYPE_INTEGER)) return(TPSSTAT_TYPECHECK);
    if((t0 = TPS_TYPE(args[2])) != TPS_TYPE(args[0])) return(TPSSTAT_TYPECHECK);
    if((index = TPS_INTEGER_OF(args[1])) < 0) return(TPSSTAT_RANGECHECK);
    if(!TPS_ISWRITEABLE(args[2])) return TPSSTAT_INVALIDACCESS;
    switch (t0) {
	case TPSTYPE_STRING:
	    {
		Tps_String* s2 = TPS_STRING_OF(args[0]);
		Tps_String* s0 = TPS_STRING_OF(args[2]);
		if(s0->length() < s2->length()+index) return(TPSSTAT_RANGECHECK);
		MEMCPY(&s0->contents()[index],s2->contents(),s2->length());
	    }
	    break;
	case TPSTYPE_ARRAY:
	    {
		Tps_Array* a2 = TPS_ARRAY_OF(args[0]);
		Tps_Array* a0 = TPS_ARRAY_OF(args[2]);
		if(a0->length() < a2->length()+index) return(TPSSTAT_RANGECHECK);
		MEMCPY(
		      (char*)&a0->contents()[index],(char*)a2->contents(),
		      sizeof(Tps_Value)*a2->length());
	    }
	    break;
	default:
	    return(TPSSTAT_TYPECHECK);
    }
    TPS_POPN(intrp,3);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_quit(Tps_Interp* /*intrp*/, Tps_Value* /*args*/, long /*nargs*/)
{
    return TPSSTAT_QUIT;
}

Tps_Status
Tps_op_rbrace(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    Tps_Status ok;

    /* do the equivalent of {counttomark array astore exch pop cvx} */
    ok = TPS_CALL_PRIM(intrp,Tps_op_counttomark);/* mark a0..an-1 n */
    if(ok != TPSSTAT_OK) return ok;
    ok = TPS_CALL_PRIM(intrp,Tps_op_array);/* mark a0...an-1 a*/
    if(ok != TPSSTAT_OK) return ok;
    ok = TPS_CALL_PRIM(intrp,Tps_op_astore);  /* mark a*/
    if(ok != TPSSTAT_OK) return ok;
    ok = TPS_CALL_PRIM(intrp,Tps_op_exch);  /* a mark*/
    if(ok != TPSSTAT_OK) return ok;
    TPS_POPN(intrp,1);        			  /* a */
    ok = TPS_CALL_PRIM(intrp,Tps_op_cvx);  /* a */
    if(ok != TPSSTAT_OK) return ok;
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_rbracket(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    Tps_Status ok;

    /* do the equivalent of {counttomark array astore exch pop} */
    ok = TPS_CALL_PRIM(intrp,Tps_op_counttomark);/* mark a0..an-1 n */
    if(ok != TPSSTAT_OK) return ok;
    ok = TPS_CALL_PRIM(intrp,Tps_op_array);/* mark a0...an-1 a*/
    if(ok != TPSSTAT_OK) return ok;
    ok = TPS_CALL_PRIM(intrp,Tps_op_astore);  /* mark a*/
    if(ok != TPSSTAT_OK) return ok;
    ok = TPS_CALL_PRIM(intrp,Tps_op_exch);  /* a mark*/
    if(ok != TPSSTAT_OK) return ok;
    TPS_POPN(intrp,1);        			/* a */
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_rcheck(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    boolean b = TRUE;

    switch (TPS_TYPE(args[0])) {
	case TPSTYPE_STRING:
	case TPSTYPE_ARRAY:
	case TPSTYPE_DICT:
	case TPSTYPE_STREAM: b = TPS_ISREADABLE(args[0]); break;
	case TPSTYPE_OPERATOR: b = FALSE; break;
	case TPSTYPE_NULL:
	case TPSTYPE_MARK:
	case TPSTYPE_BOOLEAN:
	case TPSTYPE_INTEGER:
	case TPSTYPE_REAL:
	case TPSTYPE_NAME:
	default: b = TRUE; break;
    }
    if(b)
	args[0] = TPS__CONST(TPS__TRUE);
    else
	args[0] = TPS__CONST(TPS__FALSE);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_read(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    long c;
    if(!TPS_ISTYPE(args[1],TPSTYPE_STREAM)) return(TPSSTAT_TYPECHECK);
    c = TPS_STREAM_OF(args[1])->read();
    if(c != EOF) { /* not EOF */
	TPS_MAKEVALUE(args[0],TPSTYPE_INTEGER,c);
	TPS_PUSH(intrp,TPS__CONST(TPS__TRUE));
    } else {
	args[0] = TPS__CONST(TPS__FALSE);
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_readline(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_String* ss;
    long i;
    long c;
    Tps_Stream* strm;
    char* s;

    if(!TPS_ISTYPE(args[1],TPSTYPE_STREAM)) return(TPSSTAT_TYPECHECK);
    strm = TPS_STREAM_OF(args[1]);
    switch (TPS_TYPE(args[0])) {
	case TPSTYPE_STRING:
	    /* semantics here are not obvious since our strings
		are expandable; use allocated length as control */
	    ss = TPS_STRING_OF(args[0]);
	    for(i=0;i<ss->alloc();i++) {
		c = strm->read();
		if(c == EOF || c == '\n') goto stopread;
		ss->contents()[i] = (char)c;
	    }
	    return(TPSSTAT_RANGECHECK); /* filled string before eol/eof*/
stopread:
	    TESTERR(ss->setlength(i));
	    break;
	case TPSTYPE_NULL:
	    if(intrp->_tokenbuf.rewind() != TPSSTAT_OK)
		return(TPSSTAT_SYSTEMERROR);
	    while(1) {
		c = strm->read();
		if(c == EOF) break;
		if(c == '\n') break;
		if(!c) abort();
		if(intrp->_tokenbuf.write(c) != TPSSTAT_OK)
		    return(TPSSTAT_SYSTEMERROR);
	    }
	    intrp->_tokenbuf.ends(); /* mark hwm */
	    s = intrp->_tokenbuf.contents();
	    ss = new Tps_String(s,strlen(s));
	    if(!ss) return(TPSSTAT_SYSTEMERROR);
	    TPS_MAKEVALUE(args[0],TPSTYPE_STRING,ss);
	    break;
	default: return(TPSSTAT_TYPECHECK);
    }
    args[1] = args[0];
    args[0] = (c != EOF)?TPS__CONST(TPS__TRUE):TPS__CONST(TPS__FALSE);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_readonly(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    switch (TPS_TYPE(args[0])) {
	case TPSTYPE_STRING:
	case TPSTYPE_ARRAY:
	case TPSTYPE_DICT:
	case TPSTYPE_STREAM:
		TPS_SET_ACCESS(args[0],Tps_access_readonly);
		break;
	case TPSTYPE_OPERATOR:
	case TPSTYPE_NULL:
	case TPSTYPE_MARK:
	case TPSTYPE_BOOLEAN:
	case TPSTYPE_INTEGER:
	case TPSTYPE_REAL:
	case TPSTYPE_NAME:
	default: break;
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_readstring(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_String* ss;
    long i;
    long c;
    Tps_Stream* strm;
    char* s;

    if(!TPS_ISTYPE(args[1],TPSTYPE_STREAM)) return(TPSSTAT_TYPECHECK);
    strm = TPS_STREAM_OF(args[1]);
    switch (TPS_TYPE(args[0])) {
	case TPSTYPE_STRING:
	    if(!TPS_ISWRITEABLE(args[0])) return TPSSTAT_INVALIDACCESS;
	    ss = TPS_STRING_OF(args[0]);
	    for(i=0;i<ss->length();i++) {
		c = strm->read();
		if(c == EOF) break;
		ss->contents()[i] = (char)c;
	    }
	    TESTERR(ss->setlength(i));
	    break;
	case TPSTYPE_NULL:
	    if(intrp->_tokenbuf.rewind() != TPSSTAT_OK)
		return(TPSSTAT_SYSTEMERROR);
	    while((c = strm->read()) != EOF) {
		if(intrp->_tokenbuf.write(c) != TPSSTAT_OK)
		    return(TPSSTAT_SYSTEMERROR);
	    }
	    intrp->_tokenbuf.ends(); /* mark hwm */
	    s = intrp->_tokenbuf.contents();
	    ss = new Tps_String(s,strlen(s));
	    if(!ss) return(TPSSTAT_SYSTEMERROR);
	    TPS_MAKEVALUE(args[0],TPSTYPE_STRING,ss);
	    break;
	default: return(TPSSTAT_TYPECHECK);
    }
    args[1] = args[0];
    args[0] = (c != EOF)?TPS__CONST(TPS__TRUE):TPS__CONST(TPS__FALSE);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_realtime(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    long timediff;
    Tps_Value v;
    struct timeval t;

    if(gettimeofday(&t,(struct timezone*)0) < 0) return(TPSSTAT_SYSTEMERROR);
    if(t.tv_usec < intrp->_realtime.tv_usec) {
	t.tv_usec += 1000000;
	t.tv_sec -= 1;
    }
    timediff = (t.tv_sec - intrp->_realtime.tv_sec) * 1000000;
    timediff += (t.tv_usec - intrp->_realtime.tv_usec);
    TPS_GUARANTEE(intrp,1);
    TPS_MAKEVALUE(v,TPSTYPE_INTEGER,timediff);
    TPS_PUSH(intrp,v);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_remove(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;
    Tps_Dict* d1 = TPS_DICT_OF(args[1]);
    Tps_Dictpair pair;

    if(!TPS_ISTYPE(args[1],TPSTYPE_DICT)) return(TPSSTAT_TYPECHECK);
    ok = d1->remove(args[0],&pair);
    switch (ok) {
	case TPSSTAT_OK:
	case TPSSTAT_UNDEFINED:
	    break;
	default:
	    return ok;
    }
    TPS_POPN(intrp,2);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_repeat(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    /* prefix form is:
	repeat n body -> -
    */
    Tps_Frame_Repeat* f;
    long n;

    if(!TPS_ISTYPE(args[1],TPSTYPE_INTEGER)) return TPSSTAT_TYPECHECK;
    if((n = TPS_INTEGER_OF(args[1])) < 0) return TPSSTAT_RANGECHECK;
    if(n > 0) {
	if(!(f = (Tps_Frame_Repeat*)Tps_create_frame(intrp,&Tps_handler_repeat,sizeof(Tps_Frame_Repeat)))) return TPSSTAT_VMERROR;
	f->_body = args[0];
	f->_current = n;
    }
    TPS_POPN(intrp,2); /*clear stack*/
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_repeat_unwind(TPS_UNWIND_ARGS)
{
    Tps_Frame_Repeat* f = (Tps_Frame_Repeat*)frame;
    Tps_Status ok = Tps_unwind_frame(intrp,f);
    if(ok != TPSSTAT_OK) return ok;  
    if(thrown
	&& TPS_ISTYPE(intrp->_throwflag,TPSTYPE_NAME)
	&& TPS_NAME_OF(intrp->_throwflag) == TPS_NM(TPS_NMEXIT)) {
	return TPSSTAT_STOP;
    }
    return ok;
}

static
Tps_Status
Tps_repeat_reenter(TPS_REENTER_ARGS)
{
    Tps_Frame_Repeat* f = (Tps_Frame_Repeat*)frame;
    if(f->_current-- <= 0) return TPSSTAT_POPFRAME;
    /* rexecute body */
    Tps_create_source(intrp,f->_body);
    return TPSSTAT_RETRYFRAME;
}

static
Tps_Status
Tps_repeat_trace(TPS_TRACE_ARGS)
{
    Tps_Frame_Repeat* f = (Tps_Frame_Repeat*)frame;
    Tps_trace0(intrp,strm,f);
    strm->printf(" current=%d",f->_current);
    strm->write(" body=");
    (void)Tps_cvts1(*strm,f->_body,TRUE,-1);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_repeat_export(TPS_EXPORT_ARGS)
{
    Tps_Value v;
    Tps_Frame_Repeat* f = (Tps_Frame_Repeat*)frame;

    /* assume length and kind will be dumped by caller */
    TPS_GUARANTEE(intrp,1);
    TPS_MAKEVALUE(v,TPSTYPE_INTEGER,f->_current);
    TPS_PUSH(intrp,v);
    count++;
    return Tps_loop_export(intrp,frame,count,flags);
}

static
Tps_Status
Tps_repeat_import(TPS_IMPORT_ARGS)
{
    Tps_Frame_Repeat* f = (Tps_Frame_Repeat*)fr;
    Tps_Status ok;
    Tps_Value v;

    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt < 1) return TPSSTAT_RANGECHECK;
    cnt--;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_Repeat*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_Repeat));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    }
    if((ok = Tps_loop_import(intrp,h,cnt,f,flags)) != TPSSTAT_OK) return ok;
    v = TPS_POP(intrp);
    if(TPS_ISTYPE(v,TPSTYPE_INTEGER)) return TPSSTAT_TYPECHECK;
    f->_current = TPS_INTEGER_OF(v);
    if(f->_current < 0) return TPSSTAT_RANGECHECK;
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_repeat_parms = {"Repeat"};

Tps_Handler Tps_handler_repeat = {
	&Tps_repeat_parms,
	Tps_repeat_unwind,
	Tps_repeat_reenter,
	Tps_repeat_trace,
	Tps_loop_mark,
	Tps_repeat_export,
	Tps_repeat_import
};

Tps_Status
Tps_op_resetstream(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Stream* strm;
    if(!TPS_ISTYPE(args[0],TPSTYPE_STREAM)) return(TPSSTAT_TYPECHECK);
    if(intrp->safe() && TPS_ISUNSAFE(args[0])) return TPSSTAT_UNSAFE;
    strm = TPS_STREAM_OF(args[0]);
    TPS_POPN(intrp,1);
    strm->rewind();
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_roll(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    long range; /* depth in the stack to roll */
    long count; /* amount to roll */
    long left; /* range - |count| */
    Tps_Value* first;
    Tps_Value* last;
    Tps_Value* split;
    int i;

    /* treat as if arity == 2 */
    if(!TPS_ISTYPE(args[1],TPSTYPE_INTEGER)) return(TPSSTAT_TYPECHECK);
    range = TPS_INTEGER_OF(args[1]);
    if(range < 0) return(TPSSTAT_RANGECHECK);
    if(TPS_DEPTH(intrp) < range+2) return(TPSSTAT_STACKUNDERFLOW);
    if(!TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return(TPSSTAT_TYPECHECK);
    count = TPS_INTEGER_OF(args[0]);
    TPS_POPN(intrp,2);
    /* adjust args */
    args += 2;
    if(count == 0) goto done;
    last = args + range;
    first = args;
    /* key is convert all rolls to single direction
       using the fact that x y roll == x (x-y) roll when y < 0
    */
    if(count < 0) {
	count = -count;
	count = range - count;
    }
    left = range - count;
    split = args + count;
    TPS_GUARANTEE(intrp,left);
    args = TPS_PUSHN(intrp,left);
    MEMCPY((char*)args,(char*)split,left*sizeof(Tps_Value));
    /* need to move the bottom range values up by left spaces */
    /* would prefer to use memcpy, but cant trust it to do overlapping ranges*/
    for(i=0;i<range;i++) {
	*(--last) = *(--split);	
    }
    TPS_POPN(intrp,left);
done:
    return TPSSTAT_OK;    
}

Tps_Status
Tps_op_rrangle(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    Tps_Status ok;
    long count;
    Tps_Value v;

    /* counttomark */
    ok = TPS_CALL_PRIM(intrp,Tps_op_counttomark);/* mark a0..an-1 n */
    if(ok != TPSSTAT_OK) return ok;
    /* stash n */
    v = TPS_POP(intrp);
    count = TPS_INTEGER_OF(v);
    /* check for even number of values */
    if(count % 2 != 0) return TPSSTAT_RANGECHECK;
    /* make a dict of size count/2 */
    TPS_CHANGEVALUE(v,TPSTYPE_INTEGER,count/2);
    TPS_PUSH(intrp,v);
    ok = TPS_CALL_PRIM(intrp,Tps_op_dict);/* mark a0...an-1 d */
    if(ok != TPSSTAT_OK) return ok;
    /* stash d */
    v = TPS_POP(intrp);
    TPS_PUSH(intrp,v);
    ok = TPS_CALL_PRIM(intrp,Tps_op_begin);/* mark a0...an-1 */
    if(ok != TPSSTAT_OK) return ok;
    /* repeatedly insert pairs into d */
    while(count > 0) {
	ok = TPS_CALL_PRIM(intrp,Tps_op_def);  /* mark a0...an-3 an-2 */
	if(ok != TPSSTAT_OK) return ok;
	count -= 2;
    }
    /* mark */
    TPS_POPN(intrp,1);
    ok = TPS_CALL_PRIM(intrp,Tps_op_end);
    if(ok != TPSSTAT_OK) return ok;
    TPS_PUSH(intrp,v);
    return TPSSTAT_OK;
}

/* runstream:  stream runstream -
    set up to read and execute tokens from the stream.
    acts like stopped with respect to errors and throws.
*/

Tps_Status
Tps_op_runstream(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Frame_Runstream* f;

    if(!TPS_ISTYPE(args[0],TPSTYPE_STREAM)) return(TPSSTAT_TYPECHECK);
    if(!(f = (Tps_Frame_Runstream*)Tps_create_frame(intrp,&Tps_handler_runstream,sizeof(Tps_Frame_Runstream)))) return TPSSTAT_VMERROR;
    f->_strm = TPS_STREAM_OF(args[0]);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_runstream_unwind(TPS_UNWIND_ARGS)
{
    Tps_Frame_Runstream* f = (Tps_Frame_Runstream*)frame;
    Tps_Status ok = Tps_unwind_frame(intrp,f);
    if(ok != TPSSTAT_OK) return ok;  
    if(thrown) {
	if(TPS_ISTYPE(intrp->_throwflag,TPSTYPE_NAME)
	   && TPS_NAME_OF(intrp->_throwflag) == TPS_NM(TPS_NMSTOP)) {
	    /* push true and signal unwind stop */
	    TPS_GUARANTEE(intrp,1);
	    TPS_PUSH(intrp,TPS__CONST(TPS__TRUE));
	    return TPSSTAT_STOP;
	}
	/* else we have a throw of something other than /stopped;
	   let it pass up
        */
    } else {
	/* normal exit, push false */
	TPS_GUARANTEE(intrp,1);
	TPS_PUSH(intrp,TPS__CONST(TPS__FALSE));
    }
    return ok;
}

static
Tps_Status
Tps_runstream_reenter(TPS_REENTER_ARGS)
{
    Tps_Frame_Runstream* f = (Tps_Frame_Runstream*)frame;
    Tps_Status ok;
    Tps_Value v;
    ok = Tps_get_token(intrp->_tokenbuf,f->_strm,&v,1);
    if(ok == TPSSTAT_EOF)
	return TPSSTAT_POPFRAME; /* will invoke unwind */
    if(ok != TPSSTAT_OK)
	return ok;
    intrp->_object = v;
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_runstream_trace(TPS_TRACE_ARGS)
{
    Tps_Frame_Runstream* f = (Tps_Frame_Runstream*)frame;
    Tps_trace0(intrp,strm,f);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_runstream_mark(TPS_MARK_ARGS1)
{
    Tps_Frame_Runstream* f = (Tps_Frame_Runstream*)frame;
    f->_strm->mark();
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_runstream_export(TPS_EXPORT_ARGS1)
{
    Tps_Value v;
    Tps_Frame_Runstream* f = (Tps_Frame_Runstream*)frame;

    TPS_GUARANTEE(intrp,1);
    TPS_MAKEVALUE(v,TPSTYPE_STREAM,f->_strm);
    TPS_PUSH(intrp,v);
    count++;
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_runstream_import(TPS_IMPORT_ARGS1)
{
    Tps_Frame_Runstream* f = (Tps_Frame_Runstream*)fr;
    Tps_Value v;

    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt < 1) return TPSSTAT_RANGECHECK;
    cnt--;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_Runstream*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_Runstream));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    }
    v = TPS_POP(intrp);
    if(!TPS_ISTYPE(v,TPSTYPE_STREAM)) return TPSSTAT_TYPECHECK;
    f->_strm = TPS_STREAM_OF(v);
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_runstream_parms = {"Runstream"};

Tps_Handler Tps_handler_runstream = {
	&Tps_runstream_parms,
	Tps_runstream_unwind,
	Tps_runstream_reenter,
	Tps_runstream_trace,
	Tps_runstream_mark,
	Tps_runstream_export,
	Tps_runstream_import
};

Tps_Status
Tps_op_search(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_String* s0;
    Tps_String* s1;
    long i;
    long r0;
    long l0;
    long c0;
    char* p0;
    char* p1;

    if(!TPS_ISTYPE(args[0],TPSTYPE_STRING)
	|| !TPS_ISTYPE(args[1],TPSTYPE_STRING))
	return(TPSSTAT_TYPECHECK);
    s0 = TPS_STRING_OF(args[1]);
    s1 = TPS_STRING_OF(args[0]);
    /* figure out the range in s1 to search
       assuming we start at the first char and
       and move forward
    */
    r0 = s0->length() - s1->length();
    if(r0 < 0) goto notfound;
    /* repeatedly search forward for the first char of s1 */
    p1 = s1->contents();
    c0 = *p1;
    l0 = s1->length();
    p0 = s0->contents();
    for(i=0;i<=r0;i++) {
	if(*p0 == c0) {
	    /* see if the whole string occurs */
	    if(MEMCMP(p1,p0,l0) == 0) { /* success */
		/* need to construct 3 substrings */
		Tps_Value pre;
		Tps_Value mid;
		Tps_Value post;

		s1 = new Tps_String(s0->contents(),i);
		TPS_MAKEVALUE(pre,TPSTYPE_STRING,s1);
		s1 = new Tps_String(s0->contents()+i,l0);
		TPS_MAKEVALUE(mid,TPSTYPE_STRING,s1);
		s1 = new Tps_String(s0->contents()+(i+l0),
				     s0->length() - (i+l0));
		TPS_MAKEVALUE(post,TPSTYPE_STRING,s1);
		/* stick things on the stack in right order */
		args[1] = post;
		args[0] = mid;
		TPS_PUSH(intrp,pre);
		TPS_MAKEVALUE(pre,TPSTYPE_BOOLEAN,1); /* reuse pre*/
		TPS_PUSH(intrp,pre);
		return TPSSTAT_OK;
	    }
	}
	p0++;
    }
notfound:
    TPS_MAKEVALUE(args[0],TPSTYPE_BOOLEAN,0);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_setarity(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Operator* op;
    long arity;

    if(!TPS_ISTYPE(args[1],TPSTYPE_OPERATOR)) return TPSSTAT_TYPECHECK;
    if(!TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return TPSSTAT_TYPECHECK;
    op = TPS_OPERATOR_OF(args[1]);
    arity = TPS_INTEGER_OF(args[0]);
    if(arity < 0) return TPSSTAT_RANGECHECK;
    op->arity(arity);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_sleep(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    long sec;
    if(!TPS_ISTYPE(args[0],TPSTYPE_INTEGER))
	return(TPSSTAT_TYPECHECK);    
    sec = TPS_INTEGER_OF(args[0]);
    TPS_POPN(intrp,1);
    SLEEP(sec);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_status(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    if(!TPS_ISTYPE(args[0],TPSTYPE_STREAM)) return(TPSSTAT_TYPECHECK);
     if(TPS_STREAM_OF(args[0])->isopen())
	args[0] = TPS__CONST(TPS__TRUE);
    else
	args[0] = TPS__CONST(TPS__FALSE);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_stop(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    /* do /stop throw */
    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,TPS__NM(TPS_NMSTOP));
    return TPS_CALL_PRIM(intrp,Tps_op_throw);
}

/* simulate the equivalent of
    /stop {catch {dup /stop eq {pop true} {throw} ifelse} {false} ifelse} def
*/
Tps_Status
Tps_op_stopped(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    Tps_Value proc;

    proc = TPS_POP(intrp);
    if(TPS_ISEXARRAY(proc)) {
	Tps_Frame_Stopped* f;
	if(!(f = (Tps_Frame_Stopped*)Tps_create_frame(intrp,&Tps_handler_stopped,sizeof(Tps_Frame_Stopped)))) return TPSSTAT_VMERROR;
	f->_body = proc;
	return Tps_create_source(intrp,proc);
    }
    TPS_PUSH(intrp,TPS__CONST(TPS__FALSE));
    return TPSSTAT_OK;

}

static
Tps_Status
Tps_stopped_unwind(TPS_UNWIND_ARGS)
{
    Tps_Frame_Stopped* f = (Tps_Frame_Stopped*)frame;
    Tps_Status ok = Tps_unwind_frame(intrp,f);
    if(ok != TPSSTAT_OK) return ok;  
    /* test for /stop */
    if(thrown) {
	if(TPS_ISTYPE(intrp->_throwflag,TPSTYPE_NAME)
	   && TPS_NAME_OF(intrp->_throwflag) == TPS_NM(TPS_NMSTOP)) {
	    /* push true and signal unwind stop */
	    TPS_GUARANTEE(intrp,1);
	    TPS_PUSH(intrp,TPS__CONST(TPS__TRUE));
	    return TPSSTAT_STOP;
	}
	/*else we have a throw of something other than /stopped;
	  let it pass up
	*/
    } else {
	/* push false and signal normal unwind */
	TPS_GUARANTEE(intrp,1);
	TPS_PUSH(intrp,TPS__CONST(TPS__FALSE));
    }
    return ok;
}

static
Tps_Status
Tps_stopped_reenter(TPS_REENTER_ARGS0)
{
    return TPSSTAT_POPFRAME;
}

static
Tps_Status
Tps_stopped_trace(TPS_TRACE_ARGS)
{
    Tps_Frame_Stopped* f = (Tps_Frame_Stopped*)frame;
    Tps_trace0(intrp,strm,f);
    strm->write(" body=");
    (void)Tps_cvts1(*strm,f->_body,TRUE,-1);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_stopped_mark(TPS_MARK_ARGS1)
{
    Tps_Frame_Stopped* f = (Tps_Frame_Stopped*)frame;
    Tps_mark(f->_body);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_stopped_export(TPS_EXPORT_ARGS1)
{
    Tps_Frame_Stopped* f = (Tps_Frame_Stopped*)frame;

    /* assume length and kind will be dumped by caller */
    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,f->_body);
    count++;
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_stopped_import(TPS_IMPORT_ARGS1)
{
    Tps_Frame_Stopped* f = (Tps_Frame_Stopped*)fr;

    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt < 1) return TPSSTAT_RANGECHECK;
    cnt--;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_Stopped*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_Stopped));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    }
    f->_body = TPS_POP(intrp);
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_stopped_parms = {"Stopped"};

Tps_Handler Tps_handler_stopped = {
	&Tps_stopped_parms,
	Tps_stopped_unwind,
	Tps_stopped_reenter,
	Tps_stopped_trace,
	Tps_stopped_mark,
	Tps_stopped_export,
	Tps_stopped_import
};

Tps_Status
Tps_op_store(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Value* ds;
    Tps_Status ok;
    long where;
    Tps_Dictpair pair;
    Tps_Dict* d;
    Tps_Value dv;

    ok = Tps_dictstack_lookup(TPS_DTOSP(intrp),TPS_DDEPTH(intrp),
			      args[0],&where,(Tps_Dictpair**)0);
    switch (ok) {
	case TPSSTAT_OK: break;
	case TPSSTAT_UNDEFINED: where = 0; break;
	default: return(TPSSTAT_SYSTEMERROR);
    }
    ds = TPS_DTOSP(intrp);
    dv = ds[where];
    if(!TPS_ISWRITEABLE(dv)) return TPSSTAT_INVALIDACCESS;
    d = TPS_DICT_OF(dv);
    pair._key = args[1];
    pair._value = args[0];
    ok = (d)->insert(pair,(Tps_Value*)0);
    if(ok != TPSSTAT_OK) return ok;
    TPS_POPN(intrp,2);
    return TPSSTAT_OK;
}

/*
Stream is an unsafe operator, so it will have the
previous safety state on top of stack
*/
Tps_Status
Tps_op_stream(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;
    const char* which;
    long wlen;
    const char* fname;
    long flen;
    const char* md;
    long mdlen;

    /*
	  name|string /r /file stream => --stream(name)--
	| name|string /w /file stream => --stream(name)--
	| null /w /string stream => --stream--
	| string /r /string stream => --stream--
    */

    /* pop previous safety state */
    TPS_POPN(intrp,1);
    args++;
    if(intrp->safe()) return TPSSTAT_UNSAFE;
    if(!TPS_ISTYPE(args[0],TPSTYPE_NAME)
       && !TPS_ISTYPE(args[0],TPSTYPE_STRING))
	return TPSSTAT_TYPECHECK;
    if(!TPS_ISTYPE(args[1],TPSTYPE_NAME)
       && !TPS_ISTYPE(args[1],TPSTYPE_STRING))
	return TPSSTAT_TYPECHECK;
    ok = Tps_string_or_name(args[0],&which,&wlen);
    if(ok != TPSSTAT_OK) return ok;
    ok = Tps_string_or_name(args[1],&md,&mdlen);
    if(ok != TPSSTAT_OK) return ok;
    if(TPS_ISTYPE(args[2],TPSTYPE_NAME)
	|| TPS_ISTYPE(args[2],TPSTYPE_STRING)) {
	ok = Tps_string_or_name(args[2],&fname,&flen);
	if(ok != TPSSTAT_OK) return ok;
	if(flen <= 0) return TPSSTAT_INVALIDSTREAMACCESS;
    } else if(TPS_ISTYPE(args[2],TPSTYPE_NULL)) {
	fname = (char*)0;
	flen = 0;
    } else
	return TPSSTAT_TYPECHECK;

    if(STRNCMP(which,TPS_NM(TPS_NMFILE),(int)wlen) == 0) {
	Tps_Stream_File* strm;
	if(!fname) return TPSSTAT_INVALIDSTREAMACCESS;
	if(!(strm = new Tps_Stream_File)) return TPSSTAT_VMERROR;
	strm->open(fname,flen,Tps_mode_of(md,mdlen));
	if(!(*strm)) return TPSSTAT_INVALIDSTREAMACCESS;
	TPS_MAKEVALUE(args[2],TPSTYPE_STREAM,strm);
    } else if(STRNCMP(which,TPS_NM(TPS_NMSTRING),wlen) == 0) {
	Tps_Stream_String* strm;
	if(!(strm = new Tps_Stream_String)) return TPSSTAT_VMERROR;
	switch (*md) {
	    case 'r':
		if(!fname) return TPSSTAT_INVALIDSTREAMACCESS;
		strm->open(fname,flen);
		if(!(*strm)) return TPSSTAT_INVALIDSTREAMACCESS;
		break;
	    case 'w':
	    case 'a':
		strm->open();
		if(!(*strm)) return TPSSTAT_INVALIDSTREAMACCESS;
		break;
	    default: return(TPSSTAT_INVALIDSTREAMACCESS);
	}
	TPS_MAKEVALUE(args[2],TPSTYPE_STREAM,strm);
    } else
	return(TPSSTAT_INVALIDSTREAMACCESS);
    TPS_POPN(intrp,2);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_streamstring(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Stream* strm;
    Tps_Stream_String* sstrm;
    Tps_String* s;
    Tps_Value v;

    if(!TPS_ISTYPE(args[0],TPSTYPE_STREAM)) return TPSSTAT_TYPECHECK;
    strm = TPS_STREAM_OF(args[0]);
    if(strm->stream_type() != Tps_stream_string)
	return TPSSTAT_INVALIDSTREAMACCESS;
    sstrm = (Tps_Stream_String*)strm;
    s = new Tps_String(sstrm->contents(),sstrm->length());
    if(!s) return TPSSTAT_VMERROR;
    TPS_MAKEVALUE(v,TPSTYPE_STRING,s);
    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,v);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_string(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_String* s;
    long len;

    if(!TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return(TPSSTAT_TYPECHECK);
    len = TPS_INTEGER_OF(args[0]);
    if(len < 0) return(TPSSTAT_RANGECHECK);
    s = new Tps_String(len);
    if(!s) return(TPSSTAT_VMERROR);
    TPS_MAKEVALUE(args[0],TPSTYPE_STRING,s);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_sub(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
#if HASFLOAT
    Tps_Typeid rtyp0;
    Tps_Typeid rtyp1;
    long ir;
    Tps_Real fr;

    rtyp1 = TPS_TYPE(args[0]);
    rtyp0 = TPS_TYPE(args[1]);
    switch (rtyp1) {
	case TPSTYPE_INTEGER:
	    switch (rtyp0) {
		case TPSTYPE_INTEGER:
		    ir = TPS_INTEGER_OF(args[1]) - TPS_INTEGER_OF(args[0]);
		    goto int_result;
		case TPSTYPE_REAL:
		    fr = TPS_REAL_OF(args[1]) - TPS_INTEGER_OF(args[0]);
		    goto real_result;
		default:
		    goto wrongtype;
	    }
	    break;
	case TPSTYPE_REAL:
	    switch (rtyp0) {
		case TPSTYPE_INTEGER:
		    fr = TPS_INTEGER_OF(args[1]) - TPS_REAL_OF(args[0]);
		    goto real_result;
		case TPSTYPE_REAL:
		    fr = TPS_REAL_OF(args[1]) - TPS_REAL_OF(args[0]);
		    goto real_result;
		default:
		    goto wrongtype;
	    }
	    break;
	default:
	    goto wrongtype;
    }
int_result:
    TPS_MAKEVALUE(args[1],TPSTYPE_INTEGER,ir);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
real_result:
    TPS_MAKEREAL(args[1],fr);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
wrongtype:
    return(TPSSTAT_TYPECHECK);
#else /*!HASFLOAT*/
    if(TPS_TYPE(args[0]) != TPSTYPE_INTEGER
       || TPS_TYPE(args[1]) != TPSTYPE_INTEGER)
	return TPSSTAT_TYPECHECK;
    long ir;
    ir = TPS_INTEGER_OF(args[1]) - TPS_INTEGER_OF(args[0]);
    TPS_MAKEVALUE(args[1],TPSTYPE_INTEGER,ir);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
#endif /*HASFLOAT*/
}

/*
throw: value throw -
	When throw is executed, the exec stack is unwound.
	If not found, then the error "invalidexit"
	is raised.  See catch for subsequent processing.
	Throw will raise stackunderflow if there is not a value
	on the stack to serve as tag identifying the exception.
	Also, if executed in unsafe mode, it does a cvunsafe
	on the throw so that it can only be caught in unsafe mode.
*/

Tps_Status
Tps_op_throw(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    Tps_Status ok;

    intrp->_throwflag = TPS_POP(intrp); /* pull off the thrown value */
    if(!intrp->safe()) TPS_SET_UNSAFE(intrp->_throwflag,1);
    /* unwind until caught or out of stack (quit) */
    ok = intrp->unwind_throw();
    switch (ok) {
	case TPSSTAT_STOP:
	    /* catch frame has been found, unwound and popped;
		presumably, it has set the stacks the way it wants,
	    */
	    /* WARNING: the catch frame should also be a source frame */
	    return TPSSTAT_OK;
	case TPSSTAT_OK:
	    return TPSSTAT_QUIT; /* we unwound whole stack */
	default: /* should be quit or systemerror */
	    return ok;
    }
/*NOTREACHED*/
}

Tps_Status
Tps_op_token(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Stream* strm;
    Tps_Status ok;
    Tps_Value v;

    TPS_GUARANTEE(intrp,3); /* slight overkill */
    switch (TPS_TYPE(args[0])) {
	case TPSTYPE_STRING: {
	    long left;
	    Tps_String* ss = TPS_STRING_OF(args[0]);
	    /* insert string into inbuf */
	    (void)intrp->_inbuf.open(ss->contents(),ss->length());
	    /* get a token */
	    if((ok = Tps_get_token(intrp->_tokenbuf,
				 &intrp->_inbuf,
				 &v,
				 0)) != TPSSTAT_OK) {
		if(ok == TPSSTAT_EOF) ok = TPSSTAT_OK;
		args[0] = TPS__CONST(TPS__FALSE);
		return ok;
	    }
	    left = intrp->_inbuf.bytesavailable();
	    (void)intrp->_inbuf.close();
	    if(ok != TPSSTAT_OK) return ok;
	    /* extract remaining characters */
	    ss = new Tps_String(ss->contents()+(ss->length()-left),left);
	    TPS_MAKEVALUE(args[0],TPSTYPE_STRING,ss);
	    TPS_PUSH(intrp,v);
	    TPS_PUSH(intrp,TPS__CONST(TPS__TRUE));
	} break;
	case TPSTYPE_STREAM: {
	    strm = TPS_STREAM_OF(args[0]);
	    ok = Tps_get_token(intrp->_tokenbuf,strm,&v,1);
	    if(ok == TPSSTAT_EOF) {
		delete strm;
		args[0] = TPS__CONST(TPS__FALSE);
		return TPSSTAT_OK;
	    }
	    if(ok != TPSSTAT_OK) return ok;
	    args[0] = v;
	    TPS_PUSH(intrp,TPS__CONST(TPS__TRUE));
	} break;
	default:
	    return(TPSSTAT_TYPECHECK);
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_type(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    args[0] = TPS__TNM(TPS_TYPE(args[0]));
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_undef(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    long where;
    Tps_Value* ds;
    Tps_Status ok;
    Tps_Dictpair* pairp;
    Tps_Dictpair pair;
    Tps_Dict* d;

    ok = Tps_dictstack_lookup(TPS_DTOSP(intrp),TPS_DDEPTH(intrp),
			      args[0],&where,&pairp);
    switch (ok) {
	case TPSSTAT_OK:
	    ds = TPS_DTOSP(intrp);
	    d = TPS_DICT_OF(ds[where]);
	    ok = (d)->remove(args[0],&pair);
	    switch (ok) {
		case TPSSTAT_OK: case TPSSTAT_UNDEFINED: break;
		default: return ok;
	    }
	    break;
	case TPSSTAT_UNDEFINED: break;
	default: return ok;
    }
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_userdict(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,intrp->__userdicts[intrp->safe()]);   
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_usertime(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    long timediff;
    Tps_Value v;

#if defined hpux || solaris2
    struct tms t;
    times(&t);
    timediff = (t.tms_utime * tpsg._clockres) - intrp->_usertime.tv_usec;
#else
    struct timeval* tp;
    struct rusage r;

    if(getrusage(RUSAGE_SELF,&r) < 0) return(TPSSTAT_SYSTEMERROR);
    /* it looks like the header files for solaris2 are
       inconsistent with the man page for getrusage.
       I think the man file is correct, and that it works
       like the 4.X getrusage and the user time is
        a struct timevale and not a timestruc_t.
    */
    tp = (struct timeval*)&r.ru_utime;
    if(tp->tv_usec < intrp->_usertime.tv_usec) {
        tp->tv_usec += 1000000;
        tp->tv_sec -= 1;
    }
    timediff = (tp->tv_sec - intrp->_usertime.tv_sec) * 1000000;
    timediff += (tp->tv_usec - intrp->_usertime.tv_usec);
#endif
    TPS_GUARANTEE(intrp,1);
    TPS_MAKEVALUE(v,TPSTYPE_INTEGER,timediff);
    TPS_PUSH(intrp,v);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_wcheck(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    boolean b = TRUE;

    switch (TPS_TYPE(args[0])) {
	case TPSTYPE_STRING:
	case TPSTYPE_ARRAY:
	case TPSTYPE_DICT:
	case TPSTYPE_STREAM: b = TPS_ISWRITEABLE(args[0]); break;
	case TPSTYPE_OPERATOR:
	case TPSTYPE_NULL:
	case TPSTYPE_MARK:
	case TPSTYPE_BOOLEAN:
	case TPSTYPE_INTEGER:
	case TPSTYPE_REAL:
	case TPSTYPE_NAME:
	default: b = FALSE; break;
    }
    if(b)
	args[0] = TPS__CONST(TPS__TRUE);
    else
	args[0] = TPS__CONST(TPS__FALSE);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_where(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    long where;
    Tps_Value* ds;
    Tps_Status ok;

    ok = Tps_dictstack_lookup(TPS_DTOSP(intrp),TPS_DDEPTH(intrp),
			      args[0],&where,(Tps_Dictpair**)0);
    switch(ok) {
	case TPSSTAT_OK:
	    ds = TPS_DTOSP(intrp);
	    args[0] = ds[where];
	    TPS_GUARANTEE(intrp,1);
	    TPS_PUSH(intrp,TPS__CONST(TPS__TRUE));
	    break;
	case TPSSTAT_UNDEFINED:
	    args[0] = TPS__CONST(TPS__FALSE);
	    break;
	default:
	    return(TPSSTAT_SYSTEMERROR);
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_while(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    /* form: expr body while */
    Tps_Frame_While* f;

    if(!(f = (Tps_Frame_While*)Tps_create_frame(intrp,&Tps_handler_while,sizeof(Tps_Frame_While)))) return TPSSTAT_VMERROR;
    f->_body = TPS_POP(intrp);
    f->_cond = TPS_POP(intrp);
    /* arrange to execute the cond */
    Tps_create_source(intrp,f->_cond);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_while_unwind(TPS_UNWIND_ARGS)
{
    Tps_Frame_While* f = (Tps_Frame_While*)frame;
    Tps_Status ok = Tps_unwind_frame(intrp,f);
    if(ok != TPSSTAT_OK) return ok;  
    if(thrown
	&& TPS_ISTYPE(intrp->_throwflag,TPSTYPE_NAME)
	&& TPS_NAME_OF(intrp->_throwflag) == TPS_NM(TPS_NMEXIT)) {
	return TPSSTAT_STOP;
    }
    return ok;
}

static
Tps_Status
Tps_while_reenter(TPS_REENTER_ARGS)
{
    Tps_Frame_While* f = (Tps_Frame_While*)frame;
    Tps_Value b;
    
    /* test the top of stack to decide if to continue */
    b = TPS_TOP(intrp);
    if(!TPS_ISTYPE(b,TPSTYPE_BOOLEAN)
	&& !TPS_ISTYPE(b,TPSTYPE_INTEGER)) return TPSSTAT_TYPECHECK;
    TPS_POPN(intrp,1);    
    if(!TPS_BOOLEAN_OF(b)) return TPSSTAT_POPFRAME; /* we are done */
    /* loop by executing the body then falling into executing the cond
       then falling into executing while reenter
    */
    Tps_create_source(intrp,f->_cond);
    Tps_create_source(intrp,f->_body);
    return TPSSTAT_RETRYFRAME;
}

static
Tps_Status
Tps_while_trace(TPS_TRACE_ARGS)
{
    Tps_Frame_While* f = (Tps_Frame_While*)frame;
    Tps_trace0(intrp,strm,f);
    strm->write(" cond=");
    (void)Tps_cvts1(*strm,f->_cond,TRUE,-1);
    strm->write(" ; body=");
    (void)Tps_cvts1(*strm,f->_body,TRUE,-1);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_while_mark(TPS_MARK_ARGS1)
{
    Tps_Frame_While* f = (Tps_Frame_While*)frame;
    Tps_mark(f->_body);
    Tps_mark(f->_cond);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_while_export(TPS_EXPORT_ARGS)
{
    Tps_Frame_While* f = (Tps_Frame_While*)frame;

    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,f->_cond);
    count++;
    return Tps_loop_export(intrp,frame,count,flags);
}

static
Tps_Status
Tps_while_import(TPS_IMPORT_ARGS)
{
    Tps_Frame_While* f = (Tps_Frame_While*)fr;
    Tps_Status ok;

    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt < 1) return TPSSTAT_RANGECHECK;
    cnt--;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_While*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_While));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    }
    if((ok = Tps_loop_import(intrp,h,cnt,f,flags)) != TPSSTAT_OK) return ok;
    f->_cond = TPS_POP(intrp);
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_while_parms = {"While"};

Tps_Handler Tps_handler_while = {
	&Tps_while_parms,
	Tps_while_unwind,
	Tps_while_reenter,
	Tps_while_trace,
	Tps_while_mark,
	Tps_while_export,
	Tps_while_import
};

Tps_Status
Tps_op_write(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;
    if(!TPS_ISTYPE(args[1],TPSTYPE_STREAM)) return(TPSSTAT_TYPECHECK);
    if(!TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return(TPSSTAT_TYPECHECK);
    ok = TPS_STREAM_OF(args[1])->write(TPS_INTEGER_OF(args[0]));
    if(ok != TPSSTAT_OK) return ok;
    TPS_POPN(intrp,2);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_writestring(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_String* s;
    Tps_Status ok;

    if(!TPS_ISTYPE(args[1],TPSTYPE_STREAM)) return(TPSSTAT_TYPECHECK);
    if(!TPS_ISTYPE(args[0],TPSTYPE_STRING)) return(TPSSTAT_TYPECHECK);
    s = TPS_STRING_OF(args[0]);
    ok = TPS_STREAM_OF(args[1])->write(s->contents(),s->length());
    if(ok != TPSSTAT_OK) return ok;
    TPS_POPN(intrp,2);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_xcheck(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    if(TPS_ISEXECUTABLE(args[0]))
	args[0] = TPS__CONST(TPS__TRUE);
    else
	args[0] = TPS__CONST(TPS__FALSE);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_xor(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    u_long bits = 0; /* so initial and will be correct */
    long i;
    int isint = 0;
    Tps_Value result;

    for(i=0;i<2;i++) {
	switch (TPS_TYPE(args[i])) {
	    case TPSTYPE_INTEGER:
		isint = 1;
	    case TPSTYPE_BOOLEAN:
		bits ^= TPS_ANY_OF(args[i]);
		break;
	    default: return TPSSTAT_TYPECHECK;
	}
    }
    if(isint)
	TPS_MAKEVALUE(result,TPSTYPE_INTEGER,bits);
    else
	TPS_MAKEVALUE(result,TPSTYPE_BOOLEAN,bits);
    TPS_RETURNVAL(intrp,result,2);
    return TPSSTAT_OK;
}

/**************************************************/
/**************************************************/
/* ERROR OPERATORS */
/**************************************************/

/*
error handling:

The error sequence operates differently than
e.g. in Postscript in order to simplify interactions
with safe mode.

When an error is detected, the object and errorname
are pushed onto the stack and the routine
errortrap is executed.

Errortrap currently is defined to
be equivalent to {stop} which is equivalent to {/stop throw}.
Normally, this is caught by a stopped operator
and the routine handleerror is executed to actually report the error
from the values on the stack.
*/

Tps_Status
Tps_op_errortrap(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    /* do /stop throw */
    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,TPS__NM(TPS_NMSTOP));
    return TPS_CALL_PRIM(intrp,Tps_op_throw);
}

/*
This version of handleerror assumes combines the functions
of old errortrap and errorhandler
on entry, stack contains :  ... object /<errname>
*/

Tps_Status
Tps_op_handleerror(Tps_Interp* intrp, Tps_Value* args, long nargs)
{
    Tps_Status ok;
    boolean errmode;

    /* do some validity checks */
    if(nargs < 2) return TPSSTAT_STACKUNDERFLOW;
    if(!TPS_ISTYPE(args[0],TPSTYPE_NAME)) return TPSSTAT_TYPECHECK;

    intrp->_tokenbuf.rewind();
    ok = intrp->_tokenbuf.write("Error: ");
    if(ok != TPSSTAT_OK) return ok;
    ok = Tps_cvts1(intrp->_tokenbuf,args[0],FALSE,-1);
    if(ok != TPSSTAT_OK) return ok;
    ok = intrp->_tokenbuf.write("; executing: ");
    if(ok != TPSSTAT_OK) return ok;
    ok = Tps_cvts1(intrp->_tokenbuf,args[1],FALSE,-1);
    if(ok != TPSSTAT_OK) return ok;
    ok = intrp->_tokenbuf.write(" . \n");
    if(ok != TPSSTAT_OK) return ok;
    intrp->_tokenbuf.ends(); /* null terminate */
    /* extract the written string and write to stderr */
    ok = (intrp->stdstream(Tps_stderr))->write(intrp->_tokenbuf.contents());
    TPS_POPN(intrp,2);
    return ok;
}

/**************************************************/
/* TRACE OPERATORS */
/**************************************************/

/* Trace related postfix operators */

Tps_Status
Tps_op_cvuntrace(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    if(intrp->safe()) return TPSSTAT_OK;
    TPS_SET_TRACEOFF(args[0],1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_tracecheck(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    if(TPS_ISTRACEOFF(args[0]))
	args[0] = TPS__CONST(TPS__FALSE);
    else
	args[0] = TPS__CONST(TPS__TRUE);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_traceexec(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;

    ok = Tps_create_trace(intrp,1);
    if(ok != TPSSTAT_OK) return ok;
    ok = Tps_create_source(intrp,args[0],FALSE);
    if(ok != TPSSTAT_OK) return ok;
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

/* Unwind to a trace frame, and reset trace flag */
Tps_Status
Tps_op_tracereturn(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    /* do /exittrace throw */
    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,TPS__NM(TPS_NMEXITTRACE));
    return TPS_CALL_PRIM(intrp,Tps_op_throw);
}

Tps_Status
Tps_op_tracetrap(Tps_Interp* intrp, Tps_Value* args, long nargs)
{
    /* on entry, stack contains :  ... object */
    /* This version just prints the object
       and causes it to be executed.
    */
    /* do some validity checks */
    if(nargs < 1) return TPSSTAT_STACKUNDERFLOW;
    Tps_Stream* out = intrp->stdstream(Tps_stdout);
    (void)Tps_cvts1(*out,args[0],TRUE,-1);
    out->write("\n");
    return Tps_op_tracereturn(intrp,args,1);
}

/**************************************************/

#if HASFLOAT
Tps_Status
Tps_op_atan(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Real f;

    if(!TPS_ISTYPE(args[0],TPSTYPE_REAL)) return(TPSSTAT_TYPECHECK);
    f = TPS_REAL_OF(args[0]);
    f = atan(f);
    TPS_MAKEREAL(args[0],f);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_ceiling(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Real fr;

    if(TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return TPSSTAT_OK;
    if(!TPS_ISTYPE(args[0],TPSTYPE_REAL)) return(TPSSTAT_TYPECHECK);
    fr = TPS_REAL_OF(args[0]);
    fr = ceil(fr);
    TPS_MAKEREAL(args[0],fr);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_cos(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Real f;

    if(!TPS_ISTYPE(args[0],TPSTYPE_REAL)) return(TPSSTAT_TYPECHECK);
    f = TPS_REAL_OF(args[0]);
    f = cos(f);
    TPS_MAKEREAL(args[0],f);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_exp(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Real f;

    if(!TPS_ISTYPE(args[0],TPSTYPE_REAL)) return(TPSSTAT_TYPECHECK);
    f = TPS_REAL_OF(args[0]);
    f = exp(f);
    TPS_MAKEREAL(args[0],f);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_floor(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Real fr;

    if(TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return TPSSTAT_OK;
    if(!TPS_ISTYPE(args[0],TPSTYPE_REAL)) return(TPSSTAT_TYPECHECK);
    fr = TPS_REAL_OF(args[0]);
    fr = floor(fr);
    TPS_MAKEREAL(args[0],fr);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_ln(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Real f;

    if(!TPS_ISTYPE(args[0],TPSTYPE_REAL)) return(TPSSTAT_TYPECHECK);
    f = TPS_REAL_OF(args[0]);
    f = log(f);
    TPS_MAKEREAL(args[0],f);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_log(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Real f;

    if(!TPS_ISTYPE(args[0],TPSTYPE_REAL)) return(TPSSTAT_TYPECHECK);
    f = TPS_REAL_OF(args[0]);
    f = log10(f);
    TPS_MAKEREAL(args[0],f);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_round(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Real fr;

    if(TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return TPSSTAT_OK;
    if(!TPS_ISTYPE(args[0],TPSTYPE_REAL)) return(TPSSTAT_TYPECHECK);
    fr = TPS_REAL_OF(args[0]);
#if defined hpux
    if(fr < 0) fr += -0.5; else fr += 0.5;
    long i = (long)fr;
    fr = i;    
#else
/* remind: not sure that rint is right choice here */
    fr = rint(fr);
#endif
    TPS_MAKEREAL(args[0],fr);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_sin(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Real f;

    if(!TPS_ISTYPE(args[0],TPSTYPE_REAL)) return(TPSSTAT_TYPECHECK);
    f = TPS_REAL_OF(args[0]);
    f = sin(f);
    TPS_MAKEREAL(args[0],f);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_sqrt(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Real f;

    if(!TPS_ISTYPE(args[0],TPSTYPE_REAL)) return(TPSSTAT_TYPECHECK);
    f = TPS_REAL_OF(args[0]);
    f = sqrt(f);
    TPS_MAKEREAL(args[0],f);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_truncate(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    Tps_Real fr;

    if(TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return TPSSTAT_OK;
    if(!TPS_ISTYPE(args[0],TPSTYPE_REAL)) return(TPSSTAT_TYPECHECK);
    fr = TPS_REAL_OF(args[0]);
    fr = floor(fr);
    TPS_MAKEREAL(args[0],fr);
    return TPSSTAT_OK;
}
#endif /*HASFLOAT*/

#if HASRAND
Tps_Status
Tps_op_rand(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    int i;
    Tps_Value v;

    TPS_GUARANTEE(intrp,1);
    i = rand();
    TPS_MAKEVALUE(v,TPSTYPE_INTEGER,i);
    TPS_PUSH(intrp,v);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_rrand(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    int i;
    Tps_Value v;

    TPS_GUARANTEE(intrp,1);
    /* I think this works */
    i = rand();
    TPS_MAKEVALUE(v,TPSTYPE_INTEGER,i);
    TPS_PUSH(intrp,v);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_srand(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    long i;

    if(!TPS_ISTYPE(args[0],TPSTYPE_INTEGER)) return(TPSSTAT_TYPECHECK);
    i = TPS_INTEGER_OF(args[0]);
    SRAND(i);
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}
#endif /*HASRAND*/

/**************************************************/

Tps_Status
Tps_op_gc(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    intrp->gc();
    return TPSSTAT_OK;
}

/**************************************************/

Tps_Status
Tps_op_stateexec(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;

    ok = Tps_create_statemark(intrp);
    if(ok != TPSSTAT_OK) return ok;
    ok = Tps_create_source(intrp,args[0],FALSE);
    if(ok != TPSSTAT_OK) return ok;
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_statesave(Tps_Interp* intrp, Tps_Value* /*args*/, long /*nargs*/)
{
    Tps_Status ok;
    char* s;
    Tps_String* ss;
    Tps_Value v;

    TPS_GUARANTEE(intrp,2);
    /* save state as string on the operand stack */
    intrp->_tempbuf.rewind();
    ok = Tps_export(intrp,&intrp->_tempbuf);
    if(ok != TPSSTAT_OK) return ok;
    intrp->_tempbuf.ends();
    s = intrp->_tempbuf.contents();
    ss = new Tps_String(s);
    TPS_MAKEVALUE(v,TPSTYPE_STRING,ss);
    /* return <string> TRUE as the result */
    TPS_PUSH(intrp,v);
    TPS_PUSH(intrp,TPS__CONST(TPS__TRUE));
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_staterestore(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;
    Tps_String* ss;
    Tps_Stream* f;
    boolean clr;

    if(!TPS_ISTYPE(args[0],TPSTYPE_BOOLEAN)) return TPSSTAT_TYPECHECK;
    clr = TPS_BOOLEAN_OF(args[0]);
    switch (TPS_TYPE(args[1])) {
	case TPSTYPE_STRING:
	    ss = TPS_STRING_OF(args[1]);
	    f = &intrp->_inbuf;
	    ok = ((Tps_Stream_String*)f)->open(ss->contents(),ss->length());
	    if(ok != TPSSTAT_OK) return ok;
	    break;
	case TPSTYPE_STREAM:
	    f = TPS_STREAM_OF(args[1]);
	    break;
	default: return TPSSTAT_TYPECHECK;
    }
    TPS_POPN(intrp,2); /* remove args */
    /* decode the state */
    ok = Tps_import(intrp,f,clr);
    if(ok != TPSSTAT_OK) return ok;
    /* Push FALSE */
    TPS_GUARANTEE(intrp,1);
    TPS_PUSH(intrp,TPS__CONST(TPS__FALSE));
    return ok;
}

/**************************************************/
Tps_Status
Tps_op_cvunsafe(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    if(intrp->safe()) return TPSSTAT_OK;
    TPS_SET_UNSAFE(args[0],1);
    return TPSSTAT_OK;
}

/* Get the inverse of the unsafe flag for a value */
Tps_Status
Tps_op_safecheck(Tps_Interp* /*intrp*/, Tps_Value* args, long /*nargs*/)
{
    boolean b = TPS_ISUNSAFE(args[0]);
    if(b)
	args[0] = TPS__CONST(TPS__FALSE);
    else
	args[0] = TPS__CONST(TPS__TRUE);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_safeexec(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Status ok;

    if(!intrp->safe()) {
	ok = Tps_create_safety(intrp,TRUE);
	if(ok != TPSSTAT_OK) return ok;
    }
    ok = Tps_create_source(intrp,args[0],FALSE);
    if(ok != TPSSTAT_OK) return ok;
    TPS_POPN(intrp,1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_safestate(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    TPS_GUARANTEE(intrp,1);
    if(intrp->safe())
	TPS_PUSH(intrp,TPS__CONST(TPS__TRUE));
    else
	TPS_PUSH(intrp,TPS__CONST(TPS__FALSE));
    return TPSSTAT_OK;
}

/**************************************************/
#if OO

Tps_Status
Tps_op_cvmethod(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    TPS_SET_METHOD(args[0],1);
    TPS_SET_EXECUTABLE(args[0],1);
    return TPSSTAT_OK;
}

Tps_Status
Tps_op_clonedeep(Tps_Interp* intrp, Tps_Value* args, long /*nargs*/)
{
    Tps_Dict* dthis; /* current OLD dict */
    Tps_Dict* dcopy; /* copy of current old dict */
    Tps_Dict* dprev; /* last dict copy */
    Tps_Status ok;
    Tps_Value v;
    Tps_Dictpair pair;
    Tps_Dictpair* pairp;

    v = *args;
    if(!TPS_ISTYPE(v,TPSTYPE_DICT)) return TPSSTAT_TYPECHECK;
    /* get starting dict */
    dthis = TPS_DICT_OF(v);
    dprev = (Tps_Dict*)0;
    pair._key = TPS__NM(TPS_NMSUPER);
    while(1) {
	/* dup current dict */
	dcopy = new Tps_Dict_Tcl;
	if(!dcopy) return TPSSTAT_VMERROR;
	ok = dcopy->copy(dthis);
	if(ok != TPSSTAT_OK) return ok;
	/* relink the super sequence */
	if(dprev) {
	    TPS_MAKEVALUE(pair._value,TPSTYPE_DICT,dcopy);
	    TPS_SET_EXECUTABLE(pair._value,1);
	    ok = dprev->insert(pair,&v);
	    if(ok != TPSSTAT_OK) return ok;
	}
	dprev = dcopy;
	/* get next dict up super chain */
	ok = dthis->lookup(pair._key,&pairp);
	if(ok == TPSSTAT_UNDEFINED) break;
	if(ok != TPSSTAT_OK) return ok;
	if(!TPS_ISEXDICT(pairp->_value)) break;
	dthis = TPS_DICT_OF(pairp->_value);
    }
    /* chain stopped; overwrite old starting dict */
    TPS_MAKEVALUE(*args,TPSTYPE_DICT,dcopy);
    TPS_SET_EXECUTABLE(*args,1);
    return TPSSTAT_OK;
}

#endif
