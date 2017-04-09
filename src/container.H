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
#ifndef CONTAINER_HH
#define CONTAINER_HH 1

/*
Parent class for complex objects that need to be
reclaimed when no longer used.
*/

class Tps_List {
    public:
	Tps_List() {_next = 0; _prev = 0;}
	virtual ~Tps_List() {unlink();}
    public:
	Tps_List* next() const {return _next;}
    public:
	void link(Tps_List* lp)
		{
		    _next = lp->_next;
		    lp->_next = this;
		    _prev = lp;
		    if(_next) _next->_prev = this;
		}
	void unlink()
		{
		    if(_next) _next->_prev = _prev;
		    if(_prev) _prev->_next = _next;
		}
    protected:
	Tps_List*	_next;
	Tps_List*	_prev;
};

class Tps_Container : public Tps_List {
    public:
	Tps_Container(Tps_Typeid t);
	virtual ~Tps_Container() {}
    public:
	boolean marked() const {return _marked;}
	Tps_Typeid tid() const {return _tid;}
	long uid() const {return _uid;}
    public:
	virtual void mark() {_marked = TRUE;}
	void unmark() {_marked = FALSE;}
	void setuid(long u) {_uid = u;}
    protected:
	boolean		_marked; /* mark flag for gc */
	Tps_Typeid	_tid;    /* type of this object */
	long		_uid;    /* for state marshalling */
};

#define TPS_GC_OF(p) ((Tps_Container*)(p)._value._any)
EXTERNC void Tps_mark(Tps_Value v);

#endif /*CONTAINER_HH*/
