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

#if VERBOSE
#include "debug.H"
#endif

Tps_Container::Tps_Container(Tps_Typeid t)
{
    _tid = t;
    unmark();
    link(tpsg._objects);
    setuid(-1);
}

void
Tps_mark(Tps_Value v)
{
    switch (TPS_TYPE(v)) {
	case TPSTYPE_DICT:
	case TPSTYPE_ARRAY:
	case TPSTYPE_STRING:
	case TPSTYPE_STREAM:
		TPS_GC_OF(v)->mark(); break;
	default: break; /* no need to mark */
    }
}

void
Tps_Interp::gc(void)
{
    register Tps_Value* vp;
    register Tps_Dict* dp;
    register Tps_List* lp;
    register char* ep;
    register Tps_Frame* eframe;
    register long i;
    register long sf;

    GLOBAL_LOCK();
    /* walk the operand stack and mark */
    for(vp=TPS_TOSP(this),i=TPS_DEPTH(this);i--;vp++) {
	Tps_mark(*vp);
    }
    /* walk the dict stacks and mark */
    for(sf=0;sf<2;sf++) {
	for(vp=TPS_DTOSPSF(this,sf),i=TPS_DDEPTHSF(this,sf);i--;vp++) {
	    if(TPS_ISTYPE(*vp,TPSTYPE_DICT)) {
		dp = TPS_DICT_OF(*vp);
		dp->mark();
	    }
	}
    }
    /* walk the exec stack and mark */
    for(ep=TPS_ETOSP(this),i=TPS_EFRAMECOUNT(this);i--;) {
	eframe = (Tps_Frame*)ep;
	(void)TPS_FRAME_MARK(this,eframe);
	ep += TPS_FRAME_LENGTH(this,eframe);
    }
    /* mark misc other items */
    _userdicts[0]->mark();
    _userdicts[1]->mark();
    _inbuf.mark();
    _tokenbuf.mark();
    _tempbuf.mark();
    Tps_mark(_object);
    tpsg._stdcons->mark();
    tpsg._stdin->mark();
    tpsg._stdout->mark();
    tpsg._stderr->mark();
    tpsg._tempbuf->mark();
    tpsg._systemdict->mark();
    tpsg._nametable->mark();

    /* go thru and reclaim unused objects, and unmark others */
    for(lp=tpsg._objects->next();lp;lp=lp->next()) {
	register Tps_Container* q = (Tps_Container*)lp;
	if(!q->marked()) {
#if VERBOSE > 2
	    /* indicate what is being reclaimed */
	    register Tps_Typeid tid = q->tid();
	    switch (tid) {
		case TPSTYPE_DICT:
		case TPSTYPE_ARRAY:
		case TPSTYPE_STRING:
		case TPSTYPE_STREAM: {
		    register char* s;
		    Tps_Value v;
		    TPS_MAKEVALUE(v,tid,q);
		    s = debugobject(v);
		    TPS_STDCONS->printf("reclaimed: %s\n",s);
		    break;
		}
		default: /* better not happen */
		    TPS_STDCONS->printf("!gc: attempt to reclaim object of type %s\n", TPS_TNM(tid));
		    break;
		
	    }
#endif /*VERBOSE*/
	    delete q;
	} else {
	    q->unmark();
	}
    }
    GLOBAL_UNLOCK();
}
