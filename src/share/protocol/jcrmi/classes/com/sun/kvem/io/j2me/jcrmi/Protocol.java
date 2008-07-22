/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.kvem.io.j2me.jcrmi;

import javax.microedition.io.Connection;
import java.io.*;

/**
 * JCRMI connection to card application.
 */
public class Protocol extends com.sun.midp.io.j2me.jcrmi.Protocol {

    private static int md = -1;

    private byte[] apduCommand;
    private byte[] apduResponse;


    /*
     * Pass the APDU that will be sent to APDUManager.exchangeAPDU()
     * to the com.sun.kvem.io.j2me.apdu.Protocol
     */
    protected void netmonSaveApdu(byte[] APDUBuffer) {
        if (APDUBuffer == null) {
            apduCommand = new byte[0];
        } else {
            apduCommand = new byte[APDUBuffer.length];
            System.arraycopy(APDUBuffer, 0, apduCommand, 0, APDUBuffer.length);
        }
    }

    /*
     * Pass the response that received from APDUManager.exchangeAPDU()
     * to the com.sun.kvem.io.j2me.apdu.Protocol
     */
    protected void netmonSaveResponse(byte[] response) {
        if (response == null) {
            apduResponse = new byte[0];
        } else {
            apduResponse = new byte[response.length];
            System.arraycopy(response, 0, apduResponse, 0, response.length);                
        }
    }

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
