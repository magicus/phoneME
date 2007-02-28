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

#include <stdio.h> // NULL, fprintf()
#include <string.h> // strcmp()
#include <jump_messaging.h>
#include "javavm/include/porting/threads.h"

void jsr135_jumpdriver_listener(JUMPMessage *in);

int close_driver = 0;
void jsr135DriverMain(int argc, char **argv) {
    JUMPMessage in;
    int ret;
    printf("Function name: %s\n", argv[0]);
    printf("Library name: %s\n", argv[1]);

    ret = jumpMessageStart();
    
    do {
        in = jumpMessageWaitFor((JUMPPlatformCString)"mm/jsr135", 0);
        jsr135_jumpdriver_listener(&in);
    } while (0 == close_driver);

    sleep(1);
    
    jumpMessageShutdown();
}
