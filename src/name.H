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
	Tps_Nameid newname(char* nm, boolean ro=FALSE);
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
	virtual u_long namehash(char*);
};

#endif /*NAME_HH*/
