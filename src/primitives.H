#ifndef TPS_PRIMITIVES_HH
#define TPS_PRIMITIVES_HH


/*
All primitives are called with the following signature:
	Tps_Interp*	this	- the interpreter instance
	Tps_Value*	args	- ptr to the base of the top n
				  arguments on the stack, where n is the
				  arity of the operator.
				  In practice, since the arg stack
				  grows down, this is a ptr to the
				  "top" (lowest address) element of the stack.
	int		nargs	- depth of the operand stack.
*/

/* Macro to call a primitive operator directly */
#define TPS_CALL_PRIM(intrp,code) \
	((code))((intrp),TPS_TOSP(intrp),TPS_DEPTH(intrp))

/* allow storing of result over the last arg passed */
/* do not rely on args being set correctly */
#define TPS_RETURNVAL(intrp,value,arity) \
	(TPS_POPN(intrp,(arity)-1),*(TPS_TOSP(intrp)) = (value))

/**************************************************/

/* Define the frame handlers and creators */
EXTERNC Tps_Handler Tps_handler_catch;
EXTERNC Tps_Handler Tps_handler_stopped;
EXTERNC Tps_Handler Tps_handler_runstream;
EXTERNC Tps_Handler Tps_handler_loop;
EXTERNC Tps_Handler Tps_handler_while;
EXTERNC Tps_Handler Tps_handler_repeat;
EXTERNC Tps_Handler Tps_handler_forall;
EXTERNC Tps_Handler Tps_handler_for;

/**************************************************/
EXTERNC Tps_Status Tps_op_abs(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_abs(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_add(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_aload(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_anchorsearch(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_and(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_append(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_array(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_astore(Tps_Interp*, Tps_Value*, long);
#if HASFLOAT
EXTERNC Tps_Status Tps_op_atan(Tps_Interp*, Tps_Value*, long);
#endif
EXTERNC Tps_Status Tps_op_begin(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_bind(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_bitshift(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_bytesavailable(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_catch(Tps_Interp*, Tps_Value*, long);
#if HASFLOAT
EXTERNC Tps_Status Tps_op_ceiling(Tps_Interp*, Tps_Value*, long);
#endif
EXTERNC Tps_Status Tps_op_clear(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_cleartomark(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_closestream(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_copy(Tps_Interp*, Tps_Value*, long);
#if HASFLOAT
EXTERNC Tps_Status Tps_op_cos(Tps_Interp*, Tps_Value*, long);
#endif
EXTERNC Tps_Status Tps_op_count(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_countdictstack(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_countexecstack(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_counttomark(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_currentdict(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_currentstream(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_cvi(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_cvlit(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_cvn(Tps_Interp*, Tps_Value*, long);
#if HASFLOAT
EXTERNC Tps_Status Tps_op_cvr(Tps_Interp*, Tps_Value*, long);
#endif
EXTERNC Tps_Status Tps_op_cvrs(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_cvs(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_cvts(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_cvx(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_def(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_deletefile(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_dict(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_dictstack(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_div(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_dload(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_dup(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_dupn(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_end(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_eq(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_eqeqsign(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_exch(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_exec(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_execstack(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_executeonly(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_exit(Tps_Interp*, Tps_Value*, long);
#if HASFLOAT
EXTERNC Tps_Status Tps_op_exp(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_floor(Tps_Interp*, Tps_Value*, long);
#endif
EXTERNC Tps_Status Tps_op_flushstream(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_for(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_forall(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_ge(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_get(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_getinterval(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_gt(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_idiv(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_if(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_ifelse(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_index(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_interrupt(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_known(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_le(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_length(Tps_Interp*, Tps_Value*, long);
#if HASFLOAT
EXTERNC Tps_Status Tps_op_ln(Tps_Interp*, Tps_Value*, long);
#endif
EXTERNC Tps_Status Tps_op_load(Tps_Interp*, Tps_Value*, long);
#if HASFLOAT
EXTERNC Tps_Status Tps_op_log(Tps_Interp*, Tps_Value*, long);
#endif
EXTERNC Tps_Status Tps_op_loop(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_lt(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_maxlength(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_mod(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_mul(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_ne(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_neg(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_noaccess(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_noop(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_not(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_operator(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_or(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_pop(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_put(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_putinterval(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_quit(Tps_Interp*, Tps_Value*, long);
#if HASRAND
EXTERNC Tps_Status Tps_op_rand(Tps_Interp*, Tps_Value*, long);
#endif
EXTERNC Tps_Status Tps_op_rbrace(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_rbracket(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_rcheck(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_read(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_readline(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_readonly(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_readstring(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_realtime(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_remove(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_repeat(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_resetstream(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_roll(Tps_Interp*, Tps_Value*, long);
#if HASFLOAT
EXTERNC Tps_Status Tps_op_round(Tps_Interp*, Tps_Value*, long);
#endif
#if HASRAND
EXTERNC Tps_Status Tps_op_rrand(Tps_Interp*, Tps_Value*, long);
#endif
EXTERNC Tps_Status Tps_op_rrangle(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_runstream(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_search(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_setarity(Tps_Interp*, Tps_Value*, long);
#if HASFLOAT
EXTERNC Tps_Status Tps_op_sin(Tps_Interp*, Tps_Value*, long);
#endif
EXTERNC Tps_Status Tps_op_sleep(Tps_Interp*, Tps_Value*, long);
#if HASFLOAT
EXTERNC Tps_Status Tps_op_sqrt(Tps_Interp*, Tps_Value*, long);
#endif
#if HASRAND
EXTERNC Tps_Status Tps_op_srand(Tps_Interp*, Tps_Value*, long);
#endif
EXTERNC Tps_Status Tps_op_status(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_stop(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_stopped(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_store(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_stream(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_streamstring(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_string(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_sub(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_throw(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_token(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_truncate(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_type(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_undef(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_userdict(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_usertime(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_wcheck(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_where(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_while(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_write(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_writestring(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_xcheck(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_xor(Tps_Interp*, Tps_Value*, long);

/**************************************************/
/* ERROR OPERATORS */
/**************************************************/

EXTERNC Tps_Status Tps_op_errortrap(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_handleerror(Tps_Interp*, Tps_Value*, long);

/**************************************************/
/* TRACE OPERATORS */
/**************************************************/

EXTERNC Tps_Status Tps_op_cvuntrace(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_tracecheck(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_traceexec(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_tracereturn(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_tracetrap(Tps_Interp*, Tps_Value*, long);

/**************************************************/
/* GARBAGE COLLECTION OPERATORS */
/**************************************************/

EXTERNC Tps_Status Tps_op_gc(Tps_Interp*, Tps_Value*, long);

/**************************************************/
/* STATE SAVE/RESTORE OPERATORS */
/**************************************************/

EXTERNC Tps_Status Tps_op_stateexec(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_statesave(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_staterestore(Tps_Interp*, Tps_Value*, long);

/**************************************************/
/* SAFETY OPERATORS */
/**************************************************/

EXTERNC Tps_Status Tps_op_cvunsafe(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_safecheck(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_safeexec(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_safestate(Tps_Interp*, Tps_Value*, long);

/**************************************************/
/* PROTOTYPE OPERATORS */
/**************************************************/

#if OO
EXTERNC Tps_Status Tps_op_cvmethod(Tps_Interp*, Tps_Value*, long);
EXTERNC Tps_Status Tps_op_clonedeep(Tps_Interp*, Tps_Value*, long);
#endif

/**************************************************/

#endif /*TPS_PRIMITIVES_HH*/
