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

package com.sun.midp.content;

import com.sun.midp.log.*;
import javax.microedition.midlet.MIDlet;
import javax.microedition.content.*;

/**
 * The tool for start given Content Handler from the platform.
 */
class Invoker extends MIDlet implements Runnable {

    Registry r;

    /**
     * Default constructor.
     */
    public Invoker() {
        r = Registry.getRegistry(this.getClass().getName());
        new Thread(this).start();
    }

    /**
     * Standard MIDlet life-cycle call-back.
     */
    protected void startApp() {
    }

    /**
     * Standard MIDlet life-cycle call-back.
     */
    protected void pauseApp() {
    }

    /**
     * Standard MIDlet life-cycle call-back.
     */
    protected void destroyApp(boolean unconditional) {
    }
    
    /**
     * Loads Invocation parameters.
     * According current implementation Invocation parameters consist of
     * <ContentHandlerID>, <URL> and <Type> command line arguments provided 
     * for this MIDlet.
     */
    private Invocation getNativeInvocation() {
        String id = getAppProperty("arg-0");
        String url = getAppProperty("arg-1");
        String action = getAppProperty("arg-2"); /* optional */
        String type = null;
        boolean responseRequired = false;

        return new Invocation(url, type, id, responseRequired, action);
    }

    /**
     * Actual handler invoking is performed in the separated thread.
     */
    public void run() {
        Invocation i = getNativeInvocation();
        try { 
            r.invoke(i);
        } catch (Throwable t) {
            Logging.report(Logging.ERROR, LogChannels.LC_AMS,
                "Exception during invoking native invocation: " + t);
        }
        notifyDestroyed();
    }
}
