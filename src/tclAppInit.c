/* 
 * tclAppInit.c --
 *
 *	Provides a default version of the Tcl_AppInit procedure.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#ifndef lint
static char rcsid[] = "$Header: /user6/ouster/tcl/RCS/tclAppInit.c,v 1.6 93/08/26 14:34:55 ouster Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "tcl.h"

#if defined(__GNUC__) && defined(__osf__)
/* The include files in osf1 are flawed */
typedef long clockid_t;
#endif

#include <sys/time.h>
#include <time.h>
#include <sys/resource.h>

EXTERN int gettimeofday _ANSI_ARGS_((struct timeval*,struct timezone*));
EXTERN int getrusage _ANSI_ARGS_((int,struct rusage*));

EXTERN int UsertimeCmd _ANSI_ARGS_((ClientData,Tcl_Interp*,int,char**));
EXTERN int RealtimeCmd _ANSI_ARGS_((ClientData,Tcl_Interp*,int,char**));
EXTERN int NoopCmd _ANSI_ARGS_((ClientData,Tcl_Interp*,int,char**));

/*
 * The following variable is a special hack that allows applications
 * to be linked using the procedure "main" from the Tcl library.  The
 * variable generates a reference to "main", which causes main to
 * be brought in from the library (and all of Tcl with it).
 */

extern int main();
int *tclDummyMainPtr = (int *) main;

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp->result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{
    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

    if (Tcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */

    Tcl_CreateCommand(interp,"usertime",UsertimeCmd,(ClientData)0,0);
    Tcl_CreateCommand(interp,"realtime",RealtimeCmd,(ClientData)0,0);
    Tcl_CreateCommand(interp,"noop",NoopCmd,(ClientData)0,0);

    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */

    tcl_RcFileName = "~/.tclshrc";
    return TCL_OK;
}


int
UsertimeCmd(
    ClientData cd,
    Tcl_Interp *interp,			/* current interpreter. */
    int argc,				/* number of arguments. */
    char **argv)			/* argument strings. */
{
    register long timediff;
    struct rusage r;

    if(getrusage(RUSAGE_SELF,&r) < 0) {
	interp->result = "unix getrusage operation failed";
	return TCL_ERROR;
    }
    timediff = r.ru_utime.tv_sec * 1000;
    timediff += r.ru_utime.tv_usec / 1000;
    sprintf(interp->result,"%d",timediff);
    return TCL_OK;
}

int
RealtimeCmd(
    ClientData cd,
    Tcl_Interp *interp,			/* current interpreter. */
    int argc,				/* number of arguments. */
    char **argv)			/* argument strings. */
{
    register long timediff;
    struct timeval t;

    if(gettimeofday(&t,(struct timezone*)0) < 0) {
	interp->result = "unix gettimeofday operation failed";
	return TCL_ERROR;
    }
    timediff = t.tv_sec * 1000;
    timediff += t.tv_usec / 1000;
    sprintf(interp->result,"%d",timediff);
    return TCL_OK;
}

int
NoopCmd(
    ClientData cd,
    Tcl_Interp *interp,			/* current interpreter. */
    int argc,				/* number of arguments. */
    char **argv)			/* argument strings. */
{
    return TCL_OK;
}
