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

/**
 * @file native_handlers.c
 * @ingroup CHAPI
 * @brief Native handlers
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <pthread.h>

#include "javacall_invoke.h"
#include "javacall_registry.h"


typedef void* native_handler_info;

typedef struct _native_invocation_handle_impl {
	unsigned long id;
	char* handler_id;
	char* cmd_line;
	char *apppath;
	char* action;
	char* url;
	pthread_t *monitor_thread;
	pthread_mutex_t locked;
} native_invocation_handle_impl;

static native_invocation_handle_impl* alloc_invoc_handle(){
		native_invocation_handle_impl* invoc = (native_invocation_handle_impl*)malloc (sizeof(native_invocation_handle_impl));
		if (!invoc) return 0;
		memset(invoc, 0, sizeof(native_invocation_handle_impl));
		return invoc;
}

static void native_invocation_lock(native_invocation_handle_impl* invoc){
	if (!invoc->locked){
		pthread_mutex_init(&invoc->locked,0);
	} else {
		pthread_mutex_lock(&invoc->locked);
	}
}

static void native_invocation_unlock(native_invocation_handle_impl* invoc){
	if (invoc->locked) pthread_mutex_unlock(&invoc->locked);
}

static void free_invoke_handle(native_invocation_handle_impl* invoc){
	if (!invoc) return;
	free(invoc);
}


static int monitor_run(){
	return -1;
}

static int register_monitor(native_invocation_handle_impl* invoc){
	return -1;
}



/*******************************************************
********				PUBLIC API				********
********************************************************/

void native_handler_exec_wait(native_invocation_handle hinvoc){
}

void native_handler_exec_cleanup_monitor(native_invocation_handle invoc){
}



native_invocation_handle native_handler_exec_invoke(const unsigned short* content_handler_id, const char* action, const char* url){
	native_invocation_handle_impl* invoc=0;
	return invoc;
}


