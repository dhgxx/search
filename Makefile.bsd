# BSD makefile for project 'search'

PROG=search
SRCS=search.c functions.c

BINDIR=${HOME}/bin
BINOWN=${LOGNAME}
BINMODE=0755
NO_MAN=true
STRIP=-s

.if !defined(.OBJDIR)
.OBJDIR=.
.endif

.if !defined(OSNAME)
.if defined(OSTYPE)
OSNAME=${OSTYPE}
.else
OSNAME=`uname -s`
.endif
.endif

.if defined(DEBUG)
DEBUG_FLAGS+=-ggdb
CFLAGS+=-D__DEBUG__
.endif

.if defined(CFLAGS)
CFLAGS+=-D_${OSNAME}_
.else
CFLAGS=-O2 -pipe -D_${OSNAME}_
.endif

.if !defined(NOMAN)
NOMAN=${NO_MAN}
.endif

.if defined(GROUP)
BINGRP=${GROUP}
.else
BINGRP=`id -g`
.endif

.include <bsd.prog.mk>
