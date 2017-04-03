/* char marking table for parsing */
EXTERNC unsigned int Tps_chartable[256];

/* Define the char parsing table flags; see also build.C */
#define TPSC_WS		0x01	/* whitespace char */
#define TPSC_EOL	0x02	/* end of line char */
#define TPSC_SPECIAL	0x04	/* special prescript char (e.g. lbrace) */
#define TPSC_NAME	0x08	/* acceptable name character */
#define TPSC_OCTDIGIT	0x10	/* acceptable octal digit character */
#define TPSC_PRINTABLE	0x20	/* name, special, or blank. */
#define TPSC_HEXDIGIT	0x40	/* acceptable hex digit character */
#define TPSC_DECDIGIT	0x80	/* acceptable decimal digit character */
#define TPSC_WIDTH 8 /*bits*/

/* misc. lexically relevant defs */
#ifndef EOS
#  define EOS '\0'
#endif
#ifndef EOL
#  define EOL '\n'
#endif

/* avoid emacs delimiter matching  */
#define LPAREN '('
#define RPAREN ')'
#define LBRACE '{'
#define RBRACE '}'
#define LBRACKET '['
#define RBRACKET ']'
#define LANGLE '<'
#define RANGLE '>'
#define DQUOTE '"'
#define SQUOTE '\''
#define ESCAPER '\\'
#define SEMICOLON ';'
#define COLON ':'

#define LPARENSTR "("
#define RPARENSTR ")"
#define LBRACESTR "{"
#define RBRACESTR "}"
#define LBRACKETSTR "["
#define RBRACKETSTR "]"
#define LANGLESTR "<"
#define RANGLESTR ">"
#define LLANGLESTR "<<"
#define RRANGLESTR ">>"
#define DQUOTESTR "\""
#define SQUOTESTR "'"
#define ESCAPERSTR "\\"

#define ISWHITESPACE(c) ((c)!=EOF && (Tps_chartable[c] & TPSC_WS))
#define ISEOL(c) ((c)!=EOF && (Tps_chartable[c] & TPSC_EOL))
#define ISSPECIAL(c) ((c)!=EOF && (Tps_chartable[c] & TPSC_SPECIAL))
#define ISNAMECHAR(c)  ((c)!=EOF && (Tps_chartable[c] & TPSC_NAME))
#define ISOCTDIGIT(c)  ((c)!=EOF && (Tps_chartable[c] & TPSC_OCTDIGIT))
#define ISHEXDIGIT(c)  ((c)!=EOF && (Tps_chartable[c] & TPSC_HEXDIGIT))
#define ISDECDIGIT(c)  ((c)!=EOF && (Tps_chartable[c] & TPSC_DECDIGIT))
#define ISPRINTABLE(c)  ((c)!=EOF && (Tps_chartable[c] & TPSC_PRINTABLE))
