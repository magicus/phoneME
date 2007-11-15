/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#define JUMP_MQ_THREADSAFE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>
#include <dirent.h>
/* NOTE: even if JUMP_MQ_THREADSAFE is not defined, we need pthread.h
   for pthread_self(). */
#include <pthread.h>

#include "porting/JUMPMessageQueue.h"
#include "porting/JUMPProcess.h"
#include "porting/JUMPThread.h"

#ifdef JUMP_MQ_THREADSAFE
#define mutex_init(mutex, attr)	pthread_mutex_init(mutex, attr)
#define mutex_destroy(mutex)	pthread_mutex_destroy(mutex)
#define mutex_lock(mutex)	pthread_mutex_lock(mutex)
#define mutex_unlock(mutex)	pthread_mutex_unlock(mutex)
#else
#define mutex_init(mutex, attr)
#define mutex_destroy(mutex)
#define mutex_lock(mutex)
#define mutex_unlock(mutex)
#endif

/*
 * This implementation uses named pipes (FIFOs) because they do the
 * job and they always exist in the kernel.  Posix message queues and
 * unix domain or IP datagram sockets may have simpler
 * implementations, but they may not be configured into the kernel.
 * SystemV message queues aren't suitable because they can't be timed
 * out easily.
 */

/*
 * Each queue created (for read) or opened (for write) is represented
 * by a struct jump_message_queue.
 *
 * For read queues, a messageType is mapped to a jump_message_queue by
 * keeping a copy of the messageType in the jump_message_queue,
 * keeping the jump_message_queues in a doubly-linked list, and
 * searching the list.  The list is self-organizing: get_message_queue
 * moves each jump_message_queue it returns to the front of the list.
 * The prev/next pointers are built in to struct jump_message_queue to
 * make things simple.  FIXME: A hash table would be better, but it's
 * not clear whether hsearch_r is available and I don't want to write
 * one.
 *
 * For write queues, only the fd field is used, but using
 * jump_message_queue for both read and write queues allows the
 * create/destroy code to be reused easily.  NOTE: there's not much
 * reuse, so maybe a write queue should just be an fd and the
 * open/close code should be refactored.
 */

struct jump_message_queue {
#ifdef JUMP_MQ_THREADSAFE
    /* Accesses to prev, next, and useCount must be serialized with
       queue_list_mutex. */
#endif
    struct jump_message_queue *prev;
    struct jump_message_queue *next;
    /* useCount is incremented for each call to jumpMessageQueueCreate
       and decremented for each call to jumpMessageQueueDestroy.  It
       is also incremented while waiting for or reading from the
       queue.  The jump_message_queue will not actually be destroyed
       until useCount falls to zero, at which point we know it is
       not in use by any code. */
    int useCount;

    char *messageType;
    char *name;			/* Name of FIFO to unlink on destroy,
				   or NULL if no unlink is necessary. */

#ifdef JUMP_MQ_THREADSAFE
    /* Reads are serialized on read_mutex so jumpMessageQueueReceive
       can read a message atomically.  Reading a message requires
       reading the length then the message, and may also require
       iterating to handle partial reads.  read_mutex also serializes
       access to error. */
    pthread_mutex_t read_mutex;
#endif

    int fd;			/* FIFO file descriptor. */

    /* If we ever get an error reading, we won't be able to recover,
       since we can't know where the message boundaries are.  Set
       error to fail all future reads without trying them. */
    int error;
};

/* The doubly-linked jump_message_queue list.  queue_list is a dummy
   element which is always the head and tail of the list; this ensures
   the list is never empty so list managment is trivial. */
static struct jump_message_queue queue_list = {
    &queue_list, &queue_list,
};

#ifdef JUMP_MQ_THREADSAFE
/* All accesses to queue_list are serialized by queue_list_mutex. */
static pthread_mutex_t queue_list_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * This is how message FIFOs are named, based on a pid and a messageType.
 */
#define JUMP_MQ_PATH_DIR	"/tmp"
#define JUMP_MQ_PATH_PREFIX	"jump-mq-%d-"
#define JUMP_MQ_PATH_PATTERN	JUMP_MQ_PATH_DIR "/" JUMP_MQ_PATH_PREFIX "%s"

/* Puts the jump_message_queue at the head of the list. */
static void
put_message_queue (struct jump_message_queue *p)
{
    p->next = queue_list.next;
    p->next->prev = p;
    p->prev = &queue_list;
    p->prev->next = p;
}

/* Removes the jump_message_queue from the list. */
static void
remove_message_queue(const struct jump_message_queue *p)
{
    p->next->prev = p->prev;
    p->prev->next = p->next;
}

/* Returns the existing jump_message_queue for messageType, or NULL if
   there is none. */
static struct jump_message_queue *
get_message_queue(const JUMPPlatformCString messageType)
{
    struct jump_message_queue *p;
    for (p = queue_list.next; p != &queue_list; p = p->next) {
	if (strcmp(p->messageType, messageType) == 0) {

	    /* Move to the front of the list for next time, if it's
	       not there already.  This may not be worthwhile. */

	    if (p->prev != &queue_list) {
		remove_message_queue(p);
		put_message_queue(p);
	    }

	    return p;
	}
    }
    return NULL;
}

/* Escapes occurences of '/' in name so we can use it as a filename.
   On success returns the clean name (which must be freed).  On
   failure (out of memory) returns NULL. */
static char *
clean_name(const char *name)
{
    char *clean_name;
    const char *src;
    char *dst;

    /* Escape "/" to "%@" and "%" to "%%". */

    /* Easy case if there is nothing to escape. */

    if (strpbrk(name, "/%") == NULL) {
	return strdup(name);
    }

    /* Allocate the worst case amount of space we'll need. */

    clean_name = malloc(strlen(name) * 2 + 1);
    if (clean_name == NULL) {
	return NULL;
    }

    /* Copy and escape characters. */

    dst = clean_name;
    for (src = name; *src; src++) {
	switch (*src) {
	  case '/':
	    *dst++ = '%';
	    *dst++ = '@';
	    break;
	  case '%':
	    *dst++ = '%';
	    *dst++ = '%';
	    break;
	  default:
	    *dst++ = *src;
	    break;
	}
    }
    *dst = 0;

    return clean_name;
}

/* Constructs a queue (FIFO) name from the pid and messageType.
   On success returns the name (which must be freed) and sets
   code to JUMP_MQ_SUCCESS.  On failure returns NULL and sets
   code to one of JUMP_MQ_OUT_OF_MEMORY or JUMP_MQ_FAILURE. */
static char *
make_queue_name(pid_t pid, const JUMPPlatformCString messageType,
		JUMPMessageQueueStatusCode* code)
{
    char *messageTypeClean = NULL;
    size_t len;
    char *name = NULL;
    size_t written;

    /* Get a clean version of the messageType that we can use in
       a filename. */

    messageTypeClean = clean_name(messageType);
    if (messageTypeClean == NULL) {
	*code = JUMP_MQ_OUT_OF_MEMORY;
	goto error;
    }

    /* Allocate (more than enough) space to hold the text, pid, and
       messageType. */

    len = sizeof(JUMP_MQ_PATH_PATTERN) + 10 + strlen(messageTypeClean);

    name = malloc(len);
    if (name == NULL) {
	*code = JUMP_MQ_OUT_OF_MEMORY;
	goto error;
    }

    written = snprintf(name, len, JUMP_MQ_PATH_PATTERN, pid, messageTypeClean);
    if (written == -1 || written >= len) {
	*code = JUMP_MQ_FAILURE;
	goto error;
    }

    free(messageTypeClean);
    return name;

  error:
    free(messageTypeClean);
    free(name);
    return NULL;
}

/* Creates/opens a jump_message_queue for reading or writing.  On
   success returns the queue and sets code to JUMP_MQ_SUCCESS.  On
   failure returns NULL and sets code to one of JUMP_MQ_OUT_OF_MEMORY,
   JUMP_MQ_TARGET_NONEXISTENT, or JUMP_MQ_FAILURE on failure. */
static struct jump_message_queue *
message_queue_create(pid_t processId,
		     JUMPPlatformCString messageType,
		     JUMPMessageQueueStatusCode* code,
		     int forRead)
{
    struct jump_message_queue *jmq;
    char *name = NULL;

    jmq = malloc(sizeof(*jmq));
    if (jmq == NULL) {
	*code = JUMP_MQ_OUT_OF_MEMORY;
	goto fail;
    }

    jmq->messageType = NULL;
    jmq->name = NULL;
    jmq->fd = -1;

    if (forRead) {
	jmq->messageType = strdup(messageType);
	if (jmq->messageType == NULL) {
	    *code = JUMP_MQ_OUT_OF_MEMORY;
	    goto fail;
	}
    }

    name = make_queue_name(processId, messageType, code);
    if (name == NULL) {
	goto fail;
    }

    /* The FIFO node is created only if we're opening for read.  If it
       was created when opening for write, then the reader might not
       be able to unlink it on destroy if it's running with a
       different uid. */

    if (forRead) {
	int status;

	/* Make sure the FIFO exists. */

	status = mknod(name, S_IFIFO | 0666, 0);
	if (status == -1) {
	    if (errno != EEXIST) {
		*code = JUMP_MQ_FAILURE;
		goto fail;
	    }
	}

	/* Save the name to indicate the node needs to be unlinked
	   when jmq is freed. */

	jmq->name = name;

	/* Make sure the FIFO is read/write for everybody (see open,
	   below).  We can't do this with umask()+mknod() since that's
	   not thread-safe. */

	status = chmod(name, 0666);
	if (status == -1) {
	    *code = JUMP_MQ_FAILURE;
	    goto fail;
	}
    }

    /* Open the FIFO for read/write.  If we're reading from the FIFO,
       then having it open for write ensures there are no problems
       with EOF since the number of writers never falls to zero.  If
       we're writing to this FIFO, then having it open for read
       ensures we never get a SIGPIPE since there is always a reader.
       O_NONBLOCK puts the fd in non-blocking mode which we need.  It
       doesn't affect the open since we're opening for read/write
       which won't block anyway. */

    jmq->fd = open(name, O_RDWR | O_NONBLOCK);
    if (jmq->fd == -1) {
	if (!forRead && errno == ENOENT) {
	    *code = JUMP_MQ_TARGET_NONEXISTENT;
	}
	else {
	    *code = JUMP_MQ_FAILURE;
	}
	goto fail;
    }

    /* Set the close-on-exec flag.  There's no reason to keep the fd
       open if we ever exec. */

    {
	int fd_flags;
	int status;

	fd_flags = fcntl(jmq->fd, F_GETFD);
	if (fd_flags == -1) {
	    *code = JUMP_MQ_FAILURE;
	    goto fail;
	}

	fd_flags |= FD_CLOEXEC;
	status = fcntl(jmq->fd, F_SETFD, fd_flags);
	if (status == -1) {
	    *code = JUMP_MQ_FAILURE;
	    goto fail;
	}
    }

    /* Success. */

    if (!forRead) {
	free(name);
    }

    mutex_init(&jmq->read_mutex, NULL);

    jmq->error = 0;

    *code = JUMP_MQ_SUCCESS;
    return jmq;

  fail:
    if (jmq != NULL) {
	if (jmq->fd != -1) {
	    close(jmq->fd);
	}
	if (jmq->name != NULL) {
	    unlink(jmq->name);
	}
	free(jmq->messageType);
	free(jmq);
    }
    free(name);
    return NULL;
}

/* Destroys/closes a message queue and cleans up.  On success returns
   0.  On failure returns -1, and all cleanup has been attempted. */
static int
message_queue_destroy(struct jump_message_queue *jmq)
{
    int ret = 0;

    if (close(jmq->fd) == -1) {
	/* Close failed.  We free everything anyway since there's not
	   much we can so about failure, and the jump_message_queue
	   shouldn't be used after this anyway. */
	ret = -1;
    }

    if (jmq->name != NULL) {
	/* Unlink the FIFO.  It's ok if this fails.  This is just for
	   neatness in the filesystem. */

	unlink(jmq->name);
	free(jmq->name);
    }

    /* Free the jump_message_queue. */

    mutex_destroy(&jmq->read_mutex);

    free(jmq->messageType);
    free(jmq);

    return ret;
}

/* Decrements the queue's useCount, destroying it when the useCount
   falls to zero.  Must be called with queue_list_mutex locked. */
static int
decrement_usecount_maybe_free(struct jump_message_queue *jmq)
{
    jmq->useCount--;

    /* If somebody still needs the queue, then don't destroy it. */

    if (jmq->useCount > 0) {
	return 0;
    }

    remove_message_queue(jmq);

    return message_queue_destroy(jmq);
}

/* Returns the existing jump_message_queue for messageType, or NULL if
   there is none.  Locks queue_list_mutex and increments the queue's
   useCount. */
static struct jump_message_queue *
lock_and_acquire_message_queue (JUMPPlatformCString messageType)
{
    struct jump_message_queue *jmq;

    mutex_lock(&queue_list_mutex);

    jmq = get_message_queue(messageType);
    if (jmq != NULL) {
	/* Increment useCount so jmq won't be freed while we're using it. */
	jmq->useCount++;
    }

    mutex_unlock(&queue_list_mutex);

    return jmq;
}

/* Locks queue_list_mutex and decrements the queue's useCount,
   destroying it when the useCount falls to zero. */
static void
lock_and_release_message_queue (struct jump_message_queue *jmq)
{
    mutex_lock(&queue_list_mutex);
    decrement_usecount_maybe_free(jmq);
    mutex_unlock(&queue_list_mutex);
}

/*
 * The message queue porting layer
 */
void 
jumpMessageQueueCreate(JUMPPlatformCString messageType,
		       JUMPMessageQueueStatusCode* code) 
{
    struct jump_message_queue *jmq;

    mutex_lock(&queue_list_mutex);

    jmq = get_message_queue(messageType);

    if (jmq != NULL) {
	jmq->useCount++;
	*code = JUMP_MQ_SUCCESS;
    }
    else {
	jmq = message_queue_create(jumpProcessGetId(), messageType, code, 1);
	if (jmq != NULL) {
	    jmq->useCount = 1;
	    put_message_queue(jmq);
	}
    }

    mutex_unlock(&queue_list_mutex);
}

int
jumpMessageQueueDestroy(JUMPPlatformCString messageType) 
{
    struct jump_message_queue *jmq;
    int ret;

    mutex_lock(&queue_list_mutex);

    jmq = get_message_queue(messageType);
    if (jmq == NULL) {
	ret = -1;
    }
    else {
	ret = decrement_usecount_maybe_free(jmq);
    }

    mutex_unlock(&queue_list_mutex);
    return ret;
}


JUMPMessageQueueHandle 
jumpMessageQueueOpen(int processId, JUMPPlatformCString type,
		     JUMPMessageQueueStatusCode* code)

{
    return message_queue_create(processId, type, code, 0);
}

void 
jumpMessageQueueClose(JUMPMessageQueueHandle handle) 
{
    struct jump_message_queue *jmq = (struct jump_message_queue *) handle;
    message_queue_destroy(jmq);
}

int 
jumpMessageQueueDataOffset(void) 
{
    return 0;
}

int 
jumpMessageQueueSend(JUMPMessageQueueHandle handle,
		     char *buffer,
		     int messageDataSize,
		     JUMPMessageQueueStatusCode* code)
{
    struct jump_message_queue *jmq = (struct jump_message_queue *) handle;
    struct iovec iovec[2];

    if (messageDataSize < 0) {
	*code = JUMP_MQ_BAD_MESSAGE_SIZE;
	return -1;
    }

    /* Fail if the message is too large to be written atomically,
       i.e., without being interleaved with writes from other
       processes. */
    if (messageDataSize > JUMP_MESSAGE_QUEUE_MAX_MESSAGE_SIZE) {
	*code = JUMP_MQ_BAD_MESSAGE_SIZE;
	return -1;
    }

    /* Prepare to write the length followed by the message. */

    iovec[0].iov_base = &messageDataSize;
    iovec[0].iov_len = sizeof(messageDataSize);

    iovec[1].iov_base = buffer;
    iovec[1].iov_len = messageDataSize;

    /* This write is non-blocking.  If it would block, we return
       JUMP_MQ_WOULD_BLOCK.  It's atomic, so there are no issues with
       partial writes. */

    while (1) {
	ssize_t status = writev(jmq->fd, iovec, 2);
	if (status != -1) {
	    /* All data written successfully. */
	    *code = JUMP_MQ_SUCCESS;
	    return 0;
	}
	if (errno == EINTR) {
	    /* Interrupted before data was written, retry. */
	    continue;
	}
	/* write failed or would block. */
	if (errno == EAGAIN) {
	    *code = JUMP_MQ_WOULD_BLOCK;
	}
	else {
	    *code = JUMP_MQ_FAILURE;
	}
	return -1;
    }
}

int
jumpMessageQueueWaitForMessage(JUMPPlatformCString type, int32 timeout_millis,
			       JUMPMessageQueueStatusCode* code)
{
    struct timeval deadline;
    struct jump_message_queue *jmq;
    int ret;

    /* If we're using a timeout, calculate deadline = absolute timeout time. */

    if (timeout_millis != 0) {
	int timeout_usec = (timeout_millis % 1000) * 1000;
	int timeout_sec  = timeout_millis / 1000;

	if (gettimeofday(&deadline, NULL) == -1) {
	    *code = JUMP_MQ_FAILURE;
	    return -1;
	}

	deadline.tv_usec += timeout_usec;
	if (deadline.tv_usec >= 1000000) {
	    deadline.tv_usec -= 1000000;
	    deadline.tv_sec++;
	}
	deadline.tv_sec += timeout_sec;
    }

    jmq = lock_and_acquire_message_queue(type);
    if (jmq == NULL) {
	*code = JUMP_MQ_NO_SUCH_QUEUE;
	return -1;
    }

    /* NOTE: for JUMP_MQ_THREADSAFE we may read a stale false value
       for jmq->error since we're not locking jmq->read_mutex, but
       that won't hurt anything since we're not actually reading. */
    if (jmq->error) {
	*code = JUMP_MQ_FAILURE;
	goto fail;
    }

    /* Wait for the fd to become readable, or for timeout. */

    while (1) {
	fd_set readfds;
	struct timeval timeout;
	int status;

	FD_ZERO(&readfds);
	FD_SET(jmq->fd, &readfds);

	if (timeout_millis != 0) {
	    /* Calculate the timeout time = deadline - current time. */

	    if (gettimeofday(&timeout, NULL) == -1) {
		*code = JUMP_MQ_FAILURE;
		goto fail;
	    }

	    timeout.tv_usec = deadline.tv_usec - timeout.tv_usec;
	    if (timeout.tv_usec < 0) {
		timeout.tv_usec += 1000000;
		timeout.tv_sec++;
	    }
	    timeout.tv_sec = deadline.tv_sec - timeout.tv_sec;
	    if (timeout.tv_sec < 0) {
		/* Timed out. */
		*code = JUMP_MQ_TIMEOUT;
		goto fail;
	    }
	}

	status = select(jmq->fd + 1, &readfds, NULL, NULL,
			(timeout_millis != 0) ? &timeout : NULL);
	switch (status) {
	  case 1:
	    /* Ready to read. */
	    *code = JUMP_MQ_SUCCESS;
	    goto succeed;
	  case 0:
	    /* Timed out. */
	    *code = JUMP_MQ_TIMEOUT;
	    goto fail;
	  case -1:
	    if (errno == EINTR) {
		/* Try again. */
		continue;
	    }
	    break;
	  default:
	    break;
	}

	/* Some error. */

	*code = JUMP_MQ_FAILURE;
	goto fail;
    }

  fail:
    ret = -1;
    goto out;
  succeed:
    ret = 0;
  out:
    lock_and_release_message_queue(jmq);
    return ret;
}

/* Returns 1 for success, 0 for would block, -1 for error. */
static int
read_fully(int fd, void *buf, size_t count)
{
    char *cbuf = buf;
    int first = 1;

    while (count != 0) {
	ssize_t status = read(fd, cbuf, count);

	if (status > 0) {
	    cbuf += status;
	    count -= status;
	    first = 0;
	    continue;
	}

	if (status == -1) {
	    if (errno == EINTR) {
		/* Interrupted, try again. */
		continue;
	    }

	    if (errno == EAGAIN) {
		/* Read would block. */
		if (first) {
		    return 0;
		}
	    }
	}

	/* read failed, or would block after a partial read. */

	return -1;
    }

    return 1;
}

int 
jumpMessageQueueReceive(JUMPPlatformCString type,
			char *buffer, int bufferLength,
			JUMPMessageQueueStatusCode* code)
{
    struct jump_message_queue *jmq;
    int messageDataSize;
    ssize_t status;
    int ret;

    jmq = lock_and_acquire_message_queue(type);
    if (jmq == NULL) {
	*code = JUMP_MQ_NO_SUCH_QUEUE;
	return -1;
    }

    /* Use the read_mutex to ensure only one thread is reading, so we
       can do separate reads for the length and the message, and
       partial reads if necessary. */

    mutex_lock(&jmq->read_mutex);

    /* If this jump_message_queue failed earlier, just fail again. */

    if (jmq->error) {
	goto unrecoverable_error;
    }

    /* Read the message size, and fail on error or if the read would block. */

    status = read_fully(jmq->fd,
			&messageDataSize, sizeof(messageDataSize));
    if (status == -1) {
	goto unrecoverable_error;
    }
    else if (status == 0) {
	/* read_fully would block, so there is no message after all.
	   The jump_message_queue is ok, just fail. */
	*code = JUMP_MQ_WOULD_BLOCK;
	goto recoverable_error;
    }

    /* Check for "unblock" message. */

    if (messageDataSize == -1) {
	*code = JUMP_MQ_UNBLOCKED;
	goto recoverable_error;
    }

    /* Sanity check. */

    if (messageDataSize < 0 ||
	messageDataSize > PIPE_BUF - sizeof(messageDataSize))
    {
	goto unrecoverable_error;
    }

    if (messageDataSize > bufferLength) {
	/* The buffer is not big enough.  About all we can do is
	   attempt to recover by reading the message in small pieces
	   and discarding it. */
	char buf[128];
	while (messageDataSize > 0) {
	    int size = messageDataSize;
	    if (size > sizeof(buf)) {
		size = sizeof(buf);
	    }
	    status = read_fully(jmq->fd, buf, size);
	    if (status != 1) {
		goto unrecoverable_error;
	    }
	    messageDataSize -= size;
	}
	*code = JUMP_MQ_BUFFER_SMALL;
	goto recoverable_error;
    }

    /* Read the message. */

    status = read_fully(jmq->fd, buffer, messageDataSize);
    if (status != 1) {
	goto unrecoverable_error;
    }

    ret = messageDataSize;
    *code = JUMP_MQ_SUCCESS;
    goto out;

  unrecoverable_error:
    jmq->error = 1;
    *code = JUMP_MQ_FAILURE;
  recoverable_error:
    ret = -1;

  out:
    mutex_unlock(&jmq->read_mutex);
    lock_and_release_message_queue(jmq);
    return ret;
}

void
jumpMessageQueueUnblock(JUMPPlatformCString messageType,
			JUMPMessageQueueStatusCode* code)
{
    /* The reader is unblocked by sending it a message of length -1,
       which it checks for. */

    struct jump_message_queue *jmq;

    /* Use the read queue's fd.  This avoids out of memory problems
       which could happen if we tried to create a new write queue and
       use its fd. */

    jmq = lock_and_acquire_message_queue(messageType);
    if (jmq == NULL) {
	*code = JUMP_MQ_NO_SUCH_QUEUE;
	return;
    }

    while (1) {
	int length = -1;

	/* This is a non-blocking atomic write. */
	ssize_t status = write(jmq->fd, &length, sizeof(length));
	if (status != -1) {
	    *code = JUMP_MQ_SUCCESS;
	    break;
	}
	if (errno == EINTR) {
	    /* Interrupted before data was written, retry. */
	    continue;
	}
	/* If write would block consider it a success, since it means
	   the reader has something to read and will unblock. */
	if (errno == EAGAIN) {
	    *code = JUMP_MQ_SUCCESS;
	    break;
	}
	/* write failed. */
	*code = JUMP_MQ_FAILURE;
	break;
    }

    lock_and_release_message_queue(jmq);
}

int
jumpMessageQueueGetFd(JUMPPlatformCString messageType)
{
    struct jump_message_queue *jmq;
    int fd;

    jmq = lock_and_acquire_message_queue(messageType);

    if (jmq == NULL) {
	fd = -1;
    }
    else {
	fd = jmq->fd;
    }

    lock_and_release_message_queue(jmq);

    return fd;
}

/*
 * Destroy all message queues created by this process, regardless of
 * useCount.
 */
void
jumpMessageQueueInterfaceDestroy(void)
{
    struct jump_message_queue *p;

    mutex_lock(&queue_list_mutex);

    p = queue_list.next;
    while (p != &queue_list) {
	struct jump_message_queue *pnext = p->next;
	remove_message_queue(p);
	message_queue_destroy(p);
	p = pnext;
    }

    mutex_unlock(&queue_list_mutex);
}

void
jumpMessageQueueCleanQueuesOf(int cpid)
{
    char prefix[sizeof(JUMP_MQ_PATH_PREFIX) + 20];
    int prefixLen;
    DIR* dir;
    struct dirent* ptr;

    if (jumpProcessIsAlive(cpid) == 0) {
	return;
    }

    prefixLen = snprintf(prefix, sizeof(prefix), JUMP_MQ_PATH_PREFIX, cpid);

    dir = opendir(JUMP_MQ_PATH_DIR);
    if (dir == NULL) {
	perror("opendir");
	return;
    }

    while ((ptr = readdir(dir)) != NULL) {
	if (strncmp(ptr->d_name, prefix, prefixLen) == 0) {
	    char filename[sizeof(JUMP_MQ_PATH_DIR) + 1 + NAME_MAX + 1];
	    int len;

	    len = snprintf(filename, sizeof(filename),
			   JUMP_MQ_PATH_DIR "/%s", ptr->d_name);
	    if (len < 0 || len >= sizeof(filename)) {
		continue;
	    }

	    printf("Exiting process %d has message queue %s\n",
		   cpid, filename);
	    if (unlink(filename) == -1) {
		perror("mq file delete");
	    } else {
		printf("  removed file %s\n", filename);
	    }
	}
    }

    closedir(dir);
}

/*
 * The thread porting layer
 */
int
jumpThreadGetId(void)
{
    return (int)pthread_self();
}
