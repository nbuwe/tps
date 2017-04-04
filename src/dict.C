/*
Copyright (c) 1993 Dennis Heimbigner
All rights reserved.

This software was developed as part of the Arcadia project
at the University of Colorado, Boulder.

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

/**************************************************/

const long DEFAULT_DICT_SIZE = 64;
const long DEFAULT_DICT_CHAINS = 8;


#define EXPANSION(alloc) ((alloc) + (alloc)/4 + 1)

/**************************************************/

u_long
Tps_Dict::stringhash(Tps_String* key)
{
    /* do something with the bytes in the string */
    register long i;
    register u_long h;
    h = 0;
    for(h=0,i=0;i<key->length();i++) {
	h += (h<<3)+key->contents()[i];
    }
    return randomize(h);
}

boolean
Tps_Dict::stringmatch(Tps_String* sp1, Tps_String* sp2)
{
    if(sp1->length() != sp2->length()) return FALSE;
    if(sp1->length() == 0) return TRUE; /* differs from bcmp*/
    return (MEMCMP(sp1->contents(),sp2->contents(),sp1->length())==0);
}

void
Tps_Dict::mark(void)
{
    if(marked()) return;
    Tps_Container::mark();  /* mark self */
    /* walk all entries and have them marked */
    for(int i=0;i<range();i++) {
	Tps_Dictpair* pairp;
	register Tps_Status ok = ith(i,pairp);
	if(ok == TPSSTAT_UNDEFINED) continue;
	if(ok != TPSSTAT_OK) break;
	Tps_mark(pairp->_key);
	Tps_mark(pairp->_value);
    }
}

/**************************************************/

#include <stdio.h>
/* 
 * tclHash.c --
 *
 *	Implementation of in-memory hash tables for Tcl and Tcl-based
 *	applications.
 *
 * Copyright (c) 1991-1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#define panic(msg) fprintf(stderr,msg)
#define ckalloc(n) Tps_malloc(n)
#define ckfree(p) Tps_free((char*)p)

#define REBUILD_MULTIPLIER 3

#define TPS_MAXCHAINLEN 4

/*
 *----------------------------------------------------------------------
 *
 * Tps_InitHashTable --
 *
 *	Given storage for a hash table, set up the fields to prepare
 *	the hash table for use.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TablePtr is now ready to be passed to Tps_FindHashEntry and
 *	Tps_CreateHashEntry.
 *
 *----------------------------------------------------------------------
 */

Tps_Dict_Tcl::Tps_Dict_Tcl(long bucketcount, const char* nm)
	: Tps_Dict(Tps_tcldict,nm)
{
    buckets = staticBuckets;
    numBuckets = TPS_SMALL_HASH_TABLE;
    numEntries = 0;
    downShift = 28;
    mask = 3;
    if(bucketcount > TPS_SMALL_HASH_TABLE) {
	while(numBuckets < bucketcount) {
	    numBuckets *= 4;
	    downShift -= 2;
	    mask = (mask << 2) + 3;
	}
	buckets = (Tps_Bucket*) ckalloc((unsigned)
	    (numBuckets * sizeof(Tps_Bucket)));
    }
    MEMSET((char*)buckets,0,numBuckets*sizeof(Tps_Bucket));
    rebuildSize = REBUILD_MULTIPLIER * numBuckets;
    maxlen = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tps_DeleteHashTable --
 *
 *	Free up everything associated with a hash table except for
 *	the record for the table itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The hash table is no longer useable.
 *
 *----------------------------------------------------------------------
 */

void
Tps_Dict_Tcl::clear(void)
{
    register Tps_HashEntry *hPtr, *nextPtr;
    int i;

    /*
     * Free up all the entries in the table.
     */

    for (i = 0; i < numBuckets; i++) {
	hPtr = buckets[i].chain;
	while (hPtr != NULL) {
	    nextPtr = hPtr->nextPtr;
	    ckfree((char *) hPtr);
	    hPtr = nextPtr;
	}
	buckets[i].chainlen = 0;
    }
    numEntries = 0;
    maxlen = 0;
}

Tps_Dict_Tcl::~Tps_Dict_Tcl(void)
{
    clear();

    /*
     * Free up the bucket array, if it was dynamically allocated.
     */

    if (buckets != staticBuckets) {
	ckfree((char *) buckets);
    }
}

Tps_Status
Tps_Dict_Tcl::remove(Tps_Value key, Tps_Dictpair* pairp)
{
    register Tps_Bucket* b;
    register Tps_HashEntry *hPtr;
    register Tps_HashEntry *prev;
    register u_long index;
    register u_long h;

#ifdef NOINLINE
    h = hashfcn(key);
#else
    register Tps_Typeid kid = TPS_TYPE(key);
    if(kid == TPSTYPE_STRING)
	h = stringhash(TPS_STRING_OF(key));
    else
	h = wordhash(key);
#endif

    index = (h >> downShift) & mask;

    b = buckets + index;
    for (hPtr = b->chain; hPtr != NULL; hPtr = hPtr->nextPtr) {
	if(h == hPtr->hashvalue && match(key,hPtr->pair._key)) {
	    if(pairp) *pairp = hPtr->pair;
	    b->chainlen--;
	    if(hPtr->bucketPtr->chain == hPtr) {
		hPtr->bucketPtr->chain = hPtr->nextPtr;
	    } else {
		for(prev= hPtr->bucketPtr->chain;;prev=prev->nextPtr) {
		    if(prev == NULL) {
			panic("malformed bucket chain in Tps_DeleteHashEntry");
		    }
		    if(prev->nextPtr == hPtr) {
			prev->nextPtr = hPtr->nextPtr;
			break;
		    }
		}
	    }
	    numEntries--;
	    ckfree((char *) hPtr);
	}
    }
    return TPSSTAT_OK;
}

Tps_Status
Tps_Dict_Tcl::lookup(Tps_Value key, Tps_Dictpair** pairp)
{
    register Tps_HashEntry *hPtr;
    register u_long index;
    register u_long h;

#ifdef NOINLINE
    h = hashfcn(key);
#else
    register Tps_Typeid kid = TPS_TYPE(key);
    if(kid == TPSTYPE_STRING)
	h = stringhash(TPS_STRING_OF(key));
    else
	h = wordhash(key);
#endif

    index = (h >> downShift) & mask;

    /*
     * Search all of the entries in the appropriate bucket.
     */

    for (hPtr = buckets[index].chain; hPtr != NULL; hPtr = hPtr->nextPtr) {
#ifdef NOINLINE
	if(h == hPtr->hashvalue && match(key,hPtr->pair._key))
#else /*!NOINLINE*/
	Tps_Value e = hPtr->pair._key;
	if(h == hPtr->hashvalue && kid == TPS_TYPE(e)
	   && ((kid == TPSTYPE_STRING
		  && stringmatch(TPS_STRING_OF(key),TPS_STRING_OF(e)))
	       || TPS_ANY_OF(key) == TPS_ANY_OF(e)))
#endif /*NOINLINE*/
	{
	    if(pairp) *pairp = &hPtr->pair;
	    return TPSSTAT_OK;
	}
    }
    return TPSSTAT_UNDEFINED;
}

Tps_Status
Tps_Dict_Tcl::insert(Tps_Dictpair& pair, Tps_Value* oldvalue, boolean suppress)
{
    register Tps_HashEntry *hPtr;
    register u_long index;
    register u_long h;

#ifdef NOINLINE
    h = hashfcn(pair._key);
#else
    register Tps_Typeid kid = TPS_TYPE(pair._key);
    if(kid == TPSTYPE_STRING)
	h = stringhash(TPS_STRING_OF(pair._key));
    else
	h = wordhash(pair._key);
#endif

    index = (h >> downShift) & mask;

    /*
     * Search all of the entries in this bucket.
     */
    for (hPtr = buckets[index].chain; hPtr != NULL; hPtr = hPtr->nextPtr) {
	if(h == hPtr->hashvalue && match(pair._key,hPtr->pair._key)) {
	    if(oldvalue) *oldvalue = hPtr->pair._value;
	    if(!suppress) hPtr->pair = pair;
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

/*
 *----------------------------------------------------------------------
 *
 * RebuildTable --
 *
 *	This procedure is invoked when the ratio of entries to hash
 *	buckets becomes too large.  It creates a new table with a
 *	larger bucket array and moves all of the entries into the
 *	new table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets reallocated and entries get re-hashed to new
 *	buckets.
 *
 *----------------------------------------------------------------------
 */

void
Tps_Dict_Tcl::RebuildTable(void)
{
    long oldSize, index;
    Tps_Bucket* oldBuckets;
    register Tps_Bucket* oldBPtr;
    register Tps_HashEntry *hPtr,*chain;

    oldSize = numBuckets;
    oldBuckets = buckets;

    /*
     * Allocate and initialize the new bucket array, and set up
     * hashing constants for new array size.
     */

    numBuckets *= 4;
    buckets = (Tps_Bucket*) ckalloc((unsigned)
	    (numBuckets * sizeof(Tps_Bucket)));
    MEMSET((char*)buckets,0,numBuckets*sizeof(Tps_Bucket));
    downShift -= 2;
    mask = (mask << 2) + 3;
    rebuildSize *= 4;

    /*
     * Rehash all of the existing entries into the new bucket array.
     */

    maxlen = 0;
    for (oldBPtr = oldBuckets; oldSize > 0; oldSize--, oldBPtr++) {
	for (hPtr = oldBPtr->chain; hPtr != NULL; hPtr = chain) {
	    chain = hPtr->nextPtr;
	    index = (hPtr->hashvalue >> downShift) & mask;
	    hPtr->bucketPtr = &(buckets[index]);
	    hPtr->nextPtr = hPtr->bucketPtr->chain;
	    hPtr->bucketPtr->chain = hPtr;
	    hPtr->bucketPtr->chainlen++;
	    if(hPtr->bucketPtr->chainlen > maxlen) 
		maxlen = hPtr->bucketPtr->chainlen;
	}
    }

    /*
     * Free up the old bucket array, if it was dynamically allocated.
     */

    if (oldBuckets != staticBuckets) {
	ckfree((char *) oldBuckets);
    }
}


Tps_Status
Tps_Dict_Tcl::ith(long i, Tps_Dictpair*& pairp)
{
    register Tps_HashEntry* p;
    register long rng = range();
    register long j;

    /* pretend that the range is 0..numBuckets*maxlen */

    if(i >= rng)
	return TPSSTAT_RANGECHECK;
    p = buckets[i/maxlen].chain;
    for(j=i%maxlen;j && p;j--,p=p->nextPtr);
    if(!p) return TPSSTAT_UNDEFINED;
    pairp = &p->pair;
    return TPSSTAT_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Tps_HashStats --
 *
 *	Return statistics describing the layout of the hash table
 *	in its hash buckets.
 *
 * Results:
 *	The return value is a malloc-ed string containing information
 *	about tablePtr.  It is the caller's responsibility to free
 *	this string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char*
Tps_Dict_Tcl::stats(void)
{
#define NUM_COUNTERS 10
    int count[NUM_COUNTERS], overflow, i, j;
    double average, tmp;
    register Tps_HashEntry *hPtr;
    char *result, *p;

    /*
     * Compute a histogram of bucket usage.
     */

    for (i = 0; i < NUM_COUNTERS; i++) {
	count[i] = 0;
    }
    overflow = 0;
    average = 0.0;
    for (i = 0; i < numBuckets; i++) {
	j = 0;
	for (hPtr = buckets[i].chain; hPtr != NULL; hPtr = hPtr->nextPtr) {
	    j++;
	}
#if CONFIGTEST
	if(j != buckets[i].chainlen)
	    abort();
#endif
	if (j < NUM_COUNTERS) {
	    count[j]++;
	} else {
	    overflow++;
	}
	tmp = j;
	average += (tmp+1.0)*(tmp/numEntries)/2.0;
    }

    /*
     * Print out the histogram and a few other pieces of information.
     */

    result = (char *) ckalloc((unsigned) ((NUM_COUNTERS*60) + 300));
    sprintf(result, "%ld entries in table, %ld buckets\n",
	    numEntries, numBuckets);
    p = result + strlen(result);
    for (i = 0; i < NUM_COUNTERS; i++) {
	sprintf(p, "number of buckets with %d entries: %d\n",
		i, count[i]);
	p += strlen(p);
    }
    sprintf(p, "number of buckets with %d or more entries: %d\n",
	    NUM_COUNTERS, overflow);
    p += strlen(p);
    sprintf(p, "average search distance for entry: %.1f", average);
    return result;
}

/**************************************************/
void
Tps_Dict_Tcl::mark(void)
{
    register long i;

    if(marked()) return;
    Tps_Container::mark();  /* mark self */
    /* walk all entries and have them marked */
    for(i=0;i<numBuckets;i++) {
	register Tps_HashEntry* p;
	for(p=buckets[i].chain;p;p=p->nextPtr) {
	    Tps_mark(p->pair._key);
	    Tps_mark(p->pair._value);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * copy -- clear dict and fill in contents from some other dict.
 *
 *----------------------------------------------------------------------
 */

Tps_Status
Tps_Dict_Tcl::copy(Tps_Dict* src)
{
    register long i;
    register long r = src->range();
    register Tps_Status ok = TPSSTAT_OK;
    Tps_Dictpair* pairp;

    clear();
    for(i=0;i<r;i++) {
	ok = src->ith(i,pairp);
	if(ok == TPSSTAT_UNDEFINED) continue; /* no entry at this index */
	if(ok == TPSSTAT_OK) {
	    ok = insert(*pairp,(Tps_Value*)0);
	}
	if(ok != TPSSTAT_OK) return ok; /* some form of error */
    }
    return TPSSTAT_OK;
}
