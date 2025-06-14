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
#include "mem.H"

/* min to extend an string: must be at least 1 */
const int TPS_STRING_MINEXTEND = 4;

boolean
Tps_String::initialize(long sz)
{
    long alloc;
    if(sz < 0) sz = 0;
    alloc = sz?sz:TPS_STRING_MINEXTEND;
    _contents = (char*)Tps_malloc(alloc);
    if(!_contents) return FALSE;
    if(sz > 0) memset((char*)_contents,0,sz);
    _alloc = alloc;
    _len = sz;
    return TRUE;
}

Tps_String::Tps_String(long sz) : Tps_Container(TPSTYPE_STRING)
{
    initialize(sz);
}

Tps_String::Tps_String(const char* initstr, long len) : Tps_Container(TPSTYPE_STRING)
{
    initialize(len);
    memcpy(_contents,initstr,len);
}

Tps_String::Tps_String(const char* initstr)
	    : Tps_Container(TPSTYPE_STRING)
{
    size_t len = strlen(initstr);
    initialize(len);
    memcpy(_contents,initstr,len);
}

Tps_String::~Tps_String()
{
    if(_contents) Tps_free((char*)_contents);
}

Tps_Status
Tps_String::extend(long need)
{
    long newalloc;
    char* newcontents;

    if(need <= TPS_STRING_MINEXTEND) need = TPS_STRING_MINEXTEND;
    newalloc = _len+need;
    if(_alloc >= newalloc) return TPSSTAT_OK;
    newcontents = (char*)Tps_malloc(newalloc*sizeof(char));
    if(!newcontents) return TPSSTAT_SYSTEMERROR;
    memcpy((char*)newcontents,(char*)_contents,_len*sizeof(char));
    memset(newcontents+_len,'\0',need*sizeof(char));
    _alloc = newalloc;
    _contents = newcontents;
    return TPSSTAT_OK;
}

Tps_Status
Tps_String::setlen(long newlen)
{
    Tps_Status ok;
    if(newlen < 0) return TPSSTAT_RANGECHECK;
    if(newlen > _alloc) {
	ok = extend(newlen - _alloc);
	if(ok != TPSSTAT_OK) return ok;
    }   
    if(newlen > _len) {
	memset((char*)(_contents+_len),0,(newlen - _len));
    }
    _len = newlen;
    return TPSSTAT_OK;
}

Tps_Status
Tps_String::append(const char* s, long slen)
{
    if((_len + slen) > _alloc) {
	Tps_Status ok;
	ok = extend(slen);
	if(ok != TPSSTAT_OK) return ok;
    }
    memcpy(&_contents[_len],s,slen);
    _len += slen;
    return TPSSTAT_OK;
}

Tps_Status
Tps_String::append(char c)
{
    if(_len >= _alloc) {
	Tps_Status ok;
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
    char* s = _contents;
    initialize(0);
    return s;
}
