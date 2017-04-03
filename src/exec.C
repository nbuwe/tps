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

#include "tps.H"
#include "exec.H"
#include "util.H"

/**************************************************/

/* This function is inlined elsewhere; look for ifdef NOINLINE */

Tps_Frame*
Tps_create_frame(Tps_Interp* intrp, Tps_Handler* handler, long framelen)
{
    register Tps_Frame* f;

    if(TPS_ROOM(intrp) < framelen) return (Tps_Frame*)0;
    f = (Tps_Frame*)TPS_EPUSHN(intrp,framelen);
    f->_handler = handler;
    f->_framelength = framelen;
    TPS_EFRAMECOUNT(intrp)++;
    return f;
}

/* This should be called to actually remove frame */
/* This function is inlined elsewhere; look for ifdef NOINLINE */
Tps_Status
Tps_unwind_frame(Tps_Interp* intrp, Tps_Frame* f)
{
    register long len = TPS_FRAME_LENGTH(intrp,f);
    TPS_EPOPN(intrp,len);
    TPS_EFRAMECOUNT(intrp)--;
    return TPSSTAT_OK;
}

/**************************************************/

// Default frame trace routine, partial output
Tps_Status
Tps_trace0(Tps_Interp* intrp,
	   Tps_Stream* strm,
	   Tps_Frame* f)
{
    register long len = TPS_FRAME_LENGTH(intrp,f);
    register const char* nam = TPS_FRAME_NAME(intrp,f);
    register long depth = Tps_framedepth(intrp,f);

    strm->printf("(%2ld)",depth);
    strm->printf("(%2ld) %s:",len,nam);
    return TPSSTAT_OK;
}

int
Tps_framedepth(Tps_Interp* interp, Tps_Frame* frame)
{
    register char* ep = TPS_ETOSP(interp);
    register long nframes = TPS_EFRAMECOUNT(interp);
    register Tps_Frame* eframe;

    while(ep < (char*)frame) {
	nframes--;
	eframe = (Tps_Frame*)ep;
	ep += TPS_FRAME_LENGTH(interp,eframe);
    }
    return nframes;
}

/**************************************************/

Tps_Status Tps_nullfcn_mark(TPS_MARK_ARGS0) {return TPSSTAT_OK;}
Tps_Status Tps_nullfcn_import(TPS_IMPORT_ARGS0) {return TPSSTAT_OK;}
Tps_Status Tps_nullfcn_export(TPS_EXPORT_ARGS0) {return TPSSTAT_OK;}

/**************************************************/
/* This function is inlined elsewhere; look for ifdef NOINLINE */
static
Tps_Status
Tps_source_unwind(TPS_UNWIND_ARGS1)
{
#ifdef NOINLINE
    return Tps_unwind_frame(intrp,frame);
#else /*!NOINLINE*/
    TPS_EPOPN(intrp,sizeof(Tps_Frame_Source));
    TPS_EFRAMECOUNT(intrp)--;
    return TPSSTAT_OK;
#endif /*NOINLINE*/
}

#ifdef NOINLINE
static
#endif
Tps_Status
Tps_source_reenter(TPS_REENTER_ARGS)
{
    register Tps_Frame_Source* f = (Tps_Frame_Source*)frame;
    if(f->_index < 0) {
	/* singular object */
	if(f->_index-- != -1) return TPSSTAT_POPFRAME;
	intrp->_object = f->_body;
	return TPSSTAT_TAILFRAME; /* no longer need frame */
    } else { /* f->_index >= 0 => array to step thru */
	register Tps_Array* abody = TPS_ARRAY_OF(f->_body);
	if(f->_index >= abody->length()) return TPSSTAT_POPFRAME;
	intrp->_object = abody->contents()[f->_index++];
    }
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_source_trace(TPS_TRACE_ARGS)
{
    register Tps_Frame_Source* f = (Tps_Frame_Source*)frame;

    Tps_trace0(intrp,strm,frame);
    return Tps_cvts1(*strm,f->_body,TRUE,f->_index);
}

static
Tps_Status
Tps_source_mark(TPS_MARK_ARGS1)
{
    register Tps_Frame_Source* f = (Tps_Frame_Source*)frame;
    Tps_mark(f->_body);
    return TPSSTAT_OK;
}

Tps_Status
Tps_source_export(TPS_EXPORT_ARGS1)
{
    Tps_Value v;
    register Tps_Frame_Source* f = (Tps_Frame_Source*)frame;

    /* guarantee enough room for 2 values: body, and index*/
    TPS_GUARANTEE(intrp,2);
    /* dump body, and index */
    TPS_MAKEVALUE(v,TPSTYPE_INTEGER,f->_index);
    TPS_PUSH(intrp,v);
    TPS_PUSH(intrp,f->_body);
    count += 2;
    return TPSSTAT_OK;
}

Tps_Status
Tps_source_import(TPS_IMPORT_ARGS1)
{
    register Tps_Frame_Source* f = (Tps_Frame_Source*)fr;
    Tps_Value v;

    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt != 2) return TPSSTAT_RANGECHECK;
    cnt -= 2;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_Source*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_Source));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    } 
    /* get body and index */
    f->_body = TPS_POP(intrp);
    v = TPS_POP(intrp);
    if(!TPS_ISTYPE(v,TPSTYPE_INTEGER)) return TPSSTAT_TYPECHECK;
    f->_index = TPS_INTEGER_OF(v);
    if(f->_index >= 0 && !TPS_ISEXARRAY(f->_body)) return TPSSTAT_TYPECHECK;
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_source_parms = {"Source"};

Tps_Handler Tps_handler_source = {
	&Tps_source_parms,
	Tps_source_unwind,
	Tps_source_reenter,
	Tps_source_trace,
	Tps_source_mark,
	Tps_source_export,
	Tps_source_import
};

/* This function is inlined elsewhere; look for ifdef NOINLINE */

Tps_Status
Tps_create_source(Tps_Interp* intrp, Tps_Value body, boolean singular)
{
    register Tps_Frame_Source* f;

#ifdef NOINLINE
    if(!(f = (Tps_Frame_Source*)Tps_create_frame(intrp,&Tps_handler_source,sizeof(Tps_Frame_Source)))) return TPSSTAT_EXECSTACKOVERFLOW;
#else /*!NOINLINE*/
    if(TPS_ROOM(intrp) < sizeof(Tps_Frame_Source))
	return TPSSTAT_EXECSTACKOVERFLOW;
    f = (Tps_Frame_Source*)TPS_EPUSHN(intrp,sizeof(Tps_Frame_Source));
    if(!f) return TPSSTAT_SYSTEMERROR;
    f->_handler = &Tps_handler_source;
    f->_framelength = sizeof(Tps_Frame_Source);
    TPS_EFRAMECOUNT(intrp)++;
#endif /*NOINLINE*/
    f->_body = body;
    if(singular || !TPS_ISEXARRAY(body))
	f->_index = -1;
    else
	f->_index = 0;
    return TPSSTAT_OK;
}

/**************************************************/
static
Tps_Status
Tps_trace_unwind(TPS_UNWIND_ARGS)
{
    register Tps_Frame_Trace* f = (Tps_Frame_Trace*)frame;
    register Tps_Status ok;
    Tps_Value obj;

    /* set trace flag */
    intrp->_tracing = f->_tracing;
    ok = Tps_unwind_frame(intrp,f);
    if(ok != TPSSTAT_OK) return ok;
    /* test for /exittrace */
    if(thrown) {
	if(TPS_ISTYPE(intrp->_throwflag,TPSTYPE_NAME)
	   && TPS_NAME_OF(intrp->_throwflag) == TPS_NM(TPS_NMEXITTRACE)) {
	    /* set trace flag and signal unwind stop */
	    /* turn off tracing for one step and cause traced object
	       to execute */
	    intrp->_traceskip = 1;
	    obj = TPS_POP(intrp);
	    ok = Tps_create_source(intrp,obj,TRUE);
	    if(ok == TPSSTAT_OK)
		ok = TPSSTAT_STOP; /* exittrace stops here */
	} else {
	    /* we have a throw of something other than /exittrace,
	       unwind trace frame and pass it up
	    */
	    ok = TPSSTAT_OK;
	}
    }
    return ok;
}

static
Tps_Status
Tps_trace_reenter(TPS_REENTER_ARGS0)
{
    return TPSSTAT_POPFRAME;
}

static
Tps_Status
Tps_trace_trace(TPS_TRACE_ARGS)
{
    register Tps_Frame_Trace* f = (Tps_Frame_Trace*)frame;
    Tps_trace0(intrp,strm,frame);
    strm->printf(" tracing=%s",f->_tracing?"on":"off");
    return TPSSTAT_OK;
}

Tps_Status
Tps_trace_export(TPS_EXPORT_ARGS1)
{
    register Tps_Frame_Trace* f = (Tps_Frame_Trace*)frame;
    Tps_Value v;

    /* guarantee enough room for 1 values: traceflag */
    TPS_GUARANTEE(intrp,1);
    /* dump trace flag */
    TPS_MAKEVALUE(v,TPSTYPE_BOOLEAN,f->_tracing);
    TPS_PUSH(intrp,v);
    count++;
    return TPSSTAT_OK;
}

Tps_Status
Tps_trace_import(TPS_IMPORT_ARGS1)
{
    register Tps_Frame_Trace* f = (Tps_Frame_Trace*)fr;
    Tps_Value v;

    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt != 1) return TPSSTAT_RANGECHECK;
    cnt--;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_Trace*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_Trace));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    }
    /* get and verify trace flag */
    v = TPS_POP(intrp);
    if(!TPS_ISTYPE(v,TPSTYPE_BOOLEAN)) return TPSSTAT_TYPECHECK;
    f->_tracing = TPS_BOOLEAN_OF(v)?1:0;
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_trace_parms = {"Trace"};

Tps_Handler Tps_handler_trace = {
	&Tps_trace_parms,
	Tps_trace_unwind,
	Tps_trace_reenter,
	Tps_trace_trace,
	Tps_nullfcn_mark,
	Tps_trace_export,
	Tps_trace_import
};

Tps_Status
Tps_create_trace(Tps_Interp* intrp, boolean state)
{
    register Tps_Frame_Trace* f;

    if(!(f = (Tps_Frame_Trace*)Tps_create_frame(intrp,&Tps_handler_trace,sizeof(Tps_Frame_Trace)))) return TPSSTAT_EXECSTACKOVERFLOW;
    f->_tracing = intrp->tracing();
    intrp->tracing(state);
    intrp->_traceskip = 0;
    return TPSSTAT_OK;
}

/**************************************************/
static
Tps_Status
Tps_statemark_unwind(TPS_UNWIND_ARGS1)
{
    return Tps_unwind_frame(intrp,frame);
}

static
Tps_Status
Tps_statemark_reenter(TPS_REENTER_ARGS0)
{
    return TPSSTAT_POPFRAME;
}

static
Tps_Status
Tps_statemark_trace(TPS_TRACE_ARGS)
{
    Tps_trace0(intrp,strm,frame);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_statemark_export(TPS_EXPORT_ARGS)
{
    if(!(flags & Tps_pass_state)) return TPSSTAT_STOP;
    return TPSSTAT_OK;
}

Tps_Status
Tps_statemark_import(TPS_IMPORT_ARGS)
{
    register Tps_Frame_Statemark* f = (Tps_Frame_Statemark*)fr;

    if(!(flags & Tps_pass_state)) return TPSSTAT_FAIL;
    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt != 0) return TPSSTAT_RANGECHECK;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_Statemark*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_Statemark));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    }
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_statemark_parms = {"Statemark"};

Tps_Handler Tps_handler_statemark = {
	&Tps_statemark_parms,
	Tps_statemark_unwind,
	Tps_statemark_reenter,
	Tps_statemark_trace,
	Tps_nullfcn_mark,
	Tps_statemark_export,
	Tps_statemark_import
};

Tps_Status
Tps_create_statemark(Tps_Interp* intrp)
{
    register Tps_Frame_Statemark* f;

    if(!(f = (Tps_Frame_Statemark*)Tps_create_frame(intrp,&Tps_handler_statemark,sizeof(Tps_Frame_Statemark)))) return TPSSTAT_EXECSTACKOVERFLOW;
    return TPSSTAT_OK;
}

/**************************************************/
static
Tps_Status
Tps_safety_unwind(TPS_UNWIND_ARGS)
{
    register Tps_Frame_Safety* f = (Tps_Frame_Safety*)frame;
    register Tps_Status ok = Tps_unwind_frame(intrp,f);

    if(ok != TPSSTAT_OK) return ok;  
    /* set safe flag */
    intrp->safe(f->_safe);
    /* make _dstack match _safe */
    intrp->_dstack = intrp->_dstacks + intrp->safe();
    return ok;
}

static
Tps_Status
Tps_safety_reenter(TPS_REENTER_ARGS0)
{
    return TPSSTAT_POPFRAME;
}

static
Tps_Status
Tps_safety_trace(TPS_TRACE_ARGS)
{
    register Tps_Frame_Safety* f = (Tps_Frame_Safety*)frame;
    Tps_trace0(intrp,strm,frame);
    strm->printf(" safe=%s",f->_safe?"on":"off");
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_safety_export(TPS_EXPORT_ARGS)
{
    Tps_Value v;
    register Tps_Frame_Safety* f;

    if(!(flags & Tps_pass_safety)) return TPSSTAT_STOP;
    /* guarantee enough room for 1 value */
    TPS_GUARANTEE(intrp,1);
    f = (Tps_Frame_Safety*)frame;
    /* dump safety flag */
    TPS_MAKEVALUE(v,TPSTYPE_BOOLEAN,f->_safe);
    TPS_PUSH(intrp,v);
    count++;
    return TPSSTAT_OK;
}

Tps_Status
Tps_safety_import(TPS_IMPORT_ARGS)
{
    register Tps_Frame_Safety* f = (Tps_Frame_Safety*)fr;
    Tps_Value v;

    if(!(flags & Tps_pass_safety)) return TPSSTAT_FAIL;
    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt != 1) return TPSSTAT_RANGECHECK;
    cnt--;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_Safety*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_Safety));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    }
    /* pull safety flag */
    v = TPS_POP(intrp);
    if(!TPS_ISTYPE(v,TPSTYPE_BOOLEAN)) return TPSSTAT_TYPECHECK;
    f->_safe = TPS_BOOLEAN_OF(v)?1:0;
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_safety_parms = {"Safety"};

Tps_Handler Tps_handler_safety = {
	&Tps_safety_parms,
	Tps_safety_unwind,
	Tps_safety_reenter,
	Tps_safety_trace,
	Tps_nullfcn_mark,
	Tps_safety_export,
	Tps_safety_import
};

Tps_Status
Tps_create_safety(Tps_Interp* intrp, boolean issafe)
{
    register Tps_Frame_Safety* f;
    register Tps_Status ok = TPSSTAT_OK;

    if(!(f = (Tps_Frame_Safety*)Tps_create_frame(intrp,&Tps_handler_safety,sizeof(Tps_Frame_Safety))))
	return TPSSTAT_EXECSTACKOVERFLOW;
    f->_safe = intrp->safe();
    intrp->safe(issafe);
    /* make _dstack match _safe */
    intrp->_dstack = intrp->_dstacks + intrp->safe();
    return ok;
}

/**************************************************/
#if OO

static
Tps_Status
Tps_OO_unwind(TPS_UNWIND_ARGS)
{
    register Tps_Frame_OO* f;
    register Tps_Status ok;

    f = (Tps_Frame_OO*)frame;
    intrp->_self = f->_self;
    ok = Tps_unwind_frame(intrp,frame);
    return ok;
}

static
Tps_Status
Tps_OO_reenter(TPS_REENTER_ARGS0)
{
    return TPSSTAT_POPFRAME;
}

static
Tps_Status
Tps_OO_trace(TPS_TRACE_ARGS)
{
    register Tps_Frame_OO* f = (Tps_Frame_OO*)frame;
    Tps_trace0(intrp,strm,frame);
    return Tps_cvts1(*strm,f->_self,FALSE,-1);
}

static
Tps_Status
Tps_OO_mark(TPS_MARK_ARGS1)
{
    register Tps_Frame_OO* f = (Tps_Frame_OO*)frame;
    Tps_mark(f->_self);
    return TPSSTAT_OK;
}

static
Tps_Status
Tps_OO_export(TPS_EXPORT_ARGS)
{
    register Tps_Frame_OO* f = (Tps_Frame_OO*)frame;

    /* guarantee enough room for 1 value */
    TPS_GUARANTEE(intrp,1);
    /* dump self */
    TPS_PUSH(intrp,f->_self);
    count++;
    return TPSSTAT_OK;
}

Tps_Status
Tps_OO_import(TPS_IMPORT_ARGS)
{
    register Tps_Frame_OO* f = (Tps_Frame_OO*)fr;
    Tps_Value v;

    /* Assume that the handler name has been verified */
    /* verify length */
    if(cnt != 1) return TPSSTAT_RANGECHECK;
    cnt--;
    if(!f) {
	/* allocate frame */
	f = (Tps_Frame_OO*)Tps_create_frame(intrp,h,sizeof(Tps_Frame_OO));
	if(!f) return TPSSTAT_EXECSTACKOVERFLOW;
    } 
    /* get self */
    v = TPS_POP(intrp);
    if(!TPS_ISTYPE(v,TPSTYPE_DICT)) return TPSSTAT_TYPECHECK;
    /* force it to be executable */
    TPS_SET_EXECUTABLE(v,1);
    f->_self = v;
    return TPSSTAT_OK;
}

Tps_Frame_Parameters Tps_OO_parms = {"OO"};

Tps_Handler Tps_handler_OO = {
	&Tps_OO_parms,
	Tps_OO_unwind,
	Tps_OO_reenter,
	Tps_OO_trace,
	Tps_OO_mark,
	Tps_OO_export,
	Tps_OO_import
};

Tps_Status
Tps_create_OO(Tps_Interp* intrp, Tps_Value newself)
{
    register Tps_Frame_OO* f;

    if(!(f = (Tps_Frame_OO*)Tps_create_frame(intrp,&Tps_handler_OO,sizeof(Tps_Frame_OO)))) return TPSSTAT_EXECSTACKOVERFLOW;
    f->_self = intrp->_self;
    intrp->_self = newself;
    return TPSSTAT_OK;
}
#endif /*OO*/
