/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/

package com.sun.kvem.io.j2me.apdu;

import javax.microedition.io.*;

import java.io.*;

public class Protocol extends com.sun.midp.io.j2me.apdu.Protocol {

    private int md = -1;

    public Protocol() {};

    public Connection openPrim(String name, int mode, boolean timeouts)
                                                        throws IOException {
        Connection c = super.openPrim(name, mode, timeouts);
        long groupid = System.currentTimeMillis();
        md = open0(name,  groupid);
        exchangeAPDU0(md, getConnectionResult());
        return c;
    }

    public byte[] exchangeAPDU(byte[] commandAPDU) throws
        IOException, InterruptedIOException {
            byte[] b = super.exchangeAPDU(commandAPDU);
            //If an exception has not been thrown:
            exchangeAPDU0(md, commandAPDU);
            exchangeAPDU0(md, b);
            return b;
    }

    private static native int open0(String buf, long groupid);

    private static native void exchangeAPDU0(int md, byte[] buf);
}
