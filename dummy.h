/*	Public domain	*/

#ifndef _CADTOOLS_DUMMY_H_
#define _CADTOOLS_DUMMY_H_

#include <agar/sg.h>
#include <agar/sg/sg_view.h>

#include "begin_code.h"

typedef struct cad_dummy {
	struct ag_object obj;
	Uint flags;
} CAD_Dummy;

__BEGIN_DECLS
CAD_Dummy *CAD_DummyNew(void *, const char *);
void	  CAD_DummyInit(void *, const char *);
void	  CAD_DummyReinit(void *);
void	  CAD_DummyDestroy(void *);
void	 *CAD_DummyEdit(void *);
int	  CAD_DummyLoad(void *, AG_Netbuf *);
int	  CAD_DummySave(void *, AG_Netbuf *);
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_DUMMY_H_ */
