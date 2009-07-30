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

// IMPL NOTE: leave this in the unnamed package until we change it
// to use the i3 test framework and we find a better place for it.

import javax.microedition.midlet.*;
import com.sun.midp.security.*;
import com.sun.midp.events.*;

/**
 * III (Integrated Internal Interface) testing stand-alone MIDlet.
 */
public class PauseTest extends MIDlet {

    boolean firstTime = true;
    boolean suspended = false;

    /**
     * Inner class to request security token from SecurityInitializer.
     * SecurityInitializer should be able to check this inner class name.
     */
    static private class SecurityTrusted
        implements ImplicitlyTrustedClass {};

    /** Security token to allow access to implementation APIs */
    private static SecurityToken myToken =
        SecurityInitializer.requestToken(new SecurityTrusted());

    private EventQueue myQ;

    public PauseTest() {
	myQ = EventQueue.getEventQueue(myToken);
    }

    public void startApp() {
	if (firstTime) {
	    firstTime = false;
	}
	UnitTestEvent suspendEvent
		= new UnitTestEvent(EventTypes.SYSTEM_EVENT);
	suspendEvent.intParam1 = (EventTypes.SUSPEND_ALL);

	if (!suspended) {
	    System.out.println("startApp() called on startup");
	    /* generate Suspend Event */
	    myQ.post(suspendEvent);
	} else {
	    System.out.println("PASSED: startApp() called on resume");
	    notifyDestroyed();
	}
    }

    public void pauseApp() {
	suspended = true;
	System.out.println("pauseApp() called");

	UnitTestEvent resumeEvent
		= new UnitTestEvent(EventTypes.SYSTEM_EVENT);
	resumeEvent.intParam1 = (EventTypes.RESUME_ALL);

	/* generate Resume Event */
	myQ.post(resumeEvent);
    }

    public void destroyApp(boolean unconditional) {
	System.out.println("destroyApp() called");
    }
}
