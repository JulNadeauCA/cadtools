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

/*
 * Main graphical user interface for cadtools.
 */

#include <agar/core.h>
#include <agar/gui.h>

#include <config/have_agar_dev.h>
#ifdef HAVE_AGAR_DEV
#include <agar/dev.h>
#endif

#include <string.h>

#ifdef HAVE_GETOPT
#include <unistd.h>
#endif

#include "cadtools.h"

#include <config/debug.h>
#include <config/have_getopt.h>
#include <config/enable_nls.h>
#include <config/localedir.h>

AG_Menu *appMenu = NULL;
AG_Object vfsRoot;

static void *objFocus = NULL;
static AG_Mutex objLock;

static void
RegisterClasses(void)
{
	AG_RegisterClass(&camProgramClass);
	AG_RegisterClass(&cadPartClass);
	AG_RegisterClass(&camMachineClass);
	AG_RegisterClass(&camLatheClass);
	AG_RegisterClass(&camMillClass);
}

static void
SaveAndClose(AG_Object *obj, AG_Window *win)
{
	AG_ObjectDetach(win);
	AG_ObjectPageOut(obj);
}

static void
SaveChangesReturn(AG_Event *event)
{
	AG_Window *win = AG_PTR(1);
	AG_Object *obj = AG_PTR(2);

	SaveAndClose(obj, win);
}

static void
SaveChangesDlg(AG_Event *event)
{
	AG_Window *win = AG_SELF();
	AG_Object *obj = AG_PTR(1);

	if (!AG_ObjectChanged(obj)) {
		SaveAndClose(obj, win);
	} else {
		AG_Button *bOpts[3];
		AG_Window *wDlg;

		wDlg = AG_TextPromptOptions(bOpts, 3,
		    _("Save changes to %s?"), AGOBJECT(obj)->name);
		AG_WindowAttach(win, wDlg);
		
		AG_ButtonText(bOpts[0], _("Save"));
		AG_SetEvent(bOpts[0], "button-pushed", SaveChangesReturn,
		    "%p,%p,%i", win, obj, 1);
		AG_WidgetFocus(bOpts[0]);
		AG_ButtonText(bOpts[1], _("Discard"));
		AG_SetEvent(bOpts[1], "button-pushed", SaveChangesReturn,
		    "%p,%p,%i", win, obj, 0);
		AG_ButtonText(bOpts[2], _("Cancel"));
		AG_SetEvent(bOpts[2], "button-pushed", AGWINDETACH(wDlg));
	}
}

static void
WindowGainedFocus(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
	
	AG_MutexLock(&objLock);
	if (AG_OfClass(obj, "SK:*") ||
	    AG_OfClass(obj, "CAD_Part:*") ||
	    AG_OfClass(obj, "CAM_Program:*")) {
		objFocus = obj;
	} else {
		objFocus = NULL;
	}
	AG_MutexUnlock(&objLock);
}

static void
WindowLostFocus(AG_Event *event)
{
	AG_MutexLock(&objLock);
	objFocus = NULL;
	AG_MutexUnlock(&objLock);
}

AG_Window *
CAD_CreateEditionWindow(void *p)
{
	AG_Object *obj = p;
	AG_Window *win;

	win = obj->cls->edit(obj);
	AG_SetEvent(win, "window-close", SaveChangesDlg, "%p", obj);
	AG_AddEvent(win, "window-gainfocus", WindowGainedFocus, "%p", obj);
	AG_AddEvent(win, "window-lostfocus", WindowLostFocus, "%p", obj);
	AG_AddEvent(win, "window-hidden", WindowLostFocus, "%p", obj);
	AG_WindowShow(win);
	return (win);
}

static void
NewObject(AG_Event *event)
{
	AG_ObjectClass *cls = AG_PTR(1);

	CAD_CreateEditionWindow(AG_ObjectNew(&vfsRoot, NULL, cls));
}

static void
EditMachine(AG_Event *event)
{
	AG_Object *mach = AG_PTR(1);

	CAD_CreateEditionWindow(mach);
}

void
CAD_SetArchivePath(void *obj, const char *path)
{
	const char *c;

	AG_ObjectSetArchivePath(obj, path);

	if ((c = strrchr(path, PATHSEPC)) != NULL && c[1] != '\0') {
		AG_ObjectSetNameS(obj, &c[1]);
	} else {
		AG_ObjectSetNameS(obj, path);
	}
}

/* Load an object file from native cadtools format. */
static void
OpenSketch(AG_Event *event)
{
	char *path = AG_STRING(1);
	AG_Object *obj;

	obj = AG_ObjectNew(&vfsRoot, NULL, &skClass);
	if (AG_ObjectLoadFromFile(obj, path) == -1) {
		AG_TextMsgFromError();
		AG_ObjectDetach(obj);
		AG_ObjectDestroy(obj);
		return;
	}
	CAD_SetArchivePath(obj, path);
	CAD_CreateEditionWindow(obj);
}

static void
OpenDlg(AG_Event *event)
{
	AG_Window *win;
	AG_FileDlg *fd;
	AG_FileType *ft;
	AG_Pane *hPane;

	win = AG_WindowNew(0);
	AG_WindowSetCaptionS(win, _("Open..."));

	hPane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	fd = AG_FileDlgNewMRU(hPane->div[0], "cadtools.mru.parts",
	    AG_FILEDLG_LOAD|AG_FILEDLG_ASYNC|AG_FILEDLG_EXPAND);
	AG_FileDlgSetOptionContainer(fd, hPane->div[1]);

	AG_FileDlgAddType(fd, _("cadtools sketch"), "*.sk",
	    OpenSketch, NULL);
	CAD_PartOpenMenu(fd);

	AG_WindowShow(win);
	AG_WindowSetGeometry(win, AGWIDGET(win)->x-100, AGWIDGET(win)->y,
	                          AGWIDGET(win)->w+200, AGWIDGET(win)->h);
}

static void
SaveSketchToSK(AG_Event *event)
{
	SK *sk = AG_PTR(1);
	char *path = AG_STRING(2);

	if (AG_ObjectSaveToFile(sk, path) == -1) {
		AG_TextMsgFromError();
	}
	CAD_SetArchivePath(sk, path);
}

static void
SaveSketchToDXF(AG_Event *event)
{
//	SK *sk = AG_PTR(1);
//	char *path = AG_STRING(2);

	AG_TextMsg(AG_MSG_ERROR, "Not implemented yet");
}

static void
SaveAsDlg(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
	AG_Window *win;
	AG_FileDlg *fd;
	AG_FileType *ft;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Save %s as..."), obj->name);
	fd = AG_FileDlgNewMRU(win, "cadtools.mru.parts",
	    AG_FILEDLG_SAVE|AG_FILEDLG_CLOSEWIN|AG_FILEDLG_EXPAND);
	AG_FileDlgSetOptionContainer(fd, AG_BoxNewVert(win, AG_BOX_HFILL));

	if (AG_OfClass(obj, "SK:*")) {
		/*
		 * Save to 2D sketch format
		 */
		AG_FileDlgAddType(fd, _("cadtools sketch"), "*.sk",
		    SaveSketchToSK, "%p", obj);
		ft = AG_FileDlgAddType(fd, _("AutoCAD DXF"), "*.dxf",
		    SaveSketchToDXF, "%p", obj);
		{
			AG_FileOptionNewBool(ft, _("Binary format"),
			    "dxf.binary", 1);
		}
	} else if (AG_OfClass(obj, "CAD_Part:*")) {
		CAD_PartSaveMenu(fd, (CAD_Part *)obj);
	}
	AG_WindowShow(win);
}

static void
Save(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);

	if (AGOBJECT(obj)->archivePath == NULL) {
		SaveAsDlg(event);
		return;
	}
	if (AG_ObjectSave(obj) == -1) {
		AG_TextMsg(AG_MSG_ERROR, _("Error saving object: %s"),
		    AG_GetError());
	} else {
		AG_TextInfo("saved-object",
		    _("Saved object %s successfully"),
		    AGOBJECT(obj)->name);
	}
}

static void
ConfirmQuit(AG_Event *event)
{
	SDL_Event nev;

	nev.type = SDL_USEREVENT;
	SDL_PushEvent(&nev);
}

static void
AbortQuit(AG_Event *event)
{
	AG_Window *win = AG_PTR(1);

	agTerminating = 0;
	AG_ObjectDetach(win);
}

static void
Quit(AG_Event *event)
{
	AG_Object *obj;
	AG_Window *win;
	AG_Box *box;

	if (agTerminating) {
		ConfirmQuit(NULL);
	}
	agTerminating = 1;

	AGOBJECT_FOREACH_CHILD(obj, &vfsRoot, ag_object) {
		if (AG_ObjectChanged(obj))
			break;
	}
	if (obj == NULL) {
		ConfirmQuit(NULL);
	} else {
		if ((win = AG_WindowNewNamedS(AG_WINDOW_MODAL|AG_WINDOW_NOTITLE|
		    AG_WINDOW_NORESIZE, "QuitCallback")) == NULL) {
			return;
		}
		AG_WindowSetCaptionS(win, _("Exit application?"));
		AG_WindowSetPosition(win, AG_WINDOW_CENTER, 0);
		AG_WindowSetSpacing(win, 8);
		AG_LabelNewString(win, 0,
		    _("There is at least one object with unsaved changes.  "
	              "Exit application?"));
		box = AG_BoxNew(win, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS|
		                                   AG_VBOX_HFILL);
		AG_BoxSetSpacing(box, 0);
		AG_BoxSetPadding(box, 0);
		AG_ButtonNewFn(box, 0, _("Discard changes"),
		    ConfirmQuit, NULL);
		AG_WidgetFocus(AG_ButtonNewFn(box, 0, _("Cancel"),
		    AbortQuit, "%p", win));
		AG_WindowShow(win);
	}
}

static void
FileMenu(AG_Event *event)
{
	AG_MenuItem *m = AG_SENDER();
	AG_MenuItem *node;

	AG_MenuActionKb(m, _("New sketch..."), agIconDoc.s, SDLK_s, KMOD_ALT,
	    NewObject, "%p", &skClass);
	AG_MenuActionKb(m, _("New part..."), agIconDoc.s, SDLK_p, KMOD_ALT,
	    NewObject, "%p", &cadPartClass);
	AG_MenuActionKb(m, _("New program..."), agIconDoc.s, SDLK_c, KMOD_ALT,
	    NewObject, "%p", &camProgramClass);

	AG_MenuActionKb(m, _("Open..."), agIconLoad.s, SDLK_o, KMOD_CTRL,
	    OpenDlg, NULL);

	AG_MutexLock(&objLock);
	if (objFocus == NULL) { AG_MenuDisable(m); }

	AG_MenuActionKb(m, _("Save"), agIconSave.s, SDLK_s, KMOD_CTRL,
	    Save, "%p", objFocus);
	AG_MenuAction(m, _("Save as..."), agIconSave.s,
	    SaveAsDlg, "%p", objFocus);
	
	if (objFocus == NULL) { AG_MenuEnable(m); }
	AG_MutexUnlock(&objLock);
	
	AG_MenuSeparator(m);

	node = AG_MenuNode(m, _("Machines"), NULL);
	{
		CAM_Machine *mach;

		AG_MenuAction(node, _("New lathe..."), agIconDoc.s,
		    NewObject, "%p", &camLatheClass);
		AG_MenuAction(node, _("New mill..."), agIconDoc.s,
		    NewObject, "%p", &camMillClass);

		AG_MenuSeparator(node);

		AGOBJECT_FOREACH_CLASS(mach, &vfsRoot, cam_machine,
		    "CAM_Machine:*") {
			AG_MenuAction(node, AGOBJECT(mach)->name, agIconGear.s,
			    EditMachine, "%p", mach);
		}
	}
	
	AG_MenuSeparator(m);
	
	AG_MenuActionKb(m, _("Quit"), NULL, SDLK_q, KMOD_CTRL, Quit, NULL);
}

static void
Undo(AG_Event *event)
{
	/* TODO */
	printf("undo!\n");
}

static void
Redo(AG_Event *event)
{
	/* TODO */
	printf("redo!\n");
}

static void
EditMenu(AG_Event *event)
{
	AG_MenuItem *m = AG_SENDER();
	
	AG_MutexLock(&objLock);
	if (objFocus == NULL) { AG_MenuDisable(m); }
	AG_MenuActionKb(m, _("Undo"), NULL, SDLK_z, KMOD_CTRL,
	    Undo, "%p", objFocus);
	AG_MenuActionKb(m, _("Redo"), NULL, SDLK_r, KMOD_CTRL,
	    Redo, "%p", objFocus);
	if (objFocus == NULL) { AG_MenuEnable(m); }
	AG_MutexUnlock(&objLock);
}

static void
FeaturesMenu(AG_Event *event)
{
	extern AG_ObjectClass cadExtrudedBossClass;
	AG_MenuItem *m = AG_SENDER();
	CAD_Part *part;
	
	AG_MutexLock(&objLock);
	if (objFocus != NULL) {
		if (AG_OfClass(objFocus, "CAD_Part:*")) {
			CAD_Part *part = objFocus;

			AG_MenuAction(m, _("Extruded boss/base"), NULL,
			    CAD_PartInsertFeature, "%p,%p,%s", part,
			    &cadExtrudedBossClass, _("Extrusion"));
		}
	}
	AG_MutexUnlock(&objLock);
}

int
main(int argc, char *argv[])
{
	int c, i, fps = -1;
	char *s;

#ifdef ENABLE_NLS
	bindtextdomain("cadtools", LOCALEDIR);
	bind_textdomain_codeset("cadtools", "UTF-8");
	textdomain("cadtools");
#endif

	if (AG_InitCore("cadtools", AG_VERBOSE|AG_CREATE_DATADIR) == -1) {
		fprintf(stderr, "%s\n", AG_GetError());
		return (1);
	}
	
	AG_TextParseFontSpec("Vera.ttf:15");

#ifdef HAVE_GETOPT
	while ((c = getopt(argc, argv, "?dDvfFbBt:T:r:")) != -1) {
		extern char *optarg;

		switch (c) {
# ifdef DEBUG
		case 'd':
			agDebugLvl = 5;
			break;
		case 'D':
			agDebugLvl = 10;
			break;
# endif
		case 'v':
			exit(0);
		case 'f':
			AG_SetBool(agConfig, "view.full-screen", 1);
			break;
		case 'F':
			AG_SetBool(agConfig, "view.full-screen", 0);
			break;
		case 'r':
			fps = atoi(optarg);
			break;
		case 'b':
			AG_SetBool(agConfig, "font.freetype", 0);
			break;
		case 'B':
			AG_SetBool(agConfig, "font.freetype", 1);
			break;
		case 'T':
			AG_SetString(agConfig, "font-path", optarg);
			break;
		case 't':
			AG_TextParseFontSpec(optarg);
			break;
		case '?':
		default:
			printf("%s [-dDvfFbB] [-r fps] [-t fontspec] "
			       "[-T font-path]\n", agProgName);
			exit(0);
		}
	}
#endif /* HAVE_GETOPT */

	if (AG_InitVideo(800,600,32,AG_VIDEO_OPENGL|AG_VIDEO_RESIZABLE)
	    == -1) {
		fprintf(stderr, "%s\n", AG_GetError());
		return (-1);
	}
	SG_InitSubsystem();
	SK_InitSubsystem();
	AG_SetRefreshRate(fps);
	AG_BindGlobalKey(AG_KEY_ESCAPE, AG_KEYMOD_ANY, AG_QuitGUI);
	AG_BindGlobalKey(AG_KEY_F8, AG_KEYMOD_ANY, AG_ViewCapture);

	AG_ObjectInitStatic(&vfsRoot, NULL);
	AG_ObjectSetName(&vfsRoot, _("Editor VFS"));
	AG_MutexInit(&objLock);

	/* Register our classes. */
	RegisterClasses();

	/* Create the application menu. */ 
	appMenu = AG_MenuNewGlobal(0);
	AG_MenuDynamicItem(appMenu->root, _("File"), NULL, FileMenu, NULL);
	AG_MenuDynamicItem(appMenu->root, _("Edit"), NULL, EditMenu, NULL);
	AG_MenuDynamicItem(appMenu->root, _("Features"), NULL, FeaturesMenu,
	    NULL);

#if defined(HAVE_AGAR_DEV) && defined(DEBUG)
	DEV_InitSubsystem(0);
	if (agDebugLvl >= 5) {
		DEV_Browser(&vfsRoot);
	}
	DEV_ToolMenu(AG_MenuNode(appMenu->root, _("Debug"), NULL));
#endif
	AG_EventLoop();
	AG_ObjectDestroy(&vfsRoot);
	AG_Destroy();
	return (0);
}
