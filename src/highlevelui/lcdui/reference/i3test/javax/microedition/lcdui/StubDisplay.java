/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

import com.sun.midp.main.MIDletControllerEventProducer;
import com.sun.midp.security.SecurityToken;

public class StubDisplay extends Display {

    /**
     * Creates a simple stub display and tells it that it is in the 
     * foreground.
     */
    public StubDisplay() {
        super(null);
        consumer.handleDisplayForegroundNotifyEvent(true);
    }

    /**
     * Creates a stub display, in the foreground, with a stubbed out midlet
     * controller event producer. This is sometimes necessary to prevent 
     * certain display actions (such as setCurrent) from flooding the AMS 
     * event queue with foreground-change requests.
     */
    public StubDisplay(SecurityToken token) {
        super(null);
        midletControllerEventProducer = new StubProducer(token);
        consumer.handleDisplayForegroundNotifyEvent(true);
    }

    /**
     * A stub midlet controller event producer. Stubs out all the
     * event-sending methods.
     */
    class StubProducer extends MIDletControllerEventProducer {

        public StubProducer(SecurityToken token) {
            super(token, null, 0, 0);
        }

        public void sendMIDletStartErrorEvent(
            int midletExternalAppId,
            String midletSuiteId, 
            String midletClassName,
            int error) {
        }

        public void sendMIDletCreateNotifyEvent(
            int midletExternalAppId,
            int midletDisplayId, 
            String midletSuiteId,
            String midletClassName, 
            String midletDisplayName) {
        }

        public void sendMIDletActiveNotifyEvent(int midletDisplayId) {
        }

        public void sendMIDletPauseNotifyEvent(int midletDisplayId) {
        }

        public void sendMIDletDestroyNotifyEvent(int midletDisplayId) {
        }

        public void sendMIDletDestroyRequestEvent(int midletDisplayId) {
        }
        
        public void sendMIDletForegroundTransferEvent(
            String originMIDletSuiteId,
            String originMIDletClassName,
            String targetMIDletSuiteId,
            String targetMIDletClassName) {
        }

        public void sendSetForegroundByNameRequestEvent(String suiteId, 
            String className) {
        }
    
        public void sendDisplayForegroundRequestEvent(int midletDisplayId,
            boolean isAlert) {
        }

        public void sendDisplayBackgroundRequestEvent(int midletDisplayId) {
        } 

        public void sendDisplayPreemptStartEvent(int midletDisplayId) {
        }

        public void sendDisplayPreemptStopEvent(int midletDisplayId) {
        }

        public void sendMIDletResumeRequest(int midletDisplayId) {
        }
    }

}
