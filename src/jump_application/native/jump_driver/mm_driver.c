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
#include <JUMPMessages.h>
#include "javavm/include/porting/threads.h"

void jsr135_jumpdriver_listener(JUMPMessage *m__, jmpMessageQueue queue__, void *context__);

int mm_driver(int argc, const char **argv) {
    char *context = "";
    JUMPMessage *m;
    int quit;
    JUMPMessageFlag quitFlag;
    int ready;
    JUMPMessageFlag readyFlag;
    unsigned char buf[NMSG_MAX_LENGTH];
    if (context == NULL || jumpMessageQueueCreate() < 0) {
        fprintf(stderr, "Cannot create queue!!\n");
        return -1;
    }
    readyFlag = jumpMessageFlagCreate("executive/ready", &ready);
    quitFlag = jumpMessageFlagCreate("internal/quit", &quit);
    jumpMessageQueueAddHandlerOnType("mm/jsr135", jsr135_jumpdriver_listener, context);
    m = jumpMessageCreateInBuffer("executive/iamready", buf, sizeof buf);
    jumpMessageSend(jumpProcessGetExecutiveId(), m);
    jumpMessageFlagWait(readyFlag);
    jumpMessageFlagWait(quitFlag);
    m = jumpMessageCreateInBuffer("internal/quit", buf, sizeof buf);
    jumpMessageSend(jumpProcessGetId(), m);
    jumpMessageQueueDestroy();
    return 0;
}


