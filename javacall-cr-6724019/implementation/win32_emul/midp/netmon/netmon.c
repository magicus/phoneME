/*
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

#include "lime.h"

#define LIME_PACKAGE "com.sun.kvem.midp"
#define LIME_EVENT_CLASS "EventBridge"

int isNetworkMonitorActive()
{
    static int result = -1;

    /* The result can not be changed during the execution */
    /* hence it can be requested only one time */
    if (result == -1) {
        /* boolean isNetworkMonitorActive() */
        LimeFunction *f =NewLimeFunction(LIME_PACKAGE,
				         LIME_EVENT_CLASS,
				         "isNetworkMonitorActive");
        f->call(f, &result);
        DeleteLimeFunction(f);
    }
    return result;
}

