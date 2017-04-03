struct Tps_Operator_Defines {
	char*	_name;
	char*	_body;
}; 

/* safe operators defined in system dict */
extern struct Tps_Operator_Defines textdefs[];

/* unsafe operators defined in system dict */
extern struct Tps_Operator_Defines utextdefs[];

#if OO
/* OO operators defined in system dict */
extern struct Tps_Operator_Defines ootextdefs[];
#endif
