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
#ifndef TPS_EXEC_HH
#define TPS_EXEC_HH

/*
A Frame Handler struct defines how to manage any given frame.
In effect, it is the "virtual" for that frame; but virtual
and subtyping were not used to avoid compiler dependencies.

Every stack frame handle must support the following actions:
params:
    Given a pointer to the frame, return a number of misc. parameters
    about that type of frame; this is NOT a procedure pointer.
unwind:
    Clean up the top frame on the exec stack.
reenter:
    Called when the frame is being reentered for execution.
    For a source frame, for example, it might move to the
    next word of an executable array.
trace:
    Print out a trace of the contents of a stack frame.
mark:
    Step thru a frame and call a marking function for each complex tps
    value contained in the frame.
export:
    push the contents of the frame onto the operand stack.
import:
    take frame info off the operand stack and onto the exec stack.
*/

/**************************************************/

struct Tps_Frame_Parameters {
	const char*	_name;
};

/**************************************************/
/* Define flags to control the export and import operations */
typedef long Tps_Exec_Pass;
const long Tps_pass_none = 0; /* dump to first safety or state frame */
const long Tps_pass_safety = 1; /* dump past safety frames */
const long Tps_pass_state = 2; /* dump past state frames */
const long Tps_pass_all = Tps_pass_safety | Tps_pass_state; /*dump all frames*/

/**************************************************/
/* Define some generic argument sequences */

struct Tps_Frame; /*Forward*/
struct Tps_Handler; /*Forward*/

#define TPS_UNWIND_ARGS0 struct Tps_Interp*, struct Tps_Frame*, boolean
#define TPS_UNWIND_ARGS1 struct Tps_Interp* intrp, struct Tps_Frame* frame, boolean
#define TPS_UNWIND_ARGS struct Tps_Interp* intrp, struct Tps_Frame* frame, boolean thrown

#define TPS_REENTER_ARGS0 struct Tps_Interp*, struct Tps_Frame*
#define TPS_REENTER_ARGS struct Tps_Interp* intrp, struct Tps_Frame* frame

#define TPS_TRACE_ARGS0 struct Tps_Interp*, struct Tps_Frame*, struct Tps_Stream*
#define TPS_TRACE_ARGS struct Tps_Interp* intrp, struct Tps_Frame* frame, struct Tps_Stream* strm

#define TPS_MARK_ARGS0 struct Tps_Interp*, struct Tps_Frame*
#define TPS_MARK_ARGS1 struct Tps_Interp*, struct Tps_Frame* frame
#define TPS_MARK_ARGS struct Tps_Interp* intrp, struct Tps_Frame* frame

#define TPS_EXPORT_ARGS0 struct Tps_Interp*, struct Tps_Frame*, long&, Tps_Exec_Pass
#define TPS_EXPORT_ARGS1 struct Tps_Interp* intrp, struct Tps_Frame* frame, long& count, Tps_Exec_Pass
#define TPS_EXPORT_ARGS struct Tps_Interp* intrp, struct Tps_Frame* frame, long& count, Tps_Exec_Pass flags

#define TPS_IMPORT_ARGS0 struct Tps_Interp*, struct Tps_Handler*, long&, struct Tps_Frame*, Tps_Exec_Pass
#define TPS_IMPORT_ARGS1 struct Tps_Interp* intrp, struct Tps_Handler* h, long& cnt, struct Tps_Frame* fr, Tps_Exec_Pass
#define TPS_IMPORT_ARGS struct Tps_Interp* intrp, struct Tps_Handler* h, long& cnt, struct Tps_Frame* fr, Tps_Exec_Pass flags

typedef Tps_Status (*Tps_Unwindfcn)(TPS_UNWIND_ARGS0);
typedef Tps_Status (*Tps_Reenterfcn)(TPS_REENTER_ARGS0);
typedef Tps_Status (*Tps_Tracefcn)(TPS_TRACE_ARGS0);
typedef Tps_Status (*Tps_Markfcn)(TPS_MARK_ARGS0);
typedef Tps_Status (*Tps_Exportfcn)(TPS_EXPORT_ARGS0);
typedef Tps_Status (*Tps_Importfcn)(TPS_IMPORT_ARGS0);

/**************************************************/
struct Tps_Handler {
	Tps_Frame_Parameters*	_parms;
	Tps_Unwindfcn		_unwind;
	Tps_Reenterfcn		_reenter;
	Tps_Tracefcn		_trace;
	Tps_Markfcn		_mark;
	Tps_Exportfcn		_export;
	Tps_Importfcn		_import;
};

/*Accessors*/
#define TPS_FRAME_NAME(interp,f) ((f)->_handler->_parms->_name)
#define TPS_FRAME_UNWIND(interp,f,t) (((f)->_handler->_unwind)(interp,f,t))
#define TPS_FRAME_REENTER(interp,f) (((f)->_handler->_reenter)(interp,f))
#define TPS_FRAME_TRACE(interp,f,s) (((f)->_handler->_trace)(interp,f,s))
#define TPS_FRAME_MARK(interp,f) (((f)->_handler->_mark)(interp,f))
#define TPS_FRAME_EXPORT(interp,f,flen,pf) (((f)->_handler->_export)(interp,f,flen,pf))
#define TPS_FRAME_IMPORT(interp,f,pf) (((f)->_handler->_import)(interp,f,pf))

/*
Track known handlers
*/
struct Tps_Handler_List {
	struct Tps_Handler_List* _next;
	struct Tps_Handler* _handler;
};

/* handler list manager functions */

EXTERNC Tps_Status Tps_add_handler(Tps_Handler*);
EXTERNC Tps_Status Tps_remove_handler(Tps_Handler*);
EXTERNC Tps_Handler* Tps_lookup_handler(const char*);

/**************************************************/

EXTERNC Tps_Status Tps_nullfcn_mark(TPS_MARK_ARGS0);
EXTERNC Tps_Status Tps_nullfcn_export(TPS_EXPORT_ARGS0);
EXTERNC Tps_Status Tps_nullfcn_import(TPS_IMPORT_ARGS0);

/**************************************************/
/* Misc. */
/**************************************************/

/* Generic Trace support */
EXTERNC Tps_Status Tps_trace0(Tps_Interp*, Tps_Stream*, Tps_Frame*);

/* Given a frame ptr, figure out its depth (in terms of frames) in exec stack*/
EXTERNC int Tps_framedepth(Tps_Interp*, Tps_Frame*);

/**************************************************/
/*
The exec frame is kept as a stack of variable length records
conforming to the following struct.
*/

struct Tps_Frame {
	long		_framelength;    /* in bytes */
	Tps_Handler*	_handler;   /* ptr to the frame handler */
};

#define TPS_FRAME_LENGTH(interp,f) ((f)->_framelength)

EXTERNC Tps_Frame* Tps_create_frame(Tps_Interp*, Tps_Handler*, long);
EXTERNC Tps_Status Tps_unwind_frame(Tps_Interp*, Tps_Frame*);

/**************************************************/
/*
Source Frame:
	The exec frame contains a pointer to an executing piece
	of code (which might be any tps object, not just and array).
*/

struct Tps_Frame_Source : Tps_Frame {
	Tps_Value	_body;
	long		_index;
};

#define TPS_FRAME_SOURCE_LENGTH (sizeof(Tps_Frame_Source))

/* Define the frame handler and creator */
EXTERNC Tps_Handler Tps_handler_source;
EXTERNC Tps_Status Tps_create_source(Tps_Interp*, Tps_Value,
				     boolean singular=FALSE);

/**************************************************/
/*
Trace Frame:
    Mark the current state of the trace flag
    so that it can turn on and off at various points.
*/

struct Tps_Frame_Trace : Tps_Frame {
	boolean	_tracing;
};

#define TPS_FRAME_TRACE_LENGTH (sizeof(Tps_Frame_Trace))

/* Define the frame handler and creator */
EXTERNC Tps_Handler Tps_handler_trace;
EXTERNC Tps_Status Tps_create_trace(Tps_Interp*, boolean);

/**************************************************/
/*
Statemark Frame:
    Mark the exec stack to indicate the start
    of a transportable computation.
*/

struct Tps_Frame_Statemark : Tps_Frame {};

#define TPS_FRAME_STATEMARK_LENGTH (sizeof(Tps_Frame_Statemark))

/* Define the frame handler and creator */
EXTERNC Tps_Handler Tps_handler_statemark;
EXTERNC Tps_Status Tps_create_statemark(Tps_Interp*);

/**************************************************/
/*
safety Frame:
    Mark the exec stack and dict stacks
    and indicate the change in state of safety flag.
*/

struct Tps_Frame_Safety : Tps_Frame {
    boolean	_safe; /* previous flag state */
};

#define TPS_FRAME_SAFETY_LENGTH (sizeof(Tps_Frame_Safety))

/* Define the frame handler and creator */
EXTERNC Tps_Handler Tps_handler_safety;
EXTERNC Tps_Status Tps_create_safety(Tps_Interp*, boolean);

/**************************************************/
/*
OO Frame:
    track current self object.
*/

#if OO

struct Tps_Frame_OO : Tps_Frame {
    Tps_Value		 _self;
};

/* Define the frame handler and creator */
EXTERNC Tps_Handler Tps_handler_OO;
EXTERNC Tps_Status Tps_create_OO(Tps_Interp*, Tps_Value);
#endif /*OO*/

#endif /*TPS_EXEC_H*/
