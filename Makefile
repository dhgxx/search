# BSD makefile for project 'search'

.if defined(OSTYPE)
OSNAME=			${OSTYPE}
.else
OSNAME!=		uname -s
.endif

INSTALL_USER!=		id -n -u
OPT_DESTDIR=		/opt/local
OPT_BINDIR=		${OPT_DESTDIR}/bin
OPT_INCDIR=		${OPT_DESTDIR}/include
OPT_LIBDIR=		${OPT_DESTDIR}/lib
OPT_MANDIR=		${OPT_DESTDIR}/man

OPT_INC=		-I${OPT_INCDIR}
OPT_LIB=		-L${OPT_LIBDIR} -lmi

PROG=			search
MAN=			${PROG}.1
SRCS=			functions.c plan.c search.c
HDRS=			search.h
OBJS=			functions.o plan.o search.o

.if ${OSNAME} == "FreeBSD"
.if exists(/usr/bin/clang)
CC=				clang
.else
CC=				cc
.endif
BINGRP=			wheel
MFILE=			${MAN}.gz
MANDIR=			${OPT_MANDIR}/man1
MKWHATIS=		/usr/bin/makewhatis
.elif ${OSNAME} == "OpenBSD"
CC=				cc
BINGRP=			bin
MFILE=			${MAN:S/.1$/.cat0/g}
MANDIR=			${OPT_MANDIR}/cat1
MKWHATIS=		/usr/libexec/makewhatis
.endif

.if defined(CFLAGS)
MYCFLAGS=		-D_${OSNAME}_ ${CFLAGS} -Wall
.else
MYCFLAGS=		-D_${OSNAME}_ -O2 -pipe -fno-strict-aliasing -Wall
.endif

.if defined(DEBUG)
STRIP=
MYCFLAGS+=		-D_DEBUG_
.if defined(CC) && ${CC} == "clang"
MYCFLAGS=		-O0 -pipe -D_DEBUG_ -Wall
DEBUG_FLAGS=		-g
.else
MYCFLAGS=		-O -pipe -D_DEBUG_ -Wall
DEBUG_FLAGS=		-ggdb
.endif
.else
STRIP=			-s
DEBUG_FLAGS=
.endif

.if ${INSTALL_USER} == "root"
INST_TYPE=		sys-install
.else
INST_TYPE=		user-install
.endif

all: ${OBJS} ${PROG} makeman

install: all ${INST_TYPE}

sys-install: install-bin install-man

${OBJS}: ${SRCS} ${HDRS}
.for i in ${SRCS}
	${CC} ${MYCFLAGS} ${OPT_INC} ${DEBUG_FLAGS} -c $i
.endfor

${PROG}: ${SRCS} ${HDRS}
	${CC} ${MYCFLAGS} ${OPT_LIB} -o ${PROG} ${OBJS}

makeman:
.if ${OSNAME} == "OpenBSD"
. if !exists(${.CURDIR}/${PROG}.cat0)
	mandoc ${MAN} > ${PROG}.cat0
. endif
.elif ${OSNAME} == "FreeBSD"
. if !exists(${.CURDIR}/${MAN}.gz)
	gzip -cn ${MAN} > ${MAN}.gz
. endif
.endif

install-bin:
	${INSTALL} ${STRIP} -o root -g ${BINGRP} -m 0755 ${PROG} ${OPT_BINDIR}

install-man:
	${INSTALL} -o root -g ${BINGRP} -m 0444 ${MFILE} ${MANDIR}/${MFILE:S/.cat0$/.0/g}
	${MKWHATIS} ${OPT_MANDIR}

user-install:
	${INSTALL} ${STRIP} -o `id -u` -g `id -g` -m 0755 ${PROG} ${HOME}/bin

clean:
	rm -f ${PROG} *.o *.cat* *.gz
