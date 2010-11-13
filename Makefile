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

CC=			cc
OPT_INC=		-I${OPT_INCDIR}
OPT_LIB=		-L${OPT_LIBDIR} -lmi

PROG=			search
MAN=			${PROG}.1
SRCS=			functions.c search.c
HDRS=			search.h
OBJS=			functions.o search.o

.if defined(CFLAGS)
MYCFLAGS=		${CFLAGS} -D_${OSNAME}_
.else
MYCFLAGS=		-O2 -pipe -D_{OSNAME}_
.endif

.if ${OSNAME} == "FreeBSD"
BINGRP=			wheel
MFILE=			${MAN}.gz
MANDIR=			${OPT_MANDIR}/man1
.elif ${OSNAME} == "OpenBSD"
BINGRP=			bin
MFILE=			${MAN:S/.1$/.cat0/g}
MANDIR=			${OPT_MANDIR}/cat1
.endif

.if ${INSTALL_USER} == "root"
INST_TYPE=		sys-install
.else
INST_TYPE=		user-install
.endif

all: ${OBJS} ${PROG} makeman

install: ${INST_TYPE}

${OBJS}: ${SRCS} ${HDRS}
.for i in ${SRCS}
	${CC} ${MYCFLAGS} ${OPT_INC} -c $i
.endfor

${PROG}:
	${CC} ${MYCFLAGS} ${OPT_LIB} -o ${PROG} ${OBJS}

makeman:
.if ${OSNAME} == "OpenBSD"
	mandoc ${MAN} > ${PROG}.cat0
.elif ${OSNAME} == "FreeBSD"
	gzip -cn ${MAN} > ${MAN}.gz
.endif

sys-install:
	${INSTALL} -o root -g ${BINGRP} -m 0755 ${PROG} ${OPT_BINDIR}
	${INSTALL} -o root -g ${BINGRP} -m 0444 ${MFILE} ${MANDIR}
	ldconfig -m ${OPT_LIBDIR}
	makewhatis ${OPT_MANDIR}

user-install:
	${INSTALL} -o `id -u` -g `id -g` -m 0755 ${PROG} ${HOME}/bin

clean:
	rm -f ${PROG} *.o *.cat* *.gz

