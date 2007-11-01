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

package com.sun.midp.main;

import javax.microedition.midlet.MIDlet;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Form;

import com.sun.midp.events.EventQueue;
import com.sun.midp.security.ImplicitlyTrustedClass;
import com.sun.midp.security.SecurityInitializer;
import com.sun.midp.security.SecurityToken;

/**
 * A midlet to start NAMS Test Service.
 */
public class NamsTestServiceMidlet extends MIDlet {
    /**
     * Inner class to request security token from SecurityInitializer.
     * SecurityInitializer should be able to check this inner class name.
     */
    static private class SecurityTrusted
        implements ImplicitlyTrustedClass {}

    /**
     * Midlet constructor.
     */
    public NamsTestServiceMidlet() {
        SecurityToken internalSecurityToken =
            SecurityInitializer.requestToken(new SecurityTrusted());

        NamsTestService.init(internalSecurityToken,
                EventQueue.getEventQueue(internalSecurityToken));
    }

    /**
     * Starts the midlet.
     */
    public void startApp() {
        Form f = new Form("NAMS Test Service");
        f.append("Run \"telnet 127.0.0.1 " + NamsTestService.PORT +
                 "\" to send commands to the AMS.");
        Display.getDisplay(this).setCurrent(f);
    }

    /**
     * Pauses the midlet.
     */
    public void pauseApp() {}

    /**
     * Destroys the midlet.
     *
     * @param unconditional is ignored; this object always
     * destroys itself when requested.
     */
    public void destroyApp(boolean unconditional) {}
}

