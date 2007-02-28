/*
 *
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

#include <stdio.h>
#include <stdlib.h> /* malloc() */
#include <pthread.h>

struct _THREAD_WAIT_BLOCK {
    long handle;    // an abstract handle
    int waitFor;    // a type of a signal
    int waiting;    // number of waiting threads
    int predicate;  // Boolean variable (0 - wait, 1 - shell go)
    pthread_cond_t cond;    // a pthread conditional variable
    struct _THREAD_WAIT_BLOCK *next; // link to the next block
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static struct _THREAD_WAIT_BLOCK *waiting_list = NULL;

int jumpThreadWaitForSignal(long handle, int waitFor) {
    struct _THREAD_WAIT_BLOCK *block, *p;
    int rez;
    
    pthread_mutex_lock(&lock);
    for (p = waiting_list; p != NULL; p = p->next) {
        if (p->waitFor == waitFor && p->handle == handle) {
            block = p;
            block->waiting++;
            break;
        }
    }
    
    if (block == NULL) {
        block = malloc(sizeof block*);
        if (block == NULL) {
            pthread_mutex_unlock(&lock);
            return -1;
        }
        block->handle = handle;
        block->waitFor = waitFor;
        block->waiting = 1;
        block->predicate = 0;
        if (pthread_cond_init(&block->cond, NULL) < 0) {
            pthread_mutex_unlock(&lock);
            free(block);
            return -1;
        }
    }
    block->next = waiting_list;
    waiting_list = block;
    
    do {
        rez = pthread_cond_wait(&block->cond, &lock);
    } while (rez >= 0 && block->predicate == 0);
    if (rez < 0) {
        fprintf(stderr, "pthread_cond_wait returned %d\n", rez);
    }
    if (block->predicate != 0) {
        int removed = 0;
        
        block->waiting--;
        if (block->waiting == 0) {
            if (waiting_list == block) {
                waiting_list = block->next;
                removed = 1;
            } else {
                for (p = waiting_list; p->next != NULL; p = p->next) {
                    if (p->next == block) {
                        p->next = block->next;
                        removed = 1;
                        break;
                    }
                }
            }
            assert(removed != 0);
            free(block);
        }
    }
    pthread_mutex_unlock(&lock);
    return rez == 0 ? 0 : -1;
}

int jumpThreadSignal(long handle, int waitFor) {
    struct _THREAD_WAIT_BLOCK *block, *p;
    int rez;
    
    pthread_mutex_lock(&lock);
    for (p = waiting_list; p != NULL; p = p->next) {
        if (p->waitFor == waitFor && p->handle == handle) {
            block = p;
            block->waiting++;
            break;
        }
    }
}
