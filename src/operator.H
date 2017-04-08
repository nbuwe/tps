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
#ifndef OPERATOR_HH
#define OPERATOR_HH 1

typedef u_long Tps_Operator_Flags;
#define Tps_operator_static 0x1

class Tps_Interp; /* forward */
struct Tps_Value; /* forward */

/* define the calling sequence for a simple operator */
#define TPS_OPERATOR_ARGS Tps_Interp* interp, Tps_Value* args, long nargs

/*
It is important in init.c that the following have no virtual methods,
or constructor or destructor.
*/

struct Tps_Operator {
    public:
	Tps_Nameid name(void) const {return _name;}
	long arity(void) const {return _arity;}
	Tpsstatfcn proc(void) const {return _proc;}
	Tps_Operator_Flags flags(void) const {return _flags;}
    public:
	void arity(long a) {_arity = a;}
	void proc(Tpsstatfcn p) {_proc = p;}
	void flags(Tps_Operator_Flags f) {_flags = f;}
	void name(const char* s) {_name = s;}
    public:
	Tps_Status invoke(TPS_OPERATOR_ARGS)
	    {return (_proc)(interp,args,nargs);}
    public:
	Tps_Nameid		_name;
	Tpsstatfcn		_proc; /* c procedure to call */
	long			_arity;
	Tps_Operator_Flags	_flags;
};

#endif /*OPERATOR_HH*/
