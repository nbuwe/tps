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

#if VERBOSE
#include "debug.H"
#endif

Tps_List::~Tps_List()
{
    unlink();
}

Tps_Container::Tps_Container(Tps_Typeid t)
{
    _tid = t;
    unmark();
    link(tpsg._objects);
    setuid(-1);
}

Tps_Container::~Tps_Container()
{
    return;
}


void
Tps_Container::mark()
{
    _marked = TRUE;
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
Tps_Interp::gc()
{
    Tps_Value* vp;
    Tps_Dict* dp;
    Tps_List* lp;
    char* ep;
    Tps_Frame* eframe;
    long i;
    long sf;

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
    lp = tpsg._objects->next();
    while (lp != NULL) {
	Tps_Container* q = (Tps_Container*)lp;
	lp = lp->next();
	if(!q->marked()) {
#if VERBOSE > 2
	    /* indicate what is being reclaimed */
	    Tps_Typeid tid = q->tid();
	    switch (tid) {
		case TPSTYPE_DICT:
		case TPSTYPE_ARRAY:
		case TPSTYPE_STRING:
		case TPSTYPE_STREAM: {
		    const char* s;
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
