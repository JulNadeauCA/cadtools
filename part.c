/*
 * Copyright (c) 2007 Hypertriton, Inc. <http://www.hypertriton.com>
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

CAD_Part *
CAD_PartNew(void *parent, const char *name)
{
	CAD_Part *part;

	part = Malloc(sizeof(CAD_Part));
	AG_ObjectInit(part, &cadPartOps);
	AG_ObjectSetName(part, "%s", name);
	AG_ObjectAttach(parent, part);
	return (part);
}

static void
Init(void *obj)
{
	CAD_Part *part = obj;
	SG_Real i;
	SG_Light *lt;
	SG_Camera *cam;

	part->descr[0] = '\0';
	part->flags = 0;
	part->sg = SG_New(part, "Rendering");
	part->so = SG_ObjectNew(part->sg->root, "part");

	cam = SG_CameraNew(part->sg->root, "CameraFront");
	SG_Translate3(cam, 0.0, 0.0, 5.0);

	cam = SG_CameraNew(part->sg->root, "CameraLeft");
	SG_Translate3(cam, -5.0, 0.0, -5.0);
	SG_Rotatevd(cam, -90.0, VecJ());
	
	cam = SG_CameraNew(part->sg->root, "CameraTop");
	SG_Translate3(cam, 0.0, 5.0, -5.0);
	SG_Rotatevd(cam, -90.0, VecI());

	lt = SG_LightNew(part->sg->root, "Light0");
	SG_Translate3(lt, -6.0, 6.0, -6.0);
	lt->Kc = 0.5;
	lt->Kl = 0.05;

	lt = SG_LightNew(part->sg->root, "Light1");
	SG_Translate3(lt, 6.0, 6.0, 6.0);
	lt->Kc = 0.25;
	lt->Kl = 0.05;
}

static void
Destroy(void *obj)
{
}

static int
Load(void *obj, AG_DataSource *buf, const AG_Version *ver)
{
	CAD_Part *part = obj;

	AG_CopyString(part->descr, buf, sizeof(part->descr));
	part->flags = AG_ReadUint32(buf);
	return (0);
}

static int
Save(void *obj, AG_DataSource *buf)
{
	CAD_Part *part = obj;

	AG_WriteString(buf, part->descr);
	AG_WriteUint32(buf, part->flags);
	return (0);
}

static void
FindFeatures(AG_Tlist *tl, AG_Object *pob, int depth)
{
	AG_Object *cob;
	AG_TlistItem *it;
	int nosel = 0;
	
	if (!AG_ObjectIsClass(pob, "CAD_Feature:*")) {
		if (!TAILQ_EMPTY(&pob->children)) {
			nosel++;
		} else {
			return;
		}
	}

	it = AG_TlistAdd(tl, AG_ObjectIcon(pob), "%s", pob->name);
	it->depth = depth;
	it->cat = pob->ops->type;
	it->p1 = pob;
	if (nosel) {
		it->flags |= AG_TLIST_NO_SELECT;
	}
	if (!TAILQ_EMPTY(&pob->children)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
		if (AG_ObjectRoot(pob) == pob)
			it->flags |= AG_TLIST_VISIBLE_CHILDREN;
	}
	if ((it->flags & AG_TLIST_HAS_CHILDREN)) {
		TAILQ_FOREACH(cob, &pob->children, cobjs)
			FindFeatures(tl, cob, depth+1);
	}
}

static void
PollFeatures(AG_Event *event)
{
	AG_Tlist *tl = AG_PTR(0);
	CAD_Part *part = AG_PTR(1);

	AG_TlistClear(tl);
	AG_LockLinkage();
	FindFeatures(tl, AGOBJECT(part), 0);
	AG_UnlockLinkage();
	AG_TlistRestore(tl);
}

static void
SetViewCamera(AG_Event *event)
{
	SG_View *sgv = AG_PTR(1);
	char *which = AG_STRING(2);
	SG_Camera *cam;

	if ((cam = SG_FindNode(sgv->sg, which)) == NULL) {
		AG_TextMsg(AG_MSG_ERROR, "Cannot find camera %s!", which);
		return;
	}
	SG_ViewSetCamera(sgv, cam);
}

static void
ShowCameraSettings(AG_Event *event)
{
	SG_View *sgv = AG_PTR(1);
	AG_Window *win;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Camera settings (%s)"),
	    SGNODE(sgv->cam)->name);
	SG_CameraEdit(sgv->cam, AGWIDGET(win), sgv);
	AG_WindowShow(win);
}

static void
ShowLightSettings(AG_Event *event)
{
	SG_View *sgv = AG_PTR(1);
	char *ltname = AG_STRING(2);
	SG_Light *lt;
	AG_Window *win;

	if ((lt = SG_FindNode(sgv->sg, ltname)) == NULL) {
		AG_TextMsg(AG_MSG_ERROR, "Cannot find light: %s", ltname);
		return;
	}

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Light settings"));
	SG_LightEdit(lt, AGWIDGET(win), sgv);
	AG_WindowShow(win);
}

static void
InsertFeature(AG_Event *event)
{
	char name[AG_OBJECT_NAME_MAX];
	CAD_Part *part = AG_PTR(1);
	AG_ObjectOps *ops = AG_PTR(2);
	char *basename = AG_STRING(3);
	CAD_Feature *ft;
	Uint name_no = 0;
tryname:
	snprintf(name, sizeof(name), "%s #%d", basename, name_no++);
	if (AG_ObjectFindChild(part, name) != NULL)
		goto tryname;

	ft = Malloc(ops->size);
	AG_ObjectInit(ft, &cadFeatureOps);
	AG_ObjectSetName(ft, "%s", name);
	AG_ObjectAttach(part, ft);
}

static void *
Edit(void *obj)
{
	CAD_Part *part = obj;
	AG_Window *win;
	AG_Toolbar *toolbar;
	AG_Menu *menu;
	AG_MenuItem *pitem, *subitem;
	AG_Pane *hPane;
	SG_View *sgv;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, "%s", AGOBJECT(part)->name);

	toolbar = AG_ToolbarNew(NULL, AG_TOOLBAR_VERT, 1, 0);
	sgv = SG_ViewNew(NULL, part->sg, SG_VIEW_EXPAND);

	menu = AG_MenuNew(win, AG_MENU_HFILL);
	pitem = AG_MenuAddItem(menu, _("File"));
	{
#if 0
		AG_MenuActionKb(pitem, _("Revert"), agIconLoad.s,
		    SDLK_r, KMOD_CTRL, RevertPart, "%p", part);
		AG_MenuActionKb(pitem, _("Save"), agIconSave.s,
		    SDLK_s, KMOD_CTRL, SavePart, "%p", part);
		AG_MenuSeparator(pitem);
		AG_MenuAction(pitem, _("Document properties..."), agIconGear.s,
		    ShowDocumentProps, "%p,%p", win, part);
#endif
		AG_MenuSeparator(pitem);
		AG_MenuActionKb(pitem, _("Close document"), agIconClose.s,
		    SDLK_w, KMOD_CTRL, AGWINCLOSE(win));
	}
	
	pitem = AG_MenuAddItem(menu, _("Features"));
	{
		extern AG_ObjectOps cadExtrudedBossOps;
	
		AG_MenuAction(pitem, _("Extruded boss/base"), NULL,
		    InsertFeature, "%p,%p,%s", part, &cadExtrudedBossOps,
		    _("Extrusion"));
	}

	pitem = AG_MenuAddItem(menu, _("Edit"));
	{
		AG_MenuAction(pitem, _("Undo"), NULL, NULL, NULL);
		AG_MenuAction(pitem, _("Redo"), NULL, NULL, NULL);
	}
	
	pitem = AG_MenuAddItem(menu, _("View"));
	{
		AG_MenuAction(pitem, _("Default"), sgIconCamera.s,
		    SetViewCamera, "%p,%s", sgv, "Camera0");
		AG_MenuAction(pitem, _("Front view"), sgIconCamera.s,
		    SetViewCamera, "%p,%s", sgv, "CameraFront");
		AG_MenuAction(pitem, _("Top view"), sgIconCamera.s,
		    SetViewCamera, "%p,%s", sgv, "CameraTop");
		AG_MenuAction(pitem, _("Left view"), sgIconCamera.s,
		    SetViewCamera, "%p,%s", sgv, "CameraLeft");
		AG_MenuSeparator(pitem);
		AG_MenuAction(pitem, _("Camera settings..."), agIconGear.s,
		    ShowCameraSettings, "%p", sgv);
		AG_MenuAction(pitem, _("Light0 settings..."), sgIconLighting.s,
		    ShowLightSettings, "%p,%s", sgv, "Light0");
		AG_MenuAction(pitem, _("Light1 settings..."), sgIconLighting.s,
		    ShowLightSettings, "%p,%s", sgv, "Light1");
	}
	
	hPane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	AG_PaneSetDivisionPacking(hPane, 1, AG_BOX_HORIZ);
	{
		AG_Notebook *nb;
		AG_NotebookTab *ntab;
		AG_Tlist *tl;
		AG_Box *box;

		nb = AG_NotebookNew(hPane->div[0], AG_NOTEBOOK_EXPAND);

		ntab = AG_NotebookAddTab(nb, _("Features"), AG_BOX_VERT);
		{
			tl = AG_TlistNew(ntab, AG_TLIST_POLL|AG_TLIST_TREE|
			                       AG_TLIST_EXPAND);
			AG_SetEvent(tl, "tlist-poll", PollFeatures, "%p", part);
			AGWIDGET(tl)->flags &= ~(AG_WIDGET_FOCUSABLE);
		}

		box = AG_BoxNew(hPane->div[1], AG_BOX_VERT, AG_BOX_EXPAND);
		{
			AG_ObjectAttach(box, sgv);
			AG_WidgetFocus(sgv);
		}
		AG_ObjectAttach(hPane->div[1], toolbar);
	}
	
	AG_WindowSetGeometry(win,
	    agView->w/16, agView->h/16,
	    7*agView->w/8, 7*agView->h/8);

	return (win);
}

const AG_ObjectOps cadPartOps = {
	"CAD_Part",
	sizeof(CAD_Part),
	{ 0,0 },
	Init,
	NULL,			/* reinit */
	Destroy,
	Load,
	Save,
	Edit
};
