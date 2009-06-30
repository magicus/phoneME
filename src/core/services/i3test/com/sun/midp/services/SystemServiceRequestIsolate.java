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

package com.sun.midp.services;

import com.sun.midp.security.*;
import com.sun.midp.links.*;
import java.io.*;

/**
 * Isolate created in TestSystemServiceRequest test.
 */
public class SystemServiceRequestIsolate  {
    private static SecurityToken token = SecurityTokenProvider.getToken();

    public static void main(String[] args) 
        throws ClosedLinkException, 
               InterruptedIOException, 
               IOException {

        Link[] isolateLinks = LinkPortal.getLinks();
        SystemServiceLinkPortal.linksObtained(isolateLinks);

        SystemServiceRequestor serviceRequestor = 
            SystemServiceRequestor.getInstance(token);

        SystemServiceConnection con = null;
        con = serviceRequestor.requestService(
                TestSystemServiceRequest.SERVICE_ID + 1);
        if (con != null) {
            System.err.println("Error: Connection is expected to be null");
        }

        con = serviceRequestor.requestService(
                TestSystemServiceRequest.SERVICE_ID);
   }
}

