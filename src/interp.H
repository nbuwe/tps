#ifndef INTERP_HH
#define INTERP_HH 1

/* There is some sort of conflict between the gnu types.h file
   and the irix time.h file
*/
#ifdef irix
#define _TIME_T
typedef long ignoreignore;
#endif

#include <sys/time.h>

/**************************************************/
class Tps_Interp; /* forward */
struct Tps_Frame; /* forward */
/**************************************************/
/* Define the factory procedure for this class */

EXTERNC Tps_Interp* Tps_interp_create(void);

/**************************************************/
/* Define some auxilliary types */

/* The streams that are used by the interpreter */
typedef enum Tps_Stdio
	{Tps_stdcons, Tps_stdin, Tps_stdout, Tps_stderr} Tps_Stdio;

/* Each stack within stackspace is controlled
   by (1) pointers to the boundaries of the stack
   and (2) a top of stack ptr.
*/
struct Tps_Stack {
    long	_alloc;
    Tps_Value*	_stack;
    Tps_Value*	_last;
    Tps_Value*	_tos;
};

struct Tps_Estack {
    long	_alloc;
    char*	_stack;
    char*	_last;
    char*	_tos;
};

/**************************************************/

class Tps_Interp {
    public:
	Tps_Interp();
	virtual ~Tps_Interp();
    public:
	virtual void reset();
	virtual void gc(); // in gc.C
	virtual Tps_Status load(const char* code="start");
	virtual Tps_Value* stack(long& depth)
		{depth = _stack._last - _stack._tos; return _stack._tos;}
	virtual Tps_Status run();
	virtual Tps_Status save(char*& state);
	virtual Tps_Status restore(char*);
	virtual Tps_Status restore(int fd);
    public:
	Tps_Status status() { return _status;}
	void status(Tps_Status s) {_status = s;}
	int step() { return _step;}
	void step(boolean b) {_step = (b)?1:0;}
	int tracing() { return _tracing;}
	void tracing(boolean b) {_tracing = (b)?1:0;}
	int safe() { return _safe;}
	void safe(boolean b) {_safe=(b)?1:0;}
    public:
	Tps_Status newoperator(const char*, long, Tpsstatfcn);
	Tps_Status destroyoperator(char*);
    public:
	Tps_Status where(Tps_Value key, Tps_Value* valp=0, Tps_Value* where=0);
	Tps_Status def(Tps_Value key, Tps_Value val, Tps_Value* where=0);
    public:
	char* safeprefix() { return _safeprefix;}
	void safeprefix(char*);
	char* tpsrc() { return _tpsrc;}
	void tpsrc(char*);
    public:
	Tps_Status unwind_thru(struct Tps_Frame* fr, boolean thrown=FALSE);
	Tps_Status unwind_throw(void){ return unwind_thru((struct Tps_Frame*)0,TRUE);}
    public:
	Tps_Stream* stdstream(Tps_Stdio);
    public: /* leave interpreter state open, at least for now */

	/* start times: user and real for this interpreter */
	struct timeval _realtime;
	struct timeval _usertime;

	/* Track the _user dicts; indexed by _safe flag */
	Tps_Dict* _userdicts[2];
	Tps_Value __userdicts[2];

	Tps_Stream_String _inbuf; /* temporary space to hold input strings
				     for parsing by tokenizer. */
	Tps_Stream_String _tokenbuf; /*temporary space for collecting tokens*/
	Tps_Stream_String _tempbuf; /* temporary space for general;
					never close it */

	/* misc interpreter state */
	Tps_Status	_status;	/* Used to record the status
					   right after initialization.
					*/
	boolean		_step;		/* 1 => single step interpretation */
	boolean		_tracing;	/* 0 => dont trace, 1 => trace all */
	boolean		_traceskip;	/* 1 => dont trace for one step */

	Tps_Value	_object; /* being executed */
	Tps_Value	_throwflag;

	boolean		_safe;	/* 0 => executing in unsafe mode,
				   1 => safe mode */

	char*		_safeprefix; /* for file names */
	char*		_tpsrc; /* initialization file */

	long		_framecount; /* # of frames in estack */
	long		_uidcounter; /* for marshalling */

	Tps_Value	_self;		/* current self dict */
    public: /* Stack management */
	/* The whole set of stacks occupies a single large chunk
	   of allocated space.
	*/
	char*		_allstacks;
	long		_allstacks_alloc; /* length of _stackspace */
	Tps_Stack	_stack;
	Tps_Estack	_estack;	
	Tps_Stack	_dstacks[2]; /* indexed by _safe */
	Tps_Stack*	_dstack; /* points to _dstacks[_safe] */

    public: /* stack manipulators */
	/* operand stack */
	inline u_long depth(void)
	    {return _stack._last - _stack._tos;}
	inline u_long room(void) /* space left on the stack */
	    {return _stack._tos - _stack._stack;}
	/* caution:the following do not check for stack space */
	inline Tps_Value push(Tps_Value value)
	    {return *(--_stack._tos) = (value);}
	inline Tps_Value* pushn(long n) /* move nos to hold n more elements */
	    {(void)(_stack._tos -= (n)); return _stack._tos;}
	inline Tps_Value pop(void) /* remove & return top element */
	    {return (*_stack._tos++);}
	inline Tps_Value* popn(long n) /* remove n elements from stack */
	    {(void)(_stack._tos += (n)); return _stack._tos;}
	inline Tps_Value* tosp(void) /* get address of the top stack element */
	    {return _stack._tos;}
	inline Tps_Value top(void) /* return top element without popping it */
	    { return *(_stack._tos);}
	inline void settosp(Tps_Value* argptr) /* force stack to fixed point */
	    {_stack._tos = (argptr);}
	inline Tps_Value* base(void) /* base of the stack (= tosp + depth) */
	    {return (_stack._last);}
	inline void clear(void) /* clear the stack */
	    {_stack._tos = _stack._last;}

	/* dict stack */
	inline u_long ddepth(void)
	    {return _dstack->_last - _dstack->_tos;}
	inline u_long droom(void)
	    {return _dstack->_tos - _dstack->_stack;}
	/* caution:the following do not check for stack space */
	inline Tps_Value dpush(Tps_Value value)
	    {return *(--_dstack->_tos) = (value);}
	inline Tps_Value* dpushn(long n)
	    {(void)(_dstack->_tos -= (n)); return _dstack->_tos;}
	inline Tps_Value dpop(void)
	    {return (*_dstack->_tos++);}
	inline Tps_Value* dpopn(long n)
	    {(void)(_dstack->_tos += (n)); return _dstack->_tos;}
	inline Tps_Value* dtosp(void)
	    {return _dstack->_tos;}
	inline Tps_Value dtop(void)
	    { return *(_dstack->_tos);}
	inline void dclear(void)
	    {_dstack->_tos = _dstack->_last;}

	/* exec stack */
	inline u_long edepth(void)
	    {return _estack._last - _estack._tos;}
	inline u_long eroom(void)
	    {return _estack._tos - _estack._stack;}
	/* caution:the following do not check for stack space */
	inline char* epushn(long n)
	    {(void)(_estack._tos -= (n)); return _estack._tos;}
	inline char* epopn(long n)
	    {(void)(_estack._tos += (n)); return _estack._tos;}
	inline char* etosp(void)
	    {return _estack._tos;}
	inline void eclear(void)
	    {_estack._tos = _estack._last;}
	inline char* ebase(void)
	    {return (_estack._last);}
	inline long eoffset(char* f)
	    {return _estack._last - f;}
	inline char* eoffbase(long n)
	    {return _estack._last - n;}
};

#endif /*INTERP_HH*/
