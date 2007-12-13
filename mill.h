/*	Public domain	*/

#ifndef _CADTOOLS_MILL_H_
#define _CADTOOLS_MILL_H_

#include "begin_code.h"

typedef struct cam_mill {
	struct cam_machine _inherit;
	Uint flags;
#define CAM_MILL_SPINDLE_SWITCH		0x001	/* Spindle on/off control */
#define CAM_MILL_SPINDLE_SPEED		0x002	/* Spindle speed control */
#define CAM_MILL_SPINDLE_TACHO		0x004	/* Spindle tachometer readout */
#define CAM_MILL_SYNC_TO_SPINDLE	0x008	/* Sync program with IRQ from
						   spindle tachometer */
#define CAM_MILL_COOL_MIST		0x010	/* Mist coolant */
#define CAM_MILL_COOL_FLOOD		0x020	/* Flood coolant */
#define CAM_MILL_COOL_VORTEX		0x040	/* Vortex tube */
#define CAM_MILL_XLIMIT			0x080	/* X limit switches */
#define CAM_MILL_YLIMIT			0x100	/* Y limit switches */
#define CAM_MILL_ZLIMIT			0x200	/* Z limit switches */

	SG_Real lenTable;		/* Table length */
	SG_Real wTable;			/* Table width */
	SG_Real xTravel;		/* Longitudinal travel */
	SG_Real yTravel;		/* Cross-slide travel */
	SG_Real zTravel;		/* Cross-slide travel */

	int	spMinRPM;		/* Minimum spindle RPM */
	int	spMaxRPM;		/* Maximum spindle RPM */
	SG_Real	spPower;		/* Spindle power (watts) */
	CAM_Chuck *spChuck;		/* Spindle chuck model */
} CAM_Mill;

__BEGIN_DECLS
extern AG_ObjectClass camMillClass;
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_MILL_H_ */
