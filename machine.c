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

static void *MachineThread(void *);
static void *InactivityCheck(void *);

void
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

static void
Attached(AG_Event *event)
{
	CAM_Machine *ma = AG_SELF();
	SG_Light *lt;

	AG_ThreadCreate(&ma->thNet, MachineThread, ma);
	AG_ThreadCreate(&ma->thInact, InactivityCheck, ma);
	
	ma->model = SG_New(ma, _("Geometric model"), 0);
	{
		lt = SG_LightNew(ma->model->root, "Light0");
		SG_Translate(lt, -6.0, 6.0, -6.0);
		lt->Kc = 0.5;
		lt->Kl = 0.05;
		lt = SG_LightNew(ma->model->root, "Light1");
		SG_Translate(lt, 6.0, 6.0, 6.0);
		lt->Kc = 0.25;
		lt->Kl = 0.05;
	}
	AG_ObjectPageIn(ma->model);
}

static void
Detached(AG_Event *event)
{
	CAM_Machine *ma = AG_SELF();
	int i;

	fprintf(stderr, _("Detaching %s..."), AGOBJECT(ma)->name);
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
	if (i == 10) {
		fprintf(stderr, _("%s: Threads are not responding!\n"),
		    AGOBJECT(ma)->name);
	}
out:
	fprintf(stderr, _("done\n"));
	AG_ObjectPageOut(ma->model);
	return;
}

static void
Init(void *obj)
{
	CAM_Machine *ma = obj;

	ma->flags = 0;
	ma->descr[0] = '\0';
	ma->host[0] = '\0';
	ma->user[0] = '\0';
	ma->pass[0] = '\0';
	Strlcpy(ma->port, _PROTO_MACHCTL_PORT, sizeof(ma->port));
	ma->cons = NULL;
	ma->model = NULL;
	TAILQ_INIT(&ma->upload);
	NC_Init(&ma->sess, _PROTO_MACHCTL_NAME, _PROTO_MACHCTL_VER);
	AG_SetEvent(ma, "attached", Attached, NULL);
	AG_SetEvent(ma, "detached", Detached, NULL);
	AG_MutexInitRecursive(&ma->lock);
}

static void
Destroy(void *obj)
{
	CAM_Machine *ma = obj;

	AG_PostEvent(NULL, ma, "detached", NULL);
	NC_Destroy(&ma->sess);
	AG_MutexDestroy(&ma->lock);
}

static int
Load(void *obj, AG_DataSource *buf, const AG_Version *ver)
{
	CAM_Machine *ma = obj;

	AG_CopyString(ma->descr, buf, sizeof(ma->descr));
	ma->flags = AG_ReadUint32(buf);
	AG_CopyString(ma->host, buf, sizeof(ma->host));
	AG_CopyString(ma->port, buf, sizeof(ma->port));
	AG_CopyString(ma->user, buf, sizeof(ma->user));
	AG_CopyString(ma->pass, buf, sizeof(ma->pass));
	return (0);
}

static int
Save(void *obj, AG_DataSource *buf)
{
	CAM_Machine *ma = obj;

	AG_WriteString(buf, ma->descr);
	AG_WriteUint32(buf, ma->flags);
	AG_WriteString(buf, ma->host);
	AG_WriteString(buf, ma->port);
	AG_WriteString(buf, ma->user);
	AG_WriteString(buf, ma->pass);
	return (0);
}

static void *
MachineThread(void *obj)
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
InactivityCheck(void *obj)
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
	AG_WindowSetCaption(win, _("Light settings"));
	AG_ObjectAttach(win, SGNODE_OPS(lt)->edit(lt, sgv));
	AG_WindowShow(win);
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

static void *
Edit(void *obj)
{
	CAM_Machine *ma = obj;
	AG_Window *win;
	SG_View *sgv;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;
	AG_Table *tbl;
	AG_Textbox *tb;
	AG_Button *btn;
	AG_Pane *pane;
	AG_Label *lbl;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, "%s", AGOBJECT(ma)->name);

	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);

	ntab = AG_NotebookAddTab(nb, _("Status"), AG_BOX_VERT);
	ma->cons = AG_ConsoleNew(ntab, AG_CONSOLE_EXPAND);
	
	ntab = AG_NotebookAddTab(nb, _("Controller"), AG_BOX_VERT);
	{
		tb = AG_TextboxNew(ntab, 0, _("Hostname: "));
		AG_TextboxBindUTF8(tb, ma->host, sizeof(ma->host));
		tb = AG_TextboxNew(ntab, 0, _("Port: "));
		AG_TextboxBindUTF8(tb, ma->port, sizeof(ma->port));
		tb = AG_TextboxNew(ntab, 0, _("Username: "));
		AG_TextboxBindUTF8(tb, ma->user, sizeof(ma->user));
		tb = AG_TextboxNew(ntab, 0, _("Password: "));
		AG_TextboxBindUTF8(tb, ma->pass, sizeof(ma->pass));
		AG_TextboxSetPassword(tb, 1);

		AG_SeparatorNewHoriz(ntab);

		btn = AG_ButtonNewFn(ntab, AG_BUTTON_HFILL|AG_BUTTON_STICKY,
		    _("Enable machine"), EnableMachine, "%p", ma);
		AG_WidgetBindFlag32(btn, "state", &ma->flags,
		    CAM_MACHINE_ENABLED);
	}
	ntab = AG_NotebookAddTab(nb, _("Program Queue"), AG_BOX_VERT);
	{
		pane = AG_PaneNew(ntab, AG_PANE_VERT, AG_PANE_EXPAND);
		lbl = AG_LabelNewString(pane->div[0], 0, _("Upload queue:"));
		AG_LabelSetPaddingBottom(lbl, 4);
		tbl = AG_TableNewPolled(pane->div[0], AG_TABLE_EXPAND,
		    PollUploadQueue, "%p", ma);
		AG_TableAddCol(tbl, _("Program"), "<XXXXXXXXXXXXXX>", NULL);
		AG_TableAddCol(tbl, _("Status"), NULL, NULL);

		lbl = AG_LabelNewString(pane->div[1], 0, _("Remote queue:"));
		AG_LabelSetPaddingBottom(lbl, 4);
		tbl = AG_TableNewPolled(pane->div[1], AG_TABLE_EXPAND,
		    PollControllerQueue, "%p", ma);
		AG_TableAddCol(tbl, _("Program"), "<XXXXXXXXXXXXXX>", NULL);
		AG_TableAddCol(tbl, _("Owner"), "<XXXXXXXXX>", NULL);
		AG_TableAddCol(tbl, _("Status"), NULL, NULL);
	}
	ntab = AG_NotebookAddTab(nb, _("3D model"), AG_BOX_VERT);
	{
		sgv = SG_ViewNew(ntab, ma->model, SG_VIEW_EXPAND);
		AG_WidgetFocus(sgv);
	}

	AG_WidgetFocus(sgv);
	return (nb);
}

int
CAM_MachineUploadProgram(CAM_Machine *ma, CAM_Program *prog)
{
	NC_Session *sess = &ma->sess;
	char buf[AG_BUFFER_MAX];
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

AG_ObjectClass camMachineClass = {
	"CAM_Machine",
	sizeof(CAM_Machine),
	{ 0,0 },
	Init,
	NULL,			/* reinit */
	Destroy,
	Load,
	Save,
	Edit
};
