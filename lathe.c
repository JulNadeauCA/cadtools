/*
 * Copyright (c) 2007 Hypertriton, Inc. <http://hypertriton.com>
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
#include <errno.h>
#include <unistd.h>

#include "cadtools.h"
#include "protocol.h"

static void
Init(void *obj)
{
	CAM_Lathe *lathe = obj;

	lathe->flags = 0;

	/* Defaults based on my 9x20. */
	/* TODO: model database */
	lathe->bedSwing = 222.25e-3;
	lathe->csSwing = 127.0e-3;
	lathe->csTravel = 107.95e-3;
	lathe->carrTravel = 508.0e-3;
	lathe->compTravel = 76.2e-3;
	lathe->distCenters = 533.4e-3;
	lathe->spBore = 20.0e-3;
	lathe->spMinRPM = 170;
	lathe->spMaxRPM = 1950;
	lathe->spPower = 559.5;
	lathe->spChuck = NULL;
	lathe->tsTravel = 57.15e-3;
	lathe->tsChuck = NULL;
}

static int
Load(void *obj, AG_DataSource *buf, const AG_Version *ver)
{
	CAM_Lathe *lathe = obj;

	lathe->flags = (Uint)AG_ReadUint32(buf);
	lathe->bedSwing = M_ReadReal(buf);
	lathe->csSwing = M_ReadReal(buf);
	lathe->csTravel = M_ReadReal(buf);
	lathe->carrTravel = M_ReadReal(buf);
	lathe->compTravel = M_ReadReal(buf);
	lathe->distCenters = M_ReadReal(buf);
	lathe->spBore = M_ReadReal(buf);
	lathe->spMinRPM = (int)AG_ReadUint32(buf);
	lathe->spMaxRPM = (int)AG_ReadUint32(buf);
	lathe->spPower = M_ReadReal(buf);
	lathe->spChuck = NULL;
	lathe->tsTravel = M_ReadReal(buf);
	lathe->tsChuck = NULL;
	return (0);
}

static int
Save(void *obj, AG_DataSource *buf)
{
	CAM_Lathe *lathe = obj;

	AG_WriteUint32(buf, (Uint32)lathe->flags);
	M_WriteReal(buf, lathe->bedSwing);
	M_WriteReal(buf, lathe->csSwing);
	M_WriteReal(buf, lathe->csTravel);
	M_WriteReal(buf, lathe->carrTravel);
	M_WriteReal(buf, lathe->compTravel);
	M_WriteReal(buf, lathe->distCenters);
	M_WriteReal(buf, lathe->spBore);
	AG_WriteUint32(buf, lathe->spMinRPM);
	AG_WriteUint32(buf, lathe->spMaxRPM);
	M_WriteReal(buf, lathe->spPower);
	M_WriteReal(buf, lathe->tsTravel);
	return (0);
}

static void *
Edit(void *obj)
{
	const AG_FlagDescr latheFlags[] = {
	{ CAM_LATHE_SPINDLE_SWITCH,  N_("Spindle on/off control"),	  1 },
	{ CAM_LATHE_SPINDLE_SPEED,   N_("Spindle speed control"),	  1 },
	{ CAM_LATHE_SPINDLE_TACHO,   N_("Spindle tachometer readout"),	  1 },
	{ CAM_LATHE_SYNC_TO_SPINDLE, N_("Sync program with tachometer"),  1 },
	{ 0, NULL, 0 }
	};
	CAM_Lathe *lathe = obj;
	AG_Numerical *num;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;
	
	nb = camMachineClass.edit(lathe);

	ntab = AG_NotebookAddTab(nb, _("Options"), AG_BOX_VERT);
	{
		AG_CheckboxSetFromFlags(ntab, &lathe->flags, latheFlags);
	}
	ntab = AG_NotebookAddTab(nb, _("Specs"), AG_BOX_VERT);
	{
		num = AG_NumericalNew(ntab, 0, "mm", _("Swing over bed: "));
		M_WidgetBindReal(num, "value", &lathe->bedSwing);
		num = AG_NumericalNew(ntab, 0, "mm", _("Swing over "
		                                       "cross-slide: "));
		M_WidgetBindReal(num, "value", &lathe->csSwing);
		num = AG_NumericalNew(ntab, 0, "mm", _("Cross-slide travel: "));
		M_WidgetBindReal(num, "value", &lathe->csTravel);
		num = AG_NumericalNew(ntab, 0, "mm", _("Carriage travel: "));
		M_WidgetBindReal(num, "value", &lathe->carrTravel);
		num = AG_NumericalNew(ntab, 0, "mm", _("Compound travel: "));
		M_WidgetBindReal(num, "value", &lathe->compTravel);
		num = AG_NumericalNew(ntab, 0, "mm", _("Distance between "
		                                      "centers: "));
		M_WidgetBindReal(num, "value", &lathe->distCenters);
		num = AG_NumericalNew(ntab, 0, "mm", _("Tailstock travel: "));
		M_WidgetBindReal(num, "value", &lathe->tsTravel);
	}
	ntab = AG_NotebookAddTab(nb, _("Spindle"), AG_BOX_VERT);
	{
		num = AG_NumericalNew(ntab, 0, "mm", _("Spindle bore "
		                                       "diameter: "));
		M_WidgetBindReal(num, "value", &lathe->spBore);
		num = AG_NumericalNew(ntab, 0, NULL, _("Minimum RPM: "));
		AG_WidgetBindInt(num, "value", &lathe->spMinRPM);
		num = AG_NumericalNew(ntab, 0, NULL, _("Maximum RPM: "));
		AG_WidgetBindInt(num, "value", &lathe->spMaxRPM);
		num = AG_NumericalNew(ntab, 0, "HP", _("Spindle power: "));
		M_WidgetBindReal(num, "value", &lathe->spPower);
	}
	return AG_WidgetParentWindow(nb);
}

AG_ObjectClass camLatheClass = {
	"CAM_Machine:CAM_Lathe",
	sizeof(CAM_Lathe),
	{ 0,0 },
	Init,
	NULL,		/* reinit */
	NULL,		/* destroy */
	Load,
	Save,
	Edit
};
