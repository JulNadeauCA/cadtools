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

#include <agar/core.h>

#include "cadtools.h"
#include "exboss.h"

static void
Init(void *obj, const char *name)
{
	CAD_ExtrudedBoss *exboss = obj;

	exboss->flags = 0;
}

static int
Load(void *obj, AG_DataSource *buf)
{
	CAD_ExtrudedBoss *exboss = obj;

	if (AG_ReadObjectVersion(buf, exboss, NULL) != 0) {
		return (-1);
	}
	exboss->flags = (Uint)AG_ReadUint32(buf);
	return (0);
}

static int
Save(void *obj, AG_DataSource *buf)
{
	CAD_ExtrudedBoss *exboss = obj;

	AG_WriteVersion(buf, cadExtrudedBossOps.type, &cadExtrudedBossOps.ver);
	AG_WriteUint32(buf, (Uint32)exboss->flags);
	return (0);
}

const AG_ObjectOps cadExtrudedBossOps = {
	"CAD_Feature:CAD_ExtrudedBoss",
	sizeof(CAD_ExtrudedBoss),
	{ 0,0 },
	Init,
	NULL,			/* reinit */
	NULL,			/* destroy */
	Load,
	Save,
	NULL			/* edit */
};
