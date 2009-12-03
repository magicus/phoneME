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

#include <midpEvents.h>
#include <jvm.h>

/**
 * Timeout used to distinguish simple drag event
 * from flick event.
 */
#define flick_timeout 50

/**
 * Time when the last pen drag event happened.
 */
static jlong drag_time;

/**
 * KNI_TRUE if last pen event was MIDP_DRAGGED event.
 */
static jboolean pen_dragged = KNI_FALSE;


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
jboolean midpCheckCompoundEvent(MidpEvent* pSimpleEvent, MidpEvent* pCompoundEvent) {

    jboolean res = KNI_FALSE;
    if (pSimpleEvent->type == MIDP_PEN_EVENT) {
        switch (pSimpleEvent->ACTION) {
            case MIDP_DRAGGED:
                drag_time = JVM_JavaMilliSeconds();
                pen_dragged = KNI_TRUE;
                break;
            case MIDP_RELEASED: {
                jlong current_time = JVM_JavaMilliSeconds();
                if ((pen_dragged == KNI_TRUE)
                        && (current_time - drag_time < flick_timeout)) {
                    pCompoundEvent->type    = MIDP_PEN_EVENT;
                    pCompoundEvent->ACTION  = MIDP_FLICKERED;
                    pCompoundEvent->X_POS   = pSimpleEvent->X_POS;
                    pCompoundEvent->Y_POS   = pSimpleEvent->Y_POS;
                    res = KNI_TRUE;
                }
                pen_dragged = KNI_FALSE;
                break;
            }
            default:
                pen_dragged = KNI_FALSE;
                break;
        }
    }
    return res;

}
