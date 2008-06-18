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

package com.sun.midp.wma;
import com.sun.j2me.security.WMAPermission;
import com.sun.midp.io.j2me.push.ProtocolPush;
import java.io.IOException;
import java.io.InterruptedIOException;
import javax.microedition.io.ConnectionNotFoundException;
import com.sun.midp.midlet.MIDletSuite;

/**
 * Implementation of push behaviour.
 */
public class ProtocolPushImpl extends ProtocolPush {

    /** Instance */
    private static ProtocolPushImpl pushInstance;

    /**
     * Get instance of this class.
     * @return class instance
     */
    protected ProtocolPush getInstance() {
        if (pushInstance == null) {
            pushInstance = new ProtocolPushImpl();
        }
        return (ProtocolPush)pushInstance;
    }

    /**
     * Called when registration is checked.
     * @param connection generic connection <em>protocol</em>, <em>host</em>
     *               and <em>port number</em>
     *               (optional parameters may be included
     *               separated with semi-colons (;))
     * @param midlet  class name of the <code>MIDlet</code> to be launched,
     *               when new external data is available
     * @param filter a connection URL string indicating which senders
     *               are allowed to cause the MIDlet to be launched
     * @exception  IllegalArgumentException if the connection string is not
     *               valid
     * @exception ClassNotFoundException if the <code>MIDlet</code> class
     *               name can not be found in the current
     *               <code>MIDlet</code> suite
     */
    public void checkRegistration(String connection, String midlet,
                                  String filter) {

        // for cbs: protocol, the filter is ignored
        if (connection.startsWith("cbs://")) {
            return;
        }

        if (filter == null || filter.length() == 0) {
            throw new IllegalArgumentException("NULL of empty filter");
        }

        /*
         * for sms: the filter is compared against MSISDN field of the push message
         *
         * msisdn ::== "+" digits | digits
         * digit ::== "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
         * digits ::== digit | digit digits
         *
         * '*' and '?' are allowed according to the MIDP spec.
         */
        for (int i = (filter.charAt(0) == '+') ? 1 : 0;
                i < filter.length(); i++) {
            char ch = filter.charAt(i);
            if ((ch >= '0'  && ch <= '9') || ch == '*' || ch == '?') {
                continue;
            }
            throw new IllegalArgumentException("Invalid filter: " + filter);
        }
    }

    /**
     * Called when registration is established.
     * @param midletSuite MIDlet suite for the suite registering,
     *                   the suite only has to implement isRegistered,
     *                   checkForPermission, and getID.
     * @param connection generic connection <em>protocol</em>, <em>host</em>
     *               and <em>port number</em>
     *               (optional parameters may be included
     *               separated with semi-colons (;))
     * @param midlet  class name of the <code>MIDlet</code> to be launched,
     *               when new external data is available
     * @param filter a connection URL string indicating which senders
     *               are allowed to cause the MIDlet to be launched
     * @exception  IllegalArgumentException if the connection string is not
     *               valid
     * @exception IOException if the connection is already
     *              registered or if there are insufficient resources
     *              to handle the registration request
     * @exception ClassNotFoundException if the <code>MIDlet</code> class
     *               name can not be found in the current
     *               <code>MIDlet</code> suite
     */
    public void registerConnection(MIDletSuite midletSuite, String connection, 
        String midlet, String filter) 
        throws IllegalArgumentException, IOException, ClassNotFoundException {

        checkIsNotHost(connection, false);

        if (connection.startsWith("sms://")) {
            try {
                Class.forName("com.sun.midp.io.j2me.sms.Protocol");
            } catch (ClassNotFoundException e) {
                throw new ConnectionNotFoundException(
                    "Connection not supported");
            }

            /*
             * Check the suite permission for the connection
             * and close the connection immediately.
             */
            try {
                midletSuite.checkForPermission(WMAPermission.SMS_SERVER.getName(),
                   connection);
            } catch (InterruptedException ie) {
                throw new InterruptedIOException(
                    "Interrupted while trying to ask the user permission");
            }
        } else if (connection.startsWith("cbs://")) {
            try {
                Class.forName("com.sun.midp.io.j2me.cbs.Protocol");
            } catch (ClassNotFoundException e) {
                throw new ConnectionNotFoundException(
                    "Connection not supported");
            }

            try {
                midletSuite.checkForPermission(WMAPermission.CBS_SERVER.getName(),
                   connection);
            } catch (InterruptedException ie) {
                throw new InterruptedIOException(
                    "Interrupted while trying to ask the user permission");
            }
        }
    }
}
