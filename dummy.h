/*	Public domain	*/

#ifndef _CADTOOLS_DUMMY_H_
#define _CADTOOLS_DUMMY_H_

#include <freesg/sg.h>
#include <freesg/sg/sg_view.h>

#include "begin_code.h"

typedef struct cad_dummy {
	struct ag_object obj;
	Uint flags;
} CAD_Dummy;

__BEGIN_DECLS
extern AG_ObjectClass cadDummyClass;

CAD_Dummy *CAD_DummyNew(void *, const char *);
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_DUMMY_H_ */
