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

#include "tps.H"
#include "textdefs.H"

/* Safe defs */
const struct Tps_Operator_Defines textdefs[] = {

{"run","{/r /file stream runstream {handleerror} if}"},
{"=","{null cvts print}"},
{"flush","{stdout flushstream}"},
{"print","{dup type /integertype eq					\n\
		{stdout exch write}					\n\
		{dup type /stringtype neq {null cvs} if			\n\
		 stdout exch writestring}				\n\
	      ifelse							\n\
	 }"},
{"printline","{print eol print}"},
{"pstack","{% any(n-1)...any(0)						\n\
	    count 1 sub		% any(n-1)...any(0) n-1			\n\
	    -1 0		% any(n-1)...any(0) n-1 -1 0		\n\
	    {  % any(n-1)...any(0) i					\n\
		index		% any(n-1)...any(0) any(i)		\n\
		==		% any(n-1)...any(0)			\n\
		32 print	% any(n-1...any(0)			\n\
	    }								\n\
	    for								\n\
	    eol print		% any(n-1...any(0)			\n\
	}"},
{"stack","{  % any(n-1)...any(0)					\n\
	    count 1 sub		% any(n-1)...any(0) n-1			\n\
	    -1 0		% any(n-1)...any(0) n-1 -1 0 		\n\
	    {  % any(n-1)...any(0) i 					\n\
		index		% any(n-1)...any(0) any(i)		\n\
		=		% any(n-1)...any(0)			\n\
		32 print	% any(n-1...any(0) 			\n\
	    } 								\n\
	    for 							\n\
	    eol print		% any(n-1...any(0) 			\n\
	}"},
{"runstring","{/r /string stream runstream {handleerror} if}"},
{"file","{/file stream}"},
{"tokenizeline","% string -> {tokens...} t|f				\n\
{ % string								\n\
    [ exch        % mark string						\n\
    { 									\n\
        {token}								\n\
        stopped								\n\
        { % string ; assume syntax error				\n\
            cleartomark							\n\
            pop false exit						\n\
        } if				% mark ... string object t	\n\
	not {exit} if			% mark ... string object	\n\
	exch				% mark ... object string	\n\
    } loop								\n\
    ]  cvx true	% {tokens...} true					\n\
}"},
{"eval","{								\n\
    {									\n\
        {								\n\
            /prompt cvx exec % allow prompt to be overridden		\n\
	     stdin null readline         % string t | f			\n\
            not {exit} if                      % string			\n\
            tokenizeline                       % {tokens...} t|f	\n\
            not {\"syntaxerror\" printline } if        % {tokens...}	\n\
            gc								\n\
            {stateexec} stopped {handleerror} if			\n\
            gc								\n\
        } catch								\n\
	{\"uncaught throw\" printline print \"\" printline}		\n\
        if								\n\
   } loop								\n\
}"},
{"platformdict","{configurationdict /platformdict get}"},
{"version","{platformdict /version get}"},
{"interactive","{configurationdict /interactive get}"},
{".tpsrc","{configurationdict /.tpsrc get}"},
{"start","{interactive {{.tpsrc run} stopped pop} if clear eval}"},
#if PSMIMIC
{"prompt","{(>) print flush}"},
#else
{"prompt","{\">\" print flush}"},
#endif
{(char*)0, (char*)0} /* terminator */
};

/* Unsafe operators */
const struct Tps_Operator_Defines utextdefs[] = {

{"stream","{{dup /file eq						\n\
		{safefileprefix null copy				\n\
		 4 -1 roll						\n\
		 % check for \"..\"					\n\
		 dup \"..\" search					\n\
                 {/invalidstreamaccess stop} {pop} ifelse		\n\
		 null cvs dup 0 get					\n\
		 '/' neq {exch \"/\" append exch} if			\n\
		 append							\n\
		 3 1 roll						\n\
		} if							\n\
	    } if __stream}"},

{(char*)0, (char*)0} /* terminator */
};

#if OO
/* Safe OO defs */
const struct Tps_Operator_Defines ootextdefs[] = {

{"super","/super"},
{"self","/self"},

{"clone","{dup type /dicttype eq					\n\
	    {null copy cvx}						\n\
	    {/typecheck errortrap}					\n\
	    ifelse							\n\
	   }"},

{"subtype","{dup type /dicttype eq					\n\
	    {0 dict dup /super 4 -1 roll put cvx}			\n\
	    {/typecheck errortrap}					\n\
	    ifelse							\n\
	   }"},

{(char*)0, (char*)0} /* terminator */
};
#endif

