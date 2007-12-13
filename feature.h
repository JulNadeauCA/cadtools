/*	Public domain	*/

#ifndef _CADTOOLS_FEATURE_H_
#define _CADTOOLS_FEATURE_H_

#include "begin_code.h"

typedef struct cad_feature {
	struct ag_object obj;
	Uint flags;
#define CAD_FEATURE_SUPPRESS	0x01			/* Inactive */

	AG_TAILQ_ENTRY(cad_feature) features;
} CAD_Feature;

__BEGIN_DECLS
extern AG_ObjectClass cadFeatureClass;
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_FEATURE_H_ */
