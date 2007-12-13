/*
 * Copyright (c) 2006-2007 Hypertriton, Inc. <http://hypertriton.com>
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

#include <agar/core.h>
#include <agar/gui.h>

#include <stdarg.h>
#include <string.h>

#include "dummy.h"

#include <GL/gl.h>
#include <GL/glu.h>

CAD_Dummy *
CAD_DummyNew(void *parent, const char *name)
{
	CAD_Dummy *dummy;

	dummy = Malloc(sizeof(CAD_Dummy));
	AG_ObjectInit(dummy, name, &cadDummyClass);
	AG_ObjectAttach(parent, dummy);
	return (dummy);
}

static void
Init(void *obj, const char *name)
{
	CAD_Dummy *dummy = obj;

	dummy->flags = 0;
}

static void
Destroy(void *obj)
{
}

static int
Load(void *obj, AG_DataSource *buf, const AG_Version *ver)
{
	CAD_Dummy *dummy = obj;

	dummy->flags = (Uint)AG_ReadUint32(buf);
	return (0);
}

static int
Save(void *obj, AG_DataSource *buf)
{
	CAD_Dummy *dummy = obj;

	AG_WriteUint32(buf, (Uint32)dummy->flags);
	return (0);
}

static void *
Edit(void *obj)
{
	CAD_Dummy *dummy = obj;
	AG_Window *win;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Dummy: %s"), AGOBJECT(dummy)->name);
	return (win);
}

AG_ObjectClass cadDummyClass = {
	"CAD_Dummy",
	sizeof(CAD_Dummy),
	{ 0,0 },
	Init,
	NULL,			/* reinit */
	Destroy,
	Load,
	Save,
	Edit
};
