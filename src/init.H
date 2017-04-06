/*
 * Copyright (c) 1993 Dennis Heimbigner
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef INIT_HH
#define INIT_HH 1

/**************************************************/
/* Define the "constant" global values */
/* Following values will NEVER be changed */
/* They are used for initialization */

const char Tps_copyright[] =
"Copyright (c) 1993 Dennis Heimbigner\nAll rights reserved.";

const char Tps_operator_prefix[] = "__";

const char Tps_safefileprefix[] = "/tmp";

const char Tps_tpsrc[] = ".tpsrc";

/**************************************************/
/* Define a vector of useful constant values */
EXTERNC Tps_Value* Tps__constants;

/* Accessing macros */

/* Indices into name tables */
/* start with errornames, then typenames, then all other names*/
const int TPS_ENM_OFFSET =	 0;
const int TPS_TNM_OFFSET =	 	TPS_ERROR_COUNT;
const int GNMO =			(TPS_ERROR_COUNT+TPSTYPE_COUNT);
const int TPS_NMLPAREN =		( 0 + GNMO);
const int TPS_NMRPAREN =		( 1 + GNMO);
const int TPS_NMLBRACE =		( 2 + GNMO);
const int TPS_NMRBRACE =		( 3 + GNMO);
const int TPS_NMLBRACKET =		( 4 + GNMO);
const int TPS_NMRBRACKET =		( 5 + GNMO);
const int TPS_NMLANGLE =		( 6 + GNMO);
const int TPS_NMRANGLE =		( 7 + GNMO);
const int TPS_NMLLANGLE =		( 8 + GNMO);
const int TPS_NMRRANGLE =		( 9 + GNMO);
const int TPS_NMSEMICOLON =		(10 + GNMO);
const int TPS_NMCOLON =			(11 + GNMO);

const int TPS_NMSYSTEMDICT =		(15 + GNMO);
const int TPS_NMERRORDICT =		(16 + GNMO);
const int TPS_NMUSERDICT =		(17 + GNMO);
const int TPS_NMDERROR =		(18 + GNMO);
const int TPS_NMNEWERROR =		(19 + GNMO);
const int TPS_NMCOMMAND =		(20 + GNMO);
const int TPS_NMERRORNAME =		(21 + GNMO);
const int TPS_NMERRTRAP =		(22 + GNMO);
const int TPS_NMERRHANDLER =	 	(23 + GNMO);
const int TPS_NMHANDLEERR =		(24 + GNMO);
const int TPS_NMTRACETRAP =		(25 + GNMO);
const int TPS_NMFILE =			(26 + GNMO);
const int TPS_NMSTRING =		(27 + GNMO);
const int TPS_NMSAFEMODE =		(28 + GNMO);
const int TPS_NMSUPER =			(29 + GNMO);
const int TPS_NMSELF =			(30 + GNMO);

const int TPS_NMSTDCONS =		(31 + GNMO);
const int TPS_NMSTDIN =			(32 + GNMO);
const int TPS_NMSTDOUT =		(33 + GNMO);
const int TPS_NMSTDERR =		(34 + GNMO);

const int TPS_NMSTOP =			(35 + GNMO);
const int TPS_NMEXIT =			(36 + GNMO);
const int TPS_NMEXITSAFE =		(37 + GNMO);
const int TPS_NMEXITTRACE =		(38 + GNMO);

const int TPS_NMNULL =			(40 + GNMO);
const int TPS_NMMARK =			(41 + GNMO);
const int TPS_NMTRUE =			(42 + GNMO);
const int TPS_NMFALSE =			(43 + GNMO);
const int TPS_NMEOL =			(44 + GNMO);

const int TPS_NM_COUNT =		(45 + GNMO) /* number of pre-defined names */;

/* accessor for type names via type index */
#define TPS__TNM(i) (Tps__constants[TPS_TNM_OFFSET+(int)(i)])
/* accessor for other names */
#define TPS__NM(i) (Tps__constants[i])
/* accessor for error names via error index */
#define TPS__ENM(i) TPS__NM(i)

/* accessors directly to names */
#define TPS_TNM(i) TPS_NAME_OF(TPS__TNM(i))
#define TPS_NM(i) TPS_NAME_OF(TPS__NM(i))
#define TPS_ENM(i) TPS_NM((int)(i))

/* other constants */
const int GCO =		 TPS_NM_COUNT;
const int TPS__NULL =	( 0 + GCO);
const int TPS__MARK =	( 1 + GCO);
const int TPS__TRUE =	( 2 + GCO);
const int TPS__FALSE =	( 3 + GCO);
const int TPS__EOL =	( 4 + GCO);
const int TPS__ZERO =	( 5 + GCO);

const int TPS_ALLCONSTANTS_COUNT = (6 + GCO) /* number of pre-defined constants */;

/* accessor for type name values via type index */
#define TPS__TCONST(i) (Tps__constants[(i)+TPS_TNM_OFFSET])
/* accessor for other name values */
#define TPS__CONST(i) (Tps__constants[i])
/* accessor for error name values via error index */
#define TPS__ECONST(i) TPS__CONST(i)

/**************************************************/

struct Tps_Handler_List; /* forward */

/**************************************************/
/* Define the shared values that are need to be locked in a
   threaded environment
*/
typedef struct Tps_Global {
	/* some guaranteed place to dump messages */
	Tps_Stream_File* _stdcons;
	Tps_Value	 __stdcons;

	/* std{in,out,err} */
	Tps_Stream_File*  _stdin;
	Tps_Stream_File*  _stdout;
	Tps_Stream_File*  _stderr;

	Tps_Value	__stdin;
	Tps_Value	__stdout;
	Tps_Value	__stderr;

	/* define temporary use string stream */
	Tps_Stream_String* _tempbuf;

	struct Tps_Nametable* _nametable;

	/* Prototype dicts  */
	struct Tps_Dict* _systemdict;

	/* Tps_Value versions of above */
	Tps_Value	  __systemdict;

	struct Tps_Handler_List* _handlers; /* list of known exec handlers */
	Tps_List*	  _objects; /* list of reclaimable objects */

	long		  _clockres; /* to get to microseconds */

	boolean		_interactive;	/* 0 => started up in batch mode
					   1 => interactive mode */
} Tps_Global;

EXTERN Tps_Global tpsg;

EXTERNC Tps_Status Tps_initialize(boolean isinteractive=FALSE);
EXTERNC Tps_Status Tps_finalize();

/* Accessing macros*/

#define TPS_STDCONS tpsg._stdcons
#define TPS_STDIN tpsg._stdin
#define TPS_STDOUT tpsg._stdout
#define TPS_STDERR tpsg._stderr
#define TPS_TEMPBUF tpsg._tempbuf
#define TPS_ERRNO tpsg._errno
#define TPS_NAMETABLE tpsg._nametable
#define TPS_SYSTEMDICT tpsg._systemdict

#endif /*INIT_HH*/
