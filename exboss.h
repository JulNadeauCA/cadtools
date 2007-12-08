/*	Public domain	*/

#include <freesg/sg.h>
#include <freesg/sg/sg_view.h>

#include "feature.h"

#include "begin_code.h"

typedef struct cad_extruded_boss {
	struct cad_feature feat;
	Uint flags;
} CAD_ExtrudedBoss;

__BEGIN_DECLS
extern AG_ObjectClass cadExtrudedBossClass;
__END_DECLS

#include "close_code.h"
