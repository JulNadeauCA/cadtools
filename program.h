/*	Public domain	*/

#ifndef _CADTOOLS_PROGRAM_H_
#define _CADTOOLS_PROGRAM_H_

#include "begin_code.h"

enum cam_program_type {
	CAM_PROGRAM_FABBSD,	/* FabBSD program */
	CAM_PROGRAM_RS274NGC,	/* NIST RS274/NGC */
	CAM_PROGRAM_TYPE_LAST
};

typedef struct cam_program {
	struct ag_object obj;
	enum cam_program_type type;		/* Program language */
	Uint flags;
	char *text;				/* Program text */
	Uint textSize;				/* In bytes */
	AG_Textbox *tbText;
} CAM_Program;

__BEGIN_DECLS
extern AG_ObjectClass camProgramClass;
extern const char *camProgramTypeStrings[];
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_PROGRAM_H_ */
