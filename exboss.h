/*	Public domain	*/

#include <agar/sg.h>
#include <agar/sg/sg_view.h>

#include "feature.h"

#include "begin_code.h"

typedef struct cad_extruded_boss {
	struct cad_feature feat;
	Uint flags;
} CAD_ExtrudedBoss;

__BEGIN_DECLS
extern const AG_ObjectClass cadExtrudedBossClass;
__END_DECLS

#include "close_code.h"
