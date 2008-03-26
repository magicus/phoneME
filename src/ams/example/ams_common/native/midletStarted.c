/*
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

#include <midletStarted.h>

/** <code>true</code> if the MIDlet.startApp() method has not finished yet */
static jboolean wasStarted;

/**
 * Indicate to native layer whether the MIDlet.startApp() method has complete
 * Relevant in SVM only
 * 
 * @param status <code>true</code> is startApp() finished
 */
void setStartAppCompleted(jboolean status) {
    wasStarted = status;
}

/**
 * Check if MIDlet.startApp() was called
 * 
 * @return <code>KNI_TRUE</code> if startApp() was called already,
 *         <code>KNI_FALSE</code> otherwise
 */
jboolean isStartAppCompleted(void) {
    return wasStarted;
}
