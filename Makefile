# BSD makefile for project 'search'

.if defined(OSTYPE)
OSNAME=			${OSTYPE}
.else
OSNAME!=		uname -s
.endif

INSTALL_USER!=		id -n -u
OPT_DESTDIR=		/opt/local
OPT_BINDIR=		${OPT_DESTDIR}/bin
OPT_LIBDIR=		${OPT_DESTDIR}/lib
OPT_MANDIR=		${OPT_DESTDIR}/man

CC=			cc

PROG=			search
MAN=			${PROG}.1
SRCS=			dlist.c functions.c search.c
HDRS=			dlist.h search.h
OBJS=			dlist.o functions.o search.o

.if defined(CFLAGS)
MYCFLAGS=		${CFLAGS} -D_${OSNAME}_
.else
MYCFLAGS=		-O2 -pipe -D_{OSNAME}_
.endif

.if defined(DEBUG)
STRIP=
MYCFLAGS+=		-ggdb -D_DEBUG_
.else
STRIP=			-s
.endif

.if ${OSNAME} == "FreeBSD"
BINGRP=			wheel
MFILE=			${MAN}.gz
MANDIR=			${OPT_MANDIR}/man1
MKWHATIS=		/usr/bin/makewhatis
.elif ${OSNAME} == "OpenBSD"
BINGRP=			bin
MFILE=			${MAN:S/.1$/.cat0/g}
MANDIR=			${OPT_MANDIR}/cat1
MKWHATIS=		/usr/libexec/makewhatis
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
	${CC} ${MYCFLAGS} -c $i
.endfor

${PROG}: ${SRCS} ${HDRS}
	${CC} ${MYCFLAGS} -o ${PROG} ${OBJS}

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
