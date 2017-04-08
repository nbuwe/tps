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
#include "util.H"
#include "mem.H"
#include "debug.H"

/**************************************************/

const char*
debugobject(Tps_Value object)
{
    const char* s;

    TPS_TEMPBUF->rewind();
    (void)Tps_cvts1(*TPS_TEMPBUF,object,TRUE,-1);
    TPS_TEMPBUF->ends(); /* null terminate */
    s = STRDUP(TPS_TEMPBUF->contents());
    if(!s) s = "";
    return s;
}

const char*
debugdict(const Tps_Dict* dict)
{
    const char* s;

    TPS_TEMPBUF->rewind();
    (void)Tps_cvts1_dict_deep(*TPS_TEMPBUF,dict,FALSE);
    TPS_TEMPBUF->ends();
    s = STRDUP(TPS_TEMPBUF->contents());
    if(!s) s = "";
    return s;
}

const char*
debugdictall(const Tps_Dict* dict)
{
    const char* s;

    TPS_TEMPBUF->rewind();
    (void)Tps_cvts1_dict_deep(*TPS_TEMPBUF,dict,TRUE);
    TPS_TEMPBUF->ends();
    s = STRDUP(TPS_TEMPBUF->contents());
    if(!s) s = "";
    return s;
}

const char*
debugstack0(Tps_Interp* interp)
{
    const char* s;
    long i;
    Tps_Value* vp;

    TPS_TEMPBUF->rewind();
    vp = TPS_TOSP(interp);
    for(i=TPS_DEPTH(interp);i > 0;) {
	i--;
	(void)Tps_cvts1(*TPS_TEMPBUF,vp[i],TRUE,-1);
	TPS_TEMPBUF->write(' ');
    }
    TPS_TEMPBUF->ends();
    s = STRDUP(TPS_TEMPBUF->contents());
    if(!s) s = "";
    return s;
}

const char*
debugstack(Tps_Interp* interp)
{
    const char* s;
    long i;
    Tps_Value* vp;

    TPS_TEMPBUF->rewind();
    vp = TPS_TOSP(interp);
    for(i=TPS_DEPTH(interp);i > 0;) {
	i--;
	(void)Tps_cvts1(*TPS_TEMPBUF,vp[i],FALSE,-1);
	TPS_TEMPBUF->write(' ');
    }
    TPS_TEMPBUF->ends();
    s = STRDUP(TPS_TEMPBUF->contents());
    if(!s) s = "";
    return s;
}

const char*
debugdstack0(Tps_Interp* interp)
{
    const char* s;
    long i;
    Tps_Value* dp = TPS_DTOSP(interp);
    long dl = TPS_DDEPTH(interp);

    TPS_TEMPBUF->rewind();
    for(i=0;i < dl;i++) {
	TPS_TEMPBUF->printf("[%d]: ",i);
	(void)Tps_cvts1(*TPS_TEMPBUF,dp[i],TRUE,-1);
	TPS_TEMPBUF->endl();
    }
    TPS_TEMPBUF->ends();
    s = STRDUP(TPS_TEMPBUF->contents());
    if(!s) s = "";
    return s;
}

const char*
debugdstack(Tps_Interp* interp)
{
    const char* s;
    long i;
    Tps_Value* dv;
    long dlen;

    TPS_TEMPBUF->rewind();
    dlen = TPS_DDEPTH(interp);
    dv = TPS_DTOSP(interp) + dlen;
    for(i=0;i<dlen;i++) {
	Tps_Dict* d;
	dv--;
	d = TPS_DICT_OF(*dv);
	TPS_TEMPBUF->printf("%s ",d->name());
    }
    TPS_TEMPBUF->ends();
    s = STRDUP(TPS_TEMPBUF->contents());
    if(!s) s = "";
    return s;
}

const char*
debugdstacks(Tps_Interp* interp)
{
    const char* s;
    long i;
    long sf;
    Tps_Value* dv;
    long dlen;

    TPS_TEMPBUF->rewind();
    for(sf=0;sf<2;sf++) {
	dlen = TPS_DDEPTHSF(interp,sf);
	dv = TPS_DTOSPSF(interp,sf) + dlen;
	TPS_TEMPBUF->printf("dictstack[%d]: ",sf);
	for(i=0;i<dlen;i++) {
	    Tps_Dict* d;
	    dv--;
	    d = TPS_DICT_OF(*dv);
	    TPS_TEMPBUF->printf(" %s",d->name());
	}
	TPS_TEMPBUF->write('\n');
    }
    TPS_TEMPBUF->ends();
    s = STRDUP(TPS_TEMPBUF->contents());
    if(!s) s = "";
    return s;
}

const char*
debugexec0(Tps_Interp* interp, long nframes)
{
    char* ep = TPS_ETOSP(interp);
    char* elast = interp->_estack._last;
    Tps_Frame* eframe;
    const char* s;

    TPS_TEMPBUF->rewind();
    if(nframes > interp->_framecount) nframes = interp->_framecount;
    while(nframes-- > 0 && ep < elast) {
	eframe = (Tps_Frame*)ep;
	TPS_FRAME_TRACE(interp,eframe,TPS_TEMPBUF);
	TPS_TEMPBUF->write(".\n");
	ep += TPS_FRAME_LENGTH(interp,eframe);
    }
    TPS_TEMPBUF->ends();
    s = STRDUP(TPS_TEMPBUF->contents());
    if(!s) s = "";
    return s;
}

const char*
debugexec(Tps_Interp* interp)
{
    return debugexec0(interp,TPS_EFRAMECOUNT(interp));
}
