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
#ifndef DICT_HH
#define DICT_HH 1

#include "container.H"

/*
Dictionaries are basically just hash tables
over Tps_Value pairs.
*/

/**************************************************/

enum Tps_Dictkind {Tps_nodict, Tps_tcldict};

/**************************************************/

struct Tps_Dictpair {
	Tps_Value _key;
	Tps_Value _value;
};

/**************************************************/
/* All dicts must conform to this interface */

class Tps_Dict : public Tps_Container {
    public:
	Tps_Dict(Tps_Dictkind k, const char* nm=0) : Tps_Container(TPSTYPE_DICT)
		{_kind=k; _name = nm;}
    public:
	virtual Tps_Status lookup(Tps_Value key, Tps_Dictpair** pairp) = 0;
	virtual Tps_Status insert(Tps_Dictpair& pairp, Tps_Value* oldvalue, boolean suppress=FALSE) = 0;
	virtual Tps_Status remove(Tps_Value key, Tps_Dictpair* pairp) = 0;
	virtual Tps_Status ith(long i, Tps_Dictpair*& pairp) = 0;
	virtual Tps_Status copy(Tps_Dict* src) = 0;
	virtual void clear() = 0;
    public:
	virtual long length() const = 0; // # defined pairs in table
	virtual long maxlength() const = 0; // current capacity
	virtual long range() const = 0; // range (0..range()) for ith() method
    public:
	virtual void mark();
	Tps_Dictkind kind() const {return _kind;}
    public:
	const char* name() const { return _name?_name:"<dict>"; }	
	void name(const char* nm) {_name = nm;}
    protected:
	virtual u_long hashfcn(Tps_Value key)
	    {return (TPS_ISTYPE(key,TPSTYPE_STRING))
			? stringhash(TPS_STRING_OF(key))
			: wordhash(key);}
	/* This function is inlined elsewhere; look for ifdef NOINLINE */
	virtual boolean match(Tps_Value key, Tps_Value e)
	    {return TPS_TYPE(key) == TPS_TYPE(e)
		    && ((TPS_ISTYPE(key,TPSTYPE_STRING)
			  && stringmatch(TPS_STRING_OF(key),TPS_STRING_OF(e)))
			|| TPS_ANY_OF(key) == TPS_ANY_OF(e));}
    protected:
	u_long randomize(u_long n) {return (n)*1103515245;}
	u_long wordhash(Tps_Value key)
		{return randomize(TPS_ANY_OF(key)>>3);}
	u_long stringhash(Tps_String* key);
	boolean stringmatch(Tps_String*, Tps_String*);
    protected:
	Tps_Dictkind	_kind;
	const char*	_name; /* optional for tracing */
};


/*
Notes:
1. `remove' will only remove one match; pairp is so
   caller can free the key and value if needed.
   returns TPSSTAT_UNDEFINED if no match found
*/


/**************************************************/
/*
Definitions of particular kinds of dictionaries
*/
/**************************************************/

/**************************************************/
/* The Hash Tables from tcl */
/**************************************************/
#define TPS_SMALL_HASH_TABLE 4

struct Tps_HashEntry;

struct Tps_Bucket {
	struct Tps_HashEntry* chain;
	long chainlen;
};

class Tps_Dict_Tcl : public Tps_Dict {
    public:
	Tps_Dict_Tcl(long bucketcount=0, const char* nm=0);
	virtual ~Tps_Dict_Tcl();
    public:
	virtual Tps_Status lookup(Tps_Value key, Tps_Dictpair** pairp);
	virtual Tps_Status insert(Tps_Dictpair& pair, Tps_Value* oldvalue, boolean suppress=FALSE);
	virtual Tps_Status remove(Tps_Value key, Tps_Dictpair* pairp);
	virtual Tps_Status ith(long i, Tps_Dictpair*& pairp);
	virtual Tps_Status copy(Tps_Dict*);
	virtual void clear();
    public:
	virtual long length() const {return numEntries;}
	virtual long maxlength() const {return maxlen*numBuckets;}
	virtual long range() const {return maxlength();}
	long tablelength() const {return numBuckets;}
	const Tps_Bucket* table() const {return buckets;}
    public:
	char* stats();
	virtual void mark();
    protected:
	void RebuildTable();
    protected:
	Tps_Bucket* buckets;  /* Pointer to bucket array.  Each
				     element points to first entry in
				     bucket's hash chain, or NULL. */
	Tps_Bucket staticBuckets[TPS_SMALL_HASH_TABLE];
				 /* Bucket array used for small tables
				     (to avoid mallocs and frees). */
	long numBuckets;   /*Total number of buckets allocated
			    at **bucketPtr. */
	long numEntries;   /*Total number of entries present
			    in table. */
	long downShift;   /* Shift count used in hashing
			    function.  Designed to use high-
			    order bits of randomized keys. */
	long mask;    /* Mask value used in hashing
			function. */
	long rebuildSize;
	// track max chain length
	long maxlen;
};
    
struct Tps_HashEntry {
    struct Tps_HashEntry *nextPtr; /* Pointer to next entry in this
				      hash bucket, or NULL for end of
				      chain. */
    Tps_Bucket* bucketPtr; /* Pointer to bucket that points to
					 first entry in this entry's chain:
					 used for deleting the entry. */
    u_long hashvalue;
    Tps_Dictpair pair;
};

#endif /*DICT_HH*/
