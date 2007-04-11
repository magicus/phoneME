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

#ifndef _JUMP_EVENTS_H
#define _JUMP_EVENTS_H

#include <JUMPEvents_md.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char opaque__[sizeof (struct _JUMPEventIMPL_tag)];
} *JUMPEvent;

JUMPEvent jumpEventCreate();
void jumpEventDestroy(JUMPEvent evt);

int jumpEventWait(JUMPEvent evt);
int jumpEventHappens(JUMPEvent evt);

int jumpEventReset(JUMPEvent evt);

#ifdef __cplusplus
}
#endif

#endif /* #ifdef _JUMP_EVENTS_H */

