/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#include <kni.h>

#include <midlet.h>
#include <midpMalloc.h>
#include <midpError.h>

/**
 * @file 
 *
 * Interface between a MIDlet and native system calls,
 * such as platformRequest().
 */

/**
 * Passes the URL to the native handler. Return true to signal the suite
 * that it must exit before the request can be handled, or false if
 * the request was spawned in the background. Throw ConnectionNotFoundException
 * if we know at the time of this call the request cannot be handled.
 * <p>
 * Java declaration:
 * <pre>
 *     boolean dispatchPlatformRequest(String url)
 *             throws ConnectionNotFoundException
 * </pre>
 * Java parameter:
 * <pre>
 *     url - The URL String to handle
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_main_CldcPlatformRequest_dispatchPlatformRequest) {
    jsize urlLen;
    char* pszUrl;
    jchar* temp;
    int i;
    int connectionFound = KNI_FALSE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(urlObj);

    KNI_GetParameterAsObject(1, urlObj);
    if (!KNI_IsNullHandle(urlObj)) {
        urlLen = KNI_GetStringLength(urlObj);
        if (urlLen >= 0) {
            pszUrl = (char*)midpMalloc((urlLen + 1) * sizeof (jchar));
            if (pszUrl != NULL) {
                temp = (jchar*)pszUrl;
                KNI_GetStringRegion(urlObj, 0, urlLen, (jchar*)temp);

                /* simply convert the unicode by stripping the high byte */
                for (i = 0; i < urlLen; i++) {
                    pszUrl[i] = (char)temp[i];
                }

                pszUrl[urlLen] = 0;
                connectionFound = platformRequest(pszUrl);
                midpFree(pszUrl);
            }
        }
    }

    KNI_EndHandles();

    if (!connectionFound) {
        KNI_ThrowNew(midpConnectionNotFoundException, NULL);
    }

    KNI_ReturnBoolean(KNI_FALSE);
}


