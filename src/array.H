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
	int length() {return _len;}
	Tps_Value* contents() {return _contents;}
	int alloc() {return _alloc;}
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
