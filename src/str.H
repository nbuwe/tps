#ifndef STRING_HH
#define STRING_HH 1

#include "container.H"

/*
Strings are represented as counted sequences of characters.
It is not necessarily the case that strings are null
terminated, so they can be used as byte strings with interior
null characters.
Strings may be "base", which means that they have associated
storage.
Non-base, or psuedo, strings point to other pieces of
storage, such as other strings, or to contiguous pieces
of the arg stack.
Base strings can be extended dynamically, pseudo strings cannot.
*/

class Tps_String : public Tps_Container {
    public:
	Tps_String(long sz=0);
	Tps_String(const char* initstr, long slen);
	Tps_String(const char* initstr);
	virtual ~Tps_String();
    public:
	long length() {return _len;}
	long alloc() {return _alloc;}
	char* contents() {return _contents;}
    public:
	char* extract();
    public:
	Tps_Status setlen(long);
	Tps_Status setlength(long l) {return setlen(l);}
	Tps_Status append(char);
	Tps_Status append(char*,long);
	Tps_Status append(Tps_String*);
	Tps_Status append(char* s) {return append(s,strlen(s));}
    public:
	Tps_Status nullterminate();
	boolean isnullterminated();
    protected:
	Tps_Status extend(long);
	boolean initialize(long);
    protected:
	long	_len;
	char*	_contents;
	long	_alloc; /* space actually allocated */
};

#endif /*STRING_HH*/
