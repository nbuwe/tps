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

/**************************************************/
#include "tps.H"
#include "mem.H"
#include "chartab.H"
/**************************************************/

/*
Atom parser: an atom any simple prescript object.
It does not include 
code array objects (...{}).
*/

Tps_Status
Tps_get_atom(Tps_Stream_String& tokenbuf, Tps_Stream* f,
	   Tps_Value* vp, int escapes)
{
    long c;
    long tokenlen;
    char* tokenstring;
    char* nextchar;
    long slashed = 0;
    long colonized = 0;
    long longval;
    long havechar;
    long forcename;
#if HASFLOAT
    Tps_Real floatval;
#endif
#if PSMIMIC
    long parencount;
#endif

    tokenbuf.rewind();
loop:
    /* scan for non white-space or EOF. */
    while((c = f->read()) != EOF && ISWHITESPACE(c));
    switch (c) {
	case '%': /* comment begin*/
	    /* skip to EOF | EOL */
	    while((c = f->read()) != EOF && !ISEOL(c));
	    goto loop;
	case EOF: /* or EOS */
	    goto eof_done;
#if PSMIMIC 
	case LPAREN: /* ps string */
	    parencount = 1;
#else
	case DQUOTE: /* string */
#endif
	    while((c = f->read()) != EOF) {
		switch (c) {
#if PSMIMIC 
		    case RPAREN: if(--parencount == 0) goto done;
#else
		    case DQUOTE: goto done;
#endif
		    case ESCAPER: /* escape character */
			if(!escapes) {
			    tokenbuf.write(c);
			    break;
			}
			c = f->read();
			switch (c) {
			case EOF: goto syntaxerror;
			case 'n': tokenbuf.write('\n'); break;
			case 'r': tokenbuf.write('\r'); break;
			case 't': tokenbuf.write('\t'); break;
			case 'b': tokenbuf.write('\b'); break;
			case 'f': tokenbuf.write('\f'); break;
			case '\n': break;
			case '0': case '1': case '2':
			case '3': case '4': case '5':
			case '6': case '7': {
			    u_long d = (c - '0');
			    long i;
			    for(i=0;i<2;i++) {
				c = f->read();
				if(!ISOCTDIGIT(c)) break;
				d = d * 8 + (c - '0');
			    }
			    /* we always read 1 too far. */
			    f->pushback(c);
			    tokenbuf.write((d & 0xff));
			    } break;
			default: /* for escape chars */
			    tokenbuf.write(c);
			    break;
			} /*switch*/
			break;
		    default: /* for ordinary chars */
			tokenbuf.write(c);
			break;
		} /*switch*/
	    } /* while*/
done:
	    {
	    Tps_String* ss;
	    tokenbuf.ends();
	    tokenstring = tokenbuf.contents();
	    ss = new Tps_String(tokenstring,strlen(tokenstring));
	    TPS_MAKEVALUE(*vp,TPSTYPE_STRING,ss);
	    }
	    return TPSSTAT_OK;

	case LANGLE: /* hex string or dict */
	    c = f->read();
	    if(c == LANGLE) {
		/* this is the dict equiv of lbracket. */
		*vp = TPS__NM(TPS_NMLLANGLE);
		TPS_SET_EXECUTABLE(*vp,1);
		break;
	    } else
		(void)f->pushback(c);
	    /* assume its a hex string */
	    tokenlen = 0;
	    while((c = f->read()) != EOF && c != RANGLE) {
		if(ISWHITESPACE(c)) continue;
		if(!ISHEXDIGIT(c)) goto syntaxerror;
		tokenbuf.write(c);
		tokenlen++;
	    }
	    if(c == EOF) goto syntaxerror;
	    if(tokenlen % 2 == 1) {
		tokenbuf.write('0');
		tokenlen++;
	    }
	    {
		Tps_String* ss;
		long i,j;
		char* hexstr;
		char* h;
		hexstr = tokenbuf.contents();
		ss = new Tps_String(tokenlen/2);
		if(!ss) return(TPSSTAT_SYSTEMERROR);
		for(i=0,h=hexstr;i<ss->length();i++) {
		    register u_long d;
		    for(d=0,j=0;j<2;j++) {		
			d <<= 4;
			c = *h++;
			if(c >= '0' && c <= '9') c -= '0';
			else if(c >= 'a' && c <= 'f') c -= 'a';
			else if(c >= 'A' && c <= 'F') c -= 'A';
			d |= c;
		    }
		    ss->contents()[i] = (char)d;
		}
		TPS_MAKEVALUE(*vp,TPSTYPE_STRING,ss);
	    } break;

	/* special atoms */
	case RANGLE:
	    c = f->read();
	    if(c == RANGLE) {
		/* this is the dict equiv of rbracket. */
		*vp = TPS__NM(TPS_NMRRANGLE);
		TPS_SET_EXECUTABLE(*vp,1);
		break;
	    } else
		f->pushback(c);
	    goto syntaxerror; /* single rangle is not legal */

#if ! PSMIMIC
	case SQUOTE:
	    /* treat 'c' as a single character integer */
	    /* handle escapes */
	    c = f->read();
	    if(c == EOF) goto syntaxerror;
	    if(c == ESCAPER) {
		c = f->read();
		switch (c) {
		    case EOF: goto syntaxerror;
		    case 'n': c = '\n'; break;
		    case 'r': c = '\r'; break;
		    case 't': c = '\t'; break;
		    case 'b': c = '\b'; break;
		    case 'f': c = '\f'; break;
		    case '0': case '1': case '2':
		    case '3': case '4': case '5':
		    case '6': case '7': {
			register u_long d = (c - '0');
			long i;
			for(i=0;i<2;i++) {
			    c = f->read();
			    if(!ISOCTDIGIT(c)) break;
			    d = d * 8 + (c - '0');
			}
			/* we always read 1 too far. */
			f->pushback(c);
			c = d;
		    } break;
		    default: /* for escaped chars; just use char */
			break;
		} /*switch*/
	    } /* ESCAPE */
	    TPS_MAKEVALUE(*vp,TPSTYPE_INTEGER,c);
	    /* check for trailing squote */
	    c = f->read();
	    if(c != SQUOTE) goto syntaxerror;
	    break;
#endif /*!PSMIMIC*/

	case '/': /* definitely a name */
	    slashed = 1;
	    forcename = 1;
	    c = f->read();
	    if((c == EOF || !ISNAMECHAR(c)))
		goto syntaxerror;
	    goto getname;

#if OO
	case ':': /* definitely a name */
	    colonized = 1;
	    forcename = 1;
	    c = f->read();
	    if((c == EOF || !ISNAMECHAR(c)))
		goto syntaxerror;
	    goto getname;
#endif /*OO*/

	default: /* namelike chars */
	    forcename=0;
getname:
	    tokenbuf.write(c);
	    if(!ISSPECIAL(c) || forcename) {
		while((c = f->read()) != EOF) {
		    havechar=1;
		    if(c == ESCAPER) {
			havechar=0;
			forcename=1;
			c = f->read();
			switch (c) {
			case EOF: goto syntaxerror;
			case 'n': tokenbuf.write('\n'); break;
			case 'r': tokenbuf.write('\r'); break;
			case 't': tokenbuf.write('\t'); break;
			case 'b': tokenbuf.write('\b'); break;
			case 'f': tokenbuf.write('\f'); break;
			case '\n': break;
			case '0': case '1': case '2':
			case '3': case '4': case '5':
			case '6': case '7': {
			    register u_long d = (c - '0');
			    long i;
			    for(i=0;i<2;i++) {
				c = f->read();
				if(!ISOCTDIGIT(c)) break;
				d = d * 8 + (c - '0');
			    }
			    /* we always read 1 too far. */
			    f->pushback(c);
			    /* never allow for nulls to be inserted */
			    d &= 0xff;
			    if(d) tokenbuf.write(d);
			    } break;
			default: /* for escape chars */
			    tokenbuf.write(c);
			    break;
			} /*switch*/
		    } else if(ISNAMECHAR(c) && !ISSPECIAL(c))
			tokenbuf.write(c);
		    else
			break;
		}
		if(c != EOF && havechar)
		    f->pushback(c); /* always read too far */
	    }
	    /* must be null terminated */
	    tokenbuf.ends();
	    /* get the current string */
	    tokenstring = tokenbuf.contents();
	    tokenlen = strlen(tokenstring);
	    if(forcename) goto truename;
	    /* see if this is a number */
#if PSMIMIC
	    longval = strtol(tokenstring,&nextchar,10); /* allow only decimal*/
#else
	    longval = strtol(tokenstring,&nextchar,0); /* use c conventions */
#endif
	    if(nextchar != tokenstring && !*nextchar) {
		/* this is an simple integer */
		TPS_MAKEVALUE(*vp,TPSTYPE_INTEGER,longval);
		return TPSSTAT_OK;
	    }
	    /* check for a radix'd integer */
	    if(*nextchar == '#') {
		long radix = longval;
		if(radix != 2 && radix != 8 && radix != 10 && radix != 16)
		    goto truename;
		tokenstring = nextchar + 1;
		longval = STRTOL(tokenstring,&nextchar,radix);
		if(nextchar != tokenstring && !*nextchar) {
		    TPS_MAKEVALUE(*vp,TPSTYPE_INTEGER,longval);
		    return TPSSTAT_OK;
		}
	    }
#if HASFLOAT
	    floatval = strtod(tokenstring,&nextchar);
	    if(nextchar != tokenstring && !*nextchar) {
		/* this is a float */
		TPS_MAKEREAL(*vp,floatval);
		return TPSSTAT_OK;
	    }
#endif
	    /* ok, must be a true identifier. */
/*assumeid:*/
truename:
	    {
	    Tps_Nameid n = TPS_NAMETABLE->newname(tokenstring);
	    TPS_MAKEVALUE(*vp,TPSTYPE_NAME,n);
	    TPS_SET_EXECUTABLE(*vp,slashed?0:1);
#if OO
	    TPS_SET_METHOD(*vp,colonized?1:0);
#endif
	    }
	    return TPSSTAT_OK;
    } /*switch*/
    return TPSSTAT_OK;

eof_done: /* eof encountered */
    return(TPSSTAT_EOF);
syntaxerror:
    return(TPSSTAT_SYNTAXERROR);
}


/**************************************************/

/*
Token parser: a token is any simple prescript object
plus  well as executable array objects (...{}).
*/

/* utility for actually doing the construction */

enum Compounds { isdict, isarray };

static
Tps_Status
scancompound(Tps_Stream_String& tokenbuf, Tps_Stream* f,
		  enum Compounds delimiter, Tps_Value* result, int escapes)
{
    register Tps_Status ok;
    Tps_Value object;
    register long i;
    register Tps_Nameid rightside
		= (delimiter == isdict? TPS_NM(TPS_NMRRANGLE) : TPS_NM(TPS_NMRBRACE));

    Tps_Array* a; /* will hold both dict and array elements until end */

    a = new Tps_Array(16);
    i = 0;
    while(1) {
	ok = Tps_get_atom(tokenbuf,f,&object,escapes);
	if(ok != TPSSTAT_OK)
	    goto failed;
	if(TPS_ISTYPE(object,TPSTYPE_NAME)) {
	    if(TPS_NAME_OF(object) == rightside)
		goto done;
	    else if(TPS_NAME_OF(object) == TPS_NM(TPS_NMLBRACE)) {
		/* looks like the start of a constant array , recurse */
		ok= scancompound(tokenbuf,f,isarray,&object,escapes);
		if(ok != TPSSTAT_OK)
		    goto failed;
	    }
	}
	if(i >= a->alloc()) {
	    ok = a->setlength(a->alloc()+16);
	    if(ok != TPSSTAT_OK)
		goto failed;
	}
	a->contents()[i++] = object;
    }
done:
    if(delimiter == isarray) {
	/* set the length of the array correctly */
	a->setlength(i);
	TPS_MAKEVALUE(*result,TPSTYPE_ARRAY,a);
	TPS_SET_EXECUTABLE(*result,1);
    }
    return TPSSTAT_OK;
failed:
    if(a) Tps_free((char*)a);
    return ok;
}


Tps_Status
Tps_get_token(Tps_Stream_String& tokenbuf, Tps_Stream* f, Tps_Value* vp, int escapes)
{
    register Tps_Status ok;
    Tps_Value object;

    ok = Tps_get_atom(tokenbuf,f,&object,escapes);
    if(ok != TPSSTAT_OK) return ok;
    if(TPS_ISTYPE(object,TPSTYPE_NAME)) {
	if(TPS_NAME_OF(object) == TPS_NM(TPS_NMLBRACE)) {
	    /* looks like the start of a constant array */
	    ok = scancompound(tokenbuf,f,isarray,&object,escapes);
	}
    }
    if(ok == TPSSTAT_OK) { if(vp) *vp = object; }
    return ok;
}
