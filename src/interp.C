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

#include "tps.H"
#include "exec.H"
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
#if VERBOSE
#include "debug.H"
#endif

#if defined sun && ! defined solaris2
EXTERNC int gettimeofday(struct timeval*,struct timezone*);
#endif

EXTERNC int getrusage(int,struct rusage*);

#ifndef NOINLINE
Tps_Status Tps_source_reenter(TPS_REENTER_ARGS);
#endif /*NOINLINE*/

/*
Define the default parameters.
*/

#define RECURSIONDEPTH	"256"

/* Stacklengths are in bytes */
#define STACKALLOC	"8192" /* 1024 operands */
#define DSTACKALLOC	"512"  /* 64 dicts; must hold at least 4 dicts */
#define ESTACKALLOC	"4096"     /* approx. 256 frames */

/**************************************************/

static
long
findparm(const char* pname, const char* alternate)
{
    long value;
    const char* parm;
    parm = getenv(pname);
    if(!parm) {
	if(!alternate) return -1;
	parm = alternate;
    }
    value = strtol(parm,(char**)0,0);
    return value;
}

static
int
roundup(long value, u_long size)
{
    return ((value+size-1)/size)*size;
}

/**************************************************/

Tps_Interp::Tps_Interp()
{
    int i;

#if defined hpux || solaris2
    {
	struct tms t;
	times(&t);
	_usertime.tv_usec = t.tms_utime * tpsg._clockres;
	if(gettimeofday(&_realtime,(struct timezone*)0) < 0) goto syserr;
    }
#else
    {
	struct rusage r;
	if(getrusage(RUSAGE_SELF,&r) < 0) goto syserr;
	_usertime = *(struct timeval*)&r.ru_utime;
	if(gettimeofday(&_realtime,(struct timezone*)0) < 0) goto syserr;
    }
#endif
    if(_tempbuf.open() != TPSSTAT_OK) goto syserr;
    if(_tokenbuf.open() != TPSSTAT_OK) goto syserr;
    if(!_tokenbuf.guarantee(4096)) goto syserr;

    { long salloc,dalloc,ealloc;
    /* create stacks */
    salloc = findparm("STACKALLOC",STACKALLOC);
    /* round up to units of sizeof(Tps_Value) */
    salloc = roundup(salloc,sizeof(Tps_Value));

    dalloc = findparm("DSTACKALLOC",DSTACKALLOC);
    /* round up to units of sizeof(Tps_Value) */
    dalloc = roundup(dalloc,sizeof(Tps_Value));

    ealloc = findparm("ESTACKALLOC",ESTACKALLOC);
    /* round up to units of sizeof(Tps_Frame) */
    ealloc = roundup(ealloc,sizeof(Tps_Frame));

    _allstacks_alloc = salloc+ealloc+2*dalloc;

    _allstacks = Tps_malloc(_allstacks_alloc);
    if(!_allstacks) goto vmerr;

    /* now set up the stacks within allstacks */
    _stack._alloc = salloc;
    _stack._stack = (Tps_Value*)_allstacks;
    _stack._last = (Tps_Value*)(_stack._alloc+(char*)_stack._stack);
    _stack._tos = _stack._last;

    _dstacks[0]._alloc = dalloc;
    _dstacks[0]._stack = (Tps_Value*)_stack._last;
    _dstacks[0]._last = (Tps_Value*)(_dstacks[0]._alloc
				     + (char*)_dstacks[0]._stack);
    _dstacks[0]._tos = _dstacks[0]._last;

    _dstacks[1]._alloc = dalloc;
    _dstacks[1]._stack = (Tps_Value*)_dstacks[0]._last;
    _dstacks[1]._last = (Tps_Value*)(_dstacks[1]._alloc
				     + (char*)_dstacks[1]._stack);
    _dstacks[1]._tos = _dstacks[1]._last;

    _dstack = &_dstacks[0];

    _estack._alloc = ealloc;
    _estack._stack = (char*)_dstacks[1]._last;
    _estack._last = (_estack._alloc+(char*)_estack._stack);
    _estack._tos = _estack._last;

    TPS_EFRAMECOUNT(this) = 0;
    }

    /* create safety mode specific dicts */
    for(i=0;i<2;i++) {
	_userdicts[i] = new Tps_Dict_Tcl(1,i?"safeuserdict":"userdict");
	if(!_userdicts[i]) goto vmerr;
	TPS_MAKEVALUE(__userdicts[i],TPSTYPE_DICT,_userdicts[i]);
    }

    /* initialize the dict stacks */
    _dstack = _dstacks + 1;
    TPS_DPUSH(this,tpsg.__systemdict);
    TPS_DPUSH(this,__userdicts[1]);
    _dstack = _dstacks + 0;
    TPS_DPUSH(this,tpsg.__systemdict);
    TPS_DPUSH(this,__userdicts[0]);
    /* _dstack = _dstacks + 0; */

#if VERBOSE > 1
    TPS_STDCONS->printf("dstack:\n%s\n",debugdstacks(this));
#endif

    /* initialize misc. interpreter state */
    _step = 0;
    _tracing = 0;
    _traceskip = 0;

    _object = TPS__CONST(TPS__NULL);
    _throwflag = TPS__CONST(TPS__NULL);
    _status = TPSSTAT_OK;

    _self = TPS__CONST(TPS__NULL);

    _safe = 0; /* start in unsafe mode */
    _safeprefix = STRDUP(Tps_safefileprefix);
    _tpsrc = STRDUP(Tps_tpsrc);

#if VERBOSE > 1
    TPS_STDCONS->printf("userdict: %s\n",debugobject(__userdicts[0]));
#endif

    return;
vmerr:
    _status = TPSSTAT_VMERROR;
    return;
syserr:
    _status = TPSSTAT_SYSTEMERROR;
    return;
}

Tps_Interp::~Tps_Interp()
{
    Tps_free(_allstacks);
    Tps_free(_safeprefix);
    _tokenbuf.close();
}

/**************************************************/
void
Tps_Interp::safeprefix(const char* s)
{
    if(_safeprefix) Tps_free(_safeprefix);
    _safeprefix = STRDUP(s);
}

void
Tps_Interp::tpsrc(const char* s)
{
    if(_tpsrc) Tps_free(_tpsrc);
    _tpsrc = STRDUP(s);
}

/**************************************************/
Tps_Status
Tps_Interp::where(Tps_Value key, Tps_Value* valp, Tps_Value* where)
{
    Tps_Status ok;
    long w;
    Tps_Dictpair* pairp;

    ok = Tps_dictstack_lookup(TPS_DTOSP(this),TPS_DDEPTH(this),
			      key,&w,&pairp);
    if(ok != TPSSTAT_OK) return ok;
    if(valp) *valp = pairp->_value;
    where = TPS_DTOSP(this) + w;
    return TPSSTAT_OK;
}

Tps_Status
Tps_Interp::def(Tps_Value key, Tps_Value val, Tps_Value* where)
{
    Tps_Status ok;
    long w;
    Tps_Dictpair pair;

    pair._key = key;
    pair._value = val;
    ok = Tps_dictstack_define(TPS_DTOSP(this),TPS_DDEPTH(this),pair,&w);
    if(ok != TPSSTAT_OK) return ok;
    where = TPS_DTOSP(this) + w;
    return TPSSTAT_OK;
}
/**************************************************/

void
Tps_Interp::reset()
{
    _object = TPS__CONST(TPS__NULL);
    _throwflag = TPS__CONST(TPS__NULL);
    _tracing = FALSE;
    _traceskip = FALSE;
    TPS_CLEAR(this);
    TPS_DCLEARSF(this,0);
    TPS_DCLEARSF(this,1);
    TPS_ECLEAR(this);
    TPS_EFRAMECOUNT(this) = 0;
    _dstack = &_dstacks[0];
    _safe = 0;
    _step = 0;
    _tracing = 0;
    _traceskip = 0;
    _object = TPS__CONST(TPS__NULL);
    _throwflag = TPS__CONST(TPS__NULL);
    _status = TPSSTAT_OK;
}
/**************************************************/
/*
Find an object and execute it
*/

Tps_Status
Tps_Interp::run()
{
    Tps_Status ok;
    Tps_Frame* frame;
    boolean traceoff;
    long nargs;
    Tps_Operator *op;
    Tps_Value* args;
    Tps_Nameid nm;
    Tps_Value object;

    /* run forever, unless stepping, or until quit */
    do {
retry:
	if(TPS_EFRAMECOUNT(this) == 0) {
	    ok = TPSSTAT_QUIT;
	    goto quiting;
	}
	frame = (Tps_Frame*)TPS_ETOSP(this);
	/* invoke the frame specific reenter function */
#ifndef NOINLINE
	if(frame->_handler->_reenter == Tps_source_reenter) {
	    Tps_Frame_Source* f = (Tps_Frame_Source*)frame;
	    if(f->_index < 0) {
		/* singular object */
		object = (_object = f->_body);
		/* unwind this frame directly */
		TPS_EPOPN(this,sizeof(Tps_Frame_Source));
		TPS_EFRAMECOUNT(this)--;
		if(f->_index-- != -1) goto retry; /* pop frame */
		/* tail frame */
	    } else { /* f->_index >= 0 => array to step thru */
		Tps_Array* abody = TPS_ARRAY_OF(f->_body);
		if(f->_index >= abody->length()) {
		    /* unwind this frame directly */
		    TPS_EPOPN(this,sizeof(Tps_Frame_Source));
		    TPS_EFRAMECOUNT(this)--;
		    goto retry;
		}
		object = (_object = abody->contents()[f->_index++]);
	    }
	    goto dereffed;
	}
#endif /*NOINLINE*/
	ok = TPS_FRAME_REENTER(this,frame); // result in _object
	switch (ok) {
	    case TPSSTAT_OK:
		/* Found an object; frame not finished */
		break;
	    case TPSSTAT_RETRYFRAME:
		/* new frame established, retry */
		goto retry;
	    case TPSSTAT_POPFRAME:
		/* completed frame encountered before obtaining object */
		/* pop to an appropriate frame */
#ifdef NOINLINE
		(void)unwind_thru(frame);
#else /*!NOINLINE*/
		/* unwind this frame directly */
		(void)TPS_FRAME_UNWIND(this,frame,FALSE);
#endif /*NOINLINE*/
		goto retry;
	    case TPSSTAT_TAILFRAME:
		/* completed frame encountered as well as providing object */
		/* pop to an appropriate frame */
#ifdef NOINLINE
		(void)unwind_thru(frame);
#else /*!NOINLINE*/
		/* unwind this frame directly */
		(void)TPS_FRAME_UNWIND(this,frame,FALSE);
#endif /*NOINLINE*/
		break;
	    case TPSSTAT_INTERRUPT:
	    case TPSSTAT_QUIT:
		goto quiting;
	    default:
		/* presume that this is some form of fatal error */    
		goto fatal;
	}
	/* execute the object */
	object = _object;
dereffed:
	/* trace processing */
	traceoff = TPS_ISTRACEOFF(object);
	if(_tracing) {
	    if(!_traceskip) goto traceit;
	    _traceskip = 0;
	}
	if(!TPS_ISEXECUTABLE(object)) goto stackit;
	switch(TPS_TYPE(object)) {
	    case TPSTYPE_OPERATOR:
		/* check safety of the operator */
		if(TPS_ISUNSAFE(object)) {
#ifndef TRACEALL
		    /* turn off tracing */
		     ok = Tps_create_trace(this,0);
		     if(ok != TPSSTAT_OK) goto handle_error;
#endif
		    TPS_GUARANTEE(this,1);
		    /* conditionally enter unsafe mode */
		    if(safe()) {
			/* push safety state */
			TPS_PUSH(this,TPS__CONST(TPS__TRUE));
			ok = Tps_create_safety(this,0);
			if(ok != TPSSTAT_OK) goto handle_error;
		    } else {
			TPS_PUSH(this,TPS__CONST(TPS__FALSE));
		    }
		}
		op = TPS_OPERATOR_OF(object);
		nargs = TPS_DEPTH(this);
		args = TPS_TOSP(this);
#if VERBOSE
TPS_STDCONS->printf("executing%s%s: %s%s/%d:%d\n",
		    tracing()?"+":"",
		    safe()?"*":"",
		    Tps_operator_prefix,
		    op->name(),
		    op->arity(),
		    nargs
		   );
#endif
		/* check for correct arity */
		if(op->arity() > nargs) {
		    ok = TPSSTAT_STACKUNDERFLOW;
		    goto handle_error;
		}
		ok = op->invoke(this,args,nargs);
		if(ok != TPSSTAT_OK) goto handle_error;
		break;
	    case TPSTYPE_NAME: /* actually, executable names
				or methods only */
		nm = TPS_NAME_OF(object);
#if OO
		/* handle method name */
		if(TPS_ISMETHOD(object)) {
		    /* if top element is executable dict, then
			begin a new self object.
			Otherwise, apply method against current
			self dict.
		    */
		    Tps_Dictpair* pairp;
		    Tps_Dict* d = TPS_DICT_OF(_self); /*default*/
		    if(TPS_DEPTH(this) > 0) {
			Tps_Value v = TPS_TOP(this);
			if(TPS_ISTYPE(v,TPSTYPE_DICT) && TPS_ISEXECUTABLE(v)) {
			    /* create a new self */
			    ok = Tps_create_OO(this,v);
			    TPS_POPN(this,1);
		    	    d = TPS_DICT_OF(_self);
			} else if(TPS_ISTYPE(v,TPSTYPE_NAME)) {
			    Tps_Nameid nm = TPS_NAME_OF(v);
			    if(nm == TPS_NM(TPS_NMSUPER)) {
				/* find super */
				if((d)->lookup(v,&pairp) != TPSSTAT_OK
				   || !TPS_ISTYPE(pairp->_value,TPSTYPE_DICT)){
				    /* no super parent, bail out */
				    ok = TPSSTAT_UNDEFINED;
				    goto handle_error;
				}
				TPS_POPN(this,1);
				d = TPS_DICT_OF(pairp->_value);
			    } else if(nm == TPS_NM(TPS_NMSUPER)) {
				TPS_POPN(this,1);
			    }
			}
		    }
		    while(1) { /* search the super chain, starting with self */
			if((d)->lookup(object,&pairp) == TPSSTAT_OK) {
			    _object = (object = pairp->_value);
			    /* check for executable array */
			    if(TPS_ISEXARRAY(object)) goto execarray;
			    goto dereffed;
			}
			/* move to next dict in super chain */
			if((d)->lookup(TPS__NM(TPS_NMSUPER),&pairp) != TPSSTAT_OK) {
			    /* no super parent, bail out */
			    ok = TPSSTAT_UNDEFINED;
			    goto handle_error;
			}
			if(!TPS_ISTYPE(pairp->_value,TPSTYPE_DICT)
			   || !TPS_ISEXECUTABLE(pairp->_value)) {
			    ok = TPSSTAT_TYPECHECK;
			    goto handle_error;
			}
			d = TPS_DICT_OF(pairp->_value);
		    } /*while*/
		    /*NOTREACHED*/
		}
#endif
		do {/* unref executable name one level for each pass*/
		    Tps_Dictpair* pairp;
#ifdef NOINLINE
		    ok = Tps_dictstack_lookup(TPS_DTOSP(this),
						TPS_DDEPTH(this),
						object,NULL,&pairp);
		    if(ok != TPSSTAT_OK) goto handle_error;
#else /*!NOINLINE*/
		    /*lookup and retry; == inline version of dictstack lookup*/
		    Tps_Value* dp;
		    long depth;
		    /* look up name in current name environment */
		    /* inline version ofTps_dictstack_lookup */
		    /* remember dstack grows down */
		    dp = TPS_DTOSP(this);
		    depth = TPS_DDEPTH(this);
		    while(depth > 0 && TPS_ISTYPE(*dp,TPSTYPE_DICT)) {
			Tps_Dict* d = TPS_DICT_OF(*dp);
			if((d)->lookup(object,&pairp) == TPSSTAT_OK) {
			    goto namefound;
			}
			dp++; depth--;
		    }
		    ok = TPSSTAT_UNDEFINED;
		    goto handle_error;

namefound:
#endif /*NOINLINE*/
		    object = pairp->_value;
		    if(TPS_NAME_OF(object) == nm && TPS_ISEXECUTABLE(object)) {
			/* loop in name lookup */
			ok = TPSSTAT_INVALIDACCESS;
			goto handle_error;
		    }
	        } while(TPS_ISEXNAME(object));
		/* check for executable array */
		if(!TPS_ISEXARRAY(object)) {
		    _object = object;
		    goto dereffed;
		}
#if OO
execarray:
#endif
		/* check safety of the operator */
		if(TPS_ISUNSAFE(object)) {
#ifndef TRACEALL
		    /* turn off tracing */
		     ok = Tps_create_trace(this,0);
		     if(ok != TPSSTAT_OK) goto handle_error;
#endif
		    TPS_GUARANTEE(this,1);
		    /* conditionally enter unsafe mode */
		    if(safe()) {
			/* push safety state */
			TPS_PUSH(this,TPS__CONST(TPS__TRUE));
			ok = Tps_create_safety(this,0);
			if(ok != TPSSTAT_OK) goto handle_error;
		    } else {
			TPS_PUSH(this,TPS__CONST(TPS__FALSE));
		    }
		}
#ifdef NOINLINE
		ok = Tps_create_source(this,object);
		if(ok != TPSSTAT_OK) goto handle_error;
#else /*!NOINLINE*/
		Tps_Frame_Source* f;
		/* also inline Tps_create_frame */
		if(TPS_ROOM(this) < sizeof(Tps_Frame_Source)) goto vmerr;
		f = (Tps_Frame_Source*)TPS_EPUSHN(this,sizeof(Tps_Frame_Source));
		f->_handler = &Tps_handler_source;
	        f->_framelength = sizeof(Tps_Frame_Source);
		TPS_EFRAMECOUNT(this)++;
		f->_body = object;
		f->_index = (TPS_ISEXARRAY(object))?0:-1;
#endif /*NOINLINE*/
		goto retry;
	    default:
stackit:
		TPS_GUARANTEE(this,1);
		TPS_PUSH(this,object);
		break;
	}
	continue; /* while(!_step)*/

traceit:
    /* turn off tracing and invoke tracetrap */
    ok = Tps_create_trace(this,0);
    if(ok != TPSSTAT_OK) goto handle_error;
    /* push object being traced */
    TPS_GUARANTEE(this,1);
    TPS_PUSH(this,object);
    /* arrange for ``tracetrap'' to be executed untraced */
    object = TPS__NM(TPS_NMTRACETRAP);
    TPS_SET_EXECUTABLE(object,1);
    ok = Tps_create_source(this,object);
    if(ok != TPSSTAT_OK) goto handle_error;
    /* continue execution */
    goto retry;

handle_error:
	if(ok == TPSSTAT_INTERRUPT
	   || ok == TPSSTAT_QUIT
	   || ok == TPSSTAT_VMERROR
	   || ok == TPSSTAT_SYSTEMERROR
	   || ok == TPSSTAT_EXECSTACKUNDERFLOW)
		goto fatal;
	/* stack current object and the error */
	TPS_GUARANTEE(this,2);
	TPS_PUSH(this,object);
	TPS_PUSH(this,TPS__ENM(ok));
#ifndef TRACEALL
	/* cause execution of errortrap with tracing suppressed */
	ok = Tps_create_trace(this,0);
	if(ok != TPSSTAT_OK) goto handle_error;
#endif
	object = TPS__NM(TPS_NMERRTRAP);
	TPS_SET_EXECUTABLE(object,1);
	ok = Tps_create_source(this,object);
	if(ok != TPSSTAT_OK) goto handle_error;
	goto retry;

/* vmerr and syserr are not fixable */
vmerr:
	ok = TPSSTAT_VMERROR;
	goto fatal;
syserr:
	ok = TPSSTAT_SYSTEMERROR;
	goto fatal;		
fatal:
	return ok;
quiting:
	return ok;

    } while(!_step);
    return TPSSTAT_OK;
}

/**************************************************/

Tps_Status
Tps_Interp::load(const char* code)
{
    Tps_Value v;
    Tps_Status ok;

    if(!code || strlen(code) == 0) return TPSSTAT_TYPECHECK;
    (void)_inbuf.open(code);
    /* get a token */
    ok = Tps_get_token(_tokenbuf,&_inbuf,&v,0);
    (void)_inbuf.close();
    if(ok == TPSSTAT_OK) {
	ok = Tps_create_source(this,v);
    }
    return ok;
}

/**************************************************/
/*
Unwind the exec stack thru a specified frame is reached
or until a TPSSTAT_STOP is returned, or error.
If lastframe is null, try to unwind everything
*/

/* This function is inlined elsewhere; look for ifdef NOINLINE */

Tps_Status
Tps_Interp::unwind_thru(Tps_Frame* lastframe, boolean thrown)
{
    char* enext;
    char* elast;
    Tps_Status ok;
    Tps_Frame* eframe;
    boolean stopsafe;

    elast = lastframe?((char*)lastframe):TPS_EBASE(this);	
    if(lastframe) elast += TPS_FRAME_LENGTH(this,lastframe);
    for(enext=TPS_ETOSP(this);enext < elast;enext=TPS_ETOSP(this)) {
	eframe = (Tps_Frame*)enext;
	stopsafe = _safe;
	switch(ok = TPS_FRAME_UNWIND(this,eframe,thrown)) {
	    case TPSSTAT_OK: // frame removed
		break;
	    case TPSSTAT_RETRYFRAME: // frame still on stack
	    case TPSSTAT_STOP: // frame removed
		goto done;
	    default:
		goto done; /* failure */
	}
    }
    /* if we reach here, then possibly ran out of exec stack */
    if(!lastframe)
	ok = TPSSTAT_EXECSTACKUNDERFLOW;

done:
    return ok;
}	

/**************************************************/

Tps_Status
Tps_Interp::save(char*& state)
{
    Tps_Status ok;
    long l;
    char* s;

    TPS_GUARANTEE(this,1);
    _tempbuf.rewind();
    ok = Tps_export(this,&_tempbuf);
    if(ok != TPSSTAT_OK) return ok;
    _tempbuf.ends();
    s = _tempbuf.contents();
    l = strlen(s);
    state = Tps_malloc(l+1);
    strcpy(state,s);
    return TPSSTAT_OK;
}

Tps_Status
Tps_Interp::restore(char* state)
{
    Tps_Status ok;

    ok = _inbuf.open(state);
    if(ok != TPSSTAT_OK) return ok;
    ok = Tps_import(this,&_inbuf,TRUE);
    return ok;
}

Tps_Status
Tps_Interp::restore(int fd)
{
    Tps_Status ok;
    Tps_Stream_File f;

    ok = f.attach(fd,"restore");
    if(ok != TPSSTAT_OK) return ok;
    ok = Tps_import(this,&f,TRUE);
    return ok;
}

/**************************************************/
Tps_Status
Tps_Interp::newoperator(const char* nm, long ar, Tpsstatfcn pr)
{
    Tps_Operator* op;
    Tps_Nameid id;
    Tps_Dictpair pair;
    char s[256+3];

    if(strlen(nm) >= 256) return TPSSTAT_FAIL;
    op = new Tps_Operator;
    if(op) {
	id = TPS_NAMETABLE->newname(nm);
	op->name(id);
	op->arity(ar);
	op->proc(pr);
	op->flags(0);
	TPS_MAKEVALUE(pair._value,TPSTYPE_OPERATOR,op);
	TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,id);
	/* make all primitives untraceable and executable */
	TPS_SET_TRACEOFF(pair._value,1);
	TPS_SET_EXECUTABLE(pair._value,1);
	/* assume user defined primitives are safe */
	TPS_SET_UNSAFE(pair._value,0);
	TPS_SYSTEMDICT->insert(pair,(Tps_Value*)NULL);
	strcpy(s,Tps_operator_prefix);
	strcat(s,id);
	id = TPS_NAMETABLE->newname(s);
	TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,id);
	TPS_SYSTEMDICT->insert(pair,(Tps_Value*)NULL);
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_Interp::destroyoperator(char* nm)
{
    Tps_Operator* op;
    Tps_Nameid id;
    Tps_Status ok;
    Tps_Dictpair* pairp;
    Tps_Value v;
    char s[256+3];

    if(strlen(nm) >= 256) return TPSSTAT_FAIL;
    id = TPS_NAMETABLE->newname(nm);
    TPS_MAKEVALUE(v,TPSTYPE_NAME,id);
    ok = TPS_SYSTEMDICT->lookup(v,&pairp);
    if(ok != TPSSTAT_OK) return ok;        
    if(!TPS_ISTYPE(pairp->_value,TPSTYPE_OPERATOR)) return TPSSTAT_UNDEFINED;
    op = TPS_OPERATOR_OF(pairp->_value);
    TPS_SYSTEMDICT->remove(v,(Tps_Dictpair*)0);

    strcpy(s,Tps_operator_prefix);
    strcat(s,id);
    id = TPS_NAMETABLE->newname(s);
    TPS_MAKEVALUE(v,TPSTYPE_NAME,id);
    ok = TPS_SYSTEMDICT->lookup(v,&pairp);
    if(ok == TPSSTAT_OK
	&& TPS_ISTYPE(pairp->_value,TPSTYPE_OPERATOR)
	&& op == TPS_OPERATOR_OF(pairp->_value)) {
	TPS_SYSTEMDICT->remove(v,(Tps_Dictpair*)0);
    }
    delete op;
    return TPSSTAT_OK;
}

/**************************************************/

/* Allow user to override what std{io,in,out} map to when the interpreter
   uses them.
*/

Tps_Stream*
Tps_Interp::stdstream(Tps_Stdio io)
{
    Tps_Value key;
    Tps_Value val;
    Tps_Stream* found = (Tps_Stream*)NULL;

    switch (io) {
	case Tps_stdcons: key = TPS__NM(TPS_NMSTDCONS); break;
	case Tps_stdin: key = TPS__NM(TPS_NMSTDIN); break;
	case Tps_stdout: key = TPS__NM(TPS_NMSTDOUT); break;
	case Tps_stderr: key = TPS__NM(TPS_NMSTDERR); break;
	default: return found;
    }
    /* get the definition for this stream */
    if(where(key, &val) == TPSSTAT_OK) {
	if(TPS_ISTYPE(val,TPSTYPE_STREAM)) {
	    found = TPS_STREAM_OF(val);
	}
    } else {
	switch (io) {
	    case Tps_stdcons: found = TPS_STDCONS; break;
	    case Tps_stdin: found = TPS_STDIN; break;
	    case Tps_stdout: found = TPS_STDOUT; break;
	    case Tps_stderr: found = TPS_STDERR; break;
	    default: break;
	}
    }
    if(!found) found = TPS_STDCONS; /* last ditch */
    return found;
}

/**************************************************/
Tps_Interp*
Tps_interp_create()
{
    Tps_Interp* intrp = new Tps_Interp;
    return intrp;
}

