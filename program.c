/*
 * Copyright (c) 2007-2010 Hypertriton, Inc. <http://hypertriton.com>
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

#include "cadtools.h"

const char *camProgramTypeStrings[] = {
	"FabBSD",
	"NIST RS274/NGC",
	NULL
};

static void
Init(void *obj)
{
	CAM_Program *prog = obj;

	prog->type = CAM_PROGRAM_FABBSD;
	prog->flags = 0;
	prog->text = Strdup("/* FabBSD program */\n");
	prog->textSize = strlen(prog->text)+1;
	prog->tbText = NULL;
}

static void
Destroy(void *obj)
{
	CAM_Program *prog = obj;

	Free(prog->text);
}

static int
Load(void *obj, AG_DataSource *ds, const AG_Version *ver)
{
	CAM_Program *prog = obj;
	char *textNew;
	size_t sizeNew;

	prog->type = (enum cam_program_type)AG_ReadUint8(ds);
	prog->flags = (Uint)AG_ReadUint32(ds);
	sizeNew = (size_t)AG_ReadUint32(ds);
	if ((textNew = TryRealloc(prog->text, sizeNew)) == NULL) {
		return (-1);
	}
	prog->text = textNew;
	prog->textSize = sizeNew;
	AG_CopyString(prog->text, ds, prog->textSize);

	if (prog->tbText != NULL) {
		AG_TextboxBindAutoASCII(prog->tbText, &prog->text,
		    &prog->textSize);
	}
	return (0);
}

static int
Save(void *obj, AG_DataSource *ds)
{
	CAM_Program *prog = obj;

	AG_WriteUint8(ds, (Uint8)prog->type);
	AG_WriteUint32(ds, (Uint32)prog->flags);
	AG_WriteUint32(ds, (Uint32)prog->textSize);
	AG_WriteString(ds, prog->text);

	return (0);
}

static void *
Edit(void *obj)
{
	CAM_Program *prog = obj;
	AG_Window *win;
	AG_Combo *com;
	int i;

	win = AG_WindowNew(0);

	AG_WindowSetCaption(win, _("Program: %s"), AGOBJECT(prog)->name);
	prog->tbText = AG_TextboxNew(win, AG_TEXTBOX_MULTILINE, NULL);
	AG_Expand(prog->tbText);
	AG_TextboxBindAutoASCII(prog->tbText, &prog->text, &prog->textSize);
	
	AG_LabelNew(win, 0, _("Program type:"));
	AG_RadioNewUint(win, 0, camProgramTypeStrings, &prog->type);

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 60, 50);
	return (win);
}

AG_ObjectClass camProgramClass = {
	"CAM_Program",
	sizeof(CAM_Program),
	{ 0,0 },
	Init,
	NULL,
	Destroy,
	Load,
	Save,
	Edit
};
