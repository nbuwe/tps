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
	Tps_Nameid name(void) {return _name;}
	long arity(void) {return _arity;}
	Tpsstatfcn proc(void) {return _proc;}
	Tps_Operator_Flags flags(void) {return _flags;}
    public:
	void arity(long a) {_arity = a;}
	void proc(Tpsstatfcn p) {_proc = p;}
	void flags(Tps_Operator_Flags f) {_flags = f;}
	void name(char* s) {_name = s;}
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
