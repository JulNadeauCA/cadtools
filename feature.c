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
#include "part.h"
#include "feature.h"

const AG_ObjectOps cadFeatureOps = {
	"CAD_Feature",
	sizeof(CAD_Feature),
	{ 0,0 },
	CAD_FeatureInit,
	NULL,			/* reinit */
	NULL,			/* destroy */
	CAD_FeatureLoad,
	CAD_FeatureSave,
	NULL,			/* edit */
};

void
CAD_FeatureInit(void *obj, const char *name)
{
	CAD_Feature *feature = obj;

	AG_ObjectInit(feature, name, &cadFeatureOps);
	feature->flags = 0;
}

int
CAD_FeatureLoad(void *obj, AG_Netbuf *buf)
{
	CAD_Feature *feature = obj;

	if (AG_ReadVersion(buf, cadFeatureOps.type, &cadFeatureOps.ver, NULL)
	    != 0) {
		return (-1);
	}
	feature->flags = (Uint)AG_ReadUint32(buf);
	return (0);
}

int
CAD_FeatureSave(void *obj, AG_Netbuf *buf)
{
	CAD_Feature *feature = obj;

	AG_WriteVersion(buf, cadFeatureOps.type, &cadFeatureOps.ver);
	AG_WriteUint32(buf, (Uint32)feature->flags);
	return (0);
}
