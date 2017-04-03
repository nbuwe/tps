/*
Copyright (c) 1993 Dennis Heimbigner
All rights reserved.

This software was developed by Dennis Heimbigner
as part of the Arcadia project at
the University of Colorado, Boulder.

Redistribution and use in source and binary forms are permitted
provided that the above copyright notice and this paragraph are
duplicated in all such forms and that any documentation,
and other materials related to such
distribution and use acknowledge that the software was developed
by Dennis Heimbigner.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <stdio.h>

#if defined hpux || solaris2
#include <unistd.h>
#endif

#include "tps.H"
#include "exec.H"
#include "primitives.H"
#include "util.H"
#include "chartab.H"

/**************************************************/
/* Define the "constant" global values */

/* Define vectors of useful names and useful other constants */
/* possibly statically filled */

Tps_Value* Tps__constants;

/**************************************************/
/* Define the "variable" global values: access needs to be
   thread protected.
*/

Tps_Global tpsg;

/**************************************************/
/* Following does not contain unsafe operators */
static
Tps_Operator safeprimitives[] = {

/* arithmetics */
{"abs",Tps_op_abs,1,Tps_operator_static},
{"add",Tps_op_add,2,Tps_operator_static},
#if HASFLOAT
{"atan",Tps_op_atan,1,Tps_operator_static},
#endif
{"bitshift",Tps_op_bitshift,2,Tps_operator_static},
#if HASFLOAT
{"ceiling",Tps_op_ceiling,1,Tps_operator_static},
{"cos",Tps_op_cos,1,Tps_operator_static},
#endif
{"div",Tps_op_div,2,Tps_operator_static},
#if HASFLOAT
{"exp",Tps_op_exp,1,Tps_operator_static},
{"floor",Tps_op_floor,1,Tps_operator_static},
#endif
{"idiv",Tps_op_idiv,2,Tps_operator_static},
#if HASFLOAT
{"ln",Tps_op_ln,1,Tps_operator_static},
{"log",Tps_op_log,1,Tps_operator_static},
#endif
{"mod",Tps_op_mod,2,Tps_operator_static},
{"mul",Tps_op_mul,2,Tps_operator_static},
{"neg",Tps_op_neg,1,Tps_operator_static},
#if HASRAND
{"rand",Tps_op_rand,0,Tps_operator_static},
#endif
#if HASFLOAT
{"round",Tps_op_round,1,Tps_operator_static},
#endif
#if HASRAND
{"rrand",Tps_op_rrand,0,Tps_operator_static},
#endif
#if HASFLOAT
{"sin",Tps_op_sin,1,Tps_operator_static},
{"sqrt",Tps_op_sqrt,1,Tps_operator_static},
#endif
#if HASRAND
{"srand",Tps_op_srand,1,Tps_operator_static},
#endif
{"sub",Tps_op_sub,2,Tps_operator_static},
#if HASFLOAT
{"truncate",Tps_op_truncate,1,Tps_operator_static},
#endif

/* boolean operators */
{"and",Tps_op_and,2,Tps_operator_static},
{"not",Tps_op_not,1,Tps_operator_static},
{"or",Tps_op_or,2,Tps_operator_static},
{"xor",Tps_op_xor,2,Tps_operator_static},

/* comparisons */
{"eq",Tps_op_eq,2,Tps_operator_static},
{"ge",Tps_op_ge,2,Tps_operator_static},
{"gt",Tps_op_gt,2,Tps_operator_static},
{"le",Tps_op_le,2,Tps_operator_static},
{"lt",Tps_op_lt,2,Tps_operator_static},
{"ne",Tps_op_ne,2,Tps_operator_static},

/* stack manipulation */
{"clear",Tps_op_clear,0,Tps_operator_static},
{"cleartomark",Tps_op_cleartomark,0,Tps_operator_static},
{"count",Tps_op_count,0,Tps_operator_static},
{"counttomark",Tps_op_counttomark,0,Tps_operator_static},
{"dup",Tps_op_dup,1,Tps_operator_static},
{"exch",Tps_op_exch,2,Tps_operator_static},
{"index",Tps_op_index,1,Tps_operator_static},
{"pop",Tps_op_pop,1,Tps_operator_static},
{"roll",Tps_op_roll,2,Tps_operator_static},

/* strings */
{"anchorsearch",Tps_op_anchorsearch,2,Tps_operator_static},
{"search",Tps_op_search,2,Tps_operator_static},
{"string",Tps_op_string,1,Tps_operator_static},
{"token",Tps_op_token,1,Tps_operator_static},

/* dicts and dictstack */
{"begin",Tps_op_begin,1,Tps_operator_static},
{"countdictstack",Tps_op_countdictstack,0,Tps_operator_static},
{"currentdict",Tps_op_currentdict,0,Tps_operator_static},
{"def",Tps_op_def,2,Tps_operator_static},
{"dict",Tps_op_dict,1,Tps_operator_static},
{"dictstack",Tps_op_dictstack,1,Tps_operator_static},
{"dload",Tps_op_dload,1,Tps_operator_static},
{"end",Tps_op_end,0,Tps_operator_static},
{"known",Tps_op_known,2,Tps_operator_static},
{"load",Tps_op_load,1,Tps_operator_static},
{"maxlength",Tps_op_maxlength,1,Tps_operator_static},
{"remove",Tps_op_remove,3,Tps_operator_static},
{"store",Tps_op_store,2,Tps_operator_static},
{"undef",Tps_op_undef,1,Tps_operator_static},
{"where",Tps_op_where,1,Tps_operator_static},
{RRANGLESTR,Tps_op_rrangle,1,Tps_operator_static},

/* arrays */
{"aload",Tps_op_aload,1,Tps_operator_static},
{"array",Tps_op_array,1,Tps_operator_static},
{"astore",Tps_op_astore,1,Tps_operator_static},
{RBRACKETSTR,Tps_op_rbracket,1,Tps_operator_static},
{RBRACESTR,Tps_op_rbrace,1,Tps_operator_static},

/* operators */
{"operator",Tps_op_operator,3,Tps_operator_static},
{"setarity",Tps_op_setarity,2,Tps_operator_static},

/* general compound value manipulations */
{"append",Tps_op_append,2,Tps_operator_static},
{"copy",Tps_op_copy,1,Tps_operator_static},
{"get",Tps_op_get,2,Tps_operator_static},
{"getinterval",Tps_op_getinterval,3,Tps_operator_static},
{"length",Tps_op_length,1,Tps_operator_static},
{"put",Tps_op_put,3,Tps_operator_static},
{"putinterval",Tps_op_putinterval,3,Tps_operator_static},

/* io */
{"bytesavailable",Tps_op_bytesavailable,1,Tps_operator_static},
{"closestream",Tps_op_closestream,1,Tps_operator_static},
{"currentstream",Tps_op_currentstream,1,Tps_operator_static},
{"deletefile",Tps_op_deletefile,1,Tps_operator_static},
{"flushstream",Tps_op_flushstream,1,Tps_operator_static},
{"read",Tps_op_read,1,Tps_operator_static},
{"readline",Tps_op_readline,2,Tps_operator_static},
{"readstring",Tps_op_readstring,2,Tps_operator_static},
{"resetstream",Tps_op_resetstream,1,Tps_operator_static},
{"status",Tps_op_status,1,Tps_operator_static},
{"streamstring",Tps_op_streamstring,1,Tps_operator_static},
{"write",Tps_op_write,2,Tps_operator_static},
{"writestring",Tps_op_writestring,2,Tps_operator_static},

/* control */
{"catch",Tps_op_catch,1,Tps_operator_static},
{"exit",Tps_op_exit,0,Tps_operator_static},
{"if",Tps_op_if,2,Tps_operator_static},
{"ifelse",Tps_op_ifelse,3,Tps_operator_static},
{"interrupt",Tps_op_interrupt,0,Tps_operator_static},
{"quit",Tps_op_quit,0,Tps_operator_static},
{"stop",Tps_op_stop,0,Tps_operator_static},
{"stopped",Tps_op_stopped,1,Tps_operator_static},
{"throw",Tps_op_throw,1,Tps_operator_static},

/* control loops using local variables */
{"for",Tps_op_for,4,Tps_operator_static},
{"forall",Tps_op_forall,2,Tps_operator_static},
{"loop",Tps_op_loop,1,Tps_operator_static},
{"repeat",Tps_op_repeat,2,Tps_operator_static},
{"while",Tps_op_while,2,Tps_operator_static},

/* exec stack manipulation */
{"exec",Tps_op_exec,1,Tps_operator_static},
{"execstack",Tps_op_execstack,0,Tps_operator_static},
{"countexecstack",Tps_op_countexecstack,0,Tps_operator_static},

/* conversions */
{"cvi",Tps_op_cvi,1,Tps_operator_static},
{"cvlit",Tps_op_cvlit,1,Tps_operator_static},
{"cvn",Tps_op_cvn,1,Tps_operator_static},
#if HASFLOAT
{"cvr",Tps_op_cvr,1,Tps_operator_static},
#endif
{"cvrs",Tps_op_cvrs,3,Tps_operator_static},
{"cvs",Tps_op_cvs,2,Tps_operator_static},
{"cvts",Tps_op_cvts,2,Tps_operator_static},
{"cvx",Tps_op_cvx,1,Tps_operator_static},

/* error operators */
{"errortrap",Tps_op_errortrap,2,Tps_operator_static},
{"handleerror",Tps_op_handleerror,0,Tps_operator_static},

/* trace operators */
{"traceexec",Tps_op_traceexec,1,Tps_operator_static},
{"tracereturn",Tps_op_tracereturn,1,Tps_operator_static},
{"tracetrap",Tps_op_tracetrap,0,Tps_operator_static},

/* value structure operators */
{"tracecheck",Tps_op_tracecheck,1,Tps_operator_static},
{"cvuntrace",Tps_op_cvuntrace,1,Tps_operator_static},

/* Access rights */
{"rcheck",Tps_op_xcheck,1,Tps_operator_static},  /* readable check */
{"wcheck",Tps_op_xcheck,1,Tps_operator_static},  /* writable check */
{"xcheck",Tps_op_xcheck,1,Tps_operator_static},  /* executable check */
{"executeonly",Tps_op_executeonly,1,Tps_operator_static},
{"noaccess",Tps_op_noaccess,1,Tps_operator_static},
{"readonly",Tps_op_readonly,1,Tps_operator_static},

/* misc. */
{"==",Tps_op_eqeqsign,1,Tps_operator_static},
{"bind",Tps_op_bind,1,Tps_operator_static},
{"noop",Tps_op_noop,0,Tps_operator_static},
{"realtime",Tps_op_realtime,0,Tps_operator_static},
{"runstream",Tps_op_runstream,1,Tps_operator_static},
{"sleep",Tps_op_sleep,1,Tps_operator_static},
{"type",Tps_op_type,1,Tps_operator_static},
{"usertime",Tps_op_usertime,0,Tps_operator_static},

/* special dict access */
{"userdict",Tps_op_userdict,0,Tps_operator_static},

/* garbage collection */
{"gc",Tps_op_gc,0,Tps_operator_static},

/* state save/restore */
{"stateexec",Tps_op_stateexec,1,Tps_operator_static},
{"statesave",Tps_op_statesave,0,Tps_operator_static},
{"staterestore",Tps_op_staterestore,2,Tps_operator_static},

/* safety */
{"cvunsafe",Tps_op_cvunsafe,1,Tps_operator_static},
{"safecheck",Tps_op_safecheck,1,Tps_operator_static},
{"safeexec",Tps_op_safeexec,0,Tps_operator_static},
{"safestate",Tps_op_safestate,0,Tps_operator_static},

#if OO
/* Prototype Dicts */
{"cvmethod",Tps_op_cvmethod,0,Tps_operator_static},
{"clonedeep",Tps_op_clonedeep,0,Tps_operator_static},
#endif

{(char*)0, (Tpsstatfcn)0, 0L, (Tps_Operator_Flags)0} /* terminator */
};

/* System dict operators that should be marked unsafe */
static
Tps_Operator unsafeprimitives[] = {

{"stream",Tps_op_stream,3,Tps_operator_static},

{(char*)0, (Tpsstatfcn)0, 0L, (Tps_Operator_Flags)0} /* terminator */
};

/**************************************************/

static
Tps_Status
buildoneoperator(Tps_Dict* d, Tps_Operator* pco, boolean unsafe) 
{
    register Tps_Nameid nm;
    register Tps_Status ok;
    Tps_Dictpair pair;
    char opname[256];

    /* first, insert under operator name directly */
    nm = tpsg._nametable->newname((char*)pco->_name);
    TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
    TPS_MAKEVALUE(pair._value,TPSTYPE_OPERATOR,pco);
    /* make all primitives untraceable and executable */
    TPS_SET_TRACEOFF(pair._value,1);
    TPS_SET_EXECUTABLE(pair._value,1);
    if(unsafe)
	TPS_SET_UNSAFE(pair._value,1);
    ok = d->insert(pair,(Tps_Value*)NULL);
    if(ok != TPSSTAT_OK) return ok;
    /* now insert under alternative operator name */
    strcpy(opname,Tps_operator_prefix);
    strcat(opname,pco->_name);
    nm = tpsg._nametable->newname(opname);
    TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
    ok = d->insert(pair,(Tps_Value*)NULL);
    return ok;
}

static
void
buildoperators()  // insert into the systemdict
{
    Tps_Operator* pco;
    register Tps_Status ok;

    for(pco=safeprimitives;pco->_name;pco++) {
	ok = buildoneoperator(tpsg._systemdict,pco,FALSE);
	if(ok != TPSSTAT_OK) {
	    TPS_STDCONS->printf("Tps_initialize: error creating safe operator %s\n",
				pco->_name);
	}
    }
    for(pco=unsafeprimitives;pco->_name;pco++) {
	ok = buildoneoperator(tpsg._systemdict,pco,TRUE);
	if(ok != TPSSTAT_OK) {
	    TPS_STDCONS->printf("Tps_initialize: error creating unsafe operator %s\n",
				pco->_name);
	}
    }
}

/**************************************************/

static
Tps_Value_Simple Tps__constants_static[TPS_ALLCONSTANTS_COUNT] = {
{{0,TPSTYPE_NAME},(void*)"ok"},
{{0,TPSTYPE_NAME},(void*)"fail"},
{{0,TPSTYPE_NAME},(void*)"dictfull"},
{{0,TPSTYPE_NAME},(void*)"dictstackoverflow"},
{{0,TPSTYPE_NAME},(void*)"dictstackunderflow"},
{{0,TPSTYPE_NAME},(void*)"execstackoverflow"},
{{0,TPSTYPE_NAME},(void*)"execstackunderflow"},
{{0,TPSTYPE_NAME},(void*)"interrupt"},
{{0,TPSTYPE_NAME},(void*)"invalidaccess"},
{{0,TPSTYPE_NAME},(void*)"invalidexit"},
{{0,TPSTYPE_NAME},(void*)"invalidstreamaccess"},
{{0,TPSTYPE_NAME},(void*)"ioerror"},
{{0,TPSTYPE_NAME},(void*)"limitcheck"},
{{0,TPSTYPE_NAME},(void*)"rangecheck"},
{{0,TPSTYPE_NAME},(void*)"stackoverflow"},
{{0,TPSTYPE_NAME},(void*)"stackunderflow"},
{{0,TPSTYPE_NAME},(void*)"syntaxerror"},
{{0,TPSTYPE_NAME},(void*)"timeout"},
{{0,TPSTYPE_NAME},(void*)"typecheck"},
{{0,TPSTYPE_NAME},(void*)"undefined"},
{{0,TPSTYPE_NAME},(void*)"undefinedfilename"},
{{0,TPSTYPE_NAME},(void*)"undefinedresult"},
{{0,TPSTYPE_NAME},(void*)"unmatchedmark"},
{{0,TPSTYPE_NAME},(void*)"unregistered"},
{{0,TPSTYPE_NAME},(void*)"vmerror"},
{{0,TPSTYPE_NAME},(void*)"systemerror"},
{{0,TPSTYPE_NAME},(void*)"aritymismatch"},
{{0,TPSTYPE_NAME},(void*)"uncaughtthrow"},
{{0,TPSTYPE_NAME},(void*)"eof"},
{{0,TPSTYPE_NAME},(void*)"quit"},
{{0,TPSTYPE_NAME},(void*)"retryframe"},
{{0,TPSTYPE_NAME},(void*)"popframe"},
{{0,TPSTYPE_NAME},(void*)"tailframe"},
{{0,TPSTYPE_NAME},(void*)"stop"},
{{0,TPSTYPE_NAME},(void*)"unsafe"},

{{0,TPSTYPE_NAME},(void*)"nulltype"},
{{0,TPSTYPE_NAME},(void*)"marktype"},
{{0,TPSTYPE_NAME},(void*)"booleantype"},
{{0,TPSTYPE_NAME},(void*)"integertype"},
{{0,TPSTYPE_NAME},(void*)"realtype"},
{{0,TPSTYPE_NAME},(void*)"nametype"},
{{0,TPSTYPE_NAME},(void*)"stringtype"},
{{0,TPSTYPE_NAME},(void*)"arraytype"},
{{0,TPSTYPE_NAME},(void*)"dicttype"},
{{0,TPSTYPE_NAME},(void*)"streamtype"},
{{0,TPSTYPE_NAME},(void*)"operatortype"},

{{0,TPSTYPE_NAME},(void*)LPARENSTR},
{{0,TPSTYPE_NAME},(void*)RPARENSTR},
{{0,TPSTYPE_NAME},(void*)LBRACESTR},
{{0,TPSTYPE_NAME},(void*)RBRACESTR},
{{0,TPSTYPE_NAME},(void*)LBRACKETSTR},
{{0,TPSTYPE_NAME},(void*)RBRACKETSTR},
{{0,TPSTYPE_NAME},(void*)LANGLESTR},
{{0,TPSTYPE_NAME},(void*)RANGLESTR},
{{0,TPSTYPE_NAME},(void*)LLANGLESTR},
{{0,TPSTYPE_NAME},(void*)RRANGLESTR},
{{0,TPSTYPE_NAME},(void*)";"},
{{0,TPSTYPE_NAME},(void*)":"},

/* hole */
{{0,TPSTYPE_NULL},(void*)0},
{{0,TPSTYPE_NULL},(void*)0},
{{0,TPSTYPE_NULL},(void*)0},

/* misc. names */
{{0,TPSTYPE_NAME},(void*)"systemdict"},
{{0,TPSTYPE_NAME},(void*)"errordict"},
{{0,TPSTYPE_NAME},(void*)"userdict"},
{{0,TPSTYPE_NAME},(void*)"$error"},
{{0,TPSTYPE_NAME},(void*)"newerror"},
{{0,TPSTYPE_NAME},(void*)"command"},
{{0,TPSTYPE_NAME},(void*)"errorname"},
{{0,TPSTYPE_NAME},(void*)"errortrap"},
{{0,TPSTYPE_NAME},(void*)"errorhandler"},
{{0,TPSTYPE_NAME},(void*)"handleerror"},
{{0,TPSTYPE_NAME},(void*)"tracetrap"},
{{0,TPSTYPE_NAME},(void*)"file"},
{{0,TPSTYPE_NAME},(void*)"string"},
{{0,TPSTYPE_NAME},(void*)"safemode"},
{{0,TPSTYPE_NAME},(void*)"super"},
{{0,TPSTYPE_NAME},(void*)"self"},
{{0,TPSTYPE_NAME},(void*)"stdcons"},
{{0,TPSTYPE_NAME},(void*)"stdin"},
{{0,TPSTYPE_NAME},(void*)"stdout"},
{{0,TPSTYPE_NAME},(void*)"stderr"},

/* throw name values */
{{0,TPSTYPE_NAME},(void*)"stop"},
{{0,TPSTYPE_NAME},(void*)"exit"},
{{0,TPSTYPE_NAME},(void*)"exitsafe"},
{{0,TPSTYPE_NAME},(void*)"exittrace"},

/* hole */
{{0,TPSTYPE_NULL},(void*)0},

/* misc. keywords */
{{0,TPSTYPE_NAME},(void*)"null"},
{{0,TPSTYPE_NAME},(void*)"mark"},
{{0,TPSTYPE_NAME},(void*)"true"},
{{0,TPSTYPE_NAME},(void*)"false"},
{{0,TPSTYPE_NAME},(void*)"eol"},

/* Non-name constants */
{{0,TPSTYPE_NULL},(void*)0},
{{0,TPSTYPE_MARK},(void*)0},
{{0,TPSTYPE_BOOLEAN},(void*)1},
{{0,TPSTYPE_BOOLEAN},(void*)0},
{{0,TPSTYPE_INTEGER},(void*)'\n'},
{{0,TPSTYPE_INTEGER},(void*)0},

};

static
void
buildnametable()
{
    Tps_Value* pcv = (Tps_Value*)Tps__constants_static;
    Tps_Operator* pco = safeprimitives;
    register Tps_Nameid nm;
    register int i;
    register int l;

    /* put in the names from the constants table first
	so that the name table entries will in fact
	correspond to what is in the constants table.
    */
    l = sizeof(Tps__constants_static)/sizeof(Tps_Value_Simple);
    for(i=0;i<l;i++,pcv++) {
	if(TPS_ISTYPE(*pcv,TPSTYPE_NAME)) {
	    nm = tpsg._nametable->newname(TPS_NAME_OF(*pcv), TRUE);
	}
    }

    for(pco=safeprimitives;pco->_name;pco++) {
	nm = tpsg._nametable->newname((char*)pco->_name, TRUE);
    }
    for(pco=unsafeprimitives;pco->_name;pco++) {
	nm = tpsg._nametable->newname((char*)pco->_name, TRUE);
    }
}

/**************************************************/
/* builtin procs defined by text */

#include "textdefs.H"

static
void
buildonetextoperator(Tps_Dict* opdict,
		      struct Tps_Operator_Defines* pod,
		      Tps_Stream_String* procbuf,
		      Tps_Stream_String* tokenbuf,
		      boolean unsafe)
{
    Tps_Nameid nm;
    Tps_Status ok;
    Tps_Dictpair pair;

    nm = tpsg._nametable->newname(pod->_name);
    ok = procbuf->open(pod->_body);
    if(ok != TPSSTAT_OK) goto fail2;
    ok = Tps_get_token(*tokenbuf,procbuf,&pair._value,1);
    (void)procbuf->close();
    if(ok != TPSSTAT_OK) goto fail2;
    /* make all primitives untraceable */
    TPS_SET_TRACEOFF(pair._value,1);
    /* handle safety */
    if(unsafe)
	TPS_SET_UNSAFE(pair._value,1);
    /* insert into opdict */
    TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
    ok = opdict->insert(pair,(Tps_Value*)NULL);
    if(ok != TPSSTAT_OK) goto fail2;
    return;

fail2:
    TPS_STDCONS->printf("%s: could not define postfix text operator\n",nm);
    return;
}


static
void
buildtextoperators()
{
    Tps_Stream_String* procbuf = 0;
    Tps_Stream_String* tokenbuf = 0;
    struct Tps_Operator_Defines* pod;

    /* ATT C++ does not like stack allocated objects with destructors
	if one is also using labels
    */
    /* open some temporary string streams */
    tokenbuf = new Tps_Stream_String;
    if(tokenbuf->open() != TPSSTAT_OK) {
	delete tokenbuf;
	return;
    }
    procbuf = new Tps_Stream_String;

    /* insert those operators defined by text*/

    /* safe text defined operators */
    for(pod=textdefs;pod->_name;pod++) {
	buildonetextoperator(tpsg._systemdict,pod,procbuf,tokenbuf,FALSE);
    }

    /* unsafe text defined operators */
    for(pod=utextdefs;pod->_name;pod++) {
	buildonetextoperator(tpsg._systemdict,pod,procbuf,tokenbuf,TRUE);
    }

#if OO
    /* safe text defined operators */
    for(pod=ootextdefs;pod->_name;pod++) {
	buildonetextoperator(tpsg._systemdict,pod,procbuf,tokenbuf,FALSE);
    }
#endif

    delete procbuf;
    delete tokenbuf;
}

/**************************************************/
/* alias defs */
static
struct Tps_Operator_Alias {
	char*	_name;
	char*	_target;
} alias_defs[] = {
{"flushfile","flushstream"},
{"closefile","closestream"},
{"resetfile","resetstream"},
{"currentfile","currentstream"},
{"trace","traceexec"},
{".","pstack"},
{"neq","ne"},
{(char*)NULL, (char*)NULL}
};

static
void
buildoperatoraliases()
{
    struct Tps_Operator_Alias* p;
    Tps_Dict* opdict;
    Tps_Nameid nm;
    Tps_Status ok;
    Tps_Dictpair pair;
    Tps_Dictpair* pairp;

    /* use systemdict */
    opdict = tpsg._systemdict;

    for(p=alias_defs;p->_name;p++) {
	/* lookup the target name */
	nm = tpsg._nametable->newname(p->_target);
	TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
	ok = opdict->lookup(pair._key,&pairp);
	if(ok != TPSSTAT_OK) {
	    TPS_STDCONS->printf("%s: could not locate alias target\n",nm);
	} else {
	    /* duplicate value with the alias */
	    nm = tpsg._nametable->newname(p->_name);
	    TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
	    pair._value = pairp->_value;	
	    /* insert into opdict */
	    ok = opdict->insert(pair,(Tps_Value*)NULL);
	    if(ok != TPSSTAT_OK) {
		TPS_STDCONS->printf("%s: could not define alias\n",nm);
	    }
	}
    }
}

/**************************************************/
/* builtin constants (in systemdict): all names here should be in
   constants table as a pre-defined name
*/
static
struct Tps_Constant_Defs {
	char*		_name;
	int		_index;
} constant_defs[] = {
{"null",TPS__NULL},
{"mark",TPS__MARK},
{"true",TPS__TRUE},
{"false",TPS__FALSE},
{"eol",TPS__EOL},
{LBRACKETSTR,TPS__MARK},
{LBRACESTR,TPS__MARK},
{LLANGLESTR,TPS__MARK},
{(char*)NULL,0}
};

static
void
buildconstantdefs()
{
    /* fill in operators defined as constant values */
    register struct Tps_Constant_Defs* pc;
    register Tps_Nameid nm;
    register Tps_Status ok;
    Tps_Dictpair pair;

    for(pc=constant_defs;pc->_name;pc++) {
	nm = tpsg._nametable->newname(pc->_name);
	TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
	pair._value = TPS__CONST(pc->_index);
	ok = tpsg._systemdict->insert(pair,(Tps_Value*)NULL);
	if(ok != TPSSTAT_OK) {
	    TPS_STDCONS->printf("Tps_initialize: error creating constant %s\n",nm);
	    return;
	}
    }
}

/**************************************************/
static
struct Tps_misc {
	char*		_name;
	Tps_Typeid	_type;
	void*		_value;
} misc_defs[] = {
{(char*)NULL,TPSTYPE_NULL}
};

static
void
buildmisc()
{
    register struct Tps_misc* pa;
    register Tps_Nameid nm;
    register Tps_Status ok;
    Tps_Dictpair pair;

    /* fill in misc. constants */
    for(pa=misc_defs;pa->_name;pa++) {
	switch (pa->_type) {
	    case TPSTYPE_STRING: {
		register Tps_String* ss;
		ss = new Tps_String((char*)pa->_value);
		TPS_MAKEVALUE(pair._value,pa->_type,ss);
	    } break;
	    case TPSTYPE_DICT: {
		Tps_Dict** dp = (Tps_Dict**)pa->_value;
		TPS_MAKEVALUE(pair._value,pa->_type,*dp);
	    } break;
	    case TPSTYPE_STREAM: {
		Tps_Stream** sp = (Tps_Stream**)pa->_value;
		TPS_MAKEVALUE(pair._value,pa->_type,*sp);
	    } break;
	    default:
		TPS_MAKEVALUE(pair._value,pa->_type,pa->_value);
		break;
	}
	nm = tpsg._nametable->newname(pa->_name,TRUE);
	TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
	ok = tpsg._systemdict->insert(pair,(Tps_Value*)NULL);
	if(ok != TPSSTAT_OK) {
	    TPS_STDCONS->printf("Tps_initialize: error creating constant %s\n",nm);
	    return;
	}
    }
}

/**************************************************/
/* misc non-operator values to mark as unsafe for various reasons */
static
struct Tps_unsafe {
	char*		_name;
} unsafemisc[] = {
{"stdin"},
{"stdout"},
{"stderr"},
{(char*)0}
};

static
void
buildunsafemisc()
{
    register struct Tps_unsafe* pa;
    register Tps_Nameid nm;
    register Tps_Status ok;
    Tps_Dictpair pair;
    Tps_Dictpair* pairp;

    /* Mark values as unsafe*/
    for(pa=unsafemisc;pa->_name;pa++) {
	nm = tpsg._nametable->newname(pa->_name,TRUE);
	TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
	ok = tpsg._systemdict->lookup(pair._key,&pairp);
	if(ok != TPSSTAT_OK) {
	    TPS_STDCONS->printf("%s: could not locate unsafe operator\n",nm);
	} else {
	    /* Mark value field as unsafe */
	    TPS_SET_UNSAFE(pairp->_value,1);
	}
    }
}

/**************************************************/
static
struct Tps_platform {
	char*		_name;
	Tps_Typeid	_type;
	void*		_value;
} platform_defs[] = {
{"version",TPSTYPE_STRING,(void*)Tps_versionstring},
{"targetarch",TPSTYPE_STRING,(void*)Tps_targetarch},
{"targetos",TPSTYPE_STRING,(void*)Tps_targetos},
{(char*)NULL,TPSTYPE_NULL}
};

static
struct Tps_config {
	char*		_name;
	Tps_Typeid	_type;
	void*		_value;
	boolean		_indirect;
} config_defs[] = {
{"copyright",TPSTYPE_STRING,(void*)Tps_copyright,0},
{"safefileprefix",TPSTYPE_STRING,(void*)Tps_safefileprefix,0},
{"interactive",TPSTYPE_BOOLEAN,(void*)&tpsg._interactive,1}, // override
{".tpsrc",TPSTYPE_STRING,(void*)Tps_tpsrc,0},
{(char*)NULL,TPSTYPE_NULL,0}
};

static
Tps_Dict*
buildplatform()
{
    register struct Tps_platform* pp;
    register Tps_Nameid nm;
    register Tps_Status ok;
    Tps_Dictpair pair;
    Tps_Dict* pldict;

    pldict = new Tps_Dict_Tcl(10,"platformdict");
    if(!pldict) {
	TPS_STDCONS->printf("Tps_initialize: error creating platform dict\n");
	return pldict;
    }
    /* fill in platform */
    for(pp=platform_defs;pp->_name;pp++) {
	switch (pp->_type) {
	    case TPSTYPE_STRING: {
		register Tps_String* ss;
		ss = new Tps_String((char*)pp->_value);
		TPS_MAKEVALUE(pair._value,pp->_type,ss);
	    } break;
	    case TPSTYPE_DICT: {
		Tps_Dict** dp = (Tps_Dict**)pp->_value;
		TPS_MAKEVALUE(pair._value,pp->_type,*dp);
	    } break;
	    case TPSTYPE_STREAM: {
		Tps_Stream** sp = (Tps_Stream**)pp->_value;
		TPS_MAKEVALUE(pair._value,pp->_type,*sp);
	    } break;
	    default:
		TPS_MAKEVALUE(pair._value,pp->_type,pp->_value);
		break;
	}
	nm = tpsg._nametable->newname(pp->_name,TRUE);
	TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
	ok = pldict->insert(pair,(Tps_Value*)NULL);
	if(ok != TPSSTAT_OK) {
	    TPS_STDCONS->printf("Tps_initialize: error creating platform constant %s\n",nm);
	}
    }
    return pldict;
}

static
void
buildconfig()
{
    register struct Tps_config* pp;
    register Tps_Nameid nm;
    register Tps_Status ok;
    Tps_Dictpair pair;
    Tps_Dict* pldict;
    Tps_Dict* confdict;

    /* construct the config dict */
    confdict = new Tps_Dict_Tcl(10,"configdict");
    if(!confdict) {
	TPS_STDCONS->printf("Tps_initialize: error creating config dict\n");
	return;
    }
    nm = tpsg._nametable->newname("configurationdict",TRUE);
    TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
    TPS_MAKEVALUE(pair._value,TPSTYPE_DICT,confdict);
    /* make read-only */
    TPS_SET_ACCESS(pair._value,Tps_access_readonly);
    ok = tpsg._systemdict->insert(pair,(Tps_Value*)NULL);
    if(ok != TPSSTAT_OK) {
	TPS_STDCONS->printf("Tps_initialize: error (2) creating config dict\n");
    }
    /* construct the platform dict */
    pldict = buildplatform();
    if(pldict) {
	/* insert in config dict */
	nm = tpsg._nametable->newname("platformdict",TRUE);
	TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
	TPS_MAKEVALUE(pair._value,TPSTYPE_DICT,pldict);
	/* make read-only */
	TPS_SET_ACCESS(pair._value,Tps_access_readonly);
	ok = confdict->insert(pair,(Tps_Value*)NULL);
	if(ok != TPSSTAT_OK) {
	    TPS_STDCONS->printf("Tps_initialize: error (2) creating platform dict\n");
	}
    }
    /* fill in config dict*/
    /* start by inserting the interactive flag */
    for(pp=config_defs;pp->_name;pp++) {
	void* pv = pp->_value;
	if(pp->_indirect) pv = *((void**)pv);
	switch (pp->_type) {
	    case TPSTYPE_STRING: {
		register Tps_String* ss;
		ss = new Tps_String((char*)pv);
		TPS_MAKEVALUE(pair._value,pp->_type,ss);
	    } break;
	    case TPSTYPE_DICT: {
		Tps_Dict** dp = (Tps_Dict**)pv;
		TPS_MAKEVALUE(pair._value,pp->_type,*dp);
	    } break;
	    case TPSTYPE_STREAM: {
		Tps_Stream** sp = (Tps_Stream**)pv;
		TPS_MAKEVALUE(pair._value,pp->_type,*sp);
	    } break;
	    case TPSTYPE_BOOLEAN: {
		TPS_MAKEVALUE(pair._value,pp->_type,(long)pv);
	    } break;
	    default:
		TPS_MAKEVALUE(pair._value,pp->_type,pv);
		break;
	}
	nm = tpsg._nametable->newname(pp->_name,TRUE);
	TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
	ok = confdict->insert(pair,(Tps_Value*)NULL);
	if(ok != TPSSTAT_OK) {
	    TPS_STDCONS->printf("Tps_initialize: error creating configure constant %s\n",nm);
	}
    }
}

/**************************************************/
static
void
bindoperators()
{
    Tps_Value fake[2];
    register int i;
    Tps_Dictpair* pairp;
    register Tps_Status ok;

    /* go thru all dicts again and try to bind all the entries in the dict.*/

    /* bind systemdict entries */
    fake[0] = tpsg.__systemdict;
    for(i=0;i<tpsg._systemdict->range();i++) {
	ok = tpsg._systemdict->ith(i,pairp);
	if(ok != TPSSTAT_OK) continue;
	if(Tps_bind1(fake,1,&pairp->_value) != TPSSTAT_OK) return;
    }
}

/**************************************************/

Tps_Status
Tps_initialize(boolean isinteractive)
{
    register Tps_Status ok;
    register Tps_Nameid nm;
    Tps_Dictpair pair;

    /* initialize the global structures */

    /* fillin the Global struct */
   
#ifdef CONFIGTEST
    if(sizeof(Tps_Value) != (2 * sizeof(void*))) {
	fprintf(stderr,"Tps_initialize: non-packed Tps_Value: %d\n",
		sizeof(Tps_Value));
    }
    if(sizeof(Tps_Value_Simple) != (2 * sizeof(void*))) {
	fprintf(stderr,"Tps_initialize: non-packed Tps_Value_Simple: %d\n",
		sizeof(Tps_Value_Simple));
    }
#endif

    tpsg._interactive = isinteractive;

#if defined hpux || solaris2
    tpsg._clockres = sysconf(_SC_CLK_TCK);
    tpsg._clockres = 1000000 / tpsg._clockres;
#else
    tpsg._clockres = 1;
#endif

    Tps__constants = (Tps_Value*)Tps__constants_static;

    tpsg._objects = new Tps_List;

    tpsg._handlers = (Tps_Handler_List*)0;
    Tps_add_handler(&Tps_handler_source);
    Tps_add_handler(&Tps_handler_trace);
    Tps_add_handler(&Tps_handler_statemark);
    Tps_add_handler(&Tps_handler_safety);
    Tps_add_handler(&Tps_handler_catch);
    Tps_add_handler(&Tps_handler_stopped);
    Tps_add_handler(&Tps_handler_runstream);
    Tps_add_handler(&Tps_handler_loop);
    Tps_add_handler(&Tps_handler_while);
    Tps_add_handler(&Tps_handler_repeat);
    Tps_add_handler(&Tps_handler_forall);
    Tps_add_handler(&Tps_handler_for);

    /* fillin the name table */    
    if(!(tpsg._nametable = new Tps_Nametable(1024))) goto systemerr;

    tpsg._stdcons = new Tps_Stream_File;
    tpsg._stdcons->attach(2,"stdcons",Tps_stream_w);
    TPS_MAKEVALUE(tpsg.__stdcons,TPSTYPE_STREAM,tpsg._stdcons);
    TPS_SET_UNSAFE(tpsg.__stdcons,1);

    /* define std{in,out,err} */
    tpsg._stdin = new Tps_Stream_File;
    tpsg._stdin->attach(0,"stdin",Tps_stream_r);
    TPS_MAKEVALUE(tpsg.__stdin,TPSTYPE_STREAM,tpsg._stdin);
    tpsg._stdout = new Tps_Stream_File;
    tpsg._stdout->attach(1,"stdout",Tps_stream_w);
    TPS_MAKEVALUE(tpsg.__stdout,TPSTYPE_STREAM,tpsg._stdout);
    tpsg._stderr = new Tps_Stream_File;
    tpsg._stderr->attach(2,"stderr",Tps_stream_w);
    TPS_MAKEVALUE(tpsg.__stderr,TPSTYPE_STREAM,tpsg._stderr);

    tpsg._tempbuf = new Tps_Stream_String;
    if(tpsg._tempbuf->open() != TPSSTAT_OK) goto systemerr;
    /* give it a very big buffer */
    if(!tpsg._tempbuf->guarantee(4096)) goto systemerr; 

    if(!(tpsg._systemdict = new Tps_Dict_Tcl(1024,"systemdict")))
	goto systemerr;
    TPS_MAKEVALUE(tpsg.__systemdict,TPSTYPE_DICT,tpsg._systemdict);

    /* insert std... into systemdict */
    nm = TPS_NAMETABLE->newname("stdcons");
    TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
    pair._value = tpsg.__stdcons;
    ok = tpsg._systemdict->insert(pair,(Tps_Value*)NULL);
    if(ok != TPSSTAT_OK) goto systemerr;

    nm = TPS_NAMETABLE->newname("stdin");
    TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
    pair._value = tpsg.__stdin;
    ok = tpsg._systemdict->insert(pair,(Tps_Value*)NULL);
    if(ok != TPSSTAT_OK) goto systemerr;

    nm = TPS_NAMETABLE->newname("stdout");
    TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
    pair._value = tpsg.__stdout;
    ok = tpsg._systemdict->insert(pair,(Tps_Value*)NULL);
    if(ok != TPSSTAT_OK) goto systemerr;

    nm = TPS_NAMETABLE->newname("stderr");
    TPS_MAKEVALUE(pair._key,TPSTYPE_NAME,nm);
    pair._value = tpsg.__stderr;
    ok = tpsg._systemdict->insert(pair,(Tps_Value*)NULL);
    if(ok != TPSSTAT_OK) goto systemerr;

    /* order in which following are called is important */
    buildnametable();
    buildconstantdefs();
    buildmisc();
    buildoperators();
    buildtextoperators();
    buildoperatoraliases();
    buildunsafemisc();
    buildconfig();
    bindoperators();

    /* insert systemdict into self */
    pair._key = TPS__NM(TPS_NMSYSTEMDICT);
    pair._value = tpsg.__systemdict;
    ok = tpsg._systemdict->insert(pair,(Tps_Value*)NULL);
    if(ok != TPSSTAT_OK) {
	TPS_STDCONS->write("Tps_initialize: error filling systemdict\n");
	goto systemerr;
    }

    /* make systemdict read-only */
    TPS_SET_ACCESS(tpsg.__systemdict,Tps_access_readonly);

#if DEBUG > 1
    {
	Tps_Value nt;
	int i;
	TPS_MAKEVALUE(nt,TPSTYPE_DICT,tpsg._nametable);
	/* print out the name table to see if it is correct */
	TPS_STDCONS->printf("nametable=\n%s\n\n",debugobject(nt));

	/* print out the constants table to see if it is correct */
	printf("TPS_ENM_OFFSET=%d\n",TPS_ENM_OFFSET);
	printf("TPS_TNM_OFFSET=%d\n",TPS_TNM_OFFSET);
	printf("GNMO=%d\n",GNMO);
	printf("TPS_NM_COUNT=%d\n",TPS_NM_COUNT);
	printf("GCO=%d\n",GCO);
	printf("TPS_ALLCONSTANTS_COUNT=%d\n",TPS_ALLCONSTANTS_COUNT);
	TPS_STDCONS->write("constants={\n");
	for(i=0;i<TPS_ALLCONSTANTS_COUNT;i++) {
	    TPS_STDCONS->printf("%d: %s\n",i,debugobject(TPS__CONST(i)));
	}
	TPS_STDCONS->write("}\n");
    }

    {
	register Tps_Dict_Tcl* d;
	d = (Tps_Dict_Tcl*)tpsg._nametable;
	printf("%s:%s\n","nametable",d->stats());
	d = (Tps_Dict_Tcl*)tpsg._systemdict;
	printf("%s:%s\n","systemdict",d->stats());
    }
#endif

    return TPSSTAT_OK;

systemerr:
    return(TPSSTAT_SYSTEMERROR);
}

Tps_Status
Tps_finalize()
{
    (void)MEMSET((char*)&tpsg,0,sizeof(Tps_Global));
    return TPSSTAT_OK;
}


Tps_Status
Tps_add_handler(Tps_Handler* h)
{
    register Tps_Handler_List* l = new Tps_Handler_List;
    if(!l) return TPSSTAT_VMERROR;
    GLOBAL_LOCK();
    l->_handler = h;
    l->_next = tpsg._handlers;
    tpsg._handlers = l;
    GLOBAL_UNLOCK();
    return TPSSTAT_OK;
}

Tps_Status
Tps_remove_handler(Tps_Handler* h)
{
    register Tps_Handler_List *l,*l2;
    register Tps_Status ok = TPSSTAT_OK;

    GLOBAL_LOCK();
    l = tpsg._handlers;
    if(!l) {ok = TPSSTAT_UNDEFINED; goto done;}
    if(h == l->_handler) {
	tpsg._handlers = tpsg._handlers->_next;
    } else {
	l2 = l;
	for(l=l->_next;l;l=l->_next) {
	    if(l->_handler == h) {
		l2->_next = l->_next;
	    }
	    l2 = l;
	}
    }
    delete l;
done:
    GLOBAL_UNLOCK();
    return ok;
}

Tps_Handler*
Tps_lookup_handler(char* s)
{
    register Tps_Handler_List* h;
    register Tps_Handler* hd = (Tps_Handler*)0;
    GLOBAL_LOCK();
    for(h=tpsg._handlers;h;h=h->_next) {
	if(strcmp(s,h->_handler->_parms->_name)==0) {
	    hd = h->_handler;
	    break;
	}
    }
    GLOBAL_UNLOCK();
    return hd;
}
