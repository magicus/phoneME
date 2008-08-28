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
package com.sun.midp.io.j2me.pipe;

import com.sun.midp.io.pipe.PipeServerConnection;
import java.io.IOException;
import javax.microedition.io.StreamConnection;

class PipeServerConnectionImpl implements PipeServerConnection {
    private String name;
    private String version;
    private boolean requestedToClose;

    PipeServerConnectionImpl(String serverName, String serverVersion) {
        name = serverName;
        version = serverVersion;
    }

    public String getName() {
        return name;
    }

    public String getVersion() {
        return version;
    }

    public StreamConnection acceptAndOpen() throws IOException {
        if (requestedToClose)
            throw new IOException();
        // here wait till client comes
        
        return new PipeClientConnectionImpl();
    }

    public void close() throws IOException {
        requestedToClose = true;
        // TODO interrupt acceptAndOpen
    }

}
