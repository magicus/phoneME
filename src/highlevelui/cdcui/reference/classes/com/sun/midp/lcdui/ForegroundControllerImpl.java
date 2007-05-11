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

package com.sun.midp.lcdui;

import javax.microedition.lcdui.*;


/**
 * Implementation of ForegroundController for CDC.
 */
public class ForegroundControllerImpl implements ForegroundController {

    /** Holds the ID of the current display, for preempting purposes. */
    protected int currentDisplayId;

    /** DisplayContainer associated with this isolate */
    protected DisplayContainer displayContainer;

    /**
     * Constructs ForegroundControllerImpl using the passed in
     * DisplayContainer instance.
     * 
     * @param displayContainer DisplayContainer instance associated with 
     *                         this isolate
     */
    ForegroundControllerImpl(DisplayContainer displayContainer) {
	this.displayContainer = displayContainer;
    }

    /**
     * Called to register a newly create Display. Must method must
     * be called before the other methods can be called.
     * This implementation does nothing.
     *
     * @param displayId ID of the Display
     * @param ownerClassName Class name of the  that owns the display
     *
     * @return a place holder displayable to used when "getCurrent()==null",
     *         if null is returned an empty form is used
     */
    public Displayable registerDisplay(int displayId, String ownerClassName) {
        currentDisplayId = displayId;
        return null;
    }

    /**
     * Called to request the foreground.
     * This implementation does nothing.
     *
     * @param displayId ID of the Display
     * @param isAlert true if the current displayable is an Alert
     */
    public void requestForeground(int displayId, boolean isAlert) {
        ForegroundEventConsumer fc =
            displayContainer.findForegroundEventConsumer(displayId);

        if (fc == null) {
            return;
        }

        setForegroundInNativeState(displayId);

        fc.handleDisplayForegroundNotifyEvent();
    }

    /**
     * Called to request the background.
     * This implementation does nothing.
     *
     * @param displayId ID of the Display
     */
    public void requestBackground(int displayId) {
        ForegroundEventConsumer fc =
            displayContainer.findForegroundEventConsumer(displayId);

        if (fc == null) {
            return;
        }

        fc.handleDisplayBackgroundNotifyEvent();
    }

    /**
     * Called to start preempting. The given display will preempt all other
     * displays for this isolate.
     *
     * @param displayId ID of the Display
     */
    public void startPreempting(int displayId) {
        requestBackground(currentDisplayId);
        requestForeground(displayId, true);
    }

    /**
     * Called to end preempting.
     *
     * @param displayId ID of the Display
     */
    public void stopPreempting(int displayId) {
        requestBackground(displayId);
        requestForeground(currentDisplayId, false);
    }

    /**
     * Set foreground display native state, so the native code will know
     * which display can draw. This method will not be needed when
     * JUMP uses GCI.
     *
     * @param displayId Display ID
     */
    private native void setForegroundInNativeState(int displayId);
}