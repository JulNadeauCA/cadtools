#	$Csoft: Makefile,v 1.1 2005/09/28 15:46:13 vedge Exp $

TOP=	.
include Makefile.config

PROG=	cadtools
SRCS=	cadtools.c part.c feature.c exboss.c program.c machine.c

CFLAGS+=${AGAR_CFLAGS} ${AGAR_SG_CFLAGS} -I.
LIBS+=	${AGAR_LIBS} ${AGAR_SG_LIBS}

all: all-subdir ${PROG}
clean: clean-prog clean-subdir
cleandir: clean-prog clean-subdir cleandir-prog cleandir-config cleandir-subdir
install: install-prog install-subdir
deinstall: deinstall-prog deinstall-subdir
depend: depend-subdir
regress: regress-subdir

configure: configure.in
	cat configure.in | manuconf > configure
	chmod 755 configure

cleandir-config:
	rm -fr config config.log

package: cleandir
	sh mk/dist.sh

release: cleandir
	sh mk/dist.sh commit

.PHONY: clean cleandir install deinstall depend regress
.PHONY: configure cleandir-config package release

include ${TOP}/mk/csoft.prog.mk
