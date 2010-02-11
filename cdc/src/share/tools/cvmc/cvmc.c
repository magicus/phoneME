/*
 * 
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <ctype.h>
#include <assert.h>

#include <jump_messaging.h>
#include "porting/JUMPProcess.h"

#define TIMEOUT 5000

static void
usage(const char* execName) 
{
    fprintf(stderr, "Usage: %s -target <targetpid> [-type <messagetype>] [-help] [-childrenexited] [-killall] [-killserver] [-warmup [-initClasses <classesList>] [-precompileMethods <methodsList>]] [-command <launchCommand>] [-testingmode <testprefix>] [-setenv <Key=value>]  [... cvm options ...]\n",
	    execName);
    jumpMessageShutdown();
    exit(1);
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

int 
main(int argc, const char** argv)
{
    int i, j;
    const char* execName;
    const char* testingprefix = NULL;
    const char* launchCommand = "JAPP";
    const char* classesList = "../../src/share/appmanager/profiles/pp/classesList.txt";
    const char* methodsList = "../../src/share/appmanager/profiles/pp/methodsList.txt";
    int   isWarmup = 0;
    int   isAsync = 0;
    int   killAll = 0;
    int   killServer = 0;
    int   noVmArgs = 0;
    int   targetpid = -1;
    JUMPAddress targetAddress;
    int   numWords; /* Number of words in the message */
    JUMPMessageStatusCode code;
    JUMPMessageMark mark;
    JUMPOutgoingMessage outMessage;
    JUMPMessage response;
    const char* messageType = "mvm/server"; /* default message type */
    
    /*
     * Essential for any process doing messaging
     */
    jumpMessageStart();
    
    execName = argv[0];
    j = 1; /* argument index to read from */
    while (j < argc) {
	if (!strcmp(argv[j], "-target")) {
	    const char* targetstr;
	    
	    j++;
	    if ((j >= argc) || (argv[j][0] == '-')) {
		usage(execName);
	    }
	    targetstr = argv[j++];
	    targetpid = strtol(targetstr, NULL, 0);
	    if (!jumpProcessIsAlive(targetpid)) {
		fprintf(stderr, "Target pid=%d not alive\n", targetpid);
		usage(execName);
	    }
	    
	    targetAddress.processId = targetpid;
	} else if (!strcmp(argv[j], "-testingmode")) {
	    j++;
	    if ((j >= argc) || (argv[j][0] == '-')) {
		usage(execName);
	    }
	    testingprefix = argv[j++];
	} else if (!strcmp(argv[j], "-type")) {
	    j++;
	    if ((j >= argc) || (argv[j][0] == '-')) {
		usage(execName);
	    }
	    messageType = argv[j++];
	} else if (!strcmp(argv[j], "-warmup")) {
	    j++;
	    isWarmup = 1;
	} else if (!strcmp(argv[j], "-childrenexited")) {
	    j++;
	    noVmArgs = 1;
	    launchCommand = "CHILDREN_EXITED";
	} else if (!strcmp(argv[j], "-killall")) {
	    j++;
	    killAll = 1;
	    launchCommand = "KILLALL";
	} else if (!strcmp(argv[j], "-killserver")) {
	    j++;
	    killServer = 1;
	    launchCommand = "JEXIT";
	} else if (!strcmp(argv[j], "-help")) {
	    j++;
	    usage(execName);
	} else if (!strcmp(argv[j], "-command")) {
	    j++;
	    if ((j >= argc) || (argv[j][0] == '-')) {
		usage(execName);
	    }
	    launchCommand = argv[j++];
	    /* All launch commands apart from JSYNC are asynchronous */
	    if (strcmp(launchCommand, "JSYNC")) {
		isAsync = 1;
	    }
	} else if (!strcmp(argv[j], "-initClasses")) {
	    j++;
	    if ((j >= argc) || (argv[j][0] == '-')) {
		usage(execName);
	    }
	    classesList = argv[j++];
	} else if (!strcmp(argv[j], "-precompileMethods")) {
	    j++;
	    if ((j >= argc) || (argv[j][0] == '-')) {
		usage(execName);
	    }
	    methodsList = argv[j++];
	} else if (!strcmp(argv[j], "-setenv")) {
	    j++;
	    if ((j >= argc) || (argv[j][0] == '-')) {
		usage(execName);
	    }
	    launchCommand = "SETENV";
	} else {
	    /* Done parsing options */
	    break;
	}
    }

    /* We need some JVM arguments as well, but only if we are not warming up */
    if (!isWarmup && !killAll & !killServer && !noVmArgs) {
	if (j == argc) {
	    usage(execName);
	} 
    }

    if (targetpid == -1) {
	usage(execName);
    }
    
    fprintf(stderr, "targetpid=%d testingmode_prefix=%s\n",
	    targetpid, testingprefix);

    if (testingprefix != NULL) {
	outMessage = jumpMessageNewOutgoingByType((char*)messageType, &code);
    
	jumpMessageAddInt(outMessage, 2);
	jumpMessageAddString(outMessage, "TESTING_MODE");
	jumpMessageAddString(outMessage, (char*)testingprefix);
	response = jumpMessageSendSync(targetAddress, outMessage, TIMEOUT, 
				       &code);
	if (response == NULL) {
	    fprintf(stderr, "send message failed\n");
	    return 1;
	}
	
	dumpMessage(response, "Testing mode response:");
	jumpMessageFreeOutgoing(outMessage);
    }

    outMessage = jumpMessageNewOutgoingByType((char*)messageType, &code);
    numWords = 0;
    
    jumpMessageMarkSet(&mark, outMessage);
    /*
     * We don't yet know how many strings we will be adding in, so
     * put in a placeholder for now. We marked the spot with &mark.
     */
    jumpMessageAddInt(outMessage, numWords);
    
    if (isWarmup) {
	jumpMessageAddString(outMessage, "S"); 
	jumpMessageAddString(outMessage, "sun.misc.Warmup");
	numWords += 2;
	
	if (classesList != NULL) {
	    jumpMessageAddString(outMessage, "-initClasses");
	    jumpMessageAddString(outMessage, (char*)classesList);
	    numWords += 2;
	}
	if (methodsList != NULL) {
	    jumpMessageAddString(outMessage, "-precompileMethods");
	    jumpMessageAddString(outMessage, (char*)methodsList);
	    numWords += 2;
	}
    } else {
	jumpMessageAddString(outMessage, (char*)launchCommand);
	for (i = j; i < argc; i++) {
	    jumpMessageAddString(outMessage, (char*)argv[i]);
	}
	if (argc >= j) {
	    numWords += argc - j + 1;
	} else {
	    numWords += 1;
	}
    }

    /* Now that we know what we are sending, patch message with count */
    jumpMessageMarkResetTo(&mark, outMessage);
    jumpMessageAddInt(outMessage, numWords);
    
    /* And now, for dumping purposes */
    jumpMessageMarkResetTo(&mark, outMessage);
    dumpMessage(outMessage, "Outgoing message:");

    /* Time to send outgoing message */
    response = jumpMessageSendSync(targetAddress, outMessage, TIMEOUT, &code);
    if (response == NULL) {
	fprintf(stderr, "send message failed\n");
	return 1;
    }
	
    dumpMessage(response, "Command response:");

#if 0
    if (!isWarmup && !isAsync && !killAll && !noVmArgs) {
    } else if (!killServer) {
	readLineAndPrint(s, 1);
    }
#endif

    jumpMessageShutdown();
    
    return 0;
}
