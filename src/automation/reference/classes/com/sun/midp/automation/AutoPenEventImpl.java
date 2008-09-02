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

package com.sun.midp.automation;
import com.sun.midp.events.*;
import com.sun.midp.lcdui.EventConstants;

final class AutoPenEventImpl 
    extends AutoEventImplBase implements AutoPenEvent {

    final static String X_ARG_NAME = "x";
    final static String Y_ARG_NAME = "y";
    final static String STATE_ARG_NAME = AutoKeyEventImpl.STATE_ARG_NAME;

    private int x;
    private int y;
    private AutoPenState penState;


    public AutoPenState getPenState() {
        return penState;
    }

    public int getX() {
        return x;
    }

    public int getY() {
        return y;
    }
    
    public String toString() {
        String typeStr = getType().getName();
        String stateStr = getPenState().getName();

        String eventStr = typeStr + " x: " + x + ", y: " + y + 
            ", state: " + stateStr;

        return eventStr;
    }

    AutoPenEventImpl(int x, int y, AutoPenState penState) {
        super(AutoEventType.PEN, 
                createNativeEvent(x, y, penState));

        if (penState == null) {
            throw new IllegalStateException("Pen state is null");
        }

        this.x = x;
        this.y = y;
        this.penState = penState;
    }

    private static NativeEvent createNativeEvent(int x, int y, 
            AutoPenState penState) {

        NativeEvent nativeEvent = new NativeEvent(EventTypes.PEN_EVENT);
        nativeEvent.intParam1 = penState.getMIDPPenState();
        nativeEvent.intParam2 = x;
        nativeEvent.intParam3 = y;

        return nativeEvent;
    }
}
