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

package javax.microedition.lcdui;

import com.sun.midp.lcdui.ForegroundController;
import com.sun.midp.security.SecurityToken;

public class StubDisplay extends Display {

    /**
     * Creates a simple stub display and tells it that it is in the 
     * foreground.
     *
     * @param ownerName class name of the MIDlet that owns this Display
     */
    public StubDisplay(String ownerName) {
        super(ownerName);
        foregroundController = new StubController();
        foregroundConsumer.handleDisplayForegroundNotifyEvent();
    }

    /**
     * Creates a stub display, in the foreground, with a stubbed out midlet
     * controller event producer. This is sometimes necessary to prevent 
     * certain display actions (such as setCurrent) from flooding the AMS 
     * event queue with foreground-change requests.
     */
    public StubDisplay() {
        this("StubDisplay");
    }

    /**
     * A stub foreground controller event producer. Stubs out all the
     * methods.
     */
    class StubController implements ForegroundController {

        StubController() {
        }

        public Displayable registerDisplay(int displayId,
                                           String ownerClassName) {
            return null;
        }

        public void requestForeground(int displayId, boolean isAlert) {
        }

        public void requestBackground(int displayId) {
        }

        public void startPreempting(int displayId) {
        }

        public void stopPreempting(int displayId) {
        }

        public void sendDisplayBackgroundRequestEvent(int midletDisplayId) {
        } 

        public void sendDisplayPreemptStartEvent(int midletDisplayId) {
        }

        public void sendDisplayPreemptStopEvent(int midletDisplayId) {
        }
    }
}
