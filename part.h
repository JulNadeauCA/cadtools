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
	AG_TAILQ_HEAD(,cad_feature) features;	/* Source features */
} CAD_Part;

__BEGIN_DECLS
extern AG_ObjectClass cadPartClass;

void CAD_PartInsertFeature(AG_Event *);
void CAD_PartSaveMenu(AG_FileDlg *, CAD_Part *);
void CAD_PartOpenMenu(AG_FileDlg *);
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_PART_H_ */
