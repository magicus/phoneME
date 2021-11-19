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

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h> /* malloc() */
// #include <sys/select.h> /* select() */
#include <fcntl.h>
#include <pthread.h>
#include <JUMPEvents.h>

#define getOpaque(e_)    (struct _JUMPEventIMPL_tag *)(e_->opaque__)

static int internal_fire_event(struct _JUMPEventIMPL_tag *evt, int failed);
static void internal_remove_listeners(struct _JUMPEventListener_tag *p);
static int internal_select_pipe(int fd, unsigned int serial);

JUMPEvent jumpEventCreate() {
    struct _JUMPEventIMPL_tag *evt;
    JUMPEvent oevt = malloc(sizeof *oevt);
    
    if (oevt == NULL) {
        LOG("No memory");
        return NULL;
    }
    
    evt = getOpaque(oevt);
    evt->magic = JUMP_EVENTS_MAGIC_MD;
    evt->flag = 0;
    pthread_mutex_init(&evt->locked, NULL);
    evt->listeners = NULL;
    
    return oevt;
}

void jumpEventDestroy(JUMPEvent oevt) {
    struct _JUMPEventIMPL_tag *evt = getOpaque(oevt);
    struct _JUMPEventListener_tag *p;

    if (evt->magic != JUMP_EVENTS_MAGIC_MD) {
        LOG("Bad magic");
        return;
    }
    pthread_mutex_lock(&evt->locked);
    if (!evt->flag) {
        internal_fire_event(evt, 1);
        evt->flag = 1;
    }
    p = evt->listeners;
    evt->listeners = NULL;
    internal_remove_listeners(p);
    pthread_mutex_unlock(&evt->locked);
    pthread_mutex_destroy(&evt->locked);
    free(oevt);
}

int jumpEventReset(JUMPEvent oevt) {
    struct _JUMPEventIMPL_tag *evt = getOpaque(oevt);
    struct _JUMPEventListener_tag *p;

    if (evt->magic != JUMP_EVENTS_MAGIC_MD) {
        LOG("Bad magic");
        return -1;
    }
    pthread_mutex_lock(&evt->locked);
    if (!evt->flag) {
        internal_fire_event(evt, 1);
    }
    evt->flag = 0;
    p = evt->listeners;
    evt->listeners = NULL;
    pthread_mutex_unlock(&evt->locked);
    internal_remove_listeners(p);
    return 0;
}

static int internal_select_pipe(int fd, unsigned int serial) {
    fd_set read_fds;
    fd_set write_fds;
    fd_set except_fds;
    int num_fds = 0, num_ready;
    unsigned int buf[2];
    
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);
    FD_SET(fd, &read_fds);
    num_fds = fd + 1;

    do {
        num_ready = select(num_fds, &read_fds, &write_fds, &except_fds, NULL);
        if (num_ready > 0) {
            if (FD_ISSET(fd, &read_fds)) {
                if (read(fd, buf, sizeof buf) != sizeof buf) {
                    LOG("Cannot read from pipe (maybe closed?)");
                    return -1;
                }
                if (buf[0] != serial) {
                    LOG("Bad serial");
                    return -1;
                }
                if (buf[1] != 0) { // failed event
                    LOG("The event didn't happen");
                    return 1;
                } else {
                    return 0;
                }
            }
            // should never happen
            LOG("Bad fd from select()");
            return -1;
        } else {
            LOG("EOF from select()");
            return -1;
        }
    } while (1);

}

int jumpEventWait(JUMPEvent oevt) {
    struct _JUMPEventIMPL_tag *evt = getOpaque(oevt);
    struct _JUMPEventListener_tag *listener;
    static unsigned int serial_gen = 1;
    unsigned int saved_serial;
    int result = -1;

    if (evt->magic != JUMP_EVENTS_MAGIC_MD) {
        LOG("Bad magic");
        return -1;
    }
    pthread_mutex_lock(&evt->locked);
    if (evt->flag) {
        pthread_mutex_unlock(&evt->locked);
        result = 1;
        goto done;
    }
    pthread_mutex_unlock(&evt->locked);
    
    listener = malloc(sizeof *listener);
    if (listener == NULL) {
        LOG("No memory");
        goto done;
    }
    listener->done = 0;
    if (pipe(listener->pipe) < 0) {
        LOG("Cannot create pipe");
        goto done;
    }
    if (fcntl(listener->pipe[1], F_SETFL, O_WRONLY | O_NONBLOCK) < 0) {
        LOG("cannot fcntl() (output)");
        goto done;
    }
    if (fcntl(listener->pipe[0], F_SETFL, O_RDONLY | O_NONBLOCK) < 0) {
        LOG("cannot fcntl() (input)");
        goto done;
    }
    pthread_mutex_lock(&evt->locked);
    if (evt->flag) {
        pthread_mutex_unlock(&evt->locked);
        close(listener->pipe[0]);
        close(listener->pipe[1]);
        free(listener);
        result = 1;
        goto done;
    }
    listener->serial = serial_gen++;
    listener->next = evt->listeners;
    evt->listeners = listener;
    saved_serial = listener->serial;
    pthread_mutex_unlock(&evt->locked);

    if ((result = internal_select_pipe(listener->pipe[0], saved_serial)) < 0) {
        goto done;
    }
    result = 0;
    
done:
    return result;
}

static int internal_fire_event(struct _JUMPEventIMPL_tag *evt, int failed) {
    struct _JUMPEventListener_tag *p;
    int result = 0;
    
/* must be called when the mutex is locked */
    for (p = evt->listeners; p != NULL; p = p->next) {
        unsigned int buf[2];
        
        buf[0] = p->serial;
        buf[1] = failed;
        if (!p->done) {
            if (write(p->pipe[1], &buf, sizeof buf) != sizeof buf) {
                // report error, but do not stop
                result = -1;
                LOG("Cannot write to pipe");
            }
            p->done = 1;
            // close(p->pipe[1]);
        }
    }
    return result;
}

static void internal_remove_listeners(struct _JUMPEventListener_tag *p) {
    
    while (p != NULL) {
        struct _JUMPEventListener_tag *pv = p;
        p = p->next;
        close(pv->pipe[0]);
        close(pv->pipe[1]);
        free(pv);
    }
}

int jumpEventHappens(JUMPEvent oevt) {
    struct _JUMPEventIMPL_tag *evt = getOpaque(oevt);
    struct _JUMPEventListener_tag *p;
    int result = 0;

    if (evt->magic != JUMP_EVENTS_MAGIC_MD) {
        LOG("Bad magic");
        return -1;
    }
    pthread_mutex_lock(&evt->locked);
    if (!evt->flag) {
        if (internal_fire_event(evt, 0) < 0) {
            result = -1;
        }
    }
    // p = evt->listeners;
    // evt->listeners = NULL;
    evt->flag = 1;
    pthread_mutex_unlock(&evt->locked);
    //internal_remove_listeners(p);
    return result;
}

