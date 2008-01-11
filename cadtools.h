#ifndef _CADTOOLS_H_
#define _CADTOOLS_H_

#include <freesg/sg.h>
#include <freesg/sg/sg_view.h>
#include <agar/net.h>

#ifdef _CADTOOLS_INTERNAL

#include <agar/core/strlcpy.h>
#include <agar/core/strlcat.h>

#define Strlcpy AG_Strlcpy
#define Strlcat AG_Strlcat

#include <agar/config/_mk_have_unsigned_typedefs.h>
#ifndef _MK_HAVE_UNSIGNED_TYPEDEFS
#define _MK_HAVE_UNSIGNED_TYPEDEFS
typedef unsigned int Uint;
typedef unsigned int Uchar;
typedef unsigned long Ulong;
#endif

#include <agar/config/enable_nls.h>
#ifdef ENABLE_NLS
# include <agar/libintl/libintl.h>
# define _(String) gettext(String)
# define gettext_noop(String) (String)
# define N_(String) gettext_noop(String)
#else
# undef _
# undef N_
# undef textdomain
# undef bindtextdomain
# define _(s) (s)
# define N_(s) (s)
# define textdomain(d)
# define bindtextdomain(p, d)
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
