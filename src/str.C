/*
Copyright (c) 1993 Dennis Heimbigner
All rights reserved.

This software was developed by Dennis Heimbigner
as part of the Arcadia project at
the University of Colorado, Boulder.

Redistribution and use in source and binary forms are permitted
provided that the above copyright notice and this paragraph are
duplicated in all such forms and that any documentation,
and other materials related to such distribution and use
acknowledge that the software was developed by
Dennis Heimbigner.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "tps.H"
#include "mem.H"

/* min to extend an string: must be at least 1 */
const int TPS_STRING_MINEXTEND = 4;

boolean
Tps_String::initialize(long sz)
{
    register long alloc;
    if(sz < 0) sz = 0;
    alloc = sz?sz:TPS_STRING_MINEXTEND;
    _contents = (char*)Tps_malloc(alloc);
    if(!_contents) return FALSE;
    if(sz > 0) MEMSET((char*)_contents,0,sz);
    _alloc = alloc;
    _len = sz;
    return TRUE;
}

Tps_String::Tps_String(long sz) : Tps_Container(TPSTYPE_STRING)
{
    initialize(sz);
}

Tps_String::Tps_String(char* initstr, long len) : Tps_Container(TPSTYPE_STRING)
{
    initialize(len);
    MEMCPY(_contents,initstr,len);
}

Tps_String::Tps_String(char* initstr)
	    : Tps_Container(TPSTYPE_STRING)
{
    register len = strlen(initstr);
    initialize(len);
    MEMCPY(_contents,initstr,len);
}

Tps_String::~Tps_String()
{
    if(_contents) Tps_free((char*)_contents);
}

Tps_Status
Tps_String::extend(long need)
{
    register long newalloc;
    register char* newcontents;

    if(need <= TPS_STRING_MINEXTEND) need = TPS_STRING_MINEXTEND;
    newalloc = _len+need;
    if(_alloc >= newalloc) return TPSSTAT_OK;
    newcontents = (char*)Tps_malloc(newalloc*sizeof(char));
    if(!newcontents) return TPSSTAT_SYSTEMERROR;
    MEMCPY((char*)newcontents,(char*)_contents,_len*sizeof(char));
    MEMSET(newcontents+_len,'\0',need*sizeof(char));
    _alloc = newalloc;
    _contents = newcontents;
    return TPSSTAT_OK;
}

Tps_Status
Tps_String::setlen(long newlen)
{
    register Tps_Status ok;
    if(newlen < 0) return TPSSTAT_RANGECHECK;
    if(newlen > _alloc) {
	ok = extend(newlen - _alloc);
	if(ok != TPSSTAT_OK) return ok;
    }   
    if(newlen > _len) {
	MEMSET((char*)(_contents+_len),0,(newlen - _len));
    }
    _len = newlen;
    return TPSSTAT_OK;
}

Tps_Status
Tps_String::append(char* s, long slen)
{
    if((_len + slen) > _alloc) {
	register Tps_Status ok;
	ok = extend(slen);
	if(ok != TPSSTAT_OK) return ok;
    }
    MEMCPY(&_contents[_len],s,slen);
    _len += slen;
    return TPSSTAT_OK;
}

Tps_Status
Tps_String::append(char c)
{
    if(_len >= _alloc) {
	register Tps_Status ok;
	ok = extend(1);
	if(ok != TPSSTAT_OK) return ok;
    }
    _contents[_len] = c;
    _len++;
    return TPSSTAT_OK;
}

Tps_Status
Tps_String::append(Tps_String* sappend)
{
    return append(sappend->contents(),sappend->length());
}

boolean
Tps_String::isnullterminated()
{
    if(_len > 0 && _contents[_len - 1] == 0) return TRUE;
    if(_len < _alloc && _contents[_len] == 0) return TRUE;
    return FALSE;
}

/* Following cheats by appending null and then reducing length */
Tps_Status
Tps_String::nullterminate()
{
    if(!isnullterminated()) {
	append((char)0);
	_len--;
    }
    return TPSSTAT_OK;
}

char*
Tps_String::extract()
{
    register char* s = _contents;
    initialize(0);
    return s;
}
