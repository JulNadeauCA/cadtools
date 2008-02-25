#ifndef _CADTOOLS_H_
#define _CADTOOLS_H_

#include <freesg/sg.h>
#include <freesg/sg/sg_view.h>
#include <agar/net.h>

#ifdef _CADTOOLS_INTERNAL

#include <agar/config/_mk_have_unsigned_typedefs.h>
#ifndef _MK_HAVE_UNSIGNED_TYPEDEFS
#define _MK_HAVE_UNSIGNED_TYPEDEFS
typedef unsigned int Uint;
typedef unsigned int Uchar;
typedef unsigned long Ulong;
#endif

#include <config/enable_nls.h>
#ifdef ENABLE_NLS
# include <libintl.h>
# define _(String) dgettext("cadtools",String)
# ifdef dgettext_noop
#  define N_(String) dgettext_noop("cadtools",String)
# else
#  define N_(String) (String)
# endif
#else
# undef _
# undef N_
# undef ngettext
# define _(s) (s)
# define N_(s) (s)
# define ngettext(Singular,Plural,Number) ((Number==1)?(Singular):(Plural))
#endif

#ifdef WIN32
# define PATHSEPC '\\'
# define PATHSEP "\\"
#else
# define PATHSEPC '/'
# define PATHSEP "/"
#endif
#endif /* _CADTOOLS_INTERNAL */

#include "program.h"

#include "fixture.h"
#include "machine.h"
#include "lathe.h"
#include "mill.h"

#include "part.h"
#include "feature.h"

#include "begin_code.h"
__BEGIN_DECLS
struct ag_menu;
extern struct ag_menu *appMenu;
extern AG_Object vfsRoot;

void       CAD_SetArchivePath(void *, const char *);
AG_Window *CAD_CreateEditionWindow(void *);
__END_DECLS
#include "close_code.h"

#endif /* _CADTOOLS_H_ */
