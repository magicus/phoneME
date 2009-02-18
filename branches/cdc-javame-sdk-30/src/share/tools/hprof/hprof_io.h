/*
 * @(#)hprof_io.h	1.10 06/10/10
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

#ifndef _HPROF_IO_H
#define _HPROF_IO_H

void hprof_write_header(unsigned char type, jint length);
void hprof_write_dev(void *buf, int len);
void hprof_write_raw(void *buf, int len);
void hprof_flush(void);
void hprof_printf(char *fmt, ...);
void hprof_write_current_ticks();
void hprof_write_u4(unsigned int i);
void hprof_write_u2(unsigned short i);
void hprof_write_u1(unsigned char i);
void hprof_write_id(void *p);
void hprof_io_setup(void);
#ifdef WATCH_ALLOCS
void hprof_io_free(void);
#endif /* WATCH_ALLOCS */

void hprof_dump_seek(char *ptr);
char * hprof_dump_cur(void);
void hprof_dump_read(void *buf, int size);
void * hprof_dump_read_id(void);
unsigned int hprof_dump_read_u4(void);
unsigned short hprof_dump_read_u2(void);
unsigned char hprof_dump_read_u1(void);

#endif /* _HPROF_IO_H */
