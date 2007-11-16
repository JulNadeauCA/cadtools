/*	Public domain	*/

#ifndef _CADTOOLS_PART_H_
#define _CADTOOLS_PART_H_

#include <agar/sg.h>
#include <agar/sg/sg_view.h>

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
extern const AG_ObjectClass cadPartClass;

CAD_Part *CAD_PartNew(void *, const char *);
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_PART_H_ */
