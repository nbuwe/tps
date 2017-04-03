#ifndef CONTAINER_HH
#define CONTAINER_HH 1

/*
Parent class for complex objects that need to be
reclaimed when no longer used.
*/

class Tps_List {
    public:
	Tps_List(void) {_next = 0; _prev = 0;}
	virtual ~Tps_List(void) {unlink();}
    public:
	Tps_List* next(void) {return _next;}
    public:
	void link(Tps_List* lp)
		{
		    _next = lp->_next;
		    lp->_next = this;
		    _prev = lp;
		    if(_next) _next->_prev = this;
		}
	void unlink(void)
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
	virtual ~Tps_Container(void) {}
    public:
	boolean marked(void) {return _marked;}
	Tps_Typeid tid(void) {return _tid;}
	long uid(void) {return _uid;}
    public:
	virtual void mark(void) {_marked = TRUE;}
	void unmark(void) {_marked = FALSE;}
	void setuid(long u) {_uid = u;}
    protected:
	boolean		_marked; /* mark flag for gc */
	Tps_Typeid	_tid;    /* type of this object */
	long		_uid;    /* for state marshalling */
};

#define TPS_GC_OF(p) ((Tps_Container*)(p)._value._any)
EXTERNC void Tps_mark(Tps_Value v);

#endif /*CONTAINER_HH*/
