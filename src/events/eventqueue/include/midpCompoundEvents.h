/*
 *
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
#ifndef _MIDP_COMPOUND_EVENTS_H_
#define _MIDP_COMPOUND_EVENTS_H_

#include <midpEvents.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Checks whether compound event occured. All arrived midpEvents should be
 * feeded to get appropriate result. Returns KNI_FALSE if there is no compound
 * event. Returns KNI_TRUE if compound event is found. In this case passed
 * argument out is filled with compound event parameters and is ready to be
 * processed.
 * @param pSimpleEvent The event received from the system
 * @param pCompoundEvent The event to be filled if compound event is been
 *        recognized
 * @return KNI_TRUE if a compound event occured, KNI_FALSE otherwise
 */
jboolean midpCheckCompoundEvent(MidpEvent* pSimpleEvent, MidpEvent* pCompoundEvent);

#ifdef __cplusplus
}
#endif
#endif // _MIDP_COMPOUND_EVENTS_H_
