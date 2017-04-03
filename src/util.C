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
#include "util.H"
#include "chartab.H"
#include "debug.H"

/**************************************************/

/* utility routine for bind */
Tps_Status
Tps_bind1(Tps_Value* ds, long dlen, Tps_Value* vp)
{
    Tps_Dictpair* pairp;

    if(!TPS_ISEXECUTABLE(*vp)) return TPSSTAT_OK;
    switch (TPS_TYPE(*vp)) {
	case TPSTYPE_NAME: {
	    if(Tps_dictstack_lookup(ds,dlen,*vp,NULL,&pairp) != TPSSTAT_OK)
		break;
	    if(TPS_ISTYPE(pairp->_value,TPSTYPE_OPERATOR)) {
		*vp = pairp->_value; /* replace */
	    }
	} break;
	case TPSTYPE_ARRAY: {
	    register Tps_Array* a;
	    register long i;
	    register Tps_Value* ap;

	    a = TPS_ARRAY_OF(*vp);
	    for(i=0,ap=a->contents();i<a->length();i++,ap++) {
		if(Tps_bind1(ds,dlen,ap) != TPSSTAT_OK)
		    return(TPSSTAT_SYSTEMERROR);
	    }
	} break;
	default: /* do nothing */
	    break;
    }
    return TPSSTAT_OK;
}

/**************************************************/
/* utility routine for cvts; also used by the debug routines */

Tps_Status
Tps_cvts1(Tps_Stream& outbuf, Tps_Value object, boolean deep, long addrctr)
{
    register Tps_Status ok;
    register Tps_Typeid rtyp;
    register Tps_String* s1;
    register char* s;
    register long len;
    register long i;
    register long c;

    rtyp = TPS_TYPE(object);
    switch (rtyp) {
	case TPSTYPE_NULL:
	    ok = outbuf.write("--nulltype--");
	    break;
	case TPSTYPE_MARK:
	    ok = outbuf.write("--marktype--");
	    break;
	case TPSTYPE_BOOLEAN:
	    s = TPS_BOOLEAN_OF(object)?"true":"false";
	    ok = outbuf.write(s);
	    break;
 	case TPSTYPE_INTEGER:
	    ok = outbuf.printf("%d",TPS_INTEGER_OF(object));
	    break;
#if HASFLOAT
 	case TPSTYPE_REAL:
	    ok = outbuf.printf("%f",TPS_REAL_OF(object));
	    break;
#endif
 	case TPSTYPE_NAME:
	    if(!TPS_ISEXECUTABLE(object)) {
		ok = outbuf.write('/');
		if(ok != TPSSTAT_OK) return ok;
	    }
	    s = TPS_NAME_OF(object);
	    len = strlen(s);
	    if(len == 1) {
		ok = outbuf.write(s);
	    } else {
		/* may need to dump char by char */
		for(i=0;i < len;i++) {
		    c = s[i];
		    switch (c) {
			case '\0': return TPSSTAT_SYSTEMERROR;
			case '\n': ok = outbuf.write("\\n"); break;
			case '\r': ok = outbuf.write("\\r"); break;
			case '\t': ok = outbuf.write("\\t"); break;
			case '\b': ok = outbuf.write("\\b"); break;
			case '\f': ok = outbuf.write("\\f"); break;
			case '\\': ok = outbuf.write("\\\\"); break;
			case '"': ok = outbuf.write("\\\""); break;
			case '\'': ok = outbuf.write("\\'"); break;
			default:
			    if(ISPRINTABLE(c)) {
				if(ISSPECIAL(c)) ok = outbuf.write('\\');
				ok = outbuf.write(c);
			    } else
				ok = outbuf.printf("\\%03o",c);
			break;
		    }
		    if(ok != TPSSTAT_OK) return ok;
		}
	    }
	    break;
 	case TPSTYPE_STRING:
	    s1 = TPS_STRING_OF(object);
	    s = s1->contents();
	    len = s1->length();
	    /* need to dump char by char */
#if PSMIMIC
	    ok = outbuf.write(LPAREN);
#else
	    ok = outbuf.write(DQUOTE);
#endif
	    if(ok != TPSSTAT_OK) return ok;
	    for(i=0;i < len;i++) {
		c = s[i];
		switch (c) {
		case '\0': ok = outbuf.write("\\0"); break;
		case '\n': ok = outbuf.write("\\n"); break;
		case '\r': ok = outbuf.write("\\r"); break;
		case '\t': ok = outbuf.write("\\t"); break;
		case '\b': ok = outbuf.write("\\b"); break;
		case '\f': ok = outbuf.write("\\f"); break;
		case ESCAPER: ok = outbuf.write("\\\\"); break;
#if PSMIMIC
		case LPAREN: ok = outbuf.write("\\("); break;
		case RPAREN: ok = outbuf.write("\\)"); break;
#else
		case DQUOTE: ok = outbuf.write("\\\""); break;
#endif
		default:
		    if(ISPRINTABLE(c))
			ok = outbuf.write(c);
		    else
			ok = outbuf.printf("\\%03o",c);
		    break;
		}
		if(ok != TPSSTAT_OK) return ok;
	    }
#if PSMIMIC
	    ok = outbuf.write(RPAREN);
#else
	    ok = outbuf.write(DQUOTE);
#endif
	    break;
	case TPSTYPE_ARRAY:
	    if(deep){
		register isexec = TPS_ISEXECUTABLE(object);
		register long i = isexec?LBRACE:LBRACKET;
		register Tps_Array* a;
		ok = outbuf.write(i);
		if(ok != TPSSTAT_OK) return ok;
		a = TPS_ARRAY_OF(object);
		for(i=0;i<a->length();i++) {
		    if(i>0) {
			ok = outbuf.write(' ');
			if(ok != TPSSTAT_OK) return ok;
		    }
		    if(isexec && addrctr == i) {
			ok = outbuf.write('*');
			if(ok != TPSSTAT_OK) return ok;
		    }
		    ok = Tps_cvts1(outbuf,a->contents()[i],deep,-1);
		    if(ok != TPSSTAT_OK) return ok;
		}
		if(isexec && addrctr == i) {
		    ok = outbuf.write('*');
		    if(ok != TPSSTAT_OK) return ok;
		}
		i = TPS_ISEXECUTABLE(object)?RBRACE:RBRACKET;
		ok = outbuf.write(i);
		if(isexec && addrctr > i) {
		    ok = outbuf.write('*');
		    if(ok != TPSSTAT_OK) return ok;
		}
	    } else {
		    ok = outbuf.write("--arraytype--");
	    }
	    break;
	case TPSTYPE_DICT:
	    if(deep) {
		ok = Tps_cvts1_dict_deep(outbuf,TPS_DICT_OF(object),FALSE);
	    } else {
		    ok = outbuf.write("--dicttype--");
	    }
	    break;
	case TPSTYPE_STREAM:
		if(deep) {
		    register Tps_Stream* strm = TPS_STREAM_OF(object);
		    ok = outbuf.write("--streamtype(");
		    if(ok != TPSSTAT_OK) return ok;
		    ok = outbuf.write(strm->name());
		    if(ok != TPSSTAT_OK) return ok;
		    ok = outbuf.write(")--");
		} else {
		    ok = outbuf.write("--streamtype--");
		}
		break;
 	case TPSTYPE_OPERATOR: {
		register Tps_Operator* op;
		op = TPS_OPERATOR_OF(object);
		ok = outbuf.write((char*)Tps_operator_prefix);
		if(ok != TPSSTAT_OK) return ok;
		/* assume that the operator name is Nameid */
		TPS_MAKEVALUE(object,TPSTYPE_NAME,op->name());
		TPS_SET_EXECUTABLE(object,1);
		ok = Tps_cvts1(outbuf,object,deep,addrctr);
		if(ok != TPSSTAT_OK) return ok;
		if(deep) {
		    outbuf.printf("/%d",op->arity());
		    if(ok != TPSSTAT_OK) return ok;
		}
		if(ok != TPSSTAT_OK) return ok;
	    } break;
	default:
	    ok = TPSSTAT_OK;
	    break; /* ignore other types */
    }    
    return ok;

}

/**************************************************/
static
Tps_Status
Tps_cvts1_tcldict_deep(Tps_Stream& accum, Tps_Dict_Tcl* dict, boolean deep)
{
    register Tps_Status ok;
    register long i;
    register long dlen,dtsz;
    Tps_Bucket* chains;
  
    dlen = dict->length();
    dtsz = dict->tablelength();
    chains = dict->table();
    accum.printf("(%d",dlen);
    ok = accum.printf("/%d)<<",dtsz);
    if(ok != TPSSTAT_OK) return ok;
    for(i=0;i<dtsz;i++,chains++) {
        register Tps_HashEntry* p = chains->chain;
	ok = accum.write("[");
	if(ok != TPSSTAT_OK) return ok;
	if(p) {
	    for(;p;p=p->nextPtr) {
		ok = accum.write("{");
		if(ok != TPSSTAT_OK) return ok;
		ok = Tps_cvts1(accum,p->pair._key,deep,-1);
		if(ok != TPSSTAT_OK) return ok;
		ok = accum.write(",");
		if(ok != TPSSTAT_OK) return ok;
		ok = Tps_cvts1(accum,p->pair._value,deep,-1);
		if(ok != TPSSTAT_OK) return ok;
		ok = accum.write("}");
		if(ok != TPSSTAT_OK) return ok;
	    }
	    ok = accum.write(";");
	    if(ok != TPSSTAT_OK) return ok;
	}
	ok = accum.write("]");
	if(ok != TPSSTAT_OK) return ok;
    }
    ok = accum.write(RRANGLESTR);
    return ok;
}

/* special deep printer for dictionaries */
Tps_Status
Tps_cvts1_dict_deep(Tps_Stream& accum, Tps_Dict* dict, boolean deep)
{
    if(dict->kind() == Tps_tcldict) {
	return Tps_cvts1_tcldict_deep(accum,(Tps_Dict_Tcl*)dict,deep);
    }
    return TPSSTAT_FAIL;
}

/**************************************************/
/* utility routine for the comparison functions.
   returns -1,0,1 as opl (<,=,>) opr
*/
int
Tps_compare(Tps_Value opl, Tps_Value opr)
{
    register Tps_Typeid tr;
    register Tps_Typeid tl;
    register char* sl;
    register char* sr;
    register long lenl;
    register long lenr;
    register long ir;
    register long minlen;
#if HASFLOAT
    Tps_Real fr;
#endif

    tr = TPS_TYPE(opr);
    tl = TPS_TYPE(opl);
    switch (tl) {
	case TPSTYPE_INTEGER:
	    switch (tr) {
		case TPSTYPE_INTEGER:
		    ir = (TPS_INTEGER_OF(opl) - TPS_INTEGER_OF(opr));
		    goto intresult;
#if HASFLOAT
		case TPSTYPE_REAL:
		    fr = (TPS_INTEGER_OF(opl) - TPS_REAL_OF(opr));
		    goto fltresult;
#endif
		default:
		    return 1; /* should never happen */
	    }
	    break;
#if HASFLOAT
	case TPSTYPE_REAL:
	    switch (tr) {
		case TPSTYPE_INTEGER:
		    fr = (TPS_REAL_OF(opl) - TPS_INTEGER_OF(opr));
		    goto fltresult;
		case TPSTYPE_REAL:
		    fr = (TPS_REAL_OF(opl) - TPS_REAL_OF(opr));
		    goto fltresult;
		default:
		    return 1; /* should never happen */
	    }
	    break;
#endif
	case TPSTYPE_NAME:
		sl = TPS_NAME_OF(opl);
		lenl = strlen(sl);
		switch (tr) {
		    case TPSTYPE_NAME:
			sr = TPS_NAME_OF(opr);
			lenr = strlen(sr);
			break;
		    case TPSTYPE_STRING:
			sr = TPS_STRING_OF(opr)->contents();
			lenr = TPS_STRING_OF(opr)->length();
			break;
		    default:
			return 1; /* should not happen */
		}
		goto cmpstr;
	case TPSTYPE_STRING:
		sl = TPS_STRING_OF(opl)->contents();
		lenl = TPS_STRING_OF(opl)->length();
		switch (tr) {
		    case TPSTYPE_NAME:
			sr = TPS_NAME_OF(opr);
			lenr = strlen(sr);
			break;
		    case TPSTYPE_STRING:
			sr = TPS_STRING_OF(opr)->contents();
			lenr = TPS_STRING_OF(opr)->length();
			break;
		    default:
			return 1; /* should not happen */
		}
		goto cmpstr;
	default:
	    if(tl != tr) ir = 1;
	    else
		ir = (TPS_ANY_OF(opl) == TPS_ANY_OF(opr))?0:1;
	    goto intresult;
    }
cmpstr:
    minlen = (lenr < lenl)?lenr:lenl;
    ir = STRNCMP(sl,sr,minlen);
    if(ir == 0) ir = (lenl - lenr);
    goto intresult;
#if HASFLOAT
fltresult:
    if(fr == 0.0) ir = 0;
    else if(fr > 0.0) ir = 1;
    else ir = -1;
#endif
intresult:
    return (ir);    
}

/**************************************************/

Tps_Status
Tps_string_or_name(Tps_Value v, char** sp, long* lenp)
{
    register char* s;
    register long len;
    switch(TPS_TYPE(v)) {
	case TPSTYPE_STRING:
	    {
		register Tps_String* str = TPS_STRING_OF(v);
		s = str->contents();
		len = str->length();
	    }
	    break;
	case TPSTYPE_NAME:
	    s = TPS_NAME_OF(v);
	    len = strlen(s);
	    break;
	default:
	    return TPSSTAT_TYPECHECK;
     }
    if(sp) *sp = s;
    if(lenp) *lenp = len;
    return TPSSTAT_OK;
}

/**************************************************/

/* This function is inlined elsewhere; look for ifdef NOINLINE */

Tps_Status
Tps_dictstack_lookup(Tps_Value* dstack, long dlen, Tps_Value key,
		   long* where, Tps_Dictpair** pairp)
{
    register Tps_Value* dp = dstack;
    register long depth = dlen;

    /* remember dstack grows down */
    while(depth > 0) {
	register Tps_Dict* d = TPS_DICT_OF(*dp);
	if((d)->lookup(key,pairp) == TPSSTAT_OK) {
	    if(where) *where = (dlen - depth);
	    return TPSSTAT_OK; /* found it */
	}
	dp++; depth--;
    }
    return TPSSTAT_UNDEFINED; /* not found */
}

Tps_Status
Tps_dictstack_define(Tps_Value* dstack, long dlen, Tps_Dictpair& pair, long* where)
{
    register Tps_Value* dp;
    register long depth;
    register Tps_Dict* d;
    register Tps_Status ok = TPSSTAT_DICTSTACKUNDERFLOW;

    /* remember dstack grows down */
    for(depth=dlen,dp=dstack;depth > 0;dp++) {
	if(TPS_ISWRITEABLE(*dp)) {
	    d = TPS_DICT_OF(*dp);
	    ok = d->insert(pair,(Tps_Value*)0);
	    if(where) *where = depth;
	    break;
	}
    }
    return ok;
}

/**************************************************/
#if 0
Tps_Dict*
Tps_builderrordict(void)
{
    register int i;
    register Tps_Dict* d;
    register Tps_Dict* derr;
    Tps_Dictpair pair;
    register Tps_Status ok;

    d = new Tps_Dict_Tcl(1,"errordict");
    /* define the default entry in the error dict */
    pair._key = TPS__NM(TPS_NMDDEFAULT);
    pair._value = TPS__NM(TPS_NMERRHANDLER);
    TPS_SET_EXECUTABLE(pair._value,1);
    ok = d->insert(pair,(Tps_Value*)NULL);
    if(ok != TPSSTAT_OK) {
	TPS_STDCONS.printf("Tps_builderrordict: error creating procedure $default\n");
	return (Tps_Dict*)0;
    }
    /* define the $error dict entry in the error dict */
    pair._key = TPS__NM(TPS_NMDERROR);
    derr = new Tps_Dict_Tcl(1,"$error");
    if(!derr) {
	TPS_STDCONS.printf("Tps_initialize: error creating $error dict\n");
	return (Tps_Dict*)0;
    }
    TPS_MAKEVALUE(pair._value,TPSTYPE_DICT,derr);
    ok = d->insert(pair,(Tps_Value*)NULL);
    if(ok != TPSSTAT_OK) {
	TPS_STDCONS.printf("Tps_initialize: error inserting $error dict\n");
	return (Tps_Dict*)0;
    }
    return d;
}
#endif /*0*/
