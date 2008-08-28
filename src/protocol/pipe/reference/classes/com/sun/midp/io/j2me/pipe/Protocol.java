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

package com.sun.midp.io.j2me.pipe;

import com.sun.cldc.io.ConnectionBaseInterface;
import java.io.IOException;
import javax.microedition.io.Connection;

public class Protocol implements ConnectionBaseInterface {

    public Connection openPrim(String name, int mode, boolean timeouts) throws IOException {
        if (name.charAt(0) != '/' || name.charAt(1) != '/')
            throw new IllegalArgumentException(
                      "Protocol must start with \"//\"");

        // server. format is: "pipe://:server-name:server-version;"
        // client. format is: "pipe://[suite-id|*]:server-name:server-version;"
        // suite-id is midlet suite's "vendor:name:version" triplet
        int colon2 = name.lastIndexOf(':');
        int colon1 = name.lastIndexOf(':', colon2-1);
        int semicolon = name.lastIndexOf(';');
        if (colon1 < 0 || semicolon < name.length() - 1)
            throw new IllegalArgumentException("Malformed server protocol name");
        String serverName = name.substring(colon1, colon2);
        String version = name.substring(colon2 + 1, semicolon);

        // check if we deal with server or client connection
        if (colon1 == 2) {
            // check if this is AMS isolate and opens connection for push purposes
            //       or this is user isolate and it needs to checkout connection from AMS
            // TODO

            return new PipeServerConnectionImpl(serverName, version);
        } else {
            Object suiteId = null;
            if (name.charAt(2) == '*') {
                if (colon1 != 3)
                    throw new IllegalArgumentException("Malformed protocol name");
            }
            
            PipeClientConnectionImpl connection = 
                    new PipeClientConnectionImpl(suiteId, serverName, version);
            connection.connect(mode);
            
            return connection;
        }
    }

    /*
    private static int parseVersion(String str) {
        int dot1 = str.indexOf('.');
        int dot2 = str.indexOf('.', dot1 + 1);
        if (dot1 < 1 || dot2 == dot1+1)
            throw new IllegalArgumentException("Malformed protocol version");
        int version = 0;
        try {
            if (dot2 < 0)
                dot2 = str.length();
            version = Integer.parseInt(str.substring(0, dot1)) * 10000 +
                    Integer.parseInt(str.substring(dot1+1, dot2)) * 100;
            if (dot2 < str.length())
                version += Integer.parseInt(str.substring(dot2));
        }
        catch (NumberFormatException ex) {
            throw new IllegalArgumentException("Malformed protocol version");
        }
        return version;
    }
     */
}
