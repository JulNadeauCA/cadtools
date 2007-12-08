TOP=	.
include Makefile.config

PROJECT=	"cadtools"
PROJINCLUDES=	configure.lua

PROG=		cadtools
PROG_TYPE=	"GUI"
PROG_GUID=	""
PROG_LINKS=	ag_core ag_gui ag_map ag_rg ag_vg ag_dev \
		pthreads SDL SDLmain opengl freetype

SRCS=	cadtools.c part.c feature.c exboss.c program.c machine.c

CFLAGS+=${AGAR_CFLAGS} ${AGAR_DEV_CFLAGS} ${FREESG_CFLAGS}
LIBS+=	${AGAR_LIBS} ${AGAR_DEV_LIBS} ${FREESG_LIBS}

all: all-subdir ${PROG}
clean: clean-prog clean-subdir
cleandir: clean-prog clean-subdir cleandir-prog cleandir-config cleandir-subdir
install: install-prog install-subdir
deinstall: deinstall-prog deinstall-subdir
depend: depend-subdir
regress: regress-subdir

configure: configure.in
	cat configure.in | mkconfigure > configure
	chmod 755 configure

cleandir-config:
	rm -fr config config.log

snapshot: cleandir
	sh mk/dist.sh snapshot

package: cleandir
	sh mk/dist.sh

release: cleandir
	sh mk/dist.sh commit

.PHONY: clean cleandir install deinstall depend regress
.PHONY: configure cleandir-config package release

include ${TOP}/mk/csoft.prog.mk
