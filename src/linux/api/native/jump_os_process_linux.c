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

static int processId = -1;
static int executivePid = -1;
static int serverPid = -1;

int 
jumpProcessGetId(void)
{
    return processId;
}

void
jumpProcessSetId(int id)
{
    processId = id;
}

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
jumpProcessGetExecutiveId(void)
{
    return executivePid;
}

int jumpProcessGetServerPid()
{
    return serverPid;
}

static void
dumpMessage(struct _JUMPMessage* mptr, char* intro)
{
    JUMPMessageReader r;
    JUMPPlatformCString* strings;
    uint32 len, i;
    JUMPMessage m = (JUMPMessage)mptr;
    
    printf("%s\n", intro);
    jumpMessageReaderInit(&r, m);
    strings = jumpMessageGetStringArray(&r, &len);
    if (r.status != JUMP_SUCCESS) {
	printf("    <failure>\n");
	return;
    }
    if (strings == NULL) {
	printf("    <null>\n");
    }
    else {
	for (i = 0; i < len; i++) {
	    printf("    \"%s\"\n", strings[i]);
	}
    }
    jumpMessageFreeStringArray(strings, len);
}

static int
getChildPid(struct _JUMPMessage* mptr)
{
    JUMPMessageReader r;
    JUMPPlatformCString* strings;
    char* pidString;
    JUMPMessage m = (JUMPMessage)mptr;
    uint32 len;
    int pid;
    
    jumpMessageReaderInit(&r, m);
    strings = jumpMessageGetStringArray(&r, &len);
    if (r.status != JUMP_SUCCESS) {
	return -1;
    }
    if (len == 0) {
	pid = -1;
	goto out;
    }
    pidString = (char*)strings[0];    
    if (strncmp(pidString, "CHILD PID=", 10) != 0) {
	pid = -1;
	goto out;
    }
    pid = (int) strtol(pidString + 10, NULL, 0);
    fprintf(stderr, "pidString=%s, pid=%d\n", pidString, pid);
  out:
    jumpMessageFreeStringArray(strings, len);
    return pid;
}

/*
 * On linux this is not how we create processes. We send a message
 * to a server to clone itself, and then we message it. That is layered
 * on top of the messaging system
 */
static int 
create_process(char **cmd_args, int argc, char** argv)
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
    
    if (cmd_args == NULL || *cmd_args == NULL) {
	/* Nothing to do */
	return -1;
    }
    outMessage = jumpMessageNewOutgoingByType(type, &code);
    jumpMessageMarkSet(&mark, outMessage);
    /*
     * We don't yet know how many strings we will be adding in, so
     * put in a placeholder for now. We marked the spot with &mark.
     */
    jumpMessageAddInt(outMessage, numWords);
    
    /* Start with the command. It should be JAPP or JNATIVE */
    jumpMessageAddString(outMessage, *cmd_args);
    numWords ++;
    cmd_args ++;
    
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

    /* Rest of the cmd argument list: e.g. ..JUMPIsolateProcessImpl */
    while (*cmd_args != NULL) {
	jumpMessageAddString(outMessage, *cmd_args);
	numWords ++;
	cmd_args ++;
    }
    
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
    /* FIXME: Must have central location for timeout values */
#define TIMEOUT 10000
    response = jumpMessageSendSync(targetAddress, outMessage, TIMEOUT, &code);
    if (response == NULL) {
        return -1;
    }
    dumpMessage(response, "Command response:");
    return getChildPid(response);
}

int 
jumpProcessCreate(int argc, char** argv)
{
    char *cmd_args[3];
    cmd_args[0] = "JAPP";
    cmd_args[1] = "com.sun.jumpimpl.isolate.jvmprocess.JUMPIsolateProcessImpl";
    cmd_args[2] = NULL;
    return create_process(cmd_args, argc, argv);
}

int 
jumpProcessRunDriver(char *driver_name, char *lib_name)
{
    int argc = 2;
    char *argv[3];
    
    argv[0] = driver_name;
    argv[1] = lib_name;
    argv[2] = NULL;
    return jumpProcessNativeCreate(argc, argv);
}

int 
jumpProcessNativeCreate(int argc, char** argv)
{
    char *cmd_args[2];
    cmd_args[0] = "JNATIVE";
    cmd_args[1] = NULL;
    return create_process(cmd_args, argc, argv);
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
