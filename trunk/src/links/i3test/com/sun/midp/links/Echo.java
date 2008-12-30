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

package com.sun.midp.links;

import java.io.IOException;

/**
 * A main class for running in another isolate.  Expects an array 
 * of two links. Continually receives messages on the first link and sends 
 * them to the second.
 */
public class Echo {
    public static void main(String[] args) throws IOException {
        Link[] la;

        la = LinkPortal.getLinks();

        if (la == null) {
            throw new IOException("getLinks() returned null");
        }

        if (la.length != 2) {
            throw new IOException("getLinks() returned wrong length array");
        }

        while (true) {
            LinkMessage lm = la[0].receive();
            la[1].send(lm);
        }
    }
}
