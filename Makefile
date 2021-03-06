TOP=	.
include Makefile.config
include Makefile.proj

PROG=		cadtools
PROG_TYPE=	"GUI"
PROG_GUID=	"666aff06-980e-432d-ad56-af1c868e7ec4"
PROG_LINKS=	ag_core ag_gui ag_map ag_rg ag_vg ag_dev \
		pthreads SDL SDLmain opengl freetype

SRCS=	cadtools.c part.c feature.c exboss.c program.c machine.c lathe.c \
	mill.c

CFLAGS+=${AGAR_SK_CFLAGS} ${AGAR_SG_CFLAGS} ${AGAR_MATH_CFLAGS} \
        ${AGAR_DEV_CFLAGS} ${AGAR_CFLAGS} ${GETTEXT_CFLAGS}
LIBS+=	${AGAR_SK_LIBS} ${AGAR_SG_LIBS} ${AGAR_MATH_LIBS} \
        ${AGAR_DEV_LIBS} ${AGAR_LIBS} ${GETTEXT_LIBS}

SUBDIR= po

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

include ${TOP}/mk/build.prog.mk
