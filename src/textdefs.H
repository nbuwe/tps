struct Tps_Operator_Defines {
	const char*	_name;
	const char*	_body;
}; 

/* safe operators defined in system dict */
extern struct Tps_Operator_Defines textdefs[];

/* unsafe operators defined in system dict */
extern struct Tps_Operator_Defines utextdefs[];

#if OO
/* OO operators defined in system dict */
extern struct Tps_Operator_Defines ootextdefs[];
#endif
