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
#ifndef NAME_HH
#define NAME_HH 1

/*
The name table is kept independent of any specific interpreter
*/

struct Tps_Nametable : public Tps_Dict_Tcl {
    public:
	Tps_Nametable(long);
	virtual ~Tps_Nametable(void);
    public:
	Tps_Nameid newname(const char* nm, boolean ro=FALSE);
    public:
	Tps_Nametable* duplicate(); // duplicate this nametable
    public: // virtual overrides
	Tps_Status lookup(Tps_Value key, Tps_Dictpair** pairp);
	Tps_Status insert(Tps_Dictpair& pairp, Tps_Value* oldvalue, boolean suppress=FALSE);
	void mark(void) /* mark self only */
	    {if(marked()) return; Tps_Container::mark();}
    protected:
	/* key MUST BE TPSTYPE_NAME!*/
	virtual u_long hashfcn(Tps_Value key)
		{return namehash(TPS_NAME_OF(key));}
	/* key MUST BE TPSTYPE_NAME!*/
	virtual boolean match(Tps_Value key, Tps_Value e)
	    {return strcmp(TPS_NAME_OF(key),TPS_NAME_OF(e))==0;}
    protected:
	virtual u_long namehash(const char*);
};

#endif /*NAME_HH*/
