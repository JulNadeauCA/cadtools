/*	Public domain	*/

#ifndef _CADTOOLS_FEATURE_H_
#define _CADTOOLS_FEATURE_H_

#include <agar/sg.h>
#include <agar/sg/sg_view.h>

#include "begin_code.h"

typedef struct cad_feature {
	struct ag_object obj;
	Uint flags;
#define CAD_FEATURE_SUPPRESS	0x01			/* Inactive */

	TAILQ_ENTRY(cad_feature) features;
} CAD_Feature;

__BEGIN_DECLS
extern const AG_ObjectOps cadFeatureOps;
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_FEATURE_H_ */
