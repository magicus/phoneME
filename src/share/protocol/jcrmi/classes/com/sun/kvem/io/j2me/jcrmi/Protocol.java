/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/

package com.sun.kvem.io.j2me.jcrmi;

import javax.microedition.io.Connection;
import java.io.*;

/**
 * JCRMI connection to card application.
 */
public class Protocol extends com.sun.midp.io.j2me.jcrmi.Protocol {

    private static int md = -1;


    protected byte[] getApduCommand() {
            return apduCommand;
    }
    
    protected byte[] getApduResponse() {
            return apduResponse;
    }

    public Connection openPrim(String name, int mode, boolean timeouts)
            throws IOException {
        Connection c = super.openPrim(name, mode, timeouts);
        long groupid = System.currentTimeMillis();
        md = open0(name, groupid);
        response0(md, getApduResponse());
        return c;
    }

    /*
     * Called from invoke method at the super class to pass the data to
     * the network monitor
     * In order to prevent from a security problems only the native call are
     * done from there.
     */
    private void callNativeMethods(int md, byte[] bufApduCommand,
                                       byte[] bufApduResponse, String method) {
        invoke0(md, bufApduCommand, method);
        response0(md, bufApduResponse);
    }

    protected void invokeImpl(String method){
        callNativeMethods( md, getApduCommand(), getApduResponse(), method);
    }

    private static native int open0(String buf, long groupid);

    private static native void invoke0(int md, byte[] buf, String method);

    private static native void response0(int md, byte[] buf);
}
