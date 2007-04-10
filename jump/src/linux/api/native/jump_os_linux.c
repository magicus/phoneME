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
#include "porting/JUMPMessageQueue.h"
#include "porting/JUMPProcess.h"
#include "porting/JUMPThread.h"
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "porting/JUMPClib.h"

#undef MESSAGE_QUEUE_DEBUG

/*
 * This is how message queues are named, based on pid, and a name
 */
#define JUMP_MQ_PATH_PATTERN "/tmp/jump-mq-%d-%s"

/*
 * A cache that keeps track of message queue keys 
 * for a given <pid,type> pair.
 */
struct _key_entry {
    struct _key_entry* next;
    pid_t pid;
    JUMPPlatformCString type;
    key_t key;
    char* fileName;
};

static struct _key_entry * head = NULL;

static void
cacheKey(pid_t pid, JUMPPlatformCString type, key_t key,
	 char* fileName)
{
    struct _key_entry* entry = calloc(1, sizeof(struct _key_entry));
    entry->next = head;
    entry->pid = pid;
    entry->type = type;
    entry->key = key;
    entry->fileName = fileName;
    head = entry;
}

/*
 * Lookup a key in the cache
 */
static key_t
getKey(pid_t pid, JUMPPlatformCString type)
{
    key_t key = (key_t)-1;
    struct _key_entry* entry = head;
    while (entry != NULL) {
	if ((entry->pid == pid) && !strcmp(entry->type, type)) {
	    /* match */
	    key = entry->key;
	    break;
	}
	entry = entry->next;
    }
    return key;
}

/*
 * Compute and cache key for <pid,type>
 * doCreate determines whether the message queue is being created
 * or just opened.
 */
static key_t
computeKey(pid_t pid, JUMPPlatformCString type, int doCreate)
{
    char* name;
    int fd;
    key_t qkey;
    char* typeInFilename;
    char* t;
    
    if ((qkey = getKey(pid, type)) != (key_t)-1) {
	return qkey;
    }
    
    name = (char*)calloc(80, sizeof(char));
    if (name == NULL) {
	return (key_t)-1;
    }
    
    typeInFilename = strdup(type);
    t = typeInFilename;
    
    while ((t = strchr(t, '/')) != NULL) {
	*t = '=';
    }
    
    snprintf(name, 80, JUMP_MQ_PATH_PATTERN, pid, typeInFilename);
    free(typeInFilename);
    
    if (doCreate) {
	/* Create a file for ftok to use */
	fd = open(name, O_CREAT|O_RDWR, 0666);
	if (fd == -1) {
	    perror("open mq file");
	    return (key_t)-1;
	}
	close(fd);
    }
    
    qkey = ftok(name, 'Q');
    cacheKey(pid, type, qkey, name);
#ifdef MESSAGE_QUEUE_DEBUG
    printf("[name=%s pid=%d type=%s key=0x%x]\n",
	   name, pid, type, qkey);
#endif
    
    return qkey;
}

/*
 * Get message queue identifier for <pid,type>
 */
static int
getMQId(pid_t pid, JUMPPlatformCString type, int doCreate)
{
    key_t qkey;
    int mqhandle;
    int mqflags;
    const char* msggetError;
    
    qkey = computeKey(pid, type, 1);
    if (qkey == (key_t)-1) {
	return -1;
    }
    
    if (doCreate) {
	mqflags = IPC_CREAT|0600;
	msggetError = "mq create";
    } else {
	mqflags = 0;
	msggetError = "mq open";
    }
    
    mqhandle = msgget(qkey, mqflags);
    if (mqhandle == -1) {
	perror(msggetError);
#ifdef MESSAGE_QUEUE_DEBUG
	printf("(pid=%d type=%s doCreate=%d qkey=0x%x)\n", 
	       pid, type, doCreate, qkey);
#endif
	
	return 1;
#ifdef MESSAGE_QUEUE_DEBUG
    } else {
	printf("getMqID success: (pid=%d type=%s doCreate=%d)\n", 
	       pid, type, doCreate);
#endif
    }
    
    
    return mqhandle;
}

/*
 * The message queue porting layer
 */
void 
jumpMessageQueueCreate(JUMPPlatformCString type,
		       JUMPMessageQueueStatusCode* code) 
{
    int mqhandle;
    
    // Create one for this process
    mqhandle = getMQId(getpid(), type, 1);
    if (mqhandle == -1) {
	*code = JUMP_MQ_FAILURE;
    } else {
	*code = JUMP_MQ_SUCCESS;
    }
}

int
jumpMessageQueueDestroy(JUMPPlatformCString type) 
{
    int msgqid = getMQId(getpid(), type, 0);
    if (msgctl(msgqid, IPC_RMID, NULL) == -1) {
	perror("mq remove");
	return 1;
    }
    return 0;
}

JUMPMessageQueueHandle 
jumpMessageQueueOpen(int processId, JUMPPlatformCString type) 
{
    int mqid;
    
    mqid = getMQId(processId, type, 0);
    
    if (mqid == -1) {
	return NULL;
    } else {
	return (JUMPMessageQueueHandle)mqid;
    }
}

void 
jumpMessageQueueClose(JUMPMessageQueueHandle handle) 
{
    /* Do nothing -- no notion of close in linux, only destroy */
}

#define offsetOf(T, f) ((int) &((T*)0)->f)

int 
jumpMessageQueueDataOffset() 
{
    return offsetOf(struct msgbuf, mtext);
}

int 
jumpMessageQueueSend(JUMPMessageQueueHandle handle,
		     char *buffer,
		     int messageDataSize)
{
    int mqid = (int)handle;
    struct msgbuf* message = (struct msgbuf *)buffer;
    int mlen = messageDataSize;
    int status;

    /* must be a positive number. For this implementation, just use 1 */
    message->mtype = 1; 
    status = msgsnd(mqid, message, mlen, IPC_NOWAIT);
    if (status == -1) {
	perror("msgsnd");
	return 1;
    } else {
	return 0;
    }
}

int
jumpMessageQueueWaitForMessage(JUMPPlatformCString type, int32 timeout)
{
    int msgqid = getMQId(getpid(), type, 0);
    int status;

    if (msgqid == -1) {
	return 1;
    }
    
    /*
     * Block waiting for a message of type 1
     * If signals interrupt, re-start. For example, SIGCHLD.
     * FIXME: We'll have to special case for SIGALRM here, and see if the
     * alarm rings for us.
     */
    do {
	status = msgrcv(msgqid, NULL, 0, 1, 0);
    } while ((status == -1) && (errno == EINTR));
    
    /* Ignore E2BIG -- that's expected as soon as message becomes available */
    if ((status == -1) && (errno != E2BIG)) {
	perror("msg check");
	return 2;
    } else {
	return 0;
    }
}

int 
jumpMessageQueueReceive(JUMPPlatformCString type,
			char *buffer, int bufferLength)
{
    int msgqid = getMQId(getpid(), type, 0);
    int status;

    /* Get a message.
     * IPC_NOWAIT is set, because we only get a message if one is already
     * found available via a call to jumpMessageQueueWaitForMessage().
     * MSG_NOERROR is NOT set, because if the buffer is not large
     * enough, we want this call to fail w/ E2BIG
     *
     * FIXME!!! There must be a way to communicate to the caller why
     * the failure is. If it's E2BIG, the caller would call with a bigger
     * buffer. Otherwise, it's I/O error of some sort.
     */
    status = msgrcv(msgqid, buffer, bufferLength, 1, IPC_NOWAIT);
    if (status == -1) {
	perror("msgsnd");
	return 1;
    } else {
	return 0;
    }
}

/*
 * Destroy all message queues created by this process
 */
void
jumpMessageQueueInterfaceDestroy()
{
    int mypid = getpid();
    struct _key_entry* entry = head;
    while (entry != NULL) {
	if (entry->pid == mypid) {
	    int msgqid = getMQId(mypid, entry->type, 0);
#ifdef MESSAGE_QUEUE_DEBUG
	    printf("deleting message queue [filename=%s id=%d]\n",
		   entry->fileName, msgqid);
#endif
	    if (msgctl(msgqid, IPC_RMID, NULL) == -1) {
		perror("mq remove");
	    }
	    if (remove(entry->fileName) == -1) {
		perror("mq file delete");
	    }
	}
	entry = entry->next;
    }
}

int
jumpThreadGetId()
{
    return (int)pthread_self();
}
