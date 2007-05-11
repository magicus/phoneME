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

import com.sun.me.gci.windowsystem.GCIGraphicsEnvironment;
import com.sun.me.gci.windowsystem.GCIDisplay;
import com.sun.me.gci.windowsystem.GCIScreenWidget;

/**
 * Foreground Controller for GCI implementation.
 * Notifies GCIScreenWidget of foreground/background changes.
 */
public class ForegroundControllerGCI extends ForegroundControllerImpl {

    /** GCIScreenWidget associated with this device. */
    private GCIScreenWidget gciScreenWidget;

    /**
     * Constructs ForegroundControllerGCI using the passed in 
     * displayContainer and gciScreenWidget.
     * @parm DisplayContainer instance  that corresponds to this isolate 
     * @param gciScreenWidget insance that  corrsponds o t this isolate
     */
    public ForegroundControllerGCI(DisplayContainer displayContainer,
				   GCIScreenWidget gciScreenWidget) {
	super(displayContainer);

	this.gciScreenWidget = gciScreenWidget;
    }


    /**
     * Called to request the foreground.
     * This implementation does nothing.
     *
     * @param displayId ID of the Display
     * @param isAlert true if the current displayable is an Alert
     */
    public void requestForeground(int displayId, boolean isAlert) {
	super.requestForeground(displayId, isAlert);
	gciScreenWidget.requestFocus();
    }

    /**
     * Called to request the background.
     * This implementation does nothing.
     *
     * @param displayId ID of the Display
     */
    public void requestBackground(int displayId) {
	super.requestBackground(displayId);
	gciScreenWidget.yieldFocus();
    }

    /**
     * Returns <code>GCIScreenWidget</code> instance 
     * that was set in the constructor.
     * @return GCIScreenWidget instance associated with this isolate
     */
    public GCIScreenWidget getGCIScreenWidget() {
	return gciScreenWidget;
    }
}