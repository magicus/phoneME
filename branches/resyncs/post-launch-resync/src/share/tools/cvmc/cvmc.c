/*
 * 
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

#define JPORT_NUM 7777

static void
usage(const char* execName) 
{
    fprintf(stderr, "Usage: %s [-help] [-childrenexited] [-killall] [-killserver] [-warmup [-initClasses <classesList>] [-precompileMethods <methodsList>]] [-command <launchCommand>] [-host <hostname>] [-port <portnum>] [-testingmode <testprefix>] [... cvm options ...]\n",
	    execName);
    exit(1);
}

static FILE*
makeBufferedStream(int socket, int bufmode, const char* mode)
{
    FILE* stream;
    stream = fdopen(socket, mode);
    if (stream != NULL) {
	setvbuf(stream, NULL, bufmode, BUFSIZ);
    } else {
	perror("fdopen");
    }
    return stream;
}
    
/* read line and print. Return if read won't succeed any more */
static int
readLineAndPrint(int connfd, int doprint)
{
#define READ_BUF_LEN 2048
    char buf[READ_BUF_LEN];
    char* bufPtr;
    char* bufEnd;
    int numread;
    
    bufPtr = buf;
    bufEnd = &buf[READ_BUF_LEN];
    bufEnd--; /* Allow for \0 termination of overflowed strings */
    while ((numread = read(connfd, bufPtr, 1)) > 0) {
	assert(bufPtr < bufEnd);
	/* Return any <CR> or <NL>-terminated non-white-space lines */
	if ((*bufPtr == '\n') || (*bufPtr == '\r')) {
	    *bufPtr = '\0';
	    /* Terminate, duplicate and return */
	    if (doprint) fprintf(stderr, "%s\n", buf);
	    return 1;
	} else {
	    bufPtr += numread;
	}
	
	if (bufPtr >= bufEnd) {
	    *bufPtr = '\0';
	    fprintf(stderr, "WARNING: Out of buffer space\n");
	    if (doprint) {
		fprintf(stderr, "%s\n", buf);
	    }
	    return 1;
	}
    }
    if (numread < 0) {
	fprintf(stderr, "Read error, pid=%d, numread=%d\n", 
		(int)getpid(), numread);
	perror("read");
	return 0;
    } else if (numread == 0) {
	// EOF
	return 0;
    }
    return 1;
}

int 
main(int argc, const char** argv)
{
    int s, retval;
    struct sockaddr_in sin;
    int i, j;
    const char* execName;
    const char* hostname = NULL;
    const char* testingprefix = NULL;
    int portNum;
    FILE* mtask;
    const char* launchCommand = "JSYNC";
    const char* classesList = "../../src/share/appmanager/profiles/pp/classesList.txt";
    const char* methodsList = "../../src/share/appmanager/profiles/pp/methodsList.txt";
    int   isWarmup = 0;
    int   isAsync = 0;
    int   killAll = 0;
    int   killServer = 0;
    int   noVmArgs = 0;
    
    /* Fill in the Internet address we will be trying to connect to
       Let's choose JPORT_NUM first */
    sin.sin_family = AF_INET;

    /* Default port */
    portNum = JPORT_NUM;

    execName = argv[0];
    j = 1; /* argument index to read from */
    while (j < argc) {
	if (!strcmp(argv[j], "-host")) {
	    struct hostent* hent;

	    j++;
	    if ((j >= argc) || (argv[j][0] == '-')) {
		usage(execName);
	    }
	    hostname = argv[j++];
	    hent = gethostbyname(hostname);
	    if (hent == NULL) {
		fprintf(stderr, "Host not found: %s\n", hostname);
		exit(1);
	    }
	    memcpy(&sin.sin_addr, hent->h_addr, hent->h_length);
	} else if (!strcmp(argv[j], "-testingmode")) {
	    j++;
	    if ((j >= argc) || (argv[j][0] == '-')) {
		usage(execName);
	    }
	    testingprefix = argv[j++];
	} else if (!strcmp(argv[j], "-warmup")) {
	    j++;
	    launchCommand = "S sun.mtask.Warmup";
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
	} else if (!strcmp(argv[j], "-port")) {
	    const char* portstr;
	    j++;
	    if ((j >= argc) || (argv[j][0] == '-')) {
		usage(execName);
	    }
	    portstr = argv[j++];
	    portNum = strtol(portstr, NULL, 0);
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

    if (hostname == NULL) {
	hostname = "localhost";
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    sin.sin_port = htons(portNum);

/*     fprintf(stderr, "hostname=%s port=%d testingmode_prefix=%s\n", */
/* 	    hostname, portNum, testingprefix); */

    /* Create a TCP socket */
    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
	perror("Socket create failed");
	exit(1);
    }

    /* Connect to server */
    retval = connect(s, (struct sockaddr*)&sin, sizeof sin);
    if (retval < 0) {
	perror("Connect failed");
	fprintf(stderr, "Is the CVM server running on %s:%d?\n",
		inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
	exit(1);
    }

    /* Create a line-buffered command stream */
    mtask = makeBufferedStream(s, _IOLBF, "w");
    if (mtask == NULL) {
	exit(1);
    }

    if (testingprefix != NULL) {
	fprintf(mtask, "TESTING_MODE %s\n", testingprefix);
	readLineAndPrint(s, 0);
    }

    fprintf(mtask, "%s ", launchCommand);
	    
    if (isWarmup) {
	if (classesList != NULL) {
	    fprintf(mtask, "-initClasses %s ", classesList);
	}
	if (methodsList != NULL) {
	    fprintf(mtask, "-precompileMethods %s ", methodsList);
	}
    } else {
	for (i = j; i < argc; i++) {
	    fprintf(mtask, "%s ", argv[i]);
	}
    }
    fprintf(mtask, "\n");
    if (!isWarmup && !isAsync && !killAll && !noVmArgs) {
	while (readLineAndPrint(s, 1)); /* Grab output */
    } else if (!killServer) {
	readLineAndPrint(s, 1);
    }
    close(s);

    return 0;
}
