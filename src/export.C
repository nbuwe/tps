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

#define IMTEST 1

#include "tps.H"
#include "exec.H"
#include "export.H"
#include "chartab.H"
#include "util.H"

/**************************************************/
/*
Given a value, dump it in some encoded form.
Assumes that containers need uid conversion,
but that they have a uid assigned; recurses with vencode.
*/
Tps_Status
Tps_encode(Tps_Interp* intrp, Tps_Value v, Tps_Stream* f)
{
    Tps_Status ok = TPSSTAT_OK;
    Tps_Typeid tid;
    Tps_Value_Union vu;
    Tps_Container* c;

    tid = TPS_TYPE(v);
    switch (tid) {
	case TPSTYPE_STRING:
	case TPSTYPE_STREAM:
	case TPSTYPE_ARRAY:
	case TPSTYPE_DICT:
	    c = TPS_CONTAINER_OF(v);
	    if(c->uid() < 0) {
		c->setuid(intrp->_uidcounter);
		intrp->_uidcounter++;
		ok = Tps_encode_container(intrp,v,f);
		if(ok != TPSSTAT_OK)
		    goto fail;
	    }
	    TPS_CHANGEVALUE(v,TPS_TYPE(v),c->uid());
	    break;
	default:
	    break;
    }
    vu.u._value = v;
    f->printf("V0x%x 0x%x ",
		vu.u._simple._flags._flags,
		vu.u._simple._flags._typeid);
    if(tid == TPSTYPE_NAME) {
	TPS_SET_EXECUTABLE(v,1);
	ok = Tps_cvts1(*f,v,0,-1);
	if(ok != TPSSTAT_OK) goto fail;
    } else if(tid == TPSTYPE_OPERATOR) {
	ok = Tps_cvts1(*f,v,0,-1);
	if(ok != TPSSTAT_OK) goto fail;
    } else {
	f->printf("0x%x",vu.u._simple._value);
    }
    f->write(EOL);
    return ok;
fail:
    return ok;
}

/*
Given a container value, dump its contents; recurses with encode.
Assumes that containers have a uid assigned by encode.
*/

Tps_Status
Tps_encode_container(Tps_Interp* intrp, Tps_Value v, Tps_Stream* f)
{
    Tps_Status ok = TPSSTAT_OK;
    long i;
    Tps_Container* c;
    long l;
    Tps_String* s;
    Tps_Array* a;
    Tps_Dict* d;
    Tps_Stream* strm;
    Tps_Value* ac;
    Tps_Dictpair* pairp;
    Tps_Typeid tid;

    tid = TPS_TYPE(v);
    c = TPS_CONTAINER_OF(v);
    f->printf("O%d\n",c->uid());
    switch (tid) {
	case TPSTYPE_ARRAY:
	    a = TPS_ARRAY_OF(v);
	    l = a->length();
	    ac = a->contents();
	    f->printf("A%d\n",l);
	    for(i=0;i<l;i++,ac++) {
		ok = Tps_encode(intrp,*ac,f);
		if(ok != TPSSTAT_OK) goto fail;
	    }
	    f->write("A\n");
	    break;
	case TPSTYPE_DICT:
	    d = TPS_DICT_OF(v);
	    l = d->range();
	    f->printf("D%d\n",d->length());
	    for(i=0;i<l;i++) {
		ok = d->ith(i,pairp);
		if(ok == TPSSTAT_UNDEFINED) continue;
		if(ok != TPSSTAT_OK) goto fail;
		ok = Tps_encode(intrp,pairp->_key,f);
		if(ok != TPSSTAT_OK) goto fail;
		ok = Tps_encode(intrp,pairp->_value,f);
		if(ok != TPSSTAT_OK) goto fail;
	    }
	    f->write("D\n");
	    ok = TPSSTAT_OK; /* overwrite undefined */
	    break;
	case TPSTYPE_STREAM:
	    /* separate strings out from files */
	    strm = TPS_STREAM_OF(v);
	    if(strm->mode() != Tps_stream_r) {
		ok = TPSSTAT_INVALIDSTREAMACCESS;
		goto fail;
	    }
	    f->write("F\n");
	    l = strm->bytesavailable();
	    if(l > 0) {
		s = new Tps_String;
		if(!s) {ok = TPSSTAT_VMERROR; goto fail;}
		while(l-- > 0) {
		    long ch = strm->read();
		    if(ch == EOF) break; /* bytesavail may lie */
		    s->append((char)ch);
		}
	    } else {
		s = new Tps_String;
	    }
	    TPS_MAKEVALUE(v,TPSTYPE_STRING,s);
	    ok = Tps_cvts1(*f,v,0,-1);
	    delete s;
	    if(ok != TPSSTAT_OK) goto fail;
	    f->write("\nD\n");
	    break;
	case TPSTYPE_STRING:
	    ok = Tps_cvts1(*f,v,0,-1);
	    if(ok != TPSSTAT_OK) goto fail;
	    f->write(EOL);
	    break;	    	    
	default:
	    abort();
    }
    f->write("O\n");
    return ok;
fail:
    return ok;
}

/**************************************************/
Tps_Status
Tps_export(Tps_Interp* intrp, Tps_Stream* f)
{
    Tps_Status ok;
    long i;
    long l;
    Tps_Value* vp;
    Tps_List* lp;
    Tps_Value v;

    /*
	output some header stuff.
    */
    f->printf(":%s\n",Tps_versionstring);
    f->printf(":%s\n",TPS_ENCODEVERSION);
    /* dump other flags */
    f->printf(":");
#if PSMIMIC
    f->write("p");
#endif
#if OO
    f->write("o");
#endif
    f->write("\n");

    /* Dump the current state of the safe flag */
    TPS_MAKEVALUE(v,TPSTYPE_BOOLEAN,intrp->safe());
    ok = Tps_encode(intrp,v,f);
    if(ok != TPSSTAT_OK)
	goto fail;

    /* reset uid's of all containers */
    for(lp=tpsg._objects->next();lp;lp=lp->next()) {
	Tps_Container* q = (Tps_Container*)lp;
	q->setuid(-1);
    }

    intrp->_uidcounter = 0;
    /* assign uid for the system dict
	because it will not be dumped.
    */

    tpsg._systemdict->setuid(intrp->_uidcounter);
    intrp->_uidcounter++;

    /* Now, materialize the following roots:
	optional depending on current mode
	    1. unsafe mode userdict
	    2. unsafe part of the exec stack
	    3. unsafe mode dict stack
	required:
	    1. safe mode userdict 
	    2. operand stack
	    3. safe part of the exec stack
	    4. safe mode dict stack
    */

    /* do optional dump of unsafe userdict */
    if(!intrp->safe()) {
	/* unsafe userdict */
	ok = Tps_encode(intrp,intrp->__userdicts[0],f);
	if(ok != TPSSTAT_OK)
	    goto fail;
    }
    /* user's userdict */
    ok = Tps_encode(intrp,intrp->__userdicts[1],f);
    if(ok != TPSSTAT_OK)
	goto fail;

    /* operand stack: dump bottom up */
    l = TPS_DEPTH(intrp);
    vp = TPS_TOSP(intrp);   
    f->printf("X%d\n",l); // X => ``section''
    for(vp+=l,i=l;i > 0; i--) {
	vp--;
	ok = Tps_encode(intrp,*vp,f);
	if(ok != TPSSTAT_OK)
	    goto fail;
    }
    f->write("X\n");

    /* exec stack: also reverse; mode will be taken into account */
    ok = Tps_export_exec(intrp,l,intrp->safe()?Tps_pass_none:Tps_pass_safety);
    if(ok != TPSSTAT_OK && ok != TPSSTAT_STOP)
	goto fail;
    vp = TPS_TOSP(intrp);   
    f->printf("X%d\n",l);
    for(vp+=l,i=l;i > 0; i--) {
	vp--;
	ok = Tps_encode(intrp,*vp,f);
	if(ok != TPSSTAT_OK) goto fail;
    }
    f->write("X\n");
    TPS_POPN(intrp,l);

    if(!intrp->safe()) {
	/* unsafe dict stack */
	l = TPS_DDEPTHSF(intrp,0);
	vp = TPS_DTOSPSF(intrp,0);
	f->printf("X%d\n",l);
	for(vp+=l,i=l;i > 0; i--) {
	    vp--;
	    ok = Tps_encode(intrp,*vp,f);
	    if(ok != TPSSTAT_OK) goto fail;
	}
	f->write("X\n");
    }
    /* safe dict stack */
    l = TPS_DDEPTHSF(intrp,1);
    vp = TPS_DTOSPSF(intrp,1);
    f->printf("X%d\n",l);
    for(vp+=l,i=l;i > 0; i--) {
	vp--;
	ok = Tps_encode(intrp,*vp,f);
	if(ok != TPSSTAT_OK) goto fail;
    }
    f->write("X\n");

    f->ends(); /* terminate stream is a string */

#if IMTEST
    f->rewind();
    ok = Tps_import(intrp,f,FALSE);
    if(ok != TPSSTAT_OK)
	goto fail;
#endif

    return ok;
fail:
    return ok;
}

/**************************************************/
static
void
skipws(Tps_Stream* f)
{
    long c;
    /* skip whitespace */
    for(;;) {
	c = f->read();
	if(c == EOF) return;
	if(!(Tps_chartable[c] & TPSC_WS)) break;
    }
    f->pushback(c);
}

static
Tps_Status
check(Tps_Stream* f, long expected)
{
    long c;
    skipws(f);
    c = f->read();
    return (c == expected)?TPSSTAT_OK:TPSSTAT_SYNTAXERROR;
}

/**************************************************/
Tps_Status
Tps_decode_value(Tps_Interp* /*intrp*/, Tps_Value& vout, Tps_Stream* f,
	   Tps_Array& da, Tps_Stream_String& buf)
{
    Tps_Status ok = TPSSTAT_OK;
    long i;
    long tid,tid2;
    Tps_Value v;
    Tps_Value_Union vu;
    Tps_Dictpair* pairp;

    ok = Tps_get_atom(buf,f,&v,1);
    if(ok != TPSSTAT_OK || !TPS_ISTYPE(v,TPSTYPE_INTEGER))
	goto fail;
    vu.u._simple._flags._flags = (Halfword)TPS_INTEGER_OF(v);
    ok = Tps_get_atom(buf,f,&v,1);
    if(ok != TPSSTAT_OK || !TPS_ISTYPE(v,TPSTYPE_INTEGER))
	goto fail;
    tid2 = TPS_INTEGER_OF(v);
    if(tid2 < 0 || tid2 >= TPSTYPE_COUNT)
	goto fail;
    vu.u._simple._flags._typeid = (Halfword)tid2;
    ok = Tps_get_atom(buf,f,&v,1);
    if(ok != TPSSTAT_OK)
	goto fail;
    tid = TPS_TYPE(v);
    i = TPS_ANY_OF(v);
    switch (tid) {
	default:
	    goto fail;
	case TPSTYPE_NAME:
	    /* verify */
	    switch (tid2) {
		default:
		    goto fail;
		case TPSTYPE_OPERATOR:
		    /* lookup the name and see if it maps
		       to an operator object
		    */
		    ok = tpsg._systemdict->lookup(v,&pairp);
		    if(ok != TPSSTAT_OK)
			goto fail;
		    if(!TPS_ISTYPE(pairp->_value,TPSTYPE_OPERATOR))
			goto fail;
		    i = TPS_ANY_OF(pairp->_value);
		    break;
		case TPSTYPE_NAME:
		    break;
	    }
	    break;
	case TPSTYPE_INTEGER:
	    /* verify */
	    switch (tid2) {
		case TPSTYPE_NULL:
		case TPSTYPE_MARK:
		    if(i != 0)
			goto fail;
		    break;
		case TPSTYPE_BOOLEAN:
		    i = i?TRUE:FALSE;
		    break;
		case TPSTYPE_INTEGER:
		case TPSTYPE_REAL:
		    break;
		case TPSTYPE_NAME:
		case TPSTYPE_OPERATOR:
		    goto fail;
		case TPSTYPE_STRING:
		case TPSTYPE_ARRAY:
		case TPSTYPE_DICT:
		case TPSTYPE_STREAM:
		    /* presumably this object has already
		       been translated.
		    */
		    if(i < 0 || i > da.length())
			goto fail;
		    v = da.contents()[i];
		    if(tid2 != TPS_TYPE(v))
			goto fail;
		    i = TPS_ANY_OF(v);
		    break;
		default:
		    goto fail;
	    }
    }
    TPS_CHANGEVALUE(vu.u._value,TPSTYPE_ANY,i);
    vout = vu.u._value;
    return ok;
fail:
    return ok;
}

Tps_Status
Tps_decode_array(Tps_Interp* intrp, Tps_Value& vout, Tps_Stream* f,
	   Tps_Array& da, Tps_Stream_String& buf)
{
    Tps_Status ok = TPSSTAT_OK;
    long l;
    long i;
    Tps_Array* a;
    Tps_Value v;

    ok = Tps_get_atom(buf,f,&v,1);
    if(ok != TPSSTAT_OK)
	goto fail;
    if(!TPS_ISTYPE(v,TPSTYPE_INTEGER))
	goto fail;
    l = TPS_INTEGER_OF(v);
    if(l < 0)
	goto fail;
    a = new Tps_Array;
    /* watch out; need to instantiate v before decoding contents */
    TPS_MAKEVALUE(vout,TPSTYPE_ARRAY,a);
    for(i=0;i<l;i++) {
	ok = Tps_decode(intrp,v,f,da,buf);
	if(ok != TPSSTAT_OK)
	    goto fail;
	a->append(v);
    }
    return ok;
fail:
    return ok;
}

Tps_Status
Tps_decode_dict(Tps_Interp* intrp, Tps_Value& vout, Tps_Stream* f,
	   Tps_Array& da, Tps_Stream_String& buf)
{
    Tps_Status ok = TPSSTAT_OK;
    long l;
    long i;
    Tps_Dict* d;
    Tps_Dictpair pair;
    Tps_Value v;

    ok = Tps_get_atom(buf,f,&v,1);
    if(ok != TPSSTAT_OK)
	goto fail;
    if(!TPS_ISTYPE(v,TPSTYPE_INTEGER))
	goto fail;
    l = TPS_INTEGER_OF(v);
    if(l < 0)
	goto fail;
    d = new Tps_Dict_Tcl;
    /* watch out; need to instantiate v before decoding contents */
    TPS_MAKEVALUE(vout,TPSTYPE_DICT,d);
    for(i=0;i<l;i++) {
	ok = Tps_decode(intrp,pair._key,f,da,buf);
	if(ok != TPSSTAT_OK)
	    goto fail;
	ok = Tps_decode(intrp,pair._value,f,da,buf);
	if(ok != TPSSTAT_OK)
	    goto fail;
	d->insert(pair,0);
    }
    return ok;
fail:
    return ok;
}

Tps_Status
Tps_decode_stream(Tps_Interp* /*intrp*/, Tps_Value& vout, Tps_Stream* f,
	   Tps_Array& /*da*/, Tps_Stream_String& buf)
{
    Tps_Status ok = TPSSTAT_OK;
    Tps_Stream_String* strm;
    Tps_String* ss;
    Tps_Value v;

    ok = Tps_get_atom(buf,f,&v,1);
    if(ok != TPSSTAT_OK)
	goto fail;
    if(!TPS_ISTYPE(v,TPSTYPE_STRING))
	goto fail;
    strm = new Tps_Stream_String;
    TPS_MAKEVALUE(vout,TPSTYPE_STREAM,strm);
    ss = TPS_STRING_OF(v);
    ok = strm->open();
    if(ok != TPSSTAT_OK)
	goto fail;
    ok = strm->write(ss->contents(),ss->length());
    if(ok != TPSSTAT_OK)
	goto fail;
    ok = strm->rewind();
    if(ok != TPSSTAT_OK)
	goto fail;
    return ok;
fail:
    return ok;
}

Tps_Status
Tps_decode_string(Tps_Interp* /*intrp*/, Tps_Value& vout, Tps_Stream* f,
	   Tps_Array& /*da*/, Tps_Stream_String& buf)
{
    Tps_Status ok = TPSSTAT_OK;
    Tps_Value v;

    ok = Tps_get_atom(buf,f,&v,1);
    if(ok != TPSSTAT_OK)
	goto fail;
    if(!TPS_ISTYPE(v,TPSTYPE_STRING))
	goto fail;
    vout = v;
    return ok;
fail:
    return ok;
}

Tps_Status
Tps_decode_container(Tps_Interp* intrp, Tps_Value& vout, Tps_Stream* f,
	   Tps_Array& da, Tps_Stream_String& buf)
{
    Tps_Status ok = TPSSTAT_OK;
    long l;
    Tps_Value v;

    ok = Tps_get_atom(buf,f,&v,1);
    if(ok != TPSSTAT_OK)
	goto fail;
    if(!TPS_ISTYPE(v,TPSTYPE_INTEGER))
	goto fail;
    l = TPS_INTEGER_OF(v);
    if(l != da.length()) // => containers given sequential ids
	goto fail;
    /* this is a bit tricky; we ned to arrange for the
	da entry to get defined before its contents
	are decoded.
    */
    da.append(TPS__CONST(TPS__NULL));
    ok = Tps_decode(intrp,da.contents()[l],f,da,buf);
    vout = da.contents()[l];
    return ok;
fail:
    return ok;
}

Tps_Status
Tps_decode(Tps_Interp* intrp, Tps_Value& vout, Tps_Stream* f,
           Tps_Array& da, Tps_Stream_String& buf)
{
    Tps_Status ok = TPSSTAT_OK;
    long c;
    Tps_Value v;

    skipws(f);
    c = f->read();
    switch (c) {
	case EOF:
	    break;
	case 'V':
	    ok = Tps_decode_value(intrp,vout,f,da,buf);
	    break;
	case 'O':
	    ok = Tps_decode_container(intrp,v,f,da,buf);
	    if(ok == TPSSTAT_OK) ok = check(f,c);
	    /* ok, containers are always followed
		by the value referring to it, and
		which is the real value to be returned
	    */
	    ok = Tps_decode(intrp,vout,f,da,buf);
	    /* do a simple verifcation */
	    if(TPS_TYPE(v) != TPS_TYPE(vout)
		|| TPS_CONTAINER_OF(v) != TPS_CONTAINER_OF(vout))
		ok = TPSSTAT_FAIL;
	    break;
	case 'A':
	    ok = Tps_decode_array(intrp,vout,f,da,buf);
	    if(ok == TPSSTAT_OK) ok = check(f,c);
	    break;
	case 'D':
	    ok = Tps_decode_dict(intrp,vout,f,da,buf);
	    if(ok == TPSSTAT_OK) ok = check(f,c);
	    break;
	case 'F':
	    ok = Tps_decode_stream(intrp,vout,f,da,buf);
	    if(ok == TPSSTAT_OK) ok = check(f,c);
	    break;
#if PSMIMIC
	case LPAREN:
#else
	case DQUOTE:
#endif
	    f->pushback(c);
	    ok = Tps_decode_string(intrp,vout,f,da,buf);
	    break;
	default:
	    ok = TPSSTAT_SYNTAXERROR;
    }
    return ok;
}

Tps_Status
Tps_decode_section(Tps_Interp* intrp, Tps_Stream* f,
		   Tps_Array& da, Tps_Stream_String& buf)
{
    Tps_Status ok = TPSSTAT_OK;
    long l;
    long i;
    long c;
    Tps_Value v;

    skipws(f);
    c = f->read();
    if(c != 'X')
	return TPSSTAT_SYNTAXERROR;
    ok = Tps_get_atom(buf,f,&v,1);
    if(ok != TPSSTAT_OK)
	return ok;
    if(!TPS_ISTYPE(v,TPSTYPE_INTEGER))
	return TPSSTAT_TYPECHECK;
    l = TPS_INTEGER_OF(v);
    if(l < 0) return TPSSTAT_RANGECHECK;
    TPS_GUARANTEE(intrp,l);
    for(i=0;i<l;i++) {
	ok = Tps_decode(intrp,v,f,da,buf);
	if(ok != TPSSTAT_OK) break;
	TPS_PUSH(intrp,v);
    }
    if(ok == TPSSTAT_OK) ok = check(f,c);
    return ok;    
}

/**************************************************/
#if IMTEST
static int Tps_equal(Tps_Value v1, Tps_Value v2);
#endif

Tps_Status
Tps_import(Tps_Interp* intrp, Tps_Stream* f, boolean clr)
{
    Tps_Status ok = TPSSTAT_OK;
    long i;
    long c;
    Tps_Value* vp;
    Tps_Value* op;
    Tps_Array* da = new Tps_Array;
    Tps_Stream_String* buf;
    long l;
    boolean safeflag;
    Tps_Value v;
    Tps_Value userdicts[2];
    long marks[5];

    buf = new Tps_Stream_String;
    if(!buf) return TPSSTAT_VMERROR;
    buf->open();
    /* pick up versions, but ignore for now */
    /* tps version */
    skipws(f);
    c= f->read();
    if(c != ':')
	goto synerr;
    ok = Tps_get_atom(*buf,f,&v,1);
    if(ok != TPSSTAT_OK)
	goto synerr;
    /* tps encoding version */
    skipws(f);
    c= f->read();
    if(c != ':')
	goto fail;
    ok = Tps_get_atom(*buf,f,&v,1);
    if(ok != TPSSTAT_OK)
	goto fail;

    /* init the object array */
    da->append(tpsg.__systemdict);

    /* Now, materialize items as defined by Tps_export */

    /* safe flag */
    ok = Tps_decode(intrp,v,f,*da,*buf);
    if(ok != TPSSTAT_OK)
	goto fail;
    if(!TPS_ISTYPE(v,TPSTYPE_BOOLEAN))
	goto synerr;
    safeflag = TPS_BOOLEAN_OF(v);
    /* See if we can continue */
    if(intrp->safe() && !safeflag)
	goto fail;

    /* optional unsafe userdict  */
    if(!safeflag) {
	ok = Tps_decode(intrp,v,f,*da,*buf);
	if(ok != TPSSTAT_OK) goto fail;
	if(!TPS_ISTYPE(v,TPSTYPE_DICT)) goto synerr;
	userdicts[0] = v;
    }
    /* safe userdict */
    ok = Tps_decode(intrp,v,f,*da,*buf);
    if(ok != TPSSTAT_OK) goto fail;
    if(!TPS_ISTYPE(v,TPSTYPE_DICT)) goto synerr;
    userdicts[1] = v;

    /* operand stack */
    marks[0] = TPS_DEPTH(intrp);
    ok = Tps_decode_section(intrp,f,*da,*buf);
    if(ok != TPSSTAT_OK)
	goto fail;

    /* exec stack */
    marks[1] = TPS_DEPTH(intrp);
    ok = Tps_decode_section(intrp,f,*da,*buf);
    if(ok != TPSSTAT_OK)
	goto fail;

    /* optional unsafe dict stack */
    marks[2] = TPS_DEPTH(intrp);
    if(!safeflag) {
	ok = Tps_decode_section(intrp,f,*da,*buf);
	if(ok != TPSSTAT_OK)
	    goto fail;
    }
    marks[3] = TPS_DEPTH(intrp);
    ok = Tps_decode_section(intrp,f,*da,*buf);
    if(ok != TPSSTAT_OK)
	goto fail;
    marks[4] = TPS_DEPTH(intrp);

    /* ok, do the state replace operation */
    intrp->_object = TPS__CONST(TPS__NULL);
    intrp->_throwflag = TPS__CONST(TPS__NULL);
    intrp->_tracing = FALSE;
    intrp->_traceskip = FALSE;

    intrp->__userdicts[0] = userdicts[0];
    intrp->__userdicts[1] = userdicts[1];
    intrp->_userdicts[0] = TPS_DICT_OF(userdicts[0]);
    intrp->_userdicts[1] = TPS_DICT_OF(userdicts[1]);

#if IMTEST
    Tps_Value* vp1;
    Tps_Value* vp2;
    long t1;
    t1 = intrp->_dstacks[1]._last - intrp->_dstacks[1]._tos;
    if(t1 != marks[4] - marks[3])
	abort();
    vp1 = intrp->_dstacks[1]._tos;
    vp2 = TPS_TOSP(intrp);
    for(i=0;i<t1;i++,vp1++,vp2++) {
	if(!Tps_equal(*vp1,*vp2))
	    abort();
    }
    TPS_POPN(intrp,t1);
    if(!safeflag) {
	t1 = intrp->_dstacks[0]._last - intrp->_dstacks[0]._tos;
	if(t1 != marks[3] - marks[2])
	    abort();
	vp1 = intrp->_dstacks[0]._tos;
	vp2 = TPS_TOSP(intrp);
	for(i=0;i<t1;i++,vp1++,vp2++) {
	    if(!Tps_equal(*vp1,*vp2))
		abort();
	}
	TPS_POPN(intrp,t1);
    }
#else
    /* safe dict stack */
    if(clr) {
	TPS_DCLEARSF(intrp,1);
    }
    l = marks[4] - marks[3];
    vp = TPS_DPUSHNSF(intrp,1,l);
    memcpy((char*)vp,(char*)TPS_TOSP(intrp),l*sizeof(Tps_Value));
    TPS_POPN(intrp,l);

    if(!safeflag) {
	/* unsafe dict stack */
	if(clr) {
	    TPS_DCLEARSF(intrp,0);
	}
	l = marks[3] - marks[2];
	vp = TPS_DPUSHNSF(intrp,0,l);
	memcpy((char*)vp,(char*)TPS_TOSP(intrp),l*sizeof(Tps_Value));
	TPS_POPN(intrp,l);
    }

#endif

#if IMTEST
    long t2;
    /* to test the exec stack, we re-export the exec stack
	and compare
    */
    t2 = marks[3] - marks[2];
    vp1 = TPS_TOSP(intrp);
    ok = Tps_export_exec(intrp,t2,intrp->safe()?Tps_pass_none:Tps_pass_safety);
    vp2 = TPS_TOSP(intrp);
    t1 = vp1 - vp2;
    if(t1 != t2)
	abort();
    for(i=0;i<t1;i++,vp1++,vp2++) {
	if(!Tps_equal(*vp1,*vp2))
	    abort();
    }
    TPS_POPN(intrp,t2);
    TPS_POPN(intrp,t1);
#else
    /* exec stack */
    if(clr) {
	TPS_ECLEAR(intrp);
	TPS_EFRAMECOUNT(intrp) = 0;
    }
    l = marks[3] - marks[2];
    ok = Tps_import_exec(intrp,l,intrp->safe()?Tps_pass_none:Tps_pass_safety); /* will pop operand stack */
    if(ok != TPSSTAT_OK)
	goto fail;
#endif

#ifndef IMTEST
    /* operand stack */
    if(clr) {
	l = marks[1] - marks[0];
	vp = TPS_BASE(intrp);
	op = vp + marks[0];
	TPS_CLEAR(intrp);
	while(l-- > 0) {
	    *(--vp) = *(--op);
	}
	TPS_SETTOSP(intrp,vp);
    }
#endif

    delete da;
    delete buf;
    return ok;
synerr:
    ok = TPSSTAT_SYNTAXERROR;
fail:
    delete da;
    delete buf;
    return ok;
}

#if IMTEST
static
int
Tps_equal1(Tps_Value v1, Tps_Value v2)
{
    long i;
    if(TPS_TYPE(v1) != TPS_TYPE(v2)) return FALSE;
    if(TPS_ANY_OF(v1) == TPS_ANY_OF(v2)) return TRUE;
    switch (TPS_TYPE(v1)) {
	case TPSTYPE_NULL:
	case TPSTYPE_MARK: return TRUE;
	case TPSTYPE_BOOLEAN: {
	    boolean b1 = TPS_BOOLEAN_OF(v1)?1:0;
	    boolean b2 = TPS_BOOLEAN_OF(v2)?1:0;
	    return b1 == b2;
	}
	case TPSTYPE_INTEGER:
	    return TPS_INTEGER_OF(v1) == TPS_INTEGER_OF(v2);	
	case TPSTYPE_REAL:
	    return TPS_REAL_OF(v1) == TPS_REAL_OF(v2);	
	case TPSTYPE_NAME:
	    return TPS_NAME_OF(v1) == TPS_NAME_OF(v2);	
	case TPSTYPE_OPERATOR:
	    return TPS_OPERATOR_OF(v1) == TPS_OPERATOR_OF(v2);	
	case TPSTYPE_STREAM:
	    return TRUE;
	case TPSTYPE_STRING: {
	    Tps_String* s1 = TPS_STRING_OF(v1);
	    Tps_String* s2 = TPS_STRING_OF(v2);
	    if(s1->length() != s2->length()) return FALSE;
	    return (memcmp(s1->contents(),s2->contents(),s1->length())==0)?1:0;
	}
	case TPSTYPE_ARRAY: {
	    Tps_Array* a1 = TPS_ARRAY_OF(v1);
	    Tps_Array* a2 = TPS_ARRAY_OF(v2);
	    if(a1->length() != a2->length()) return FALSE;
	    if(a1->marked() && a2->marked()) return TRUE;
	    if(a1->marked() != a2->marked()) return FALSE;
	    a1->mark(); a2->mark();
	    for(i=0;i<a1->length();i++) {
		if(!Tps_equal1(a1->contents()[i],a2->contents()[2]))
		    return FALSE;
	    }
	    return TRUE;
	}
	case TPSTYPE_DICT: {
	    Tps_Dict* d1 = TPS_DICT_OF(v1);
	    Tps_Dict* d2 = TPS_DICT_OF(v2);
	    if(d1->length() != d2->length()) return FALSE;
	    if(d1->marked() && d2->marked()) return TRUE;
	    if(d1->marked() != d2->marked()) return FALSE;
	    d1->mark(); d2->mark();
	    for(i=0;i<d1->range();i++) {
		Tps_Dictpair* dp1;
		Tps_Dictpair* dp2;
		Tps_Status ok;
		ok = d1->ith(i,dp1);
		if(ok != TPSSTAT_OK) continue;
		long j;
		boolean b=0;
		for(j=0;j<d2->range();j++) {
		    ok = d2->ith(j,dp2);
		    if(ok != TPSSTAT_OK) continue;
		    if(Tps_equal1(dp1->_key,dp2->_key)
		       && Tps_equal1(dp1->_value,dp2->_value)) {
			b = 1;
			break;
		    }
		}
		if(!b) return FALSE;
	    }
	    return TRUE;
	}
	default: return FALSE;
    }
}

static
int
Tps_equal(Tps_Value v1, Tps_Value v2)
{
    /* go thru and mark all containers as untouched */
    Tps_List* lp;
    for(lp=tpsg._objects->next();lp;lp=lp->next()) {
	Tps_Container* q = (Tps_Container*)lp;
	q->unmark();
    }

    int b = Tps_equal1(v1,v2);

    for(lp=tpsg._objects->next();lp;lp=lp->next()) {
	Tps_Container* q = (Tps_Container*)lp;
	q->unmark();
    }

    return b;
}
#endif /*IMTEST*/

/**************************************************/

Tps_Status
Tps_export_exec(Tps_Interp* intrp, long& ecount, Tps_Exec_Pass passflag)
{
    char* enext;
    Tps_Status ok;
    Tps_Frame* eframe;
    Tps_Nameid nm;
    long count;
    long xlen;
    long flen;
    Tps_Value v;

    ecount = TPS_DEPTH(intrp);
    enext=TPS_ETOSP(intrp);
    xlen = 0;
    for(count = TPS_EFRAMECOUNT(intrp); count > 0; count--) {
	eframe = (Tps_Frame*)enext;
	/* push contents of the frame */
	flen = 0;
	ok = TPS_FRAME_EXPORT(intrp,eframe,flen,passflag);
	if(ok != TPSSTAT_OK) break;
	/* push name and length */
	if(TPS_ROOM(intrp) < 2) {ok = TPSSTAT_STACKOVERFLOW; break;}
	nm = tpsg._nametable->newname(TPS_FRAME_NAME(intrp,eframe));
	TPS_MAKEVALUE(v,TPSTYPE_NAME,nm);
	TPS_PUSH(intrp,v);
	TPS_MAKEVALUE(v,TPSTYPE_INTEGER,flen+2);
	TPS_PUSH(intrp,v);
	xlen += flen + 2;
	enext += TPS_FRAME_LENGTH(intrp,eframe);
    }
    if(ok == TPSSTAT_STOP) ok = TPSSTAT_OK;
    /* record amount dumped */
    if(xlen != TPS_DEPTH(intrp) - ecount) {
	ok = TPSSTAT_SYSTEMERROR;
    } else
	ecount = xlen;
    return ok;
}

/**************************************************/

Tps_Status
Tps_import_exec(Tps_Interp* intrp, long count, Tps_Exec_Pass passflag)
{
    Tps_Status ok;
    Tps_Nameid nm;
    long popped;
    Tps_Handler* h;
    long xlen;
    Tps_Value v;

    /* Start with a statemark */
    ok = Tps_create_statemark(intrp);
    for(popped=0;popped < count;popped += xlen) {
	/* verify */
	if(TPS_DEPTH(intrp) < 2) {
	    ok = TPSSTAT_STACKUNDERFLOW;
	    goto fail;
	}
	v = TPS_POP(intrp);
	if(!TPS_ISTYPE(v,TPSTYPE_INTEGER)) return TPSSTAT_TYPECHECK;
	xlen = TPS_INTEGER_OF(v);
	if(xlen < 2) return TPSSTAT_RANGECHECK;
	if(TPS_DEPTH(intrp) < xlen) return TPSSTAT_STACKUNDERFLOW;
	v = TPS_POP(intrp);
	if(!TPS_ISTYPE(v,TPSTYPE_NAME)) return TPSSTAT_TYPECHECK;
	nm = TPS_NAME_OF(v);
	if(!(h = Tps_lookup_handler(nm))) {
	    ok = TPSSTAT_FAIL;
	    goto fail;
	}
	xlen -= 2;
	{long tmp = xlen;
	ok = h->_import(intrp,h,tmp,(Tps_Frame*)0,passflag);
	if(tmp != 0)
	    return TPSSTAT_RANGECHECK;
	}
    }
    return ok;
fail:
    return ok;
}
