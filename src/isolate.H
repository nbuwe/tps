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
/* ISOLATION MACROS */
/**************************************************/
/* Argument stack operators */

#define TPS_DEPTH(this) ((this)->depth())
#define TPS_ROOM(this) ((this)->room())
#define TPS_PUSH(this,value) ((this)->push(value))
#define TPS_PUSHN(this,n) ((this)->pushn(n))
#define TPS_POP(this) ((this)->pop())
#define TPS_POPN(this,n) ((this)->popn(n))
#define TPS_TOSP(this) ((this)->tosp())
#define TPS_TOP(this) ((this)->top())
#define TPS_BASE(this) ((this)->base())
#define TPS_SETTOSP(this,argptr) ((this)->settosp(argptr))
#define TPS_CLEAR(this) ((this)->clear())

/**************************************************/
/* Dict stack operators */

#define TPS_DDEPTH(this) ((this)->ddepth())
#define TPS_DROOM(this) ((this)->droom())
#define TPS_DPUSH(this,value) ((this)->dpush(value))
#define TPS_DPUSHN(this,n) ((this)->dpushn(n))
#define TPS_DPOP(this) ((this)->dpop())
#define TPS_DPOPN(this,n) ((this)->dpopn(n))
#define TPS_DTOP(this) ((this)->dtop())
#define TPS_DTOSP(this) ((this)->dtosp())
#define TPS_DCLEAR(this) ((this)->dclear())

/* for safe/unsafe dict stacks */
#define TPS_DDEPTHSF(this,i) ((this)->_dstacks[i]._last - (this)->_dstacks[i]._tos)
#define TPS_DTOSPSF(this,i) ((this)->_dstacks[i]._tos)
#define TPS_DCLEARSF(this,i) ((this)->_dstacks[i]._tos = (this)->_dstacks[i]._last)
#define TPS_DPUSHNSF(this,i,n) ((this)->_dstacks[i]._tos -= (n))

/**************************************************/
/* Exec stack operators */

#define TPS_EDEPTH(this) ((this)->edepth())
#define TPS_EROOM(this) ((this)->eroom())
#define TPS_EPUSHN(this,n) ((this)->epushn(n))
#define TPS_EPOPN(this,n) ((this)->epopn(n))
#define TPS_ETOSP(this) ((this)->etosp())
#define TPS_EBASE(this) ((this)->ebase())
#define TPS_ECLEAR(this) ((this)->eclear())
/* support estack offsets */
#define TPS_EOFFSET(this,f) ((this)->eoffset((char*)f))
#define TPS_EOFFBASE(this,n) ((this)->eoffbase(n))

/* Special Macros */

#define TPS_EFRAMECOUNT(this) ((this)->_framecount)

#define TPS_GUARANTEE(this,n) if(TPS_ROOM(this) < n) {return TPSSTAT_STACKOVERFLOW;} else {}

#define TPS_DGUARANTEE(this,n) if(TPS_DROOM(this) < n) {return TPSSTAT_DICTSTACKOVERFLOW;} else {}

#define TPS_EGUARANTEE(this,n) if(TPS_EROOM(this) < n) {return TPSSTAT_EXECSTACKOVERFLOW;} else {}
