/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.j2me.log;

/**
 * Intermediate class for logging facilities
 */
public class Logging {
    
    public static final boolean TRACE_ENABLED  = true;

    public static final int INFORMATION = 0;
    
    public static final int WARNING = 1;
    
    public static final int ERROR = 2;
    
    public static final int CRITICAL    = 3;

    public static final int REPORT_LEVEL = 4;

    /** Don't let anyone instantiate this class */
    private Logging() {
    }

    /**
     * Report a message to the Logging service. 
     *
     * A use example:
     * <pre><code>
     *     if (Logging.REPORT_LEVEL &lt;= severity) {
     *         Logging.report(Logging.&lt;severity&gt;,
     *                        Logging.&lt;channel&gt;, 
     *                        "[meaningful message]");
     *     }
     * </code></pre>
     * @param severity severity level of report
     * @param channelID area report relates to, from LogChannels.java
     * @param message message to go with the report
     */
    public static void report(int severity, int channelID, String message) {
        System.out.println(message);
    }

    /**
     * Obtain a stack trace from the Logging service, and report a message
     * to go along with it.  
     *
     * A use example:
     * <code><pre>
     * } catch (Throwable t) {
     *     if (Logging.TRACE_ENABLED) {
     *         Logging.trace(t, "[meaningful message]");
     *     }
     * }
     * </pre></code>
     *
     * @param t throwable causing this trace call
     * @param message detail message for the trace log
     */
    public static void trace(Throwable t, String message) {
        t.printStackTrace();
    }    
}
