/*
 * @(#)apdu_asynchro.c	1.3 06/04/26 @(#)
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

#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include "string.h" /* memcpy() */
#include "stdio.h" /* printf() */

#include <kni.h>
#include <sni.h>
#include <commonKNIMacros.h>
#include <kni_globals.h>
#include <midpServices.h>
#include <midp_thread.h>
#include <midpError.h>

#include <midpMalloc.h>
#include <carddevice.h>
#include "apdu_asynchro.h"

#ifdef APDU_ASYNCHRO

struct RESET_PARAMS {
    jbyte *atr;
    jsize atr_size;
    JSR177_STATUSCODE status;
};

struct XFER_PARAMS {
    jbyte *tx_buffer;
    jsize tx_length;
    jbyte *rx_buffer;
    jsize rx_length;
    JSR177_STATUSCODE status;
};

static void *reset_routine(void *p) {
    
    struct RESET_PARAMS *par = (struct RESET_PARAMS*) p;
    
    par->status = jsr177_power_up(par->atr, &par->atr_size);
    midp_thread_signal(CARD_READER_DATA_SIGNAL, SIGNAL_RESET, SIGNAL_RESET);
    return NULL;
}

static void *xfer_routine(void *p) {
    
    struct XFER_PARAMS *par = (struct XFER_PARAMS*) p;
    
    par->status = jsr177_xfer_data(par->tx_buffer, par->tx_length,
                                   par->rx_buffer, &par->rx_length);
    midp_thread_signal(CARD_READER_DATA_SIGNAL, SIGNAL_XFER, SIGNAL_XFER);
    return NULL;
}

JSR177_STATUSCODE jsr177_xfer_data_start(jbyte *tx_buffer, 
                                            jsize tx_length,
                                            jbyte *rx_buffer, jsize *rx_length,
                                            void **context) {
    struct XFER_PARAMS *par;
    pthread_t handle; // unused
    jsize rxlen = *rx_length;
    (void)rx_buffer;
    
    par = midpMalloc(sizeof *par);
    if (par == NULL) {
        return JSR177_STATUSCODE_FAIL;
    }
    par->rx_buffer = midpMalloc(rxlen);
    if (par->rx_buffer == NULL) {
        midpFree(par);
        return JSR177_STATUSCODE_FAIL;
    }
    par->rx_length = rxlen;
    par->tx_buffer = midpMalloc(tx_length);
    if (par->tx_buffer == NULL) {
        midpFree(par->rx_buffer);
        midpFree(par);
        return JSR177_STATUSCODE_FAIL;
    }
    par->tx_length = tx_length;
    memcpy(par->tx_buffer, tx_buffer, tx_length);
    par->status = JSR177_STATUSCODE_WOULD_BLOCK;
    *context = (void*)par;
    pthread_create(&handle, 0, &xfer_routine, (void*)par);
    return JSR177_STATUSCODE_WOULD_BLOCK;
}

JSR177_STATUSCODE jsr177_xfer_data_finish(jbyte *tx_buffer, 
                                            jsize tx_length,
                                            jbyte *rx_buffer, jsize *rx_length,
                                            void *context) {
    struct XFER_PARAMS *par = (struct XFER_PARAMS*) context;
    jsize rxlen = *rx_length;
    JSR177_STATUSCODE status = par->status;
    
    (void)tx_buffer;
    (void)tx_length;
    if (status == JSR177_STATUSCODE_WOULD_BLOCK) {
        return JSR177_STATUSCODE_WOULD_BLOCK;
    }
    if (status != JSR177_STATUSCODE_OK) {
        return status;
    }
    if (rxlen < par->rx_length) {
        midpFree(par->rx_buffer);
        midpFree(par);
        return JSR177_STATUSCODE_FAIL;
    }
    memcpy(rx_buffer, par->rx_buffer, par->rx_length);
    *rx_length = par->rx_length;
    midpFree(par->rx_buffer);
    midpFree(par->tx_buffer);
    midpFree(par);
    return JSR177_STATUSCODE_OK;
}
JSR177_STATUSCODE jsr177_reset_start(jbyte *rx_buffer, jsize *rx_length,
                                            void **context) {
    struct RESET_PARAMS *par;
    pthread_t handle; // unused
    jsize rxlen = *rx_length;
    
    (void)rx_buffer;
    par = midpMalloc(sizeof *par);
    if (par == NULL) {
        return JSR177_STATUSCODE_FAIL;
    }
    par->atr = midpMalloc(rxlen);
    if (par->atr == NULL) {
        midpFree(par);
        return JSR177_STATUSCODE_FAIL;
    }
    par->atr_size = rxlen;
    par->status = JSR177_STATUSCODE_WOULD_BLOCK;
    *context = (void*)par;
    pthread_create(&handle, 0, &reset_routine, (void*)par);
    return JSR177_STATUSCODE_WOULD_BLOCK;
}

JSR177_STATUSCODE jsr177_reset_finish(jbyte *rx_buffer, jsize *rx_length,
                                            void *context) {
    struct RESET_PARAMS *par = (struct RESET_PARAMS*) context;
    jsize rxlen = *rx_length;
    JSR177_STATUSCODE status = par->status; 
    
    if (status == JSR177_STATUSCODE_WOULD_BLOCK) {
        return JSR177_STATUSCODE_WOULD_BLOCK;
    }
    if (status != JSR177_STATUSCODE_OK) {
        return status;
    }
    if (rxlen < par->atr_size) {
        midpFree(par->atr);
        midpFree(par);
        return JSR177_STATUSCODE_FAIL;
    }
    memcpy(rx_buffer, par->atr, par->atr_size);
    *rx_length = par->atr_size;
    midpFree(par->atr);
    midpFree(par);
    return JSR177_STATUSCODE_OK;
}
#endif /* ASYNCHRO */

#ifdef ASYNCHRO_NOT_DEFINED

#define TASK_IDLE   0
#define TASK_RESET  1
#define TASK_XFER   2
static volatile jboolean task_running;
static volatile jint do_task;
static volatile jboolean task_done;
static volatile void *task_context;

static volatile jbyte t_tx_buffer[256];
static volatile jsize t_tx_length;
static volatile jbyte t_rx_buffer[256+2];
static volatile jsize t_rx_length;
static volatile JSR177_STATUSCODE t_status;
static int context_cnt = 0;
static volatile pthread_mutex_t mutex;

static void *task_routine(void *unused) {
    
    extern void pthread_yield();
    (void)unused;
    
    while (task_running) {
        if (!task_done) {
            counter = 0;
            switch (do_task) {
                case TASK_RESET:
                    t_status = jsr177_reset((jbyte*)t_rx_buffer, 
                        (jsize*)&t_rx_length);
                    task_done = PCSL_TRUE;
                    midp_thread_signal(CARD_READER_DATA_SIGNAL, SIGNAL_XFER, SIGNAL_XFER);
                break;
            case TASK_XFER:
                    t_status = jsr177_xfer_data((jbyte*)t_tx_buffer, 
                            (jsize)t_tx_length, 
                            (jbyte*)t_rx_buffer, 
                            (jsize*)&t_rx_length);
                    task_done = PCSL_TRUE;
                    midp_thread_signal(CARD_READER_DATA_SIGNAL, SIGNAL_XFER, SIGNAL_XFER);
                break;
            }
        } else {
            pthread_yield();
        }
    }
    return NULL;
}

static int start_thread() {
    pthread_t handle; // unused
    
    if (!task_running) {
        do_task = TASK_IDLE;
        task_done = PCSL_TRUE;
        task_running = PCSL_TRUE;
        return pthread_create(&handle, 0, &task_routine, NULL /* unused */);
    }
    return 0;
}

JSR177_STATUSCODE jsr177_xfer_data_start(jbyte *tx_buffer, 
                                            jsize tx_length,
                                            jbyte *rx_buffer, jsize *rx_length,
                                            void **context) {

    (void)rx_buffer;
    
    if (start_thread() != 0) {
        return JSR177_STATUSCODE_FAIL;
    }
    if (do_task == TASK_IDLE) {
        void *my_context = (void*)(++context_cnt);

        memcpy((jbyte*)t_tx_buffer, tx_buffer, tx_length);
        t_tx_length = tx_length;
        t_rx_length = *rx_length;
        
        if (context != NULL) {
            *context = my_context;
        }
        task_done = PCSL_FALSE;
        task_context = my_context;
        do_task = TASK_XFER;
    }
    return JSR177_STATUSCODE_WOULD_BLOCK;
}

JSR177_STATUSCODE jsr177_xfer_data_finish(jbyte *tx_buffer, 
                                            jsize tx_length,
                                            jbyte *rx_buffer, jsize *rx_length,
                                            void *context) {

    (void)tx_buffer;
    (void)tx_length;
    
    if (do_task == TASK_XFER && task_done && task_context == context) {
        JSR177_STATUSCODE status = t_status;
        memcpy(rx_buffer, (jbyte*)t_rx_buffer, t_rx_length);
        *rx_length = t_rx_length;
        do_task = TASK_IDLE;
        return status;
    }
    if (do_task == TASK_IDLE) {
        
        task_done = PCSL_FALSE;
        do_task = TASK_XFER;
        task_context = context;
    }
    return JSR177_STATUSCODE_WOULD_BLOCK;
}


JSR177_STATUSCODE jsr177_reset_start(jbyte *rx_buffer, jsize *rx_length,
                                                    void **context) {
    (void)rx_buffer;
    
    if (start_thread() != 0) {
        return JSR177_STATUSCODE_FAIL;
    }
    if (do_task == TASK_IDLE) {
        void *my_context = (void*)(++context_cnt);

        if (context != NULL) {
            *context = my_context;
        }
        t_rx_length = *rx_length;
        task_done = PCSL_FALSE;
        task_context = my_context;
        do_task = TASK_RESET;
    }
    return JSR177_STATUSCODE_WOULD_BLOCK;
}

JSR177_STATUSCODE jsr177_reset_finish(jbyte *rx_buffer, jsize *rx_length,
                                                    void *context) {
    
    if (do_task == TASK_RESET && task_done && task_context == context) {
        JSR177_STATUSCODE status = t_status;
        memcpy(rx_buffer, (jbyte*)t_rx_buffer, t_rx_length);
        *rx_length = t_rx_length;
        do_task = TASK_IDLE;
        return status;
    }
    if (do_task == TASK_IDLE) {
        
        t_rx_length = *rx_length;
        task_done = PCSL_FALSE;
        do_task = TASK_RESET;
        task_context = context;
    }
    return JSR177_STATUSCODE_WOULD_BLOCK;
}

#endif

