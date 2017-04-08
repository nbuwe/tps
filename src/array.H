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
#ifndef ARRAY_HH
#define ARRAY_HH 1

#include "container.H"

/*
Arrays are represented as counted sequences of values.
Arrays can be extended dynamically.
*/

class Tps_Array : public Tps_Container {
    public:
	Tps_Array(long sz=0);
	virtual ~Tps_Array();
    public:
	int length() const {return _len;}
	Tps_Value* contents() const {return _contents;}
	int alloc() const {return _alloc;}
    public:
	Tps_Status setlen(long);
	Tps_Status setlength(long l) {return setlen(l);}
	Tps_Status append(Tps_Value);
	Tps_Status append(Tps_Value*,long);
	Tps_Status append(Tps_Array*);
    public:
	virtual void mark(); /* override */
    protected:
	Tps_Status extend(long);
    protected:
	long		_len;
	Tps_Value*	_contents;
	long		_alloc; /* space actually allocated */
    public:
};

#endif /*ARRAY_HH*/
