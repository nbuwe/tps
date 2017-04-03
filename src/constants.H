#ifndef CONSTANTS_HH
#define CONSTANTS_HH 1

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
const int TPS_NMDDEFAULT =		(28 + GNMO);
const int TPS_NMSAFEMODE =		(29 + GNMO);

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
#define TPS__TNMID(i) (Tps__constants[TPS_TNM_OFFSET+(int)(i)])
/* accessor for other names */
#define TPS__NMID(i) (Tps__constants[i])
/* accessor for error names via error index */
#define TPS__ENMID(i) TPS__NMID(i)

/* accessors directly to names */
#ifdef FSD
#define TPS_TNMID(i) (i)
#define TPS_NMID(i) (i)
#else
#define TPS_TNMID(i) TPS_NAMEID_OF(TPS__TNMID(i))
#define TPS_NMID(i) TPS_NAMEID_OF(TPS__NMID(i))
#endif

#define TPS_TNM(i) TPS_NAME_OF(TPS__TNMID(i))
#define TPS_NM(i) TPS_NAME_OF(TPS__NMID(i))

#define TPS_ENMID(i) TPS_NMID((long)(i))
#define TPS_ENM(i) TPS_NM((long)(i))

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

#endif /*CONSTANTS_HH*/
