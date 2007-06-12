/*
 * Copyright (c) 2006 Hypertriton, Inc. <http://www.hypertriton.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <agar/core/core.h>
#include <agar/gui.h>

#include <stdarg.h>
#include <string.h>

#include "dummy.h"

#include <GL/gl.h>
#include <GL/glu.h>

const AG_ObjectOps cadDummyOps = {
	"CAD_Dummy",
	sizeof(CAD_Dummy),
	{ 0,0 },
	CAD_DummyInit,
	NULL,			/* reinit */
	CAD_DummyDestroy,
	CAD_DummyLoad,
	CAD_DummySave,
	CAD_DummyEdit
};

CAD_Dummy *
CAD_DummyNew(void *parent, const char *name)
{
	CAD_Dummy *dummy;

	dummy = Malloc(sizeof(CAD_Dummy), M_OBJECT);
	CAD_DummyInit(dummy, name);
	AG_ObjectAttach(parent, dummy);
	return (dummy);
}

void
CAD_DummyInit(void *obj, const char *name)
{
	CAD_Dummy *dummy = obj;

	AG_ObjectInit(dummy, name, &cadDummyOps);
	dummy->flags = 0;
}

void
CAD_DummyDestroy(void *obj)
{
}

int
CAD_DummyLoad(void *obj, AG_Netbuf *buf)
{
	CAD_Dummy *dummy = obj;

	if (AG_ReadVersion(buf, cadDummyOps.type, &cadDummyOps.ver, NULL)
	    != 0) {
		return (-1);
	}
	dummy->flags = (Uint)AG_ReadUint32(buf);
	return (0);
}

int
CAD_DummySave(void *obj, AG_Netbuf *buf)
{
	CAD_Dummy *dummy = obj;

	AG_WriteVersion(buf, cadDummyOps.type, &cadDummyOps.ver);
	AG_WriteUint32(buf, (Uint32)dummy->flags);
	return (0);
}

void *
CAD_DummyEdit(void *obj)
{
	CAD_Dummy *dummy = obj;
	AG_Window *win;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Dummy: %s"), AGOBJECT(dummy)->name);
	return (win);
}
