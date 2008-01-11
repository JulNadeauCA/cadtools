/*	Public domain	*/

#ifndef _CADTOOLS_FIXTURE_H_
#define _CADTOOLS_FIXTURE_H_

#include "begin_code.h"

/* Circular chuck model */
typedef struct cam_chuck {
	SG_Real oallDia;		/* Overall diameter */
	SG_Real minDia;			/* Minimum part diameter */
	SG_Real maxDia;			/* Maximum part diameter */
	SG_Real	oallDepth;		/* Overall depth */;
	SG_Real teethDepth;		/* Teeth depth */
	int	nTeeth;			/* Teeth count */
} CAM_Chuck;

#include "close_code.h"
#endif	/* _CADTOOLS_FIXTURE_H_ */
