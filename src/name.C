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
#include "mem.H"

#define ckalloc(n) Tps_malloc(n)
#define ckfree(p) Tps_free((char*)p)

/**************************************************/

Tps_Nametable::Tps_Nametable(long chains)
	: Tps_Dict_Tcl(chains,"nametable")
{return;}

Tps_Nametable::~Tps_Nametable(void)
{
    /* before deleting, go thru and clean up the writable names */
    Tps_Status ok;
    int i;
    long l;
    Tps_Dictpair* pairp;

    for(l=length(),i=0;i<l;i++) {
	ok = ith(i,pairp);
	if(ok == TPSSTAT_UNDEFINED) continue;
	if(ok != TPSSTAT_OK) break;
	if(TPS_ISWRITEABLE(pairp->_key)) {
	    delete TPS_NAME_OF(pairp->_key);
	}
    }
}

/* Assume that the arg is null terminated */
Tps_Nameid
Tps_Nametable::newname(const char* nm, boolean ro)
{
    Tps_Nameid s = 0;
    long len;

    GLOBAL_LOCK();

#ifdef NOINLINE
    Tps_Dictpair pair;
    Tps_Status ok;
    Tps_Dictpair* pairp;

    TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
    ok = lookup(pair._key,&pairp);
    if(ok == TPSSTAT_OK) {/* already exists */
	s = TPS_NAME_OF(pairp->_key);
    } else { /* does not exist, add it */
	if(ro) {
	    s = nm;
	} else {
	    len = strlen(nm);
	    char *buf = new char[len+1];
	    strcpy(buf,nm);
	    s = buf;
	}
	TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,s);
	if(ro) TPS_SET_ACCESS(pair._key,Tps_access_readonly);
	TPS_MAKEVALUE(pair._value,TPSTYPE_NULL,0);
	ok = insert(pair,(Tps_Value*)0);
	if(ok != TPSSTAT_OK) s = (Tps_Nameid)0;
    }
#else /*!NOINLINE*/
    /* inline combined lookup + possible insert */
    const char* knm;
    u_long h,c,index;
    Tps_HashEntry* hPtr;

    for(knm=nm,h=0;c=*knm++;) {h += (h<<3) + c;}
    h = randomize(h);
    index = (h >> downShift) & mask;
    for(hPtr = buckets[index].chain; hPtr != NULL; hPtr = hPtr->nextPtr) {
	Tps_Value e = hPtr->pair._key;
	if(h == hPtr->hashvalue && strcmp(nm,TPS_NAME_OF(e))==0) {
	    s = TPS_NAME_OF(hPtr->pair._key);
	    break;
	}
    }
    if(!s) {
	if(ro) {
	    s = nm;
	} else {
	    len = strlen(nm);
	    char *buf = new char[len+1];
	    strcpy(buf,nm);
	    s = buf;
	}
	/* inline insert */
	hPtr = (Tps_HashEntry *) ckalloc((unsigned)(sizeof(Tps_HashEntry)));
	hPtr->bucketPtr = &(buckets[index]);
	hPtr->nextPtr = hPtr->bucketPtr->chain;
	hPtr->hashvalue = h;
	TPS_MAKEVALUE(hPtr->pair._key,TPSTYPE_NAME,s);
	TPS_MAKEVALUE(hPtr->pair._value,TPSTYPE_NULL,0);
	hPtr->bucketPtr->chain = hPtr;
	hPtr->bucketPtr->chainlen++;
	if(hPtr->bucketPtr->chainlen > maxlen)
	    maxlen = hPtr->bucketPtr->chainlen;
	numEntries++;
    }
#endif /*NOINLINE*/
    GLOBAL_UNLOCK();
    return s;
}

/**************************************************/
/* inlined elsewhere; see NOINLINE conditionals */

u_long
Tps_Nametable::namehash(const char* kn)
{
    u_long h = 0;
    int c;

    /* do something with the bytes in the string */
    /* this is the tcl hash function*/
    while(1) {
	c = *kn;
	kn++;
	if (c == 0) break;
	h += (h<<3) + c;
    }
    return randomize(h);
}

Tps_Status
Tps_Nametable::lookup(Tps_Value key, Tps_Dictpair** pairp)
{
    Tps_HashEntry *hPtr;
    u_long index;
    u_long h;
    Tps_Nameid knm;
    Tps_Nameid kn;
    int c;

    if(TPS_TYPE(key) != TPSTYPE_NAME) return TPSSTAT_UNDEFINED;
    kn = (knm = TPS_NAME_OF(key));

#ifdef NOINLINE
    h = namehash(kn);
#else /*!NOINLINE*/
    /* do something with the bytes in the string */
    /* this is the tcl hash function*/
    h = 0;
    while(c=*kn++) {
	h += (h<<3) + c;
    }
    h = randomize(h);
#endif /*NOINLINE*/

    index = (h >> downShift) & mask;

    /*
     * Search all of the entries in the appropriate bucket.
     */

    for (hPtr = buckets[index].chain; hPtr != NULL; hPtr = hPtr->nextPtr) {
	Tps_Value e = hPtr->pair._key;
	if(h == hPtr->hashvalue && strcmp(knm,TPS_NAME_OF(e))==0) {
	    if(pairp) *pairp = &hPtr->pair;
	    return TPSSTAT_OK;
	}
    }
    return TPSSTAT_UNDEFINED;
}

Tps_Status
Tps_Nametable::insert(Tps_Dictpair& pair, Tps_Value* oldvalue, boolean)
{
    Tps_HashEntry *hPtr;
    u_long index;
    u_long h;
    Tps_Nameid knm;
    Tps_Nameid kn;
    int c;

    if(TPS_TYPE(pair._key) != TPSTYPE_NAME) return TPSSTAT_UNDEFINED;
    kn = (knm = TPS_NAME_OF(pair._key));

#ifdef NOINLINE
    h = namehash(kn);
#else /*!NOINLINE*/
    /* do something with the bytes in the string */
    /* this is the tcl hash function*/
    h = 0;
    while(c=*kn++) {
	h += (h<<3) + c;
    }
    h = randomize(h);
#endif /*NOINLINE*/

    index = (h >> downShift) & mask;

    /*
     * Search all of the entries in this bucket.
     */
    for (hPtr = buckets[index].chain; hPtr != NULL; hPtr = hPtr->nextPtr) {
	Tps_Value e = hPtr->pair._key;
	if(h == hPtr->hashvalue && strcmp(knm,TPS_NAME_OF(e))) {
	    if(oldvalue) *oldvalue = hPtr->pair._value;
	    hPtr->pair = pair;
	    return TPSSTAT_OK;
	}
    }

    /*
     * Entry not found.  Add a new one to the bucket.
     */

    hPtr = (Tps_HashEntry *) ckalloc((unsigned)(sizeof(Tps_HashEntry)));
    hPtr->bucketPtr = &(buckets[index]);
    hPtr->nextPtr = hPtr->bucketPtr->chain;
    hPtr->hashvalue = h;
    hPtr->pair = pair;
    hPtr->bucketPtr->chain = hPtr;
    hPtr->bucketPtr->chainlen++;
    if(hPtr->bucketPtr->chainlen > maxlen)
	maxlen = hPtr->bucketPtr->chainlen;
    numEntries++;

    /*
     * If the table has exceeded a decent size, rebuild it with many
     * more buckets.
     */

    if(numEntries >= rebuildSize) {
	RebuildTable();
    }
    return TPSSTAT_OK;
}
