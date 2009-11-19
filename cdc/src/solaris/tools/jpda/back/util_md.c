/*
 * @(#)util_md.c	1.1 07/08/5
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
 */

#include "javavm/export/jni.h"
#include "javavm/include/porting/time.h"
#include <unistd.h>

void
CVMformatTime(char *format, size_t format_size, time_t t)
{
    (void)strftime(format, sizeof(format),  
                "%d.%m.%Y %T.%%.3d %Z", localtime(&t));
}

int
md_init(JavaVM *jvm)
{
    return 0;
}

int 
md_write(int filedes, const void *buf, int nBytes)
{
    return write(filedes, buf, nBytes);
}
