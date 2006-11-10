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

#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midpServices.h>
#include <midpUtilKni.h>

/**
 * Gets the contents of the given entry inside a JAR file.
 * <p>
 * Java declaration:
 * <pre>
 *     readJarEntry0(Ljava/lang/String;Ljava/lang/String;)[B
 * </pre>
 *
 * @param localJarFilePath File pathname of the JAR file to read.
 *                         May be a relative pathname.
 * @param localEntryname Name of the entry to return.
 * @return The contents of the given entry as a byte array, or
 *         <tt>null</tt> it the entry was not found
 * @throw IOException if JAR is corrupt or not found
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_installer_JarReader_readJarEntry0() {
    KNI_StartHandles(3);
    KNI_DeclareHandle(fileContents);
    GET_PARAMETER_AS_PCSL_STRING(1, jarName)
    GET_PARAMETER_AS_PCSL_STRING(2, entryName) {
        if (KNI_TRUE !=
            midp_readJarEntry(&jarName, &entryName, &fileContents)) {
            KNI_ThrowNew(midpIOException,
                         "JAR not found or corrupt");
        }
    } RELEASE_PCSL_STRING_PARAMETER
    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandlesAndReturnObject(fileContents);
}
