/*
 *
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

#include <javacall_events.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


#define EVENTS_QID   "/tmp/javacall_events_qid"

struct message {
    long mtype;
    char mtext[JAVACALL_MAX_EVENT_SIZE];
};

static int events_mq = -1;

/**
 * Waits for an incoming event message and copies it to user supplied
 * data buffer.
 *
 * @param timeToWaitInMillisec maximum number of milliseconds to wait.
 *              If this value is 0, the function should poll and return
 *              immediately.
 *              If this value is -1, the function should block forever.
 * @param binaryBuffer user-supplied buffer to copy event to.
 * @param binaryBufferMaxLen maximum buffer size that an event can be
 *              copied to.
 *              If an event exceeds the binaryBufferMaxLen, then the first
 *              binaryBufferMaxLen bytes of the events will be copied
 *              to user-supplied binaryBuffer, and <tt>JAVACALL_OUT_OF_MEMORY</tt>
 *              will be returned.
 * @param outEventLen user-supplied pointer to variable that will hold actual
 *              size of the event received.
 *              Platform is responsible to set this value on success to the
 *              size of the event received, or 0 on failure.
 *              If outEventLen is NULL, the event size is not returned.
 * @return <tt>JAVACALL_OK</tt> if an event successfully received,
 *         <tt>JAVACALL_FAIL</tt> if failed or no messages are available,
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> if an event's size exceeds the
 *         binaryBufferMaxLen.
 */
javacall_result javacall_event_receive(
                            long                    timeToWaitInMillisec,
                            /*OUT*/ unsigned char*  binaryBuffer,
                            int                     binaryBufferMaxLen,
                            /*OUT*/ int*            outEventLen) {
    struct message msg;
    int len;

    if (-1 == events_mq) {
        return JAVACALL_FAIL;
    }

    if (-1 == timeToWaitInMillisec) {
        /* Block until an event arrives or the queue is destroyed. */
        len = msgrcv(events_mq, &msg, JAVACALL_MAX_EVENT_SIZE, 0, 0);
    } else {
        struct timeval tv;
        struct timezone tz;
        long msec_start;

        /* Try to receive an event within the given time-out. */
        gettimeofday(&tv, &tz);
        msec_start = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        do {
            len = msgrcv(events_mq, &msg, JAVACALL_MAX_EVENT_SIZE, 0, IPC_NOWAIT);
            gettimeofday(&tv, &tz);
        } while (-1 == len && tv.tv_sec * 1000 + tv.tv_usec / 1000 - msec_start < timeToWaitInMillisec);
    }

    if (-1 == len) {
        /* Failed to receive an event. */
        if (NULL != outEventLen) {
            *outEventLen = 0;
        }
        return JAVACALL_FAIL;
    }

    if (NULL != outEventLen) {
        *outEventLen = len;
    }

    if (len > binaryBufferMaxLen) {
        /* The incoming event does not fit in the provided buffer. */
        memcpy(binaryBuffer, msg.mtext, binaryBufferMaxLen);
        return JAVACALL_OUT_OF_MEMORY;
    }

    memcpy(binaryBuffer, msg.mtext, len);
    return JAVACALL_OK;
}

/**
 * Copies a user-supplied event message to a queue of messages.
 *
 * @param binaryBuffer a pointer to binary event buffer to send.
 *        The platform should make a private copy of this buffer as
 *        access to it is not allowed after the function call.
 * @param binaryBufferLen size of binary event buffer to send.
 * @return <tt>JAVACALL_OK</tt> if sent successfully,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_event_send(unsigned char* binaryBuffer,
                                    int binaryBufferLen) {
    struct message msg;

    if (binaryBufferLen > JAVACALL_MAX_EVENT_SIZE) {
        return JAVACALL_FAIL;
    }

    if (-1 == events_mq) {
        /* Read the event message queue ID. */
        int fd = open(EVENTS_QID, O_RDONLY);
        if (-1 == fd) {
            return JAVACALL_FAIL;
        }
        read(fd, &events_mq, sizeof(events_mq));
        close(fd);
    }

    msg.mtype = 1;
    memcpy(msg.mtext, binaryBuffer, binaryBufferLen);
    if (-1 == msgsnd(events_mq, &msg, binaryBufferLen, 0)) {
        return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}


/**
 * The function is called during Java VM startup, allowing the
 * platform to perform specific initializations. It is called in the same
 * process as javacall_event_receive() and javacall_events_finalize().
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_events_init(void) {
    int fd;
    struct msqid_ds buf;

    /* Check if the message queue already exists. */
    fd = open(EVENTS_QID, O_RDONLY, 0);
    if (-1 != fd) {
        if (sizeof(events_mq) == read(fd, &events_mq, sizeof(events_mq))) {
            if (0 == msgctl(events_mq, IPC_STAT, &buf)) {
                close(fd);
                return JAVACALL_OK;
            }
        }
        close(fd);
    }

    /* Create message queue for transmitting events between processes. */
    events_mq = msgget(IPC_PRIVATE, S_IREAD | S_IWRITE | IPC_CREAT | IPC_EXCL);
    if (-1 == events_mq) {
        return JAVACALL_FAIL;
    }

    /* Store the message queue ID. */
    fd = open(EVENTS_QID, O_WRONLY | O_CREAT, 0666);
    if (-1 == fd) {
        return JAVACALL_FAIL;
    }
    write(fd, &events_mq, sizeof(events_mq));
    close(fd);

    return JAVACALL_OK;
}

/**
 * The function is called during Java VM shutdown, allowing the platform to
 * perform specific events-related shutdown operations. It is called in the same
 * process as javacall_events_init() and javacall_event_receive().
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_events_finalize(void) {
    if (-1 == msgctl(events_mq, IPC_RMID, 0)) {
        return JAVACALL_FAIL;
    }
    unlink(EVENTS_QID);

    return JAVACALL_OK;
}
