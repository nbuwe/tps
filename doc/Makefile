# Makefile generated by imake - do not edit!
# $XConsortium: imake.c,v 1.65 91/07/25 17:50:17 rws Exp $
#
# The cpp used on this machine replaces all newlines and multiple tabs and
# spaces in a macro expansion with a single space.  Imake tries to compensate
# for this, but is not always successful.
#

SUFFIX=doc
UPFIX=..

SHELL=/bin/sh

TARGETARCH=sun4
TARGETOS=solaris1

TARGETCC=gcc
TARGETCPLUS=g++
C=$(TARGETCC)
CC=$(TARGETCC)
CPLUS=$(TARGETCPLUS)

TARGET=$(TARGETARCH).$(TARGETOS)

TARGETSHORT=$(TARGETARCH)

SYSPREFIX=tps

SYSMAJOR=2
SYSMINOR=1
SYSPATCHLEVEL=a

SYSVERSION=$(SYSMAJOR).$(SYSMINOR)

SYSNAME=$(SYSPREFIX)$(SYSVERSION)

SYSFULLNAME=$(SYSNAME)$(SYSPATCHLEVEL)

SYSHOME=/tmp_mnt/cs/research/arcadia/csci7818/dennis/tps2.0

MAKE=make
AR=ar

RANLIB=ranlib
RANLIBT= $(RANLIB) -t

MKDIRS=mkdir -p

IMAKE=imake
MAKEDEPEND=makedepend

########## OVERRIDES ##########

SYSTARGET=$(SYSHOME)/$(TARGET)
SYSTARGETSHORT=$(SYSHOME)/$(TARGETSHORT)
SYSBIN=$(SYSTARGET)/bin
SYSLIBDIR=$(SYSTARGET)/lib
SYSINCLUDES=$(SYSTARGET)/include

INSTALLDIR=$(SYSTARGET)

STAR=*

SRCTARNAME=$(SYSFULLNAME).tar

THISDIRREL=$(UPFIX)/$(SUFFIX)
THISDIRABS=$(SYSHOME)/$(SUFFIX)

SRCTARLIST=$(THISDIRABS)/INVENTORY

TARPREFIX=$(SUFFIX)

All: all

propagate:

imakeall:

tagversion:

depend:

install:

Clean:

RCS:
	-mkdir RCS

LATEX=latex
DVIPS=dvips

.SUFFIXES: .tex .dvi .ps

INPUTS=cuarcadia.sty \
	exec.code global.code handler.code imethods.code \
        interp.code load.code run.code \
        sframe.code tframe.code value.code \
	trace.id

TEXSRC=$(SYSPREFIX).tex

TEXDVI=$(TEXSRC:.tex=.dvi)

TEXPS=$(TEXSRC:.tex=.ps)

MISCSRC=Imakefile Imake.tmpl README

# Try to pass along the .ps file if possible.
SRC=$(TEXSRC) $(INPUTS) \
    $(TEXPS)

##################################################
# Disable all.

all:

ps $(TEXPS)::
	latex $(TEXSRC)
	bibtex $(SYSPREFIX)
	latex $(TEXSRC)
	latex $(TEXSRC)
	$(DVIPS) -o $(TEXPS) $(TEXDVI)
clean:
	rm -f *.dvi *.aux *.log *.bbl *.blg

