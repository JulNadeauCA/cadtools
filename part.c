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
#include "exboss.h"

#include <agar/sg/sg_load_ply.h>

static void
Init(void *obj)
{
	CAD_Part *part = obj;
/*	SG_Light *lt; */
	SG_Camera *cam;

	part->descr[0] = '\0';
	part->flags = 0;
	part->sg = SG_New(part, "Rendering", 0);
	part->so = SG_ObjectNew(part->sg->root, "Part Object");

	cam = SG_CameraNew(part->sg->root, "CameraFront");
	SG_Translate(cam, 0.0, 0.0, 5.0);

	cam = SG_CameraNew(part->sg->root, "CameraLeft");
	SG_Translate(cam, -5.0, 0.0, 0.0);
	SG_Rotatevd(cam, 90.0, M_VecJ3());
	
	cam = SG_CameraNew(part->sg->root, "CameraTop");
	SG_Translate(cam, 0.0, 5.0, -0.0);
	SG_Rotatevd(cam, 90.0, M_VecI3());
#if 0
	lt = SG_LightNew(part->sg->root, "Light0");
	SG_Translate(lt, -6.0, 6.0, -6.0);
	lt->Kc = 0.5;
	lt->Kl = 0.05;

	lt = SG_LightNew(part->sg->root, "Light1");
	SG_Translate(lt, 6.0, 6.0, 6.0);
	lt->Kc = 0.25;
	lt->Kl = 0.05;
#endif
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
	
	if (!AG_OfClass(pob, "CAD_Feature:*")) {
		if (!TAILQ_EMPTY(&pob->children)) {
			nosel++;
		} else {
			return;
		}
	}

	it = AG_TlistAddS(tl, NULL, pob->name);
	it->depth = depth;
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
	AG_LockVFS(part);
	FindFeatures(tl, AGOBJECT(part), 0);
	AG_UnlockVFS(part);
	AG_TlistRestore(tl);
}

static void
SetViewCamera(AG_Event *event)
{
	SG_View *sgv = AG_PTR(1);
	char *which = AG_STRING(2);
	SG_Camera *cam;

	if ((cam = SG_FindNode(sgv->sg, which)) == NULL) {
		AG_TextMsg(AG_MSG_ERROR, _("Cannot find Camera: %s"), which);
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
	    AGOBJECT(sgv->cam)->name);
	AG_ObjectAttach(win, SGNODE_OPS(sgv->cam)->edit(sgv->cam, sgv));
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
		AG_TextMsg(AG_MSG_ERROR, _("Cannot find Light: %s"), ltname);
		return;
	}

	win = AG_WindowNew(0);
	AG_WindowSetCaptionS(win, _("Light settings"));
	AG_ObjectAttach(win, SGNODE_OPS(lt)->edit(lt, sgv));
	AG_WindowShow(win);
}

void
CAD_PartInsertFeature(AG_Event *event)
{
	char name[AG_OBJECT_NAME_MAX];
	CAD_Part *part = AG_PTR(1);
	AG_ObjectClass *cls = AG_PTR(2);
	char *basename = AG_STRING(3);
	CAD_Feature *ft;
	Uint name_no = 0;
tryname:
	snprintf(name, sizeof(name), "%s #%d", basename, name_no++);
	if (AG_ObjectFindChild(part, name) != NULL)
		goto tryname;

	ft = Malloc(cls->size);
	AG_ObjectInit(ft, &cadFeatureClass);
	AG_ObjectSetNameS(ft, name);
	AG_ObjectAttach(part, ft);
}

/* Save part to native cadtools format. */
static void
SavePartToNative(AG_Event *event)
{
	CAD_Part *part = AG_PTR(1);
	char *path = AG_STRING(2);

	if (AG_ObjectSaveToFile(part, path) == -1) {
		AG_TextMsgFromError();
		return;
	}
	AG_SetString(part, "archive-path", path);
	AG_ObjectSetNameS(part, AG_ShortFilename(path));
}

void
CAD_PartSaveMenu(AG_FileDlg *fd, CAD_Part *part)
{
/*	AG_FileType *ft; */

	AG_FileDlgAddType(fd, _("cadtools part"), "*.part",
	    SavePartToNative, "%p", part);
#if 0
	ft = AG_FileDlgAddType(fd, _("Stanford PLY"), "*.ply",
	    SavePartToPLY, "%p", part);
	AG_FileOptionNewString(ft, _("Comment: "), "ply.comment", "");
	AG_FileOptionNewBool(ft, _("Save vertex normals"), "ply.vtxnormals", 1);
	AG_FileOptionNewBool(ft, _("Save vertex colors"), "ply.vtxcolors", 1);
	AG_FileOptionNewBool(ft, _("Save texture coords"), "ply.texcoords", 1);

	AG_FileDlgAddType(fd, _("Wavefront OBJ"), "*.obj",
	    SavePartToOBJ, "%p", part);
#endif
}

/* Open a part in native cadtools format. */
static void
OpenPartNative(AG_Event *event)
{
	char *path = AG_STRING(2);
	AG_Object *obj;

	obj = AG_ObjectNew(&vfsRoot, NULL, &cadPartClass);
	if (AG_ObjectLoadFromFile(obj, path) == -1) {
		AG_TextMsgFromError();
		AG_ObjectDetach(obj);
		AG_ObjectDestroy(obj);
		return;
	}
	AG_SetString(obj, "archive-path", path);
	AG_ObjectSetNameS(obj, AG_ShortFilename(path));
	CAD_OpenObject(obj);
}

/* Generate a new part from a mesh in PLY format. */
static void
OpenPartFromPLY(AG_Event *event)
{
/*	AG_Object *fd = AG_SELF(); */
	char *path = AG_STRING(1);
	AG_FileType *ft = AG_PTR(2);
	CAD_Part *part;
	Uint flags = 0;

	part = Malloc(sizeof(CAD_Part));
	AG_ObjectInit(part, &cadPartClass);
	AG_ObjectSetName(part, "Imported object");
	AGOBJECT(part)->flags |= AG_OBJECT_RESIDENT;
	AG_ObjectAttach(&vfsRoot, part);

	if (AG_FileOptionInt(ft, "ply.vtxnormals"))
		flags |= SG_PLY_LOAD_VTX_NORMALS;
	if (AG_FileOptionInt(ft, "ply.vtxcolors"))
		flags |= SG_PLY_LOAD_VTX_COLORS;
	if (AG_FileOptionInt(ft, "ply.texcoords"))
		flags |= SG_PLY_LOAD_TEXCOORDS;
	if (AG_FileOptionInt(ft, "ply.dups"))
		flags |= SG_PLY_DUP_VERTICES;

	if (SG_ObjectLoadPLY(part->so, path, flags) == -1) {
		goto fail;
	}
	SG_Scale(part->so, AG_FileOptionFlt(ft,"ply.scale"));
/*	SG_ObjectNormalize(part->so); */
	CAD_OpenObject(part);
	return;
fail:
	AG_TextMsg(AG_MSG_ERROR, "%s: %s", path, AG_GetError());
	AG_ObjectDestroy(part->so);
}

void
CAD_PartOpenMenu(AG_FileDlg *fd)
{
	AG_FileType *ft;

	AG_FileDlgAddType(fd, _("Native cadtools part"), "*.part",
	    OpenPartNative, NULL);
	ft = AG_FileDlgAddType(fd, _("Stanford PLY"), "*.ply",
	    OpenPartFromPLY, NULL);
	AG_FileOptionNewFlt(ft, _("Scaling factor"), "ply.scale", 1.0,
	    1e-6, 1e6, NULL);
	AG_FileOptionNewBool(ft, _("Load vertex normals"), "ply.vtxnormals", 1);
	AG_FileOptionNewBool(ft, _("Load vertex colors"), "ply.vtxcolors", 1);
	AG_FileOptionNewBool(ft, _("Load texture coords"), "ply.texcoords", 1);
	AG_FileOptionNewBool(ft, _("Detect duplicate vertices"), "ply.dups", 1);
}

static void *
Edit(void *obj)
{
	CAD_Part *part = obj;
	AG_Window *win;
	AG_Toolbar *toolbar;
	AG_Menu *menu;
	AG_MenuItem *m;
	AG_Pane *hPane;
	SG_View *sgv;

	win = AG_WindowNew(AG_WINDOW_MAIN);
	AG_WindowSetCaptionS(win, AGOBJECT(part)->name);

	toolbar = AG_ToolbarNew(NULL, AG_TOOLBAR_VERT, 1, 0);
	sgv = SG_ViewNew(NULL, part->sg, SG_VIEW_EXPAND);

	menu = AG_MenuNew(win, AG_MENU_HFILL);
	
	m = AG_MenuNode(menu->root, _("File"), NULL);
	{
		CAD_FileMenu(m, part);
	}
	m = AG_MenuNode(menu->root, _("Edit"), NULL);
	{
		CAD_EditMenu(m, part);
	}
	m = AG_MenuNode(menu->root, _("Features"), NULL);
	{
		AG_MenuAction(m, _("Extruded boss/base"), NULL,
		    CAD_PartInsertFeature, "%p,%p,%s", part,
		    &cadExtrudedBossClass, _("Extrusion"));
	}
	m = AG_MenuNode(menu->root, _("View"), NULL);
	{
		AG_MenuAction(m, _("Default"), sgIconCamera.s,
		    SetViewCamera, "%p,%s", sgv, "Camera0");
		AG_MenuAction(m, _("Front view"), sgIconCamera.s,
		    SetViewCamera, "%p,%s", sgv, "CameraFront");
		AG_MenuAction(m, _("Top view"), sgIconCamera.s,
		    SetViewCamera, "%p,%s", sgv, "CameraTop");
		AG_MenuAction(m, _("Left view"), sgIconCamera.s,
		    SetViewCamera, "%p,%s", sgv, "CameraLeft");
		AG_MenuSeparator(m);
		AG_MenuAction(m, _("Camera settings..."), agIconGear.s,
		    ShowCameraSettings, "%p", sgv);
		AG_MenuAction(m, _("Light0 settings..."), sgIconLighting.s,
		    ShowLightSettings, "%p,%s", sgv, "Light0");
		AG_MenuAction(m, _("Light1 settings..."), sgIconLighting.s,
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
	AG_PaneMoveDividerPct(hPane, 33);

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 80, 80);
	return (win);
}

AG_ObjectClass cadPartClass = {
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
