/*
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

package com.sun.cdc.io.j2me.jcrmi;

import java.rmi.*;
import javacard.framework.*;
import javacard.framework.service.*;

public class DataTestApplet extends javacard.framework.Applet
        implements MultiSelectable {

    private Dispatcher disp;
    private RemoteService serv;
    private Remote remote1;
    public static byte channel = 0;

    public DataTestApplet() {
        remote1 = new Remote1Impl();
        disp = new Dispatcher((short) 1);
        serv = new RMIService(remote1);
        disp.addService(serv, Dispatcher.PROCESS_COMMAND);
        register();
    }

    public static void install(byte[] aid, short s, byte b) {
        new DataTestApplet();
    }
    
    public void process(APDU apdu) throws ISOException {
        disp.process(apdu);
    }

    public boolean select(boolean appInstAlreadyActive) {
        return true;
    }

    public void deselect(boolean appInstStillActive) {}
}
