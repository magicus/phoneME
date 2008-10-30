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

package com.sun.midp.log;
import com.sun.midp.main.MIDletSuiteUtils;
import com.sun.midp.configurator.Constants;

/**
 * The purpose of the logging service is to provide a standard means
 * to report runtime information from within Java or native code.
 * The porting process is eased by having to modify one logging
 * service implementation in place of handling the ad hoc use of
 * <code>println()</code>, <code>printf()</code>, <code>putc()</code>,
 * and other functions currently used.
 *
 * An assert mechanism for Java code, implemented using the logging
 * service is also provided here for convenience.
 *
 * This class consists of the Java interface to the functionality
 * of the logging service.
 */
public class Logging extends LoggingBase {

    /**
     * Flag allowing client code with reporting levels less
     * than this to be compiled out of the build.  Callers
     * should use this flag as a way to remove bytecodes
     * related to unneeded levels of reporting from the
     * resulting classfiles.
     * For Example:
     * <pre><code>
     *     if (Logging.REPORT_LEVEL &lt;= severity) {
     *         Logging.report(Logging.&lt;severity&gt;,
     *                        LogChannels.&lt;channel&gt;,
     *                        "[meaningful message]");
     *     }
     * </code></pre>
     */
    public static int REPORT_LEVEL = Constants.REPORT_LEVEL;

    /**
     * Flag allowing client code with assert statements
     * to be compiled out of a production build.  Clients of
     * the assertion service should wrap calls to the
     * <code>assert()</code> method to enable them to be
     * removed from builds when desired
     * <pre><code>
     *      if (Logging.ASSERT_ENABLED) {
     *          Logging.assertTrue([eval to boolean], "message");
     *      }
     * </code></pre>
     */
    public static boolean ASSERT_ENABLED = Constants.ASSERT_ENABLED;

    /**
     * Flag to indicate whether tracing is enabled in the
     * Logging service. If the flag is <code>false</code>,
     * calls to the <code>trace()</code> method will have
     * no effect. Callers should use this flag as a type of
     * compile option to remove unnecessary bytecodes from
     * resulting classfiles.
     *
     * For example:
     * <code><pre>
     * } catch (Throwable t) {
     *     if (Logging.TRACE_ENABLED) {
     *         Logging.trace(t, "[meaningful message]");
     *     }
     * }
     * </pre></code>
     */
    public static boolean TRACE_ENABLED  = Constants.TRACE_ENABLED;

    /**
     * Loads the logging settings for the specified suite.
     *
     * @param suiteId ID of the suite for which the settings must be loaded
     */
    public static void initLogSettings(int suiteId) {
    }

    /**
     * Report a message to the Logging service. The message string should
     * include enough description that someone reading the message will
     * have enough context to diagnose and solve any problems if necessary.
     * The severity level should be one of:
     * <ul>
     *  <li>INFORMATION</li>
     *  <li>WARNING</li>
     *  <li>ERROR</li>
     *  <li>CRITICAL</li>
     * </ul>
     * and should properly reflect the severity of the message. The channel
     * identifier should be one of the pre defined channels listed in
     * the LogChannels.java file.
     * <ul>
     * </ul>
     *
     * A use example:
     * <pre><code>
     *     if (Logging.REPORT_LEVEL &lt;= severity) {
     *         Logging.report(Logging.&lt;severity&gt;,
     *                        LogChannels.&lt;channel&gt;,
     *                        "[meaningful message]");
     *     }
     * </code></pre>
     *
     * No output will occur if <code>message</code> is null.
     *
     * @param severity severity level of report
     * @param channelID area report relates to, from LogChannels.java
     * @param message message to go with the report
       */
   
    public static void report(int severity, int channelID, String message) {

        if(getAllowedSeverity(channelID) <= severity) {
            report0(severity, channelID, message);
        }

    }

    /**
     * Report a message to the Logging service. The message string should
     * include enough description that someone reading the message will
     * have enough context to diagnose and solve any problems if necessary.
     * The severity level should be one of:
     * <ul>
     *  <li>INFORMATION</li>
     *  <li>WARNING</li>
     *  <li>ERROR</li>
     *  <li>CRITICAL</li>
     * </ul>
     * and should properly reflect the severity of the message. The channel
     * identifier should be one of the pre defined channels listed in
     * the LogChannels.java file.
     * <ul>
     * </ul>
     *
     *
     * No output will occur if <code>message</code> is null.
     *
     * @param severity severity level of report
     * @param channelID area report relates to, from LogChannels.java 
     * @param message message to go with the report
     */

    private static native void report0(int severity, 
                                       int channelID,
                                       String message);


    /*
    * Gets the severity per channel using the property mechanism.
    */
    private native static int getAllowedSeverity(int channelID);
        
    /**
     * Report a message to the Logging service in the event that
     * <code>condition</code> is false. The message string should
     * include enough description that someone reading the message will
     * have enough context to find the failed assertion.
     *
     * A use example:
     * <pre><code>
     *     if (Logging.ASSERT_ENABLED){
     *         Logging.assertTrue([(boolean)conditional], "useful message");
     *     }
     * </code></pre>
     *
     * This method reports nothing if <code>message</code> is null.
     *
     * @param condition asserted to be true by the caller.
     *                  <code>message</code> is logged if false.
     * @param message message to go with the report if the assert fails
     *                (when <code>condition</code> is false.
     */

    public static void assertTrue(boolean condition, String message) {
        if(!condition) {
            report(Logging.ERROR, LogChannels.LC_NONE, "ASSERT FAILED: "
                   + message);
        }
    }
}
