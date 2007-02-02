/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "porting/JUMPProcess.h"
#include "jump_messaging.h"

/*
 * Process implementation
 */
int 
jumpProcessGetId()
{
    return getpid();
}

static int executivePid = -1;
static int serverPid = -1;

void
jumpProcessSetExecutiveId(int execPid)
{
    executivePid = execPid;
}

void
jumpProcessSetServerPid(int sPid)
{
    serverPid = sPid;
}

int 
jumpProcessGetExecutiveId()
{
    return executivePid;
}

static void
dumpMessage(struct _JUMPMessage* mptr, char* intro)
{
    JUMPMessageReader r;
    JUMPPlatformCString* strings;
    uint32 len, i;
    JUMPMessage m = (JUMPMessage)mptr;
    
    jumpMessageReaderInit(&r, m);
    strings = jumpMessageGetStringArray(&r, &len);
    printf("%s\n", intro);
    for (i = 0; i < len; i++) {
	printf("    \"%s\"\n", strings[i]);
    }
}

static int
getChildPid(struct _JUMPMessage* mptr)
{
    JUMPMessageReader r;
    JUMPPlatformCString* strings;
    char* pidString;
    JUMPMessage m = (JUMPMessage)mptr;
    uint32 len;
    
    jumpMessageReaderInit(&r, m);
    strings = jumpMessageGetStringArray(&r, &len);
    pidString = (char*)strings[0];    
    if (strncmp(pidString, "CHILD PID=", 10) != 0) {
	return -1;
    } else {
	int pid;
	pidString += 10;
	pid = strtol(pidString, NULL, 0);
	fprintf(stderr, "pidString=%s, pid=%d\n", pidString-10, pid);
	return pid;
    }
}

/*
 * On linux this is not how we create processes. We send a message
 * to a server to clone itself, and then we message it. That is layered
 * on top of the messaging system
 */
int 
jumpProcessCreate(int argc, char** argv)
{
    JUMPPlatformCString type = "mvm/server";
    JUMPOutgoingMessage outMessage;
    JUMPMessage response;
    JUMPMessageMark mark;
    JUMPAddress targetAddress;
    JUMPMessageStatusCode code;
    int numWords = 0;
    int i;
    char * vmArgs, *s;
    
    outMessage = jumpMessageNewOutgoingByType(type);
    jumpMessageMarkSet(&mark, outMessage);
    /*
     * We don't yet know how many strings we will be adding in, so
     * put in a placeholder for now. We marked the spot with &mark.
     */
    jumpMessageAddInt(outMessage, numWords);
    
    jumpMessageAddString(outMessage, "JAPP");
    
    /*
     * The argv[0] is the VM arugment, which needs to be placed
     * right after 'JAPP'.
     */
    vmArgs = argv[0];
    if (strcmp(vmArgs, "")) {
        s = strchr(vmArgs, ' ');
        while (s != NULL) {
            *s = '\0';
            jumpMessageAddString(outMessage, vmArgs);
            numWords ++;
            vmArgs = s + 1;
            s = strchr(vmArgs, ' ');
        }
        if (*vmArgs != '\0') {
            jumpMessageAddString(outMessage, vmArgs);
            numWords ++;
	}
    }

    jumpMessageAddString(outMessage,
        "com.sun.jumpimpl.isolate.jvmprocess.JUMPIsolateProcessImpl");
    numWords += 2; /* JAPP + JUMPIsolateProcessImpl */
    
    /* 
     * If we do argc, argv[] for main(), this is how we would put those in
     */
    for (i = 1; i < argc; i++) {
	jumpMessageAddString(outMessage, (char*)argv[i]);
    }
    numWords = numWords + argc - 1;

    /* Now that we know what we are sending, patch message with count */
    jumpMessageMarkResetTo(&mark, outMessage);
    jumpMessageAddInt(outMessage, numWords);
    
    /* And now, for dumping purposes */
    jumpMessageMarkResetTo(&mark, outMessage);
    dumpMessage(outMessage, "Outgoing message:");

    /* Time to send outgoing message */
    targetAddress.processId = serverPid;
    response = jumpMessageSendSync(targetAddress, outMessage, 0, &code);
    dumpMessage(response, "Command response:");
    return getChildPid(response);
}

/*
 * On linux, /proc/<pid> exists for each live process. stat() that.
 */
int
jumpProcessIsAlive(int pid)
{
    char name[40];
    struct stat s;
    int status;
    
    snprintf(name, 40, "/proc/%d", pid);
    status = stat((const char*)name, &s);
    if (status == -1) {
	return 0;
    } else {
	return 1;
    }
}
