/*
 *    
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

#include <string.h>

#include <javacall_defs.h>
#include <javacall_multimedia.h>

#include <native/common/jni_util.h>

/**
 * Returns true if audio mixing is supported.
 *
 * This method is called when the <code>supports.mixing</code> system
 * property is retrieved.
 */
JNIEXPORT jstring JNICALL
Java_com_sun_jsr135_DynamicProperties_nSupportsMixing(JNIEnv *env, jobject this) {
    jstring rv = NULL;
    if( JAVACALL_TRUE == javacall_media_supports_mixing() )
    {
        rv = JNU_NewStringPlatform(env, "true");
    } else {
        rv = JNU_NewStringPlatform(env, "false");
    }
    return rv;
}
