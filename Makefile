# BSD makefile for project 'search'

.if defined(OSTYPE)
OSNAME= 	    ${OSTYPE}
.else
OSNAME!=    	uname -s
.endif

INSTALL_USER!=	id -n -u
OPT_DESTDIR=	/opt/local

CC=				cc
OPT_INC=		-I/opt/local/include
OPT_LIB=		-L/opt/local/lib -lmi
CFLAGS=			-O2 -pipe -D_${OSNAME}_

PROG=			search
MAN=			${PROG}.1
SRCS=			functions.c search.c
OBJS=			functions.o search.o

.if ${OSNAME} == "FreeBSD"
BINGRP=			wheel
MFILE=			${MAN}.gz
BINDIR=			${OPT_DESTDIR}/bin
MANDIR=			${OPT_DESTDIR}/man/man1
.elif ${OSNAME} == "OpenBSD"
BINGRP=			bin
MFILE=			${MAN:S/.1$/.cat0/g}
BINDIR=			${OPT_DESTDIR}/bin
MANDIR=			${OPT_DESTDIR}/man/cat1
.endif

.if ${INSTALL_USER} == "root"
INST_TYPE=		sys-install
.else
INST_TYPE=		user-install
.endif

all: ${OBJS} ${PROG} makeman

install: ${INST_TYPE}

${OBJS}:
.for i in ${SRCS}
	${CC} ${CFLAGS} ${OPT_INC} -c $i
.endfor

${PROG}:
	${CC} ${CFLAGS} ${OPT_LIB} -o ${PROG} ${OBJS}

makeman:
.if ${OSNAME} == "OpenBSD"
	mandoc ${MAN} > ${PROG}.cat0
.elif ${OSNAME} == "FreeBSD"
	gzip -cn ${MAN} > ${MAN}.gz
.endif

sys-install:
	${INSTALL} -o root -g ${BINGRP} -m 0755 ${PROG} ${BINDIR}
	${INSTALL} -o root -g ${BINGRP} -m 0444 ${MFILE} ${MANDIR}

user-install:
	${INSTALL} -o `id -u` -g `id -g` -m 0755 ${PROG} ${HOME}/bin

clean:
	rm -f *.o *.cat* *.gz

