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

package com.sun.satsa.jcrmic.utils;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;

/**
 * This class reads the program output and optionally prints it using the
 * notifier object.
 */

public class StreamReader extends Thread {

    /**
     * Input stream.
     */
    private BufferedReader in;

    /**
     * Errors/warnings notifier.
     */
    private Notifier notifier;

    /**
     * Constructor.
     * @param in input stream
     * @param notifier the notifier object
     */
    public StreamReader(InputStream in, Notifier notifier) {
        this.in = new BufferedReader(new InputStreamReader(in));
        this.notifier = notifier;
    }

    /**
     * Constructor.
     * @param in input stream
     */
    public StreamReader(InputStream in) {
        this.in = new BufferedReader(new InputStreamReader(in));
    }

    /**
     * The main loop.
     */
    public void run() {

        try {
            String s;

            while ((s = in.readLine()) != null) {
                if (notifier != null)
                    notifier.output(s);
            }
        }
        catch (Exception e) {}
    }
}
