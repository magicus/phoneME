/*
 * @(#)QtToolkit.h	1.7 06/10/25
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
#ifndef _QT_TOOLKIT_H
#define _QT_TOOLKIT_H

#if (QT_VERSION >= 0x030000)
#define IS_SPONTANEOUS(_e) (_e->spontaneous())
#else
#define IS_SPONTANEOUS(_e) (TRUE)
#endif /* QT_VERSION */

#define QT_RETURN_NEW_EVENT_OBJECT(_eventSource, _event) \
{\
    if ( _event == NULL ) {                           \
        return NULL;                                  \
    }                                                 \
    QtEventObject *retValue = new QtEventObject();    \
    if ( retValue != NULL ) {                         \
        retValue->event  = _event;                    \
        retValue->source = _eventSource ;             \
    }                                                 \
    else {                                            \
        delete _event;                                \
    }                                                 \
    return retValue;                                  \
}
#endif
