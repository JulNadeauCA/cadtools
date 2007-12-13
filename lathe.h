/*	Public domain	*/

#ifndef _CADTOOLS_LATHE_H_
#define _CADTOOLS_LATHE_H_

#include "begin_code.h"

typedef struct cam_lathe {
	struct cam_machine _inherit;
	Uint flags;
#define CAM_LATHE_SPINDLE_SWITCH	0x01	/* Spindle on/off control */
#define CAM_LATHE_SPINDLE_SPEED		0x02	/* Spindle speed control */
#define CAM_LATHE_SPINDLE_TACHO		0x04	/* Spindle tachometer readout */
#define CAM_LATHE_SYNC_TO_SPINDLE	0x08	/* Sync program with IRQ from
						   spindle tachometer */

	SG_Real	bedSwing;		/* Swing over Bed */
	SG_Real	csSwing;		/* Swing over Cross Slide */
	SG_Real	csTravel;		/* Cross Slide travel */
	SG_Real	carrTravel;		/* Carriage travel */
	SG_Real compTravel;		/* Compound travel */
	SG_Real	distCenters;		/* Distance between centers */

	SG_Real	spBore;			/* Spindle thru-hole diameter */
	int	spMinRPM;		/* Minimum spindle RPM */
	int	spMaxRPM;		/* Maximum spindle RPM */
	SG_Real	spPower;		/* Spindle power (watts) */
	CAM_Chuck *spChuck;		/* Spindle chuck model */

	SG_Real   tsTravel;		/* Tailstock spindle travel */
	CAM_Chuck *tsChuck;		/* Tailstock chuck model */
} CAM_Lathe;

__BEGIN_DECLS
extern AG_ObjectClass camLatheClass;
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_LATHE_H_ */
