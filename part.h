/*	Public domain	*/

#ifndef _CADTOOLS_PART_H_
#define _CADTOOLS_PART_H_

#include "begin_code.h"

#define CAD_PART_DESCR_MAX 256

typedef struct cad_part {
	struct ag_object obj;

	char descr[CAD_PART_DESCR_MAX];		/* Part description */
	Uint32 flags;
	SG *sg;					/* Rendering scene */
	SG_Object *so;				/* Generated polygonal object */
} CAD_Part;

__BEGIN_DECLS
extern AG_ObjectClass cadPartClass;
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_PART_H_ */
