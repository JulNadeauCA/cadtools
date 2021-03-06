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

	M_Real bedSwing;		/* Swing over Bed */
	M_Real csSwing;		/* Swing over Cross Slide */
	M_Real csTravel;		/* Cross Slide travel */
	M_Real carrTravel;		/* Carriage travel */
	M_Real compTravel;		/* Compound travel */
	M_Real distCenters;		/* Distance between centers */

	M_Real     spBore;		/* Spindle thru-hole diameter */
	int        spMinRPM;		/* Minimum spindle RPM */
	int        spMaxRPM;		/* Maximum spindle RPM */
	M_Real     spPower;		/* Spindle power (watts) */
	CAM_Chuck *spChuck;		/* Spindle chuck model */

	M_Real     tsTravel;		/* Tailstock spindle travel */
	CAM_Chuck *tsChuck;		/* Tailstock chuck model */
} CAM_Lathe;

__BEGIN_DECLS
extern AG_ObjectClass camLatheClass;
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_LATHE_H_ */
