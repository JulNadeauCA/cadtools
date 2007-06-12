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
#include <errno.h>
#include <unistd.h>

#include "cadtools.h"
#include "protocol.h"

const AG_ObjectOps camMachineOps = {
	"CAM_Machine",
	sizeof(CAM_Machine),
	{ 0,0 },
	CAM_MachineInit,
	NULL,			/* reinit */
	CAM_MachineDestroy,
	CAM_MachineLoad,
	CAM_MachineSave,
	CAM_MachineEdit
};

CAM_Machine *
CAM_MachineNew(void *parent, const char *name)
{
	CAM_Machine *ma;

	ma = Malloc(sizeof(CAM_Machine), M_OBJECT);
	CAM_MachineInit(ma, name);
	AG_ObjectAttach(parent, ma);
	return (ma);
}

static void
CAM_MachineLog(CAM_Machine *ma, const char *fmt, ...)
{
	char msg[1024];
	va_list args;

	if (ma->cons == NULL) {
		return;
	}
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);
	AG_ConsoleMsg(ma->cons, "%s", msg);
}

static void *
CAM_MachineThread(void *obj)
{
	CAM_Machine *ma = obj;
	NC_Result *res;
	int doConnect;

	for (;;) {
		AG_MutexLock(&ma->lock);
		if (ma->flags & CAM_MACHINE_DETACHING) {
			ma->flags |= CAM_MACHINE_DETACHED1;
			AG_MutexUnlock(&ma->lock);
			AG_ThreadExit(NULL);
		}
		doConnect = (ma->flags & CAM_MACHINE_ENABLED);
		AG_MutexUnlock(&ma->lock);

		if (!doConnect) {
			goto wait;
		}
		if (NC_Connect(&ma->sess, ma->host, ma->port, ma->user,
		    ma->pass) == -1) {
			CAM_MachineLog(ma, "%s", AG_GetError());
			goto wait;
		}
		if ((res = NC_Query(&ma->sess, "version\n")) != NULL) {

			/* ... */

			AG_MutexLock(&ma->lock);
			ma->tPong = SDL_GetTicks();
			AG_MutexUnlock(&ma->lock);

			NC_FreeResult(res);
		}
		NC_Disconnect(&ma->sess);
wait:
		SDL_Delay(1000);
	}
}

static void *
CAM_InactivityCheck(void *obj)
{
	CAM_Machine *ma = obj;

	for (;;) {
		AG_MutexLock(&ma->lock);
		if (ma->flags & CAM_MACHINE_DETACHING) {
			ma->flags |= CAM_MACHINE_DETACHED2;
			AG_MutexUnlock(&ma->lock);
			AG_ThreadExit(NULL);
		}
		if ((ma->flags & CAM_MACHINE_ENABLED) == 0) {
			goto skip;
		}
		if (ma->flags & CAM_MACHINE_BUSY) {
			if ((SDL_GetTicks() - ma->tPong) < 2000) {
				CAM_MachineLog(ma, _("Machine is online"));
				ma->flags &= ~CAM_MACHINE_BUSY;
			}
		} else {
			if ((SDL_GetTicks() - ma->tPong) > 2000) {
				CAM_MachineLog(ma,
				    _("Machine is busy or offline"));
				ma->flags |= CAM_MACHINE_BUSY;
			}
		}
skip:
		AG_MutexUnlock(&ma->lock);
		SDL_Delay(1000);
	}
}

static void
CAM_MachineAttached(AG_Event *event)
{
	CAM_Machine *ma = AG_SELF();
	SG_Light *lt;

	printf("%s: creating thread\n", AGOBJECT(ma)->name);
	AG_ThreadCreate(&ma->thNet, NULL, CAM_MachineThread, ma);
	AG_ThreadCreate(&ma->thInact, NULL, CAM_InactivityCheck, ma);
	
	ma->model = SG_New(ma, "Geometric model");
	{
		lt = SG_LightNew(ma->model->root, "Light0");
		SG_Translate3(lt, -6.0, 6.0, -6.0);
		lt->Kc = 0.5;
		lt->Kl = 0.05;
		lt = SG_LightNew(ma->model->root, "Light1");
		SG_Translate3(lt, 6.0, 6.0, 6.0);
		lt->Kc = 0.25;
		lt->Kl = 0.05;
	}
}

static void
CAM_MachineDetached(AG_Event *event)
{
	CAM_Machine *ma = AG_SELF();
	int i;

	AG_MutexLock(&ma->lock);
	ma->flags |= CAM_MACHINE_DETACHING;
	AG_MutexUnlock(&ma->lock);
	for (i = 0; i < 10; i++) {
		AG_MutexLock(&ma->lock);
		if (ma->flags & CAM_MACHINE_DETACHED) {
			AG_MutexUnlock(&ma->lock);
			goto out;
		}
		AG_MutexUnlock(&ma->lock);
		SDL_Delay(1000);
	}
out:
	return;
}

void
CAM_MachineInit(void *obj, const char *name)
{
	CAM_Machine *ma = obj;

	AG_ObjectInit(ma, name, &camMachineOps);
	ma->flags = 0;
	ma->descr[0] = '\0';
	ma->host[0] = '\0';
	ma->user[0] = '\0';
	ma->pass[0] = '\0';
	strlcpy(ma->port, _PROTO_MACHCTL_PORT, sizeof(ma->port));
	ma->cons = NULL;
	ma->model = NULL;
	TAILQ_INIT(&ma->upload);
	NC_Init(&ma->sess, _PROTO_MACHCTL_NAME, _PROTO_MACHCTL_VER);
	AG_SetEvent(ma, "attached", CAM_MachineAttached, NULL);
	AG_SetEvent(ma, "detached", CAM_MachineDetached, NULL);
}

void
CAM_MachineDestroy(void *obj)
{
	CAM_Machine *ma = obj;

	NC_Destroy(&ma->sess);
}

int
CAM_MachineLoad(void *obj, AG_Netbuf *buf)
{
	CAM_Machine *ma = obj;

	if (AG_ReadObjectVersion(buf, ma, NULL) != 0) {
		return (-1);
	}
	AG_CopyString(ma->descr, buf, sizeof(ma->descr));
	ma->flags = AG_ReadUint32(buf);
	AG_CopyString(ma->host, buf, sizeof(ma->host));
	AG_CopyString(ma->port, buf, sizeof(ma->port));
	AG_CopyString(ma->user, buf, sizeof(ma->user));
	AG_CopyString(ma->pass, buf, sizeof(ma->pass));
	return (0);
}

int
CAM_MachineSave(void *obj, AG_Netbuf *buf)
{
	CAM_Machine *ma = obj;

	AG_WriteObjectVersion(buf, ma);
	AG_WriteString(buf, ma->descr);
	AG_WriteUint32(buf, ma->flags);
	AG_WriteString(buf, ma->host);
	AG_WriteString(buf, ma->port);
	AG_WriteString(buf, ma->user);
	AG_WriteString(buf, ma->pass);
	return (0);
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
PollSpecs(AG_Event *event)
{
	AG_Table *tbl = AG_SELF();
	CAM_Machine *ma = AG_PTR(1);
}

static void
PollUploadQueue(AG_Event *event)
{
	AG_Table *tbl = AG_SELF();
	CAM_Machine *ma = AG_PTR(1);
}

static void
PollControllerQueue(AG_Event *event)
{
	AG_Table *tbl = AG_SELF();
	CAM_Machine *ma = AG_PTR(1);
}

static void
EnableMachine(AG_Event *event)
{
	CAM_Machine *ma = AG_PTR(1);

	AG_MutexLock(&ma->lock);
	if (ma->host[0] == '\0' || ma->port[0] == '\0') {
		AG_TextMsg(AG_MSG_ERROR, _("Hostname/port not specified"));
		ma->flags &= ~CAM_MACHINE_ENABLED;
	}
	if (ma->user[0] == '\0' || ma->pass[0] == '\0') {
		AG_TextMsg(AG_MSG_ERROR, _("Username/password not specified"));
		ma->flags &= ~CAM_MACHINE_ENABLED;
	}
	if (ma->flags & CAM_MACHINE_ENABLED) {
		CAM_MachineLog(ma, _("Machine enabled"));
	}
	AG_MutexUnlock(&ma->lock);
}

void *
CAM_MachineEdit(void *obj)
{
	CAM_Machine *ma = obj;
	AG_Window *win;
	AG_Menu *menu;
	AG_MenuItem *pitem, *subitem;
	SG_View *sgv;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;
	AG_Table *tbl;
	AG_Box *box;
	AG_Textbox *tb;
	AG_Checkbox *cb;
	AG_Pane *pane;
	AG_Label *lbl;
	AG_Button *btn;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, "%s", AGOBJECT(ma)->name);

	sgv = Malloc(sizeof(SG_View), M_OBJECT);
	SG_ViewInit(sgv, ma->model, SG_VIEW_EXPAND);

	menu = AG_MenuNew(win, AG_MENU_HFILL);
	pitem = AG_MenuAddItem(menu, _("File"));
	{
		AG_ObjMgrGenericMenu(pitem, ma);
		AG_MenuSeparator(pitem);
		AG_MenuActionKb(pitem, _("Close"), CLOSE_ICON,
		    SDLK_w, KMOD_CTRL, AGWINCLOSE(win));
	}
	pitem = AG_MenuAddItem(menu, _("Edit"));
	{
		AG_MenuAction(pitem, _("Undo"), -1, NULL, NULL);
		AG_MenuAction(pitem, _("Redo"), -1, NULL, NULL);
	}
	pitem = AG_MenuAddItem(menu, _("Model View"));
	{
		AG_MenuAction(pitem, _("Default"), -1,
		    SetViewCamera, "%p,%s", sgv, "Camera0");
		AG_MenuAction(pitem, _("Light0 settings..."), -1,
		    ShowLightSettings, "%p,%s", sgv, "Light0");
		AG_MenuAction(pitem, _("Light1 settings..."), -1,
		    ShowLightSettings, "%p,%s", sgv, "Light1");
	}
	
	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
	ntab = AG_NotebookAddTab(nb, _("Status"), AG_BOX_VERT);
	{
		ma->cons = AG_ConsoleNew(ntab, AG_CONSOLE_EXPAND);
	}
	ntab = AG_NotebookAddTab(nb, _("Settings"), AG_BOX_VERT);
	{
		tb = AG_TextboxNew(ntab, AG_TEXTBOX_HFILL, _("Hostname: "));
		AG_WidgetBindString(tb, "string", ma->host, sizeof(ma->host));
		tb = AG_TextboxNew(ntab, AG_TEXTBOX_HFILL, _("Port: "));
		AG_WidgetBindString(tb, "string", ma->port, sizeof(ma->port));
		tb = AG_TextboxNew(ntab, AG_TEXTBOX_HFILL, _("Username: "));
		AG_WidgetBindString(tb, "string", ma->user, sizeof(ma->user));
		tb = AG_TextboxNew(ntab, AG_TEXTBOX_HFILL, _("Password: "));
		AG_WidgetBindString(tb, "string", ma->pass, sizeof(ma->pass));
		AG_TextboxSetPassword(tb, 1);

		AG_SeparatorNewHoriz(ntab);

		btn = AG_ButtonAct(ntab, AG_BUTTON_HFILL|AG_BUTTON_STICKY,
		    _("Enable machine"), EnableMachine, "%p", ma);
		AG_WidgetBindFlag32(btn, "state", &ma->flags,
		    CAM_MACHINE_ENABLED);
	}
	ntab = AG_NotebookAddTab(nb, _("Specs"), AG_BOX_VERT);
	{
		tbl = AG_TableNewPolled(ntab, AG_TABLE_EXPAND,
		    PollSpecs, "%p", ma);
		AG_TableAddCol(tbl, _("Parameter"), "<XXXXXXXXXXXXXX>", NULL);
		AG_TableAddCol(tbl, _("Value"), NULL, NULL);
	}
	ntab = AG_NotebookAddTab(nb, _("Program Queue"), AG_BOX_VERT);
	{
		pane = AG_PaneNew(ntab, AG_PANE_VERT, AG_PANE_EXPAND);
		lbl = AG_LabelNewStatic(pane->div[0], _("Upload queue:"));
		AG_LabelSetPaddingBottom(lbl, 4);
		tbl = AG_TableNewPolled(pane->div[0], AG_TABLE_EXPAND,
		    PollUploadQueue, "%p", ma);
		AG_TableAddCol(tbl, _("Program"), "<XXXXXXXXXXXXXX>", NULL);
		AG_TableAddCol(tbl, _("Status"), NULL, NULL);

		lbl = AG_LabelNewStatic(pane->div[1], _("Remote queue:"));
		AG_LabelSetPaddingBottom(lbl, 4);
		tbl = AG_TableNewPolled(pane->div[1], AG_TABLE_EXPAND,
		    PollControllerQueue, "%p", ma);
		AG_TableAddCol(tbl, _("Program"), "<XXXXXXXXXXXXXX>", NULL);
		AG_TableAddCol(tbl, _("Owner"), "<XXXXXXXXX>", NULL);
		AG_TableAddCol(tbl, _("Status"), NULL, NULL);
	}
	ntab = AG_NotebookAddTab(nb, _("Geometric model"), AG_BOX_VERT);
	{
		AG_ObjectAttach(ntab, sgv);
		AG_WidgetFocus(sgv);
	}
	return (win);
}

int
CAM_MachineUploadProgram(CAM_Machine *ma, CAM_Program *prog)
{
	NC_Session *sess = &ma->sess;
	char buf[BUFSIZ];
	char path[AG_OBJECT_PATH_MAX];
	char prog_name[AG_OBJECT_PATH_MAX];
	char digest[AG_OBJECT_DIGEST_MAX];
	size_t wrote = 0, len, rv;
	FILE *f;

	if (AG_ObjectSave(prog) == -1) {
		return (-1);
	}
	if (AG_ObjectCopyName(prog, prog_name, sizeof(prog_name)) == -1 ||
	    AG_ObjectCopyDigest(prog, &len, digest) == -1 ||
	    AG_ObjectCopyFilename(prog, path, sizeof(path)) == -1)
		return (-1);

	/* TODO locking */
	if ((f = fopen(path, "r")) == NULL) {
		AG_SetError("%s: %s", path, strerror(errno));
		return (-1);
	}
	if (NC_Write(sess,
	    "prog-commit\n" 
	    "prog-name=%s\n"
	    "prog-size=%lu\n"
	    "prog-digest=%s\n\n",
	    prog_name, (Ulong)len, digest) == -1)
		goto fail_close;
	
	if (NC_Read(sess, 12) <= 2 || sess->read.buf[0] != '0') {
		AG_SetError(_("Server refused data: %s"), &sess->read.buf[2]);
		goto fail_close;
	}
	while ((rv = fread(buf, 1, sizeof(buf), f)) > 0) {
		size_t nw;

		nw = write(sess->sock, buf, rv);
		if (nw == 0) {
			AG_SetError(_("EOF from server"));
			goto fail_close;
		} else if (nw < 0) {
			AG_SetError(_("Write error"));
			goto fail_close;
		}
		wrote += nw;
	}
	if (wrote < len) {
		AG_SetError(_("Upload incomplete"));
		goto fail_close;
	}
	if (NC_Read(sess, 32) < 1 || sess->read.buf[0] != '0' ||
	    sess->read.buf[1] == '\0') {
		AG_SetError(_("Commit failed: %s"), sess->read.buf);
		goto fail_close;
	}
	AG_TextTmsg(AG_MSG_INFO, 4000,
	    _("Program %s successfully uploaded to %s.\n"
	      "Size: %lu bytes\n"
	      "Response: %s\n"),
	      AGOBJECT(prog)->name, AGOBJECT(ma)->name,
	      (Ulong)wrote, &sess->read.buf[2]);

	fclose(f);
	return (0);
fail_close:
	fclose(f);
	return (-1);
}
