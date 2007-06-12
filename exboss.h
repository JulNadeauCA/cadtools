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
void	  CAD_ExtrudedBossInit(void *, const char *);
int	  CAD_ExtrudedBossLoad(void *, AG_Netbuf *);
int	  CAD_ExtrudedBossSave(void *, AG_Netbuf *);
__END_DECLS

#include "close_code.h"
