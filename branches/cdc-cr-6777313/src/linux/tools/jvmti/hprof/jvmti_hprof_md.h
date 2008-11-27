/*
 * @(#)hprof_md.h	1.7 06/10/10
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
 *
 */

#ifndef _LINUX_HPROF_MD_H_
#define _LINUX_HPROF_MD_H_

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>

#include "jni.h"
#include "jvmti_hprof.h"

void    md_init(void);
int     md_getpid(void);
void    md_sleep(unsigned seconds);
int     md_connect(char *hostname, unsigned short port);
int     md_recv(int f, char *buf, int len, int option);
int     md_shutdown(int filedes, int option);
int     md_open(const char *filename);
int     md_open_binary(const char *filename);
int     md_creat(const char *filename);
int     md_creat_binary(const char *filename);
jlong   md_seek(int filedes, jlong cur);
void    md_close(int filedes);
int     md_send(int s, const char *msg, int len, int flags);
int     md_write(int filedes, const void *buf, int nbyte);
int     md_read(int filedes, void *buf, int nbyte);
jlong   md_get_microsecs(void);
jlong   md_get_timemillis(void);
jlong   md_get_thread_cpu_timemillis(void);
void    md_get_prelude_path(char *path, int path_len, char *filename);
int     md_snprintf(char *s, int n, const char *format, ...);
int     md_vsnprintf(char *s, int n, const char *format, va_list ap);
void    md_system_error(char *buf, int len);

unsigned md_htons(unsigned short s);
unsigned md_htonl(unsigned l);
unsigned md_ntohs(unsigned short s);
unsigned md_ntohl(unsigned l);

void   md_build_library_name(char *holder, int holderlen, char *pname, char *fname);
void * md_load_library(const char *name, char *err_buf, int err_buflen);
void   md_unload_library(void *handle);
void * md_find_library_entry(void *handle, const char *name);

#ifdef LP64
#define HASH_OBJ_SHIFT 4    /* objects aligned on 32-byte boundary */
#else /* !LP64 */
#define HASH_OBJ_SHIFT 3    /* objects aligned on 16-byte boundary */
#endif /* LP64 */

#endif /*_LINUX_HPROF_MD_H_*/
