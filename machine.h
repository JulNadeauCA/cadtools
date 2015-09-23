/*	Public domain	*/

#ifndef _CADTOOLS_MACHINE_H_
#define _CADTOOLS_MACHINE_H_

#include "begin_code.h"

#define CAM_HOSTNAME_MAX	128
#define CAM_PORT_MAX		16
#define CAM_USERNAME_MAX	64
#define CAM_PASSWORD_MAX	64
#define CAM_MACHINE_DESCR_MAX	256

typedef struct cam_machine {
	struct ag_object obj;
	char descr[CAM_MACHINE_DESCR_MAX];
	char host[CAM_HOSTNAME_MAX];	 /* Machine controller hostname */
	char port[CAM_PORT_MAX];	 /* machctld port */
	char user[CAM_USERNAME_MAX];	 /* machctld login name */
	char pass[CAM_PASSWORD_MAX];	 /* machctld password */
	AG_Mutex lock;
	Uint32 flags;
#define CAM_MACHINE_ENABLED	0x01	 /* Try to contact machine */
#define CAM_MACHINE_BUSY	0x02	 /* Machine offline or busy */
#define CAM_MACHINE_DETACHING	0x04	 /* Machine being detached */
#define CAM_MACHINE_DETACHED1	0x08
#define CAM_MACHINE_DETACHED2	0x10
#define CAM_MACHINE_DETACHED	(CAM_MACHINE_DETACHED1|CAM_MACHINE_DETACHED2)

	SG *model;			 /* Simplified geometric model */
#ifdef NETWORK
	NC_Session sess;		 /* Network session with controller */
#endif
	AG_Thread thNet;		 /* Network I/O thread */
	AG_Thread thInact;		 /* Inactivity timeout thread */
	Uint32 tPong;			 /* Time of last ping */
	AG_Console *cons;		 /* Status console (or NULL) */
	AG_TAILQ_HEAD(,cam_program) upload; /* Program upload queue */
} CAM_Machine;

__BEGIN_DECLS
extern AG_ObjectClass camMachineClass;

void		  CAM_MachineLog(CAM_Machine *, const char *, ...);
int		  CAM_MachineUploadProgram(CAM_Machine *, CAM_Program *);
__END_DECLS

#include "close_code.h"
#endif	/* _CADTOOLS_MACHINE_H_ */
