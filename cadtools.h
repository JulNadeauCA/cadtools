#ifndef _CADTOOLS_H_
#define _CADTOOLS_H_

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
#endif /* _CADTOOLS_INTERNAL */

#include "program.h"
#include "machine.h"
#include "part.h"
#include "feature.h"

#endif /* _CADTOOLS_H_ */
