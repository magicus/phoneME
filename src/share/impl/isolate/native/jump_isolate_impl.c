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
#include <jni.h>
#include <jump_messaging.h>
#include <stdio.h>

/*
 * FIXME: Is there a reason for this native implementation anymore?
 * Let's keep this here for a little while more to make sure.
 */
static jboolean messaging_started = JNI_FALSE;
static void ensureInitialized() 
{
    /*
     * This is really for stand-alone execution.
     * We should really make this part of JVM initialization. 
     */
    if (!messaging_started) {
	jumpMessageStart();
	messaging_started = JNI_TRUE;
    }
}

static void
dumpMessage(JUMPMessage m, char* intro)
{
    JUMPMessageReader r;
    JUMPPlatformCString* strings;
    uint32 len, i;
    
    ensureInitialized();
    
    printf("%s\n", intro);
    jumpMessageReaderInit(&r, m);
    strings = jumpMessageGetStringArray(&r, &len);
    if (r.status != JUMP_SUCCESS) {
	printf("    <failure>\n");
	return;
    }
    if (strings == NULL) {
	printf("    <null>\n");
    }
    else {
	for (i = 0; i < len; i++) {
	    printf("    \"%s\"\n", strings[i]);
	}
    }
    jumpMessageFreeStringArray(strings, len);
}

JNIEXPORT void JNICALL
Java_com_sun_jumpimpl_isolate_jvmprocess_JUMPIsolateProcessImpl_waitForAndEchoMessage(JNIEnv *env, jclass c)
{
    JUMPOutgoingMessage returnMessage;
    JUMPPlatformCString messagestrs[1];
    JUMPMessageStatusCode code;
    JUMPMessage in;
    ensureInitialized();
    
    /* Wait for incoming message, echo it, and send a response back */
    in = jumpMessageWaitFor("mvm/client", 0, &code);
    dumpMessage(in, "waitForAndEchoMessage():");
    returnMessage = jumpMessageNewOutgoingByRequest(in, &code);
    messagestrs[0] = "YES";
    jumpMessageAddStringArray(returnMessage, messagestrs, 1);
    jumpMessageSendAsyncResponse(returnMessage, &code);
}

