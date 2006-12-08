/*
 * @(#)mtask.c	1.37 06/10/10
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
 *
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include "javavm/export/jni.h"
#include "javavm/export/cvm.h"
#include "portlibs/ansi_c/java.h"
#include "portlibs/posix/mtask.h"

#include "zip_util.h"

static CVMUint32
getNumTokens(const char *string)
{
    const char *strPtr;
    CVMUint32 numEntries;

    numEntries = 0;
    strPtr = string;
    while (strPtr != NULL) {
	numEntries++;
	strPtr = strchr(strPtr, ' ');
	if (strPtr != NULL) {
	    strPtr ++;
	}
    }
    return numEntries;
}

/*
 * Given a message that encodes command line arguments, extract
 * tokens in the form of argc, argv[]
 */
static void
tokenizeArgs(const char *message, int *argcPtr, char ***argvPtr)
{
    const char *mptr;
    const char *mptr2;
    int argc;
    char** argv;
    CVMUint32 numTokens;
    CVMBool done;
    
    numTokens = getNumTokens(message);
    argv = (char**)calloc(numTokens, sizeof(char*));
    argc = 0;
    /*
    fprintf(stderr, "TOKENIZING INTO %d TOKENS: \"%s\"\n",
	    numTokens, message);
    */
    mptr = message;
    done = CVM_FALSE;
    while (!done) {
	int len;
	
	mptr2 = strchr(mptr, ' ');
	if (mptr2 == NULL) {
	    mptr2 = mptr + strlen(mptr);
	    done = CVM_TRUE;
	}
	/* Extract current token */
	len = mptr2 - mptr;
	/* Make it into a token. */
	argv[argc] = (char*)malloc(len + 1);
	strncpy(argv[argc], mptr, len);
	argv[argc][len] = '\0'; /* terminate */
	/*
	  fprintf(stderr, "\tTOKEN: \"%s\"\n", argv[argc]);
	*/
	argc++;
	/* Skip all white space */
	while(isspace(*mptr2)) {
	    mptr2++;
	}

	mptr = mptr2;
	if (*mptr == '\0') {
	    /* Nothing more to tokenize */
	    done = CVM_TRUE;
	}
    }
    *argcPtr = argc;
    *argvPtr = argv;
}

/*
 * Free the resulting (argc, argv[]) set returned by tokenizeArgs()
 */
static void
freeArgs(int argc, char** argv)
{
    int i;
    for (i = 0; i < argc; i++) {
	if (argv[i] != NULL) {
	    free(argv[i]);
	    argv[i] = NULL;
	}
    }
    free(argv);
}
/* Return the next line. Read from connfd */
static char*
readRequestLine(int connfd)
{
#define READ_BUF_LEN 4096
    char buf[READ_BUF_LEN];
    char* bufPtr;
    char* bufEnd;
    int numread;
    CVMBool whiteSpace;
    
    /* Byte-by-byte read to get \n terminated
       strings. */
    bufPtr = buf;
    bufEnd = &buf[READ_BUF_LEN];
    bufEnd--; /* Allow for \0 termination of overflowed strings */
    /* All white space until proven otherwise */
    whiteSpace = CVM_TRUE;
    while ((numread = read(connfd, bufPtr, 1)) > 0) {
	assert(bufPtr < bufEnd);
#if 0
	{
	    bufPtr[1] = '\0';
	    fprintf(stderr, "\tREADER read '%s' (%d)\n",
		    bufPtr,
		    *bufPtr);
	}
#endif
	if (!isspace(*bufPtr)) {
#if 0
	    bufPtr[1] = '\0';
	    fprintf(stderr, "SETTING WHITE SPACE TO FALSE DUE TO \"%s\" (%d)",
		    bufPtr, *bufPtr);
#endif
	    whiteSpace = CVM_FALSE;
	}
	/* Return any <CR> or <NL>-terminated non-white-space lines */
	if ((*bufPtr == '\n') || (*bufPtr == '\r')) {
	    if (!whiteSpace) {
		*bufPtr = '\0';
		/* Terminate, duplicate and return */
		/* The command processor frees this duplicate */
		return strdup(buf);
	    } else {
#if 0
 		fprintf(stderr, "ALL WHITE SPACE, continuing to read\n");
#endif
		/* Reset buffer */
		bufPtr = buf;
	    }
	} else {
	    bufPtr += numread;
	}
	
	if (bufPtr >= bufEnd) {
	    *bufPtr = '\0';
	    fprintf(stderr, "WARNING: Out of buffer space\n");
	    return strdup(buf);
	}
    }
    if (numread == -1) {
	fprintf(stderr, "Read error, pid=%d\n", (int)getpid());
	perror("read");
    }
    /* Connection lost or EOF */
    return NULL;
}

static char* 
readRequestLineWithTimeout(int connfd, int timeout)
{
    fd_set rfds;
    struct timeval tv;
    int retval;
    int intr = 0;  /* Has select() been interrupted? */

    FD_ZERO(&rfds);
    FD_SET(connfd, &rfds);
    
    do {
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	
	retval = select(connfd + 1, &rfds, NULL, NULL, &tv);
	/* Don't rely on the value of tv now! */

	if (retval == -1) {
	    perror("select()");
	    if (errno == EINTR) {
/* 		fprintf(stderr, "Select interrupted, trying again\n"); */
		intr = 1;
	    } else {
		return NULL;
	    }
	} else if (retval > 0) {
	    /* Data is available now */
	    assert(FD_ISSET(connfd, &rfds));
	    return readRequestLine(connfd);
	} else {
	    /* Timeout */
	    fprintf(stderr, "TIMEOUT: No data within %d seconds.\n", timeout);
	    return NULL;
	}
    } while (intr);

    return NULL;
}

static void
setupRequest(JNIEnv*env, int argc, char** argv, 
	     CVMInt32 commSocket, CVMInt32 clientId)
{
    char* appclasspathStr = NULL;
    char* bootclasspathStr = NULL;
    JavaVMInitArgs* clientArgs;
    char* parse;
    int cpadd;

    clientArgs = ANSIargvToJinitArgs(argc - 1, argv + 1, 
				     &appclasspathStr,
				     &bootclasspathStr);

    /* Now take this set of init args, and parse them */
    parse = CVMparseCommandLineOptions(env, clientArgs, clientArgs->nOptions);

    if (parse != NULL) {
	/* If the parse failed for some reason, the parser must have already
	   printed out the necessary message on the console. Exit with an
	   error code. */
	fprintf(stderr, "Exiting JVM\n");
	exit(1);
    }

    /* The JVM state now reflects the arguments. Also amend the classpath
       setting with the value that we read from appclasspathStr and
       bootclasspathStr */
    cpadd = CVMclassClassPathAppend(env, appclasspathStr, bootclasspathStr);
    if (!cpadd) {
	/* If the class path add failed for some reason, the system must have
	   already printed out the necessary message on the console. Exit with
	   an error code. */
	fprintf(stderr, "Exiting JVM\n");
	exit(1);
    }

    /* Record the client ID */
    CVMmtaskClientId(env, clientId);

    /* And record the socket to be used for communication with the parent */
    CVMmtaskServerCommSocket(env, commSocket);

#ifdef CVM_JVMDI
    if (clientId != 0) {
	CVMmtaskJvmdiInit(env);
    }
#endif

#ifdef CVM_JVMPI
    if (clientId != 0) {
	CVMmtaskJvmpiInit(env);
    }
#endif

    /* Free up the (argc,arv[]) and jInitArgs that we have built */
    ANSIfreeJinitArgs(clientArgs);
    freeArgs(argc, argv);

    /* Now we are ready to execute code */
#ifdef CVM_TIMESTAMPING
    if (clientId != 0) {
	CVMmtaskTimeStampRecord(env, "Child Created", -1);
    }
#endif

}

/*
 * Initialize a server waiting on a given port
 *
 * Return fd of the server.
 */
static int
serverListen(JNIEnv* env, CVMUint16 port)
{
    int s, retval;
    struct sockaddr_in sin;
    int true = 1;
    
    /* Fill in the Internet address we will be listening on:
       Use localhost, and the given port */
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    
    /* Create a TCP socket */
    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
	perror("Socket create failed");
	exit(1);
    }
    /* Don't let the socket linger across re-starts of the server */
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &true, sizeof true);

    /* Bind it to a local address at port JPORT */
    retval = bind(s, (struct sockaddr*)&sin, sizeof sin);
    if (retval < 0) {
	perror("Bind failed");
	exit(1);
    }
    
    /* Listen */
    retval = listen(s, 5);
    if (retval < 0) {
	perror("Listen failed");
	exit(1);
    }

    /* Listening succeeded, so record this port number */
    CVMmtaskServerPort(env, port);

    return s;
}

static void child(int sig);

static int
serverAcceptConnection(JNIEnv* env, int serverfd)
{
    int connfd;
    struct sockaddr_in peer;
    int peerLen = sizeof(peer);
    int intr = 0;
    
    do {
	connfd = accept(serverfd, (struct sockaddr*)&peer, &peerLen);
	if (connfd != -1) {
	    /* fprintf(stderr, "Connection received from %s:%d\n",
	       inet_ntoa(peer.sin_addr), ntohs(peer.sin_port)); */
	} else {
	    if (errno == EINTR) {
		intr = 1;
	    }
	    perror("Accept");
	}
    } while (intr);
    return connfd;
}

/*
 * Task management 
 */
typedef struct _TaskRec {
    int pid;
    char* command;
    int commSocket;
    FILE* commSocketOutFp;

    /* testing mode specific snapshot of prefix */
    char* testingModeFilePrefixSnapshot;

    struct _TaskRec* next;
    /* Any other task info? */
} TaskRec;

/* Single process for the mtask server, so these globals are OK */
static int numTasks = 0;
static TaskRec* taskList = NULL;
static int reapChildrenFlag = 0;

static int executivePid = -1;
static int executiveCommFd = -1;
static char* executiveTestingModeFilePrefixSnapshot = NULL;

/*
 * Handle exiting children so they don't hang around as zombies.
 */
static void child(int sig)
{
    int pid = getpid();

    fprintf(stderr, "Received SIGCHLD in PID=%d\n", pid);
    reapChildrenFlag = 1;
}

static void 
dumpTaskOneWithFp(TaskRec* task, FILE *fp, int verbose)
{
    if (verbose) {
	fprintf(fp, 
		 "[Task pid=%d, command=\"%s\"(0x%x), comm=%d]\n",
		 task->pid, task->command, 
		 (unsigned int)task->command, task->commSocket);
    } else {
	fprintf(fp, "PID=%d COMMAND=\"%s\"\n", task->pid, task->command);
    }
}

static void
dumpTaskOne(TaskRec* task)
{
    // Verbose printout on stderr
    dumpTaskOneWithFp(task, stderr, 1);
}

static void
freeTask(TaskRec* task)
{
    fprintf(stderr, "FREEING TASK: ");
    dumpTaskOne(task);
    
    close(task->commSocket);
    free(task->command);
    if (task->testingModeFilePrefixSnapshot != NULL) {
	free(task->testingModeFilePrefixSnapshot);
    }
    free(task);
    numTasks--;
}

/*
 * Find TaskRec corresponding to given pid
 */
static TaskRec* 
findTaskFromPid(int taskPid)
{
    TaskRec* task;

    for (task = taskList; task != NULL; task = task->next) {
	if (task->pid == taskPid) {
	    return task;
	}
    }
    return NULL;
}

/*
 * removeTask with a given pid
 *
 * Called after the SIGCHLD signal handler signaled that there are
 * children exiting
 */
static void
removeTask(int taskPid)
{
    TaskRec* task;
    TaskRec* taskPrev = NULL;

    for (task = taskList; task != NULL; taskPrev = task, task = task->next) {
	if (task->pid == taskPid) {
	    if (taskPrev == NULL) {
		/* The first item in the list */
		taskList = task->next;
	    } else {
		taskPrev->next = task->next;
	    }
	    freeTask(task);
	    return;
	}
    }
}

/*
 * Create log file off of 'prefix', named 'kind' for 'pid'
 * Return fd
 */
static int
createLogFile(char* prefix, char* kind, int pid)
{
    /* We want to open files for stdout, stderr, and exit
       codes. */
    int prefixLen = strlen(prefix) + 1;
    const int maxPidLen = 6;
    const int kindLength = strlen(kind) + 1;
    const int maxFileLen = 
	prefixLen +
	maxPidLen +
	kindLength;
    char* fileName = calloc(1, maxFileLen);
    int fd;

    if (fileName == NULL) {
	fprintf(stderr, "Out of memory trying to open log file %s\n",
		fileName);
	return -1;
    }
    sprintf(fileName, "%s/%s.%d", prefix, kind, pid);

    /*fprintf(stderr, "%s fileName = %s\n", kind, fileName);*/
    fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
	perror(fileName);
    }
    return fd;
}

static FILE*
makeLineBufferedStream(int socket)
{
    FILE* stream;
    stream = fdopen(socket, "w");
    if (stream != NULL) {
	setvbuf(stream, NULL, _IOLBF, BUFSIZ);
    }
    return stream;
}
    
static void
printExitStatusString(FILE* out, int status)
{
    if (WIFEXITED(status)) {
	fprintf(out, "<exit code %d>\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
	fprintf(out, "<uncaught signal %d>\n", WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
	fprintf(out, "<stopped with signal %d>\n", WSTOPSIG(status));
    } 
}

/*
 * The signal handler indicated there are children to reap. Do it.
 * Return last status reaped.
 */
static int
doReapChildren(ServerState* state, int options)
{
    int cpid, status;
    struct rusage ru;

    /* Reset the flag early. If the signal handler re-raises this, we will
       discover this in the next iteration of reapChildren() */
    reapChildrenFlag = 0;
    while ((cpid = wait3(&status, options, &ru)) > 0) {
	int exitcodefd;

	/* This had better be one of ours */
	assert((executivePid == cpid) || (findTaskFromPid(cpid) != NULL));

	fprintf(stderr, "Reaping child process %d, ", cpid);
	printExitStatusString(stderr, status);
	fprintf(stderr, "\t user time %ld.%02d sec\n",
		ru.ru_utime.tv_sec,
		(int)(ru.ru_utime.tv_usec / 1000 / 10));
	fprintf(stderr, "\t sys time %ld.%02d sec\n",
		ru.ru_stime.tv_sec,
		(int)(ru.ru_stime.tv_usec / 1000 / 10));
	fprintf(stderr, "\t max res set %ld\n",
		ru.ru_maxrss);
#if 0
	fprintf(stderr, "\t integral shared mem size %ld\n",
		ru.ru_ixrss);
	fprintf(stderr, "\t integral unshared data size %ld\n",
		ru.ru_idrss);
	fprintf(stderr, "\t integral unshared stack size %ld\n",
		ru.ru_isrss);
#endif
	fprintf(stderr, "\t page reclaims (minor faults) %ld\n",
		ru.ru_minflt);
	fprintf(stderr, "\t page faults (major) %ld\n",
		ru.ru_majflt);
	fprintf(stderr, "\t swaps %ld\n",
		ru.ru_nswap);
#if 0
	fprintf(stderr, "\t block input operations %ld\n",
		ru.ru_inblock);
	fprintf(stderr, "\t block output operations %ld\n",
		ru.ru_oublock);
#endif
	if (state->isTestingMode) {
	    TaskRec* task = findTaskFromPid(cpid);
	    char* testingModeFilePrefixSnapshot;
	    
	    /* Now make a record, but only if TESTING_MODE was executed
	       _prior_ to launching this currently reaped task. We can't
	       apply current testing mode to an app that did not know
	       about it */
	    if (executivePid == cpid) {
		testingModeFilePrefixSnapshot = 
		    executiveTestingModeFilePrefixSnapshot;
	    } else {
		testingModeFilePrefixSnapshot = 
		    task->testingModeFilePrefixSnapshot;
	    }
	    if (testingModeFilePrefixSnapshot != NULL) {
		/* Use the snapshot of the file prefix created at launch */
		exitcodefd = createLogFile(testingModeFilePrefixSnapshot,
					   "exitcode", cpid);
		if (exitcodefd == -1) {
		    perror("exitcodefile");
		} else {
		    FILE* exitcodefile = makeLineBufferedStream(exitcodefd);
		    printExitStatusString(exitcodefile, status);
		    fclose(exitcodefile);
		    close(exitcodefd);
		}
	    }
	}
	if (executivePid == cpid) {
	    close(executiveCommFd);
	    /* Get rid of all traces */
	    executivePid = -1;
	    executiveCommFd = -1;
	    if (executiveTestingModeFilePrefixSnapshot != NULL) {
		free(executiveTestingModeFilePrefixSnapshot);
		executiveTestingModeFilePrefixSnapshot = NULL;
	    }
	} else {
	    removeTask(cpid);
	}
    }
    return status;
}

static void
reapChildren(ServerState* state)
{
    doReapChildren(state, WNOHANG);
}

/*
 * A new task has been created 
 */
static int
addTask(JNIEnv* env, ServerState* state,
	int taskPid, char* command, int commSocket)
{
    TaskRec* task;

    task = (TaskRec*)calloc(1, sizeof(TaskRec));
    if (task == NULL) {
	return JNI_FALSE;
    }
    task->pid = taskPid;
    task->command = strdup(command);
    task->commSocket = commSocket;
    assert(task->commSocket > 2);
    task->commSocketOutFp = makeLineBufferedStream(commSocket);
    /* Take a snapshot of the testing mode prefix here */
    if (state->isTestingMode) {
	task->testingModeFilePrefixSnapshot =
	    strdup(state->testingModeFilePrefix);
	if (task->testingModeFilePrefixSnapshot == NULL) {
	    fprintf(stderr, "Could not allocate file prefix snapshot\n");
	    free(task->command);
	    free(task);
	    return JNI_FALSE;
	}
    } else {
	task->testingModeFilePrefixSnapshot = NULL;
    }
    if (task->commSocketOutFp == NULL) {
	perror("fdopen");
	free(task->command);
	free(task);
	return JNI_FALSE;
    }
#if 0
    fprintf(stderr, "Added task=0x%x: [pid=%d, \"%s\"(0x%x)]\n",
	    task, taskPid, task->command, (unsigned int)task->command);
#endif
    numTasks++;
    /* Insert at the head */
    task->next = taskList;
    taskList = task;
    return JNI_TRUE;
}

/* Sequence numbers for outgoing messages */
static CVMUint32 messageId = 1;

/*
 * Send message.
 * If isSync == true, wait for response, and return it.
 * For any problems, return NULL
 */
static char*
sendMessageToTask(TaskRec* task, char* message, int isSync)
{
    int retval;
    CVMUint32 thisMessageId = messageId++;
    char thisMessageIdStr[11];

    sprintf(thisMessageIdStr, "%d", thisMessageId);
    /* Write to per-task socket */
    retval = fprintf(task->commSocketOutFp, "%s/%s\n",
		     message, thisMessageIdStr);
    if (retval <= 0) {
	return NULL;
    }
    if (isSync) {
	do {
	    char* line = readRequestLineWithTimeout(task->commSocket, 6);
	    char* id;
	    if (line == NULL) {
		/* Timed out or peer closed some other bad thing happened. 
		   Return failure */
		return NULL;
	    }
	    id = strchr(line, '/');
	    if (id == NULL) {
		/* No '/' in the response -- bad! */
		fprintf(stderr, "MTASK: Illegal response=%s\n", line);
		return NULL;
	    }
	    /* Split id and line */
	    *id = '\0';
	    id = id+1;
/* 	    fprintf(stderr, "MTASK: Response=%s, ID=%s\n", line, id); */
	    if (!strcmp(id, thisMessageIdStr)) {
		/* We found the matching id. Return this line */
		return line;
	    } else {
		fprintf(stderr, "MTASK: IGNORE old response=\"%s\", ID=%s\n", 
			line, id);
	    }
	} while(1);
    } else {
	return "<success>";
    }
}

static CVMBool
sendMessageToTaskWithBinaryResponse(TaskRec* task, char* message)
{
    char* buf = sendMessageToTask(task, message, 1);
    
    if (buf != NULL) {
	/*fprintf(stderr, "Read response=[%s]\n", buf);*/
	if (buf[0] == '0') {
	    return CVM_FALSE;
	} else {
	    return CVM_TRUE;
	}
    }
    return CVM_FALSE;
}

static CVMBool
broadcastToAll(char* message)
{
    TaskRec* task;
    CVMBool result = CVM_TRUE;
    
    for (task = taskList; task != NULL; task = task->next) {
	if (!sendMessageToTask(task, message, 0)) {
	    result = CVM_FALSE;
	}
    }
    return result;
}

/*
 * Send a message, expect true/false response
 */
static CVMBool
sendMessageWithBinaryResponse(int taskPid, char* message)
{
    TaskRec* task;
    task = findTaskFromPid(taskPid);
    if (task == NULL) {
	return CVM_FALSE;
    }
    return sendMessageToTaskWithBinaryResponse(task, message);
}

/*
 * Send a message, expect true/false response
 */
static char*
sendMessageWithResponse(int taskPid, char* message)
{
    TaskRec* task;
    task = findTaskFromPid(taskPid);
    if (task == NULL) {
	return NULL;
    }
    return sendMessageToTask(task, message, 1);
}

#if 0
/*
 * Send a message, do not expect a response
 */
CVMBool
sendMessage(int taskPid, char* message)
{
    TaskRec* task;
    task = findTaskFromPid(taskPid);
    if (task == NULL) {
	return CVM_FALSE;
    }
    return sendMessageToTask(task, message, 0) != NULL;
}
#endif

static CVMBool
killTaskFromTaskRec(TaskRec* task)
{
#if 0
    /*
     * Message based exit
     * Not so robust, so commenting out for now.
     */
    return sendMessageToTask(task, "EXIT");
#else
    close(task->commSocket);
    
    if (kill(task->pid, SIGKILL) == -1) {
	perror("kill");
	return CVM_FALSE;
    }
    /* Killing is "best effort" and needs to be verified via
       Java, preferably using Xlet.getState() api's */
    return CVM_TRUE;
#endif
}

static CVMBool
killTask(int taskPid)
{
    TaskRec* task;
    task = findTaskFromPid(taskPid);
    if (task == NULL) {
	return CVM_FALSE;
    }
    return killTaskFromTaskRec(task);
}

static CVMBool
killAllTasks()
{
    TaskRec* task;
    CVMBool result = CVM_TRUE;
    
    for (task = taskList; task != NULL; task = task->next) {
	if (!killTaskFromTaskRec(task)) {
	    result = CVM_FALSE;
	}
    }
    return result;
}

static void
closeAllFdsExcept(int fd)
{
    TaskRec* task;
    
    for (task = taskList; task != NULL; task = task->next) {
	assert(task->commSocket != fd);
	close(task->commSocket);
    }
    if (executiveCommFd != -1) {
	assert(executiveCommFd != fd);
	close(executiveCommFd);
    }
}

static int
numberOfTasks()
{
    int num = 0;
    TaskRec* task;
    
    for (task = taskList; task != NULL; task = task->next) {
	num++;
    }
    return num;
    
}

/*
 * List tasks
 */
static void
dumpTasks(JNIEnv* env, FILE *connfp)
{
    TaskRec* task;
    int numTasks;
    
    numTasks = numberOfTasks();
    fprintf(connfp, "MULTILINE %d\n", numTasks);
    
    for (task = taskList; task != NULL; task = task->next) {
	dumpTaskOneWithFp(task, connfp, 0);   /* Brief printout to caller */
	dumpTaskOne(task);                    /* Verbose printout to console */
    }
}

/* 
 * pthread's will not survive across a fork. Therefore, allow stopping
 * and re-starting of system threads for sanity across app launching.
 */
static void
stopSystemThreads(ServerState* state)
{
    jclass referenceClass;
    jmethodID stopMethod;
    JNIEnv *env = state->env;
    
    referenceClass = state->referenceClass;
    stopMethod = state->stopSystemThreadsID;
    
    /*
     * Call sun.misc.ThreadRegistry.waitAllSystemThreadsExit()
     * to shutdown system threads
     */
    (*env)->CallStaticVoidMethod(env, referenceClass, stopMethod);
    
    if ((*env)->ExceptionOccurred(env)) {
	/*
	 * Ignore and clear the exception
	 */
	CVMconsolePrintf("Exception occurred during "
			 "ThreadRegistry.waitAllSystemThreadsExit()\n");
	(*env)->ExceptionDescribe(env);
	return;
    }
    /* Shut down the fd's of any zip files we've cached. We can't fork
       with open fd's. */
    ZIP_Closefds();

#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)
    CVMmtaskHandleSuspendChecker();
#endif
}

static jboolean
restartSystemThreads(JNIEnv* env, ServerState* state)
{
    jclass referenceClass;
    jmethodID restartMethod;
    
    /* Ready to execute code, so re-open cached zip fd's */
    if (!ZIP_Reopenfds()) {
	return JNI_FALSE;
    }

    referenceClass = state->referenceClass;
    restartMethod = state->restartSystemThreadsID;
    
    /*
     * Call sun.misc.ThreadRegistry.waitAllSystemThreadsExit()
     * to shutdown system threads
     */
    (*env)->CallStaticVoidMethod(env, referenceClass, restartMethod);
    
    if ((*env)->ExceptionOccurred(env)) {
	/*
	 * Ignore and clear the exception
	 */
	CVMconsolePrintf("Exception occurred during "
			 "java.lang.ref.Reference.restartSystemThreads()\n");
	(*env)->ExceptionDescribe(env);
	return JNI_FALSE;
    }
    return JNI_TRUE;
}

/*
 * A JVM server. Sleep waiting for new requests. As new ones come in,
 * fork off a process to handle each and go back to sleep. 
 *
 * return JNI_FALSE if a fork'ed child executes a request.
 * return JNI_TRUE if the parent sources the command
 */
static jboolean
waitForNextRequest(JNIEnv* env, ServerState* state)
{
    char* command = NULL;
    CVMBool done;
    int   serverfd;
    int   connfd;
    FILE *connfp;
    int   childrenExited = 0; /* No one has exited yet */
    CVMBool  isSync = CVM_FALSE;
    
    done = CVM_FALSE;
    /* 
     * Set up the networking for the server
     * It might have been initialized already, in which case don't
     * touch the fd that was passed in.
     */
    assert(state->initialized);
    serverfd = state->serverfd;
    connfd = state->connfd;
    connfp = state->connfp;
    
    while (!done) {
	int argc;
	char** argv;
	int pid;
	
	/* Sleep waiting for requests. When a request comes in, extract
	   arguments, fork and execute request */
	if (connfd == -1) {
	    connfd = serverAcceptConnection(env, serverfd);
	    state->connfd = connfd;
	    /* If the server runs into problems accepting a connection
	       it should exit */
	    if (connfd == -1) {
		exit(1);
	    }
	    connfp = state->connfp = makeLineBufferedStream(connfd);
	    if (connfp == NULL) {
		perror("fdopen");
		exit(1);
	    }
	}
	
	/* Accepted a connection. Now accept commands from this 
	   connection */
	command = readRequestLine(connfd);
	while (command != NULL) {
	    int appSockets[2];
	    
	    tokenizeArgs(command, &argc, &argv);
	    /*
	     * Check for children to reap before each command
	     * is executed 
	     */
	    while (reapChildrenFlag) {
		/* Remember we have detected dead children */
		childrenExited = 1; 
		reapChildren(state);
	    }
    
	    if (!strcmp(argv[0], "JEXIT")) {
		close(connfd);
		close(serverfd);
		/* Simply exit here. That's what JEXIT is supposed to do. */
		exit(0);
	    } else if (!strcmp(argv[0], "QUIT")) {
		/* QUIT this connection */
		/* Don't forget to free up all the (argc, argv[]) mem. */
		freeArgs(argc, argv);
		free(command);
		/* Make sure */
		argc = 0;
		argv = NULL;
		command = NULL;
		break;
	    } else if (!strcmp(argv[0], "SETENV")) {
		/* set environment variable */
		if (argc != 2) {
#define SETENV_USAGE   "Usage: SETENV <keyValuePair>\n"
#define SETENV_SUCCESS "SETENV succeeded\n"
		    fprintf(connfp, SETENV_USAGE);
		} else {
		    char* pair;
		    pair = strdup(argv[1]);
		    
		    if (pair == NULL) {
			/* setenv failed */
			fprintf(connfp, SETENV_USAGE);
		    } else {
			char* name = pair;
			char* value = strchr(pair, '=');
			if (value == NULL) {
			    /* setenv failed */
			    fprintf(connfp, SETENV_USAGE);
			} else {
			    *value = '\0';
			    value++;
			    fprintf(stderr, "setenv(%s, %s)\n", name, value);
			    if (*value == '\0') {
				/* unsetenv */
#ifdef __linux__
				unsetenv(name);
#else
				value[-1] = '=';
				putenv(name);
#endif
				/* setenv succeeded */
				fprintf(connfp, SETENV_SUCCESS);
			    } else {
				/* setenv */
#ifdef __linux__
				if (setenv(name, value, 1) == 0) {
#else
				value[-1] = '=';
				if (putenv(name) == 0) {
#endif
				    /* setenv succeeded */
				    fprintf(connfp, SETENV_SUCCESS);
				} else {
				    /* setenv failed */
				    fprintf(connfp, SETENV_USAGE);
				}
			    }
			}
		    }
		}
		/* The man page does not say whether setenv
		   makes a copy of the arguments. So I don't know
		   whether I can free the strdup'ed argv[1].
		   Be conservative and retain it here. */
		freeArgs(argc, argv);
		/* Make sure */
		argc = 0;
		argv = NULL;
		free(command);
		command = readRequestLine(connfd);
		continue;
	    } else if (!strcmp(argv[0], "LIST")) {
		/* Don't forget to free up all the (argc, argv[]) mem. */
		freeArgs(argc, argv);
		/* Make sure */
		argc = 0;
		argv = NULL;
		dumpTasks(env, connfp);
		free(command);
		command = readRequestLine(connfd);
		continue;
	    } else if (!strcmp(argv[0], "CHILDREN_EXITED")) {
		/* Don't forget to free up all the (argc, argv[]) mem. */
		freeArgs(argc, argv);
		/* Make sure */
		argc = 0;
		argv = NULL;
		if (childrenExited) {
		    childrenExited = 0;
		    fprintf(connfp, "YES\n");
		} else {
		    fprintf(connfp, "NO\n");
		}
		free(command);
		command = readRequestLine(connfd);
		continue;
	    } else if (!strcmp(argv[0], "KILLALL") ||
		       !strcmp(argv[0], "KILLEMALL")) {
#define KILLALL_SUCCESS "KILL succeeded\n"
		killAllTasks();
		/* Kill succeeded. Say so */
		fprintf(connfp, KILLALL_SUCCESS);
		/* Don't forget to free up all the (argc, argv[]) mem. */
		freeArgs(argc, argv);
		/* Make sure */
		argc = 0;
		argv = NULL;
		free(command);
		command = readRequestLine(connfd);
		continue;
	    } else if (!strcmp(argv[0], "KILL")) {
		/* KILL a given task */
		if (argc != 2) {
#define KILL_USAGE   "Usage: KILL <taskId>\n"
#define KILL_SUCCESS "KILL succeeded\n"
		    fprintf(connfp, KILL_USAGE);
		} else {
		    char* pidStr = argv[1];
		    /* Try to convert to integer */
		    CVMInt32 pid = CVMoptionToInt32(pidStr);
		    
		    if (pid == -1) {
			/* Bad integer */
			fprintf(connfp, KILL_USAGE);
		    } else if (!killTask(pid)) {
			/* Kill failed for some reason */
			/* Just make a best effort here */
			fprintf(connfp, KILL_USAGE);
		    } else {
			/* Kill succeeded. Say so */
			fprintf(connfp, KILL_SUCCESS);
		    }
		}
		
		freeArgs(argc, argv);
		/* Make sure */
		argc = 0;
		argv = NULL;
		free(command);
		command = readRequestLine(connfd);
		continue;
	    } else if (!strcmp(argv[0], "BROADCAST")) {
		/* BROADCAST to all launched apps */
		if (argc != 2) {
#define BROADCAST_USAGE   "Usage: BROADCAST <message>\n"
#define BROADCAST_SUCCESS "BROADCAST succeeded\n"
		    fprintf(connfp, BROADCAST_USAGE);
		} else {
		    char* message = argv[1];
		    
		    if (!broadcastToAll(message)) {
			/* Broadcast failed for some reason */
			/* Just make a best effort here */
			fprintf(connfp, BROADCAST_USAGE);
		    } else {
			/* Broadcast succeeded. Say so */
			fprintf(connfp, BROADCAST_SUCCESS);
		    }
		}
		
		freeArgs(argc, argv);
		/* Make sure */
		argc = 0;
		argv = NULL;
		free(command);
		command = readRequestLine(connfd);
		continue;
	    }  else if (!strcmp(argv[0], "TESTING_MODE")) {
		/* Set up testing mode, with a file prefix */
		if (argc != 2) {
#define TESTING_MODE_USAGE   "Usage: TESTING_MODE <prefix>\n"
#define TESTING_MODE_SUCCESS "TESTING_MODE succeeded\n"
		    fprintf(connfp, TESTING_MODE_USAGE);
		} else {
		    char* prefix = argv[1];
		    
		    state->isTestingMode = JNI_TRUE;
		    state->testingModeFilePrefix = strdup(prefix);
		    if (state->testingModeFilePrefix == NULL) {
			fprintf(connfp, TESTING_MODE_USAGE);
		    } else {
			fprintf(connfp, TESTING_MODE_SUCCESS);
		    }
		}
		
		freeArgs(argc, argv);
		/* Make sure */
		argc = 0;
		argv = NULL;
		free(command);
		command = readRequestLine(connfd);
		continue;
	    } else if (!strcmp(argv[0], "MESSAGE")) {
		if (argc != 3) {
#define MESSAGE_USAGE   "Usage: MESSAGE <taskId> <message>\n"
#define MESSAGE_SUCCESS "MESSAGE succeeded\n"
		    fprintf(connfp, MESSAGE_USAGE);
		} else {
		    char* pidStr = argv[1];
		    char* message = argv[2];
		    /* Try to convert to integer */
		    CVMInt32 pid = CVMoptionToInt32(pidStr);
		    
		    if (pid == -1) {
			/* Bad integer */
			fprintf(connfp, MESSAGE_USAGE);
		    } else if (!sendMessageWithBinaryResponse(pid, message)) {
			/* Message failed for some reason */
			/* Just make a best effort here */
			fprintf(connfp, MESSAGE_USAGE);
		    } else {
			/* Message succeeded. Say so */
			fprintf(connfp, MESSAGE_SUCCESS);
		    }
		}
		
		freeArgs(argc, argv);
		/* Make sure */
		argc = 0;
		argv = NULL;
		free(command);
		command = readRequestLine(connfd);
		continue;
	    } else if (!strcmp(argv[0], "MESSAGE_RESPONSE")) {
		if (argc != 3) {
#define MESSAGE_RESPONSE_USAGE   "Usage: MESSAGE_RESPONSE <taskId> <message>\n"
#define MESSAGE_RESPONSE_SUCCESS "MESSAGE_RESPONSE succeeded\n"
		    fprintf(connfp, MESSAGE_RESPONSE_USAGE);
		} else {
		    char* pidStr = argv[1];
		    char* message = argv[2];
		    char* response = NULL;
		    /* Try to convert to integer */
		    CVMInt32 pid = CVMoptionToInt32(pidStr);
		    
		    if (pid == -1) {
			/* Bad integer */
			response = NULL;
		    } else {
			response = sendMessageWithResponse(pid, message);
		    }
		    if (response == NULL) {
			/* Message failed for some reason */
			/* Just make a best effort here */
			fprintf(connfp, MESSAGE_RESPONSE_USAGE);
		    } else {
			/* Message succeeded. Return response */
			fprintf(connfp, "%s\n", response);
		    }
		}
		
		freeArgs(argc, argv);
		/* Make sure */
		argc = 0;
		argv = NULL;
		free(command);
		command = readRequestLine(connfd);
		continue;
	    } else if (!strcmp(argv[0], "S")) {
		/* Always return a response to the connection. */
		fprintf(connfp, "SOURCING \"%s\"\n", command);
		/* .. and to the server console */
		fprintf(stderr, "SOURCING \"%s\"\n", command);
		free(command);
		command = NULL;
		/* In the parent process, setup request and return to caller */
		setupRequest(env, argc, argv, -1, 0);
		/* Make sure */
		argc = 0;
		argv = NULL;
		/* Who would free up argc and argv? */
		return JNI_TRUE;
	    } else if (argv[0][0] != 'J') { /* Not equal to "J*" */
		/* If the command is not J, then it is not recognized. */
		/* Read next line and re-process. */
		fprintf(connfp, "Illegal command \"%s\"\n", argv[0]);
		/* Don't forget to free up all the (argc, argv[]) mem. */
		freeArgs(argc, argv);
		/* Make sure */
		argc = 0;
		argv = NULL;
		free(command);
		command = readRequestLine(connfd);
		continue;
	    }
	    fprintf(stderr, "Executing new command: \"%s\"\n", command);
	    if (!strcmp(argv[0], "JSYNC")) {
		isSync = CVM_TRUE;
	    }
#if 0
	    {
		int j;
		for(j = 0; j < argc; j++) {
		    fprintf(stderr, "\tARGV[%d] = \"%s\"\n", j, argv[j]);
		}
	    }
#endif

	    /* OK, we are ready to fork. Let's create a socketpair() for
	       this particular application so we can communicate with it */
	    if (socketpair(AF_UNIX, SOCK_STREAM, 0, appSockets) == -1) {
		perror("socketpair");
		exit(1);
	    }
#if 0
	    fprintf(stderr, "SOCKETPAIR() = (%d, %d)\n",
		    appSockets[0], appSockets[1]);
#endif
	    
	    /* Fork off a process, and handle the request */
	    if ((pid = fork()) == 0) {
		int mypid = getpid();
		/* Child process */
#ifdef CVM_TIMESTAMPING
		if (!CVMmtaskTimeStampReinitialize(env)) {
		    fprintf(stderr, 
			    "Could not reinitialize timestamping, exiting\n");
		    exit(1);
		}
#endif		
		close(appSockets[1]); /* Close the parent part of the
					 socket pair */
		/* Make sure all the fd's we inherit from the parent
		   get freed up */
		closeAllFdsExcept(appSockets[0]);
		
		if (state->isTestingMode) {
		    char* prefix = state->testingModeFilePrefix;
		    /* We want to open files for stdout and stderr */
		    int outfd = createLogFile(prefix, "stdout", mypid);
		    int errfd = createLogFile(prefix, "stderr", mypid);
		    if ((outfd == -1) || (errfd == -1)) {
			/* Due to some error that was reported in
			   createLogFile() */
			fprintf(stderr, "MTASK: Could not set debug mode\n");
		    } else {
			/* Hook up stdout and stderr in the child process
			   to the right files */
			dup2(outfd, 1);
			dup2(errfd, 2);
			close(outfd);
			close(errfd);
		    }
		} else if (isSync) {
		    /* If we are in JSYNC execution, route stdout
		       and stderr back where the request came from */
		    dup2(connfd, 1);
		    dup2(connfd, 2);
		}

		/* First, make sure that the child PID is communicated
		   to the client connection. */
		fprintf(connfp, "CHILD PID=%d\n", mypid);

		/* No need for the connections in the child */
		state->connfd = -1;
		state->connfp = NULL;
		fclose(connfp);
		close(serverfd);
		free(command);

		/* We need these threads in the child */
		if (!restartSystemThreads(env, state)) {
		    exit(1);
		}
		
		/* In the child process, setup request and return to caller */
		setupRequest(env, argc, argv, appSockets[0], mypid);
		/* Make sure */
		argc = 0;
		argv = NULL;
		return JNI_FALSE;
	    } else if (pid == -1) {
		perror("Fork");
		freeArgs(argc, argv);
		free(command);
		isSync = CVM_FALSE;
	    } else {
		fprintf(stderr, "SPAWNED OFF PID=%d\n", pid);
		close(appSockets[0]); /* Close the child part of the
					 socket pair */
		
		/* Add this new task into our "database" unless
		   launching via JDETACH */
		if (!strcmp(argv[0], "JDETACH")) {
		    /* Remember this as a special pid. */
		    executivePid = pid;
		    executiveCommFd = appSockets[1];
		    if (state->isTestingMode) {
			executiveTestingModeFilePrefixSnapshot =
			    strdup(state->testingModeFilePrefix);
		    }
		} else {
		    addTask(env, state, pid, command, appSockets[1]);
		}
		/*
		 * If we are launching an xlet, wait for the launch to
		 * complete
		 */
		if (!strcmp(argv[0], "JXLET")) {
		    /* Tell target JVM to wait for xlet initialization */
		    sendMessageWithBinaryResponse(pid, "WAIT_FOR_LAUNCH");
		}
		free(command);
		/* The child is executing this command. The parent
		   can free the arguments */
		/* Don't forget to free up all the (argc, argv[]) mem. */
		freeArgs(argc, argv);
		/* Don't free 'command' here, since it's been put in
		   the task structure. We'll free it when we free the
		   task */
		if (isSync) {
		    /* Wait for child to exit */
		    int status = doReapChildren(state, 0);
		    printExitStatusString(connfp, status);
		    isSync = CVM_FALSE;
		    /* This being a sync call, blow the caller off. */
		    command = NULL;
		    continue;
		}
	    }
	    command = readRequestLine(connfd);
	}

	/* fprintf(stderr, "Quitting this connection, waiting for new one\n"); */
	    
	/* No commands. Close connfd and loop back */
	fclose(connfp);
	connfd = state->connfd = -1;
	connfp = state->connfp = NULL;
    }
    /* Parent exits? Should sleep until the last child exits */
    exit(0);
}

int
MTASKnextRequest(ServerState *state)
{
    JNIEnv *env = state->env;

    /* Any other server specific initialization goes here */

    /* Sleep waiting for a request. The client process
       returns here, while the parent goes back to sleep. */
    
    /* There is one exception to this, and that is the "SOURCE"
       command. This allows the server to execute a program in its
       own address space rather than a child's, allowing it to
       expand its state */
    state->wasSourceCommand = waitForNextRequest(env, state);
    if (state->wasSourceCommand) {
	/* Prepare to process any weakrefs emerging from this
	   command */
	if (!restartSystemThreads(env, state)) {
	    return 0;
	}
    } else {
	jmethodID createListenerID;
	jclass listenerClass = (*env)->FindClass(env, "sun/mtask/Listener");

	if (listenerClass == NULL) {
	    return 0;
	}
	createListenerID =
	    (*env)->GetStaticMethodID(env, listenerClass,
				      "createListener",
				      "()V");
	if (createListenerID == NULL) {
	    (*env)->DeleteLocalRef(env, listenerClass);
	    return 0;
	}
	
	(*env)->CallStaticVoidMethod(env, listenerClass, createListenerID);
	(*env)->DeleteLocalRef(env, listenerClass);
	if ((*env)->ExceptionOccurred(env)) {
	    (*env)->ExceptionDescribe(env);
	}
    }
    return 1;
}

void
MTASKserverSourceReset(ServerState *state)
{
    /* Clear sun.misc.CVM's notion of what the main class was */
    (*state->env)->CallStaticVoidMethod(state->env, state->cvmClass, state->resetMainID);
    /* And stop threads */
    stopSystemThreads(state);
}

int
MTASKserverInitialize(ServerState* state,
    CVMParsedSubOptions* serverOpts,
    JNIEnv* env, jclass cvmClass)
{
#define DEFAULT_JPORT_NUM 7777
    CVMUint32 serverPort = DEFAULT_JPORT_NUM;
    CVMBool stdIo = CVM_FALSE;
    const char* clist = CVMgetParsedSubOption(serverOpts, "initClasses");
    const char* mlist = CVMgetParsedSubOption(serverOpts, "precompileMethods");
    /* First see if a port has been set explicitly */
    const char* portStr = CVMgetParsedSubOption(serverOpts, "port");
    if (portStr == NULL) {
	const char* stdioStr = CVMgetParsedSubOption(serverOpts, "stdio");
	if (stdioStr != NULL) {
	    stdIo = CVM_TRUE;
	}
    } else {
	CVMInt32 port = CVMoptionToInt32(portStr);
	if (port >= 0xffff || port < 0) {
	    fprintf(stderr, "Invalid port %s, using default: %d\n",
		    portStr, DEFAULT_JPORT_NUM);
	} else {
	    serverPort = port;
	}
    }
    /*
     * Set these up while server is being initialized.
     * This way we don't have to look up these JNI ID's again.
     */
    state->referenceClass =
	(*env)->FindClass(env, "java/lang/ref/Reference");
    assert(state->referenceClass != NULL);

    state->restartSystemThreadsID = 
	(*env)->GetStaticMethodID(env, state->referenceClass,
				  "restartReferenceThreads",
				  "()V");
    assert(state->restartSystemThreadsID != NULL);
    
    state->stopSystemThreadsID = 
	(*env)->GetStaticMethodID(env, state->referenceClass,
				  "stopReferenceThreads",
				  "()V");
    assert(state->stopSystemThreadsID != NULL);
    
    {
	struct sigaction sa;
	sa.sa_handler = child;
	sa.sa_flags = SA_RESTART;
#ifdef SA_NOCLDSTOP
	sa.sa_flags |= SA_NOCLDSTOP;
#endif
	sigemptyset(&sa.sa_mask);
	sigaction(SIGCHLD, &sa, NULL);
    }

    if (stdIo) {
	fprintf(stderr, "Starting mTASK server to listen on stdin .... ");
	state->serverfd = -1;
	state->connfd = 0;
	state->connfp = stdout;
    } else {
	fprintf(stderr, "Starting mTASK server to listen on port %d  .... ",
		serverPort);
	state->serverfd = serverListen(env, serverPort);
	state->connfd = -1;
	state->connfp = NULL;
    }
    state->env = env;
    state->cvmClass = cvmClass;
    state->initialized = JNI_TRUE;
    state->isTestingMode = JNI_FALSE;
    state->testingModeFilePrefix = NULL;

     /*
      * Now let's warmup. Make sure we do this before stopSystemThreads(),
      * because stopSystemThreads() will also close all open fd's. We need the
      * fd's, becuase we may need to load classes.
      */
    if (clist != NULL || mlist != NULL) {
	jclass warmupClass;
        jmethodID runitID;
        jstring jclist;
        jstring jmlist;
        warmupClass = (*env)->FindClass(env, "sun/mtask/Warmup");
        if (warmupClass == NULL) {
	    goto error;
        }
        runitID = (*env)->GetStaticMethodID(env, warmupClass, 
                        "runit", "(Ljava/lang/String;Ljava/lang/String;)V");
        if (runitID == NULL) {
            goto error;
        }
        jclist = (clist == NULL) ? NULL : 
                                   (*env)->NewStringUTF(env, clist);
        if ((*env)->ExceptionOccurred(env)) {
            goto error;
        }
        jmlist = (mlist == NULL) ? NULL :
                                   (*env)->NewStringUTF(env, mlist);
        if ((*env)->ExceptionOccurred(env)) {
            goto error;
        }
        (*env)->CallStaticVoidMethod(env, warmupClass, runitID, jclist, jmlist);
    }

    stopSystemThreads(state);
    fprintf(stderr, "done!\n");

    state->resetMainID =
	(*env)->GetStaticMethodID(env, cvmClass,
				  "resetMain",
				  "()V");
    if (state->resetMainID == NULL) {
	goto error;
    }

    CVMdestroyParsedSubOptions(serverOpts);
    return 1;

error:
    CVMdestroyParsedSubOptions(serverOpts);
    return 0;
}
