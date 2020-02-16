/*
 * Copyright (c) 2007-2009 Hypertriton, Inc. <http://hypertriton.com>
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

#include <cadtools/config/have_agar_dev.h>
#ifdef HAVE_AGAR_DEV
#include <agar/dev.h>
#endif

#include <string.h>

#ifdef HAVE_GETOPT
#include <unistd.h>
#endif

#include "cadtools.h"

#include <cadtools/config/have_getopt.h>
#include <cadtools/config/cadtools_version.h>
#include <cadtools/config/enable_nls.h>
#include <cadtools/config/localedir.h>

AG_Menu *mdiMenu = NULL;
AG_Object vfsRoot;

static void *objFocus = NULL;
static int terminating = 0;

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

/* Update the objFocus pointer (only useful in MDI mode). */
static void
WindowGainedFocus(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
	
	if (AG_OfClass(obj, "SK:*") ||
	    AG_OfClass(obj, "CAD_Part:*") ||
	    AG_OfClass(obj, "CAM_Program:*")) {
		objFocus = obj;
	} else {
		objFocus = NULL;
	}
}
static void
WindowLostFocus(AG_Event *event)
{
	objFocus = NULL;
}

/* Open a given object for edition. */
AG_Window *
CAD_OpenObject(void *p)
{
	AG_Object *obj = p;
	AG_Window *win;

	if ((win = obj->cls->edit(obj)) == NULL) {
		return (NULL);
	}
	AG_SetEvent(win, "window-close", SaveChangesDlg, "%p", obj);
	AG_AddEvent(win, "window-gainfocus", WindowGainedFocus, "%p", obj);
	AG_AddEvent(win, "window-lostfocus", WindowLostFocus, "%p", obj);
	AG_AddEvent(win, "window-hidden", WindowLostFocus, "%p", obj);
	AG_SetPointer(win, "object", obj);
	AG_PostEvent(obj, "edit-open", NULL);

	AG_WindowShow(win);
	return (win);
}

/* Create a new object of specified class and open for edition. */
void
CAD_GUI_NewObject(AG_Event *event)
{
	AG_ObjectClass *cls = AG_PTR(1);

	CAD_OpenObject(AG_ObjectNew(&vfsRoot, NULL, cls));
}

#if 0
static void
OpenMachine(AG_Event *event)
{
	AG_Object *mach = AG_PTR(1);

	CAD_OpenObject(mach);
}
#endif

/* Event handler for object open from file. */
void
CAD_GUI_OpenObject(AG_Event *event)
{
	AG_ObjectClass *cls = AG_PTR(1);
	char *path = AG_STRING(2);
	AG_Object *obj;

	if ((obj = AG_ObjectNew(&vfsRoot, NULL, cls)) == NULL) {
		goto fail;
	}
	if (AG_ObjectLoadFromFile(obj, path) == -1) {
		AG_ObjectDetach(obj);
		AG_ObjectDestroy(obj);
		goto fail;
	}
	AG_SetString(obj, "archive-path", path);
	AG_ObjectSetNameS(obj, AG_ShortFilename(path));
	CAD_OpenObject(obj);
	return;
fail:
	AG_TextMsgFromError();
}

void
CAD_GUI_OpenDlg(AG_Event *event)
{
	AG_Window *win;
	AG_FileDlg *fd;
	AG_Pane *hPane;

	win = AG_WindowNew(0);
	AG_WindowSetCaptionS(win, _("Open..."));

	hPane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	fd = AG_FileDlgNewMRU(hPane->div[0], "cadtools.mru.parts",
	    AG_FILEDLG_LOAD|AG_FILEDLG_CLOSEWIN|AG_FILEDLG_EXPAND|
	    AG_FILEDLG_ASYNC);

	AG_FileDlgSetOptionContainer(fd, hPane->div[1]);

	AG_FileDlgAddType(fd, _("cadtools sketch"), "*.sk",
	    CAD_GUI_OpenObject, "%p", &skClass);
	AG_FileDlgAddType(fd, _("cadtools part"), "*.part",
	    CAD_GUI_OpenObject, "%p", &cadPartClass);
	AG_FileDlgAddType(fd, _("cadtools program"), "*.prog",
	    CAD_GUI_OpenObject, "%p", &camProgramClass);
	CAD_PartOpenMenu(fd);

	AG_WindowShow(win);
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 40, 33);
	AG_PaneMoveDividerPct(hPane, 66);
}

static void
SaveSketchToSK(AG_Event *event)
{
	SK *sk = AG_PTR(1);
	char *path = AG_STRING(2);

	if (AG_ObjectSaveToFile(sk, path) == -1) {
		AG_TextMsgFromError();
		return;
	}
	AG_SetString(sk, "archive-path", path);
	AG_ObjectSetNameS(sk, AG_ShortFilename(path));
}

void
CAD_GUI_SaveAsDlg(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
	AG_Window *win;
	AG_FileDlg *fd;

	if (obj == NULL) {
		AG_TextError(_("No object is selected for saving"));
		return;
	}

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
	} else if (AG_OfClass(obj, "CAD_Part:*")) {
		CAD_PartSaveMenu(fd, (CAD_Part *)obj);
	}
	AG_WindowShow(win);
}

void
CAD_GUI_Save(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);

	if (obj == NULL) {
		AG_TextError(_("No object is selected for saving"));
		return;
	}
	if (!AG_Defined(obj,"archive-path")) {
		CAD_GUI_SaveAsDlg(event);
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
	AG_QuitGUI();
}

static void
AbortQuit(AG_Event *event)
{
	AG_Window *win = AG_PTR(1);

	terminating = 0;
	AG_ObjectDetach(win);
}

void
CAD_GUI_Quit(AG_Event *event)
{
	AG_Object *obj;
	AG_Window *win;
	AG_Box *box;

	if (terminating) {
		ConfirmQuit(NULL);
	}
	terminating = 1;

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
		AG_LabelNewS(win, 0,
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
		
void
CAD_InitMenuMDI(void)
{
	if ((mdiMenu = AG_MenuNewGlobal(0)) == NULL) {
		AG_Verbose("App menu: %s\n", AG_GetError());
		return;
	}
	CAD_FileMenu(AG_MenuNode(mdiMenu->root, _("File"), NULL), NULL);
	CAD_EditMenu(AG_MenuNode(mdiMenu->root, _("Edit"), NULL), NULL);
#if defined(HAVE_AGAR_DEV) && defined(CAD_DEBUG)
	DEV_InitSubsystem(0);
	DEV_ToolMenu(AG_MenuNode(mdiMenu->root, _("Debug"), NULL));
#endif
}

/* Build a generic "File" menu. */
void
CAD_FileMenu(AG_MenuItem *m, void *obj)
{
	AG_MenuItem *mDevs;

	if (obj == NULL)					/* MDI */
		obj = objFocus;

	AG_MenuActionKb(m, _("New sketch..."), agIconDoc.s,
	    AG_KEY_N, AG_KEYMOD_CTRL,
	    CAD_GUI_NewObject, "%p", &skClass);
	AG_MenuAction(m, _("New part..."), agIconDoc.s,
	    CAD_GUI_NewObject, "%p", &cadPartClass);
	AG_MenuAction(m, _("New program..."), agIconDoc.s,
	    CAD_GUI_NewObject, "%p", &camProgramClass);
	
	AG_MenuSeparator(m);

	AG_MenuActionKb(m, _("Open..."), agIconLoad.s,
	    AG_KEY_O, AG_KEYMOD_CTRL,
	    CAD_GUI_OpenDlg, NULL);
	AG_MenuActionKb(m, _("Save"), agIconSave.s,
	    AG_KEY_S, AG_KEYMOD_CTRL,
	    CAD_GUI_Save, "%p", obj);
	AG_MenuAction(m, _("Save as..."), agIconSave.s,
	    CAD_GUI_SaveAsDlg, "%p", obj);
	
	AG_MenuSeparator(m);

	mDevs = AG_MenuNode(m, _("Devices"), NULL);
	{
/*		CAM_Machine *mach; */

		AG_MenuAction(mDevs, _("New CNC lathe..."), agIconDoc.s,
		    CAD_GUI_NewObject, "%p", &camLatheClass);
		AG_MenuAction(mDevs, _("New CNC mill..."), agIconDoc.s,
		    CAD_GUI_NewObject, "%p", &camMillClass);
#if 0
		AG_MenuSeparator(mDevs);

		AGOBJECT_FOREACH_CLASS(mach, &vfsRoot, cam_machine,
		    "CAM_Machine:*") {
			AG_MenuAction(mDevs, AGOBJECT(mach)->name, agIconGear.s,
			    OpenMachine, "%p", mach);
		}
#endif
	}
	
	AG_MenuSeparator(m);
	AG_MenuActionKb(m, _("Quit"), NULL,
	    AG_KEY_Q, AG_KEYMOD_CTRL,
	    CAD_GUI_Quit, NULL);
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

/* Build a generic "Edit" menu. */
void
CAD_EditMenu(AG_MenuItem *m, void *obj)
{
	if (obj == NULL)					/* MDI */
		obj = objFocus;
	
	AG_MenuActionKb(m, _("Undo"), NULL,
	    AG_KEY_Z, AG_KEYMOD_CTRL,
	    Undo, "%p", obj);
	AG_MenuActionKb(m, _("Redo"), NULL,
	    AG_KEY_R, AG_KEYMOD_CTRL,
	    Redo, "%p", obj);
}

int
main(int argc, char *argv[])
{
	int c, i;
	char *driverSpec = "<OpenGL>";

#ifdef ENABLE_NLS
	bindtextdomain("cadtools", LOCALEDIR);
	bind_textdomain_codeset("cadtools", "UTF-8");
	textdomain("cadtools");
#endif
	if (AG_InitCore("cadtools", AG_VERBOSE|AG_CREATE_DATADIR) == -1) {
		fprintf(stderr, "InitCore: %s\n", AG_GetError());
		return (1);
	}
#ifdef HAVE_GETOPT
	while ((c = getopt(argc, argv, "?vd:t:T:")) != -1) {
		extern char *optarg;

		switch (c) {
		case 'v':
			printf("cadtools %s\n", CADTOOLS_VERSION);
			return (0);
		case 'd':
			driverSpec = optarg;
			break;
		case 'T':
			AG_SetString(agConfig, "font-path", optarg);
			break;
		case 't':
			AG_TextParseFontSpec(optarg);
			break;
		case '?':
		default:
			printf("Usage: %s [-v] [-d agar-driver-spec] "
			       "[-t font-spec] [-T font-path]\n", agProgName);
			return (1);
		}
	}
#endif /* HAVE_GETOPT */

	if (AG_InitGraphics(driverSpec) == -1) {
		goto fail;
	}
	SG_InitSubsystem();
	SK_InitSubsystem();
	AG_BindGlobalKeyEv(AG_KEY_ESCAPE, AG_KEYMOD_ANY, CAD_GUI_Quit);
	AG_BindGlobalKey(AG_KEY_F8, AG_KEYMOD_ANY, AG_ViewCapture);

	AG_ObjectInitStatic(&vfsRoot, NULL);
	AG_ObjectSetName(&vfsRoot, "cadtools");

	/* Register our classes. */
	RegisterClasses();

	/* Create the application menu. */ 
	if (agDriverSw != NULL) {
		CAD_InitMenuMDI();
	} else {
		AG_Object *objNew;

		if ((objNew = AG_ObjectNew(&vfsRoot, NULL, &cadPartClass))
		    == NULL) {
			goto fail;
		}
		if (CAD_OpenObject(objNew) == NULL)
			goto fail;
	}

#ifdef HAVE_GETOPT
	for (i = optind; i < argc; i++) {
#else
	for (i = 1; i < argc; i++) {
#endif
		AG_Event ev;
		char *ext;

		Verbose("Loading: %s\n", argv[i]);
		if ((ext = strrchr(argv[i], '.')) == NULL)
			continue;

		AG_EventInit(&ev);
		if (strcasecmp(ext, ".sk") == 0) {
			AG_EventPushPointer(&ev, "", &skClass);
		} else if (strcasecmp(ext, ".part") == 0) {
			AG_EventPushPointer(&ev, "", &cadPartClass);
		} else if (strcasecmp(ext, ".prog") == 0) {
			AG_EventPushPointer(&ev, "", &camProgramClass);
		} else {
			Verbose("Ignoring argument: %s\n", argv[i]);
			continue;
		}
		AG_EventPushString(&ev, "", argv[i]);
		CAD_GUI_OpenObject(&ev);
	}

	AG_EventLoop();
	AG_ObjectDestroy(&vfsRoot);
	AG_Destroy();
	return (0);
fail:
	fprintf(stderr, "%s\n", AG_GetError());
	AG_Destroy();
	return (1);
}
