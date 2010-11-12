# BSD makefile for project 'search'

.if defined(OSTYPE)
OSNAME=${OSTYPE}
.else
OSNAME!=uname -s
.endif

PROG=search
SRCS=search.c functions.c
MAN=search.1

DESTDIR=/opt/local
BINDIR=/bin
MANDIR=/man/man

BINOWN=root
BINGRP=wheel
BINMODE=0755
NO_OBJ=true
STRIP=-s
OPT_CFLAGS=-I${DESTDIR}/include
OPT_LDFLAGS=-L${DESTDIR}/lib -lmi

.if ${OSNAME} == "OpenBSD"
BINGRP=bin
.endif

.if defined(DEBUG)
DEBUG_FLAGS+=-ggdb
CFLAGS+=-D__DEBUG__
.endif

.if defined(CFLAGS)
CFLAGS+=-D_${OSNAME}_
CFLAGS+=${OPT_CFLAGS}
.else
CFLAGS=-O2 -pipe -D_${OSNAME}_
CFLAGS+=${OPT_CFLAGS}
.endif

.if defined(LDFLAGS)
LDFLAGS+=${OPT_LDFLAGS}
.else
LDFLAGS=${OPT_LDFLAGS}
.endif

user-install:
	${INSTALL} -o `id -u` -g `id -g` -m ${BINMODE} ${.CURDIR}/${PROG} ${HOME}${BINDIR}/${PROG}

.include <bsd.prog.mk>
