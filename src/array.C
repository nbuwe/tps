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

/**************************************************/

#include "tps.H"
#include "mem.H"

/**************************************************/

/* min to extend an array: must be at least 1 */
const int TPS_ARRAY_MINEXTEND = 4;

/**************************************************/

Tps_Array::Tps_Array(long sz)
	   : Tps_Container(TPSTYPE_ARRAY)
{
    long alloc;
    if(sz < 0) sz = 0;
    alloc = sz?sz:TPS_ARRAY_MINEXTEND;
    _contents = (Tps_Value*)Tps_malloc(sizeof(Tps_Value)*alloc);
    if(!_contents) return;
    if(sz > 0) memset((char*)_contents,0,sz*sizeof(Tps_Value));
    _alloc = alloc;
    _len = sz;
}

Tps_Array::~Tps_Array()
{
    if(_contents) Tps_free((char*)_contents);
}

Tps_Status
Tps_Array::extend(long need)
{
    Tps_Value* newcontents;
    long newalloc;

    if(need <= TPS_ARRAY_MINEXTEND) need = TPS_ARRAY_MINEXTEND;
    newalloc = _len+need;
    if(_alloc >= newalloc) return TPSSTAT_OK;
    newcontents = (Tps_Value*)Tps_malloc(newalloc*sizeof(Tps_Value));
    if(!newcontents) return TPSSTAT_SYSTEMERROR;
    memcpy((char*)newcontents,(char*)_contents,_len*sizeof(Tps_Value));
    memset((char*)(newcontents+_len),'\0',need*sizeof(Tps_Value));
    _alloc = newalloc;
    _contents = newcontents;
    return TPSSTAT_OK;
}

Tps_Status
Tps_Array::setlen(long newlen)
{
    Tps_Status ok;
    if(newlen < 0) return TPSSTAT_RANGECHECK;
    if(newlen > _alloc) {
	ok = extend(newlen - _alloc);
	if(ok != TPSSTAT_OK) return ok;
    }   
    if(newlen > _len) {
	memset((char*)(_contents+_len),0,(newlen - _len)*sizeof(Tps_Value));
    }
    _len = newlen;
    return TPSSTAT_OK;
}

Tps_Status
Tps_Array::append(Tps_Value* vs, long vlen)
{
    if(_len + vlen >= _alloc) {
	Tps_Status ok;
	ok = extend(vlen);
	if(ok != TPSSTAT_OK) return ok;
    }
    memcpy((char*)&_contents[_len],(char*)vs,vlen*sizeof(Tps_Value));
    _len += vlen;
    return TPSSTAT_OK;
}

Tps_Status
Tps_Array::append(Tps_Value v)
{
    if(_len >= _alloc) {
	Tps_Status ok;
	ok = extend(1);
	if(ok != TPSSTAT_OK) return ok;
    }
    _contents[_len] = v;
    _len++;
    return TPSSTAT_OK;
}

Tps_Status
Tps_Array::append(Tps_Array* a)
{
    return append(a->_contents,a->_len);
}

void
Tps_Array::mark()
{
    Tps_Value* ap;
    int i;

    if(marked()) return;
    Tps_Container::mark();  /* mark self */
    /* walk all entries in array and have them marked */
    for(i=0,ap=contents();i<length();i++,ap++) {
	Tps_mark(*ap);
    }
}

