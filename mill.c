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
	CAM_Mill *mill = obj;

	mill->flags = 0;

	/* Defaults based on my Rong-Fu. */
	/* TODO: model database */

	mill->lenTable = 800.1;
	mill->wTable = 241.3;
	mill->xTravel = 539.75;
	mill->yTravel = 215.9;
	mill->zTravel = 431.8;

	mill->spMinRPM = 110;
	mill->spMaxRPM = 1920;
	mill->spPower = 1492.0;
	mill->spChuck = NULL;
}

static int
Load(void *obj, AG_DataSource *buf, const AG_Version *ver)
{
	CAM_Mill *mill = obj;
	
	mill->flags = (Uint)AG_ReadUint32(buf);

	mill->lenTable = M_ReadReal(buf);
	mill->wTable = M_ReadReal(buf);
	mill->xTravel = M_ReadReal(buf);
	mill->yTravel = M_ReadReal(buf);
	mill->zTravel = M_ReadReal(buf);
	
	mill->spMinRPM = (int)AG_ReadUint32(buf);
	mill->spMaxRPM = (int)AG_ReadUint32(buf);
	mill->spPower = M_ReadReal(buf);
	mill->spChuck = NULL;
	return (0);
}

static int
Save(void *obj, AG_DataSource *buf)
{
	CAM_Mill *mill = obj;
	
	AG_WriteUint32(buf, (Uint32)mill->flags);

	M_WriteReal(buf, mill->lenTable);
	M_WriteReal(buf, mill->wTable);
	M_WriteReal(buf, mill->xTravel);
	M_WriteReal(buf, mill->yTravel);
	M_WriteReal(buf, mill->zTravel);

	AG_WriteUint32(buf, mill->spMinRPM);
	AG_WriteUint32(buf, mill->spMaxRPM);
	M_WriteReal(buf, mill->spPower);
	return (0);
}

static void *
Edit(void *obj)
{
	const AG_FlagDescr millFlags[] = {
	{ CAM_MILL_SPINDLE_SWITCH,  N_("Spindle on/off control"),	1 },
	{ CAM_MILL_SPINDLE_SPEED,   N_("Spindle speed control"),	1 },
	{ CAM_MILL_SPINDLE_TACHO,   N_("Spindle tachometer readout"),	1 },
	{ CAM_MILL_SYNC_TO_SPINDLE, N_("Sync program with tachometer"),	1 },
	{ CAM_MILL_COOL_MIST,       N_("Mist coolant control"),		1 },
	{ CAM_MILL_COOL_FLOOD,      N_("Flood coolant control"),	1 },
	{ CAM_MILL_COOL_VORTEX,     N_("Vortex tube control"),		1 },
	{ CAM_MILL_XLIMIT,          N_("X limit switches"),		1 },
	{ CAM_MILL_YLIMIT,          N_("Y limit switches"),		1 },
	{ CAM_MILL_ZLIMIT,          N_("Z limit switches"),		1 },
	{ 0, NULL, 0 }
	};
	CAM_Mill *mill = obj;
	AG_Numerical *num;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;

	nb = camMachineClass.edit(mill);

	ntab = AG_NotebookAddTab(nb, _("Options"), AG_BOX_VERT);
	{
		AG_CheckboxSetFromFlags(ntab, 0, &mill->flags, millFlags);
	}
	ntab = AG_NotebookAddTab(nb, _("Specs"), AG_BOX_VERT);
	{
		num = AG_NumericalNew(ntab, 0, "mm", _("Table length: "));
		M_WidgetBindReal(num, "value", &mill->lenTable);
		num = AG_NumericalNew(ntab, 0, "mm", _("Table width: "));
		M_WidgetBindReal(num, "value", &mill->wTable);
		num = AG_NumericalNew(ntab, 0, "mm", _("X travel: "));
		M_WidgetBindReal(num, "value", &mill->xTravel);
		num = AG_NumericalNew(ntab, 0, "mm", _("Y travel: "));
		M_WidgetBindReal(num, "value", &mill->yTravel);
		num = AG_NumericalNew(ntab, 0, "mm", _("Z travel: "));
		M_WidgetBindReal(num, "value", &mill->zTravel);
	}
	ntab = AG_NotebookAddTab(nb, _("Spindle"), AG_BOX_VERT);
	{
		num = AG_NumericalNew(ntab, 0, NULL, _("Minimum RPM: "));
		AG_WidgetBindInt(num, "value", &mill->spMinRPM);
		num = AG_NumericalNew(ntab, 0, NULL, _("Maximum RPM: "));
		AG_WidgetBindInt(num, "value", &mill->spMaxRPM);
		num = AG_NumericalNew(ntab, 0, "HP", _("Spindle power: "));
		M_WidgetBindReal(num, "value", &mill->spPower);
	}

	return AG_WidgetParentWindow(nb);
}

AG_ObjectClass camMillClass = {
	"CAM_Machine:CAM_Mill",
	sizeof(CAM_Mill),
	{ 0,0 },
	Init,
	NULL,		/* reinit */
	NULL,		/* destroy */
	Load,
	Save,
	Edit
};
