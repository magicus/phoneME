/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <jvmdi.h>
#include "util.h"
#include "transport.h"
#include "debugLoop.h"
#include "debugDispatch.h"
#include "standardHandlers.h"
#include "commonRef.h"
#include "inStream.h"
#include "outStream.h"
#include "JDWP.h"
#include "threadControl.h"
#include "debugInit.h"


static void reader(void *);
static void enqueue(struct Packet *p);
static jboolean dequeue(struct Packet *p);
static void notifyTransportError(void);

struct PacketList {
    struct Packet packet;
    struct PacketList *next;
};

static volatile struct PacketList *cmdQueue;
static JVMDI_RawMonitor cmdQueueLock;
static jboolean transportError;

static jboolean
lastCommand(struct CmdPacket *cmd)
{
    if ((cmd->cmdSet == JDWP_COMMAND_SET(VirtualMachine)) &&
        ((cmd->cmd == JDWP_COMMAND(VirtualMachine, Dispose)) ||
         (cmd->cmd == JDWP_COMMAND(VirtualMachine, Exit)))) {
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}
/*
 * This is where all the work gets done.
 */
  
void
debugLoop_run()
{
    jboolean shouldListen;
    struct Packet p;
    
    /* Initialize all statics */
    /* We may be starting a new connection after an error */
    cmdQueue = NULL;
    cmdQueueLock = debugMonitorCreate("JDWP Command Queue Lock");
    transportError = JNI_FALSE; 

    shouldListen = JNI_TRUE;

    spawnNewThread(reader, NULL, "JDWP Command Reader");

    standardHandlers_onConnect();
    threadControl_onConnect();

    /* Okay, start reading cmds! */
    while (shouldListen) {
        if (!dequeue(&p)) {
            break;
        }

        if (p.type.cmd.flags & FLAGS_Reply) {
            /*
             * Its a reply packet.
             */
            
        } else {
            /*
             * Its a cmd packet.
             */
            struct CmdPacket *cmd = &p.type.cmd;
            PacketInputStream in;
            PacketOutputStream out;
            CommandHandler func;

            /* Should reply be sent to sender.
             * For error handling, assume yes, since 
             * only VM/exit does not reply
             */
            jboolean replyToSender = JNI_TRUE; 

            /* Initialize the input and output streams */
            inStream_init(&in, p);
            outStream_initReply(&out, inStream_id(&in));

            func = debugDispatch_getHandler(cmd->cmdSet,cmd->cmd);
            if (func == NULL) {
                /* we've never heard of this, so I guess we
                 * haven't implemented it.
                 * Handle gracefully for future expansion
                 * and platform / vendor expansion.
                 */
                outStream_setError(&out, 
                                   JDWP_Error_NOT_IMPLEMENTED);
            } else if (vmDead && 
             ((cmd->cmdSet) != JDWP_COMMAND_SET(VirtualMachine))) {
                /* Protect the VM from calls while dead.
                 * VirtualMachine cmdSet quietly ignores some cmds
                 * after VM death, so, it sends it's own errors.
                 */
                outStream_setError(&out, JDWP_Error_VM_DEAD);
            } else {
                /* Call the command handler */
                replyToSender = func(&in, &out);
            }

            /* Reply to the sender */
            if (replyToSender) {
                if (inStream_error(&in)) {
                    outStream_setError(&out, inStream_error(&in));
                } 
                outStream_sendReply(&out);
            }

            inStream_destroy(&in);
            outStream_destroy(&out);
                
            shouldListen = !lastCommand(cmd);
        }
    }
    threadControl_onDisconnect();
    standardHandlers_onDisconnect();

    /*
     * Cut off the transport immediately. This has the effect of 
     * cutting off any events that the eventHelper thread might
     * be trying to send.
     */
    transport_close();
    debugMonitorDestroy(cmdQueueLock);
    debugInit_reset(transportError);
}

static void 
reader(void *unused)
{
    struct Packet packet;
    struct CmdPacket *cmd;
    jboolean shouldListen = JNI_TRUE;

    while (shouldListen) {
        jint error;

        error = transport_receivePacket(&packet);

        /* Is it a valid packet? */
        if (error != 0) {
	    if (errno != EBADF) {
		perror("transport_receivePacket");
		fprintf(stderr,"Transport error, error code = %d (%d)\n",
		    error, errno);
	    }
            shouldListen = JNI_FALSE;
            notifyTransportError();
        } else {
            cmd = &packet.type.cmd;

            /* 
             * We still need to deal with high priority
             * packets and queue flushes!
             */
            enqueue(&packet);

            shouldListen = !lastCommand(cmd);
        }
    }
}

/*
 * The current system for queueing packets is highly
 * inefficient, and should be rewritten! It'd be nice
 * to avoid any additional mallocs.
 */

static void 
enqueue(struct Packet *packet)
{
    struct PacketList *pL;
    struct PacketList *walker;

    pL = jdwpAlloc((size_t)sizeof(struct PacketList));
    if (pL == NULL) {
        ALLOC_ERROR_EXIT();
    }

    pL->packet = *packet;
    pL->next = NULL;

    /*fprintf(stderr,"Starting packet enqueue.\n");*/

    debugMonitorEnter(cmdQueueLock);

    if (cmdQueue == NULL) {
        cmdQueue = pL;
        debugMonitorNotify(cmdQueueLock);
    } else {
        walker = (struct PacketList *)cmdQueue;
        while (walker->next != NULL)
            walker = walker->next;

        walker->next = pL;
    }

    debugMonitorExit(cmdQueueLock);

    /*fprintf(stderr,"Done with packet enqueue.\n");*/
}

static jboolean 
dequeue(Packet *packet) {
    struct PacketList *node = NULL;

    /*fprintf(stderr,"Starting packet dequeue.\n");*/

    debugMonitorEnter(cmdQueueLock); 

    while (!transportError && (cmdQueue == NULL)) {
        debugMonitorWait(cmdQueueLock);
        /*fprintf(stderr,"dequeue got notify!\n");*/
    }

    if (cmdQueue != NULL) {
        node = (struct PacketList *)cmdQueue;
        cmdQueue = node->next;
    }
    debugMonitorExit(cmdQueueLock);

    /*fprintf(stderr,"Done with packet dequeue.\n");*/

    if (node != NULL) {
        *packet = node->packet;
        jdwpFree(node);
    }
    return (node != NULL);
}

static void 
notifyTransportError() {
    debugMonitorEnter(cmdQueueLock); 
    transportError = JNI_TRUE;
    debugMonitorNotify(cmdQueueLock);
    debugMonitorExit(cmdQueueLock);
}
