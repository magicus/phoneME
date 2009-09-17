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

/**
 * Implements AutoPenEvent interface.
 */
final class AutoPenEventImpl 
    extends AutoEventImplBase implements AutoPenEvent {

    /** Constant for "x" argument name */
    static final String X_ARG_NAME = "x";

    /** Constant for "y" argument name */
    static final String Y_ARG_NAME = "y";

    /** Constant for "state" argument name */
    static final String STATE_ARG_NAME = AutoKeyEventImpl.STATE_ARG_NAME;


    /** x coord of pen tip */
    private int x;

    /** y coord of pen tip */
    private int y;

    /** Pen state */
    private AutoPenState penState;



    /**
     * Gets pen state.
     *
     * @return AutoPenState representing pen state
     */
    public AutoPenState getPenState() {
        return penState;
    }

    /**
     * Gets X coord of pen tip.
     *
     * @return x coord of pen tip
     */
    public int getX() {
        return x;
    }

    /**
     * Gets Y coord of pen tip.
     *
     * @return y coord of pen tip
     */
    public int getY() {
        return y;
    }
    
    /**
     * Gets string representation of event. The format is following:
     *  pen x: x_value, y: y_value, state: state_value
     * where x_value, y_value and state_value are string representation 
     * of x coord, y coord and pen state. For example:
     *  pen x: 10, y: 10, state: pressed
     */
    public String toString() {
        String typeStr = getType().getName();
        String stateStr = getPenState().getName();

        String eventStr = typeStr + " x: " + x + ", y: " + y + 
            ", state: " + stateStr;

        return eventStr;
    }

    /**
     * Gets native event (used by our MIDP implementation) 
     * corresponding to this Automation event.
     *
     * @return native event corresponding to this Automation event
     */
    NativeEvent toNativeEvent() {
        NativeEvent e = createNativeEvent();
        return e;
    }

    /**
     * Constructor.
     *
     * @param x x coord of pen tip
     * @param y y coord of pen tip
     * @param penState pen state
     */
    AutoPenEventImpl(int x, int y, AutoPenState penState) {
        super(AutoEventType.PEN);

        if (penState == null) {
            throw new IllegalStateException("Pen state is null");
        }

        this.x = x;
        this.y = y;
        this.penState = penState;
    }

    /**
     * Creates native event (used by our MIDP implementation) 
     * corresponding to this Automation event.
     *
     * @return native event corresponding to this Automation event 
     */
    private NativeEvent createNativeEvent() {
        NativeEvent nativeEvent = new NativeEvent(EventTypes.PEN_EVENT);
        nativeEvent.intParam1 = penState.getMIDPPenState();
        nativeEvent.intParam2 = x;
        nativeEvent.intParam3 = y;

        return nativeEvent;
    }
}
