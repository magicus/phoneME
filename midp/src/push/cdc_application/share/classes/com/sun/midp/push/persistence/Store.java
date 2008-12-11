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

package com.sun.midp.push.persistence;

import java.io.IOException;
import java.util.Map;

import com.sun.midp.push.controller.ConnectionInfo;

/**
 * Persistent store interface.
 *
 * <p>
 * Implementations of this interface have no intellegence of registration
 * semantics, e.g. they don't have to handle connection conflicts.
 * </p>
 */
public interface Store {

    /** Connections consumer. */
    static interface ConnectionsConsumer {
        /**
         * Consumes app suite connections.
         *
         * @param midletSuiteID ID of <code>MIDlet suite</code>
         *     <code>connections</code> are registered for
         * @param connections app suite connections
         */
        void consume(int midletSuiteID, ConnectionInfo [] connections);
    }

    /**
     * Lists all registered connections.
     *
     * <p>
     * This method MUST NOT list <code>MIDlet suites</code>
     * without regsitered connections.
     * </p>
     *
     * @param connectionsConsumer connection consumer
     */
    void listConnections(ConnectionsConsumer connectionsConsumer);

    /**
     * Adds new registered connection.
     *
     * <p><strong>Precondition</strong>: <code>connection</code> MUST not be
     * already registered (the method doesn't check it)</p>
     *
     * @param midletSuiteID ID of <code>MIDlet suite</code> to register
     *   connection for
     *
     * @param connection Connection to register
     *
     * @throws IOException if the content store failed to perform operation
     */
    void addConnection(int midletSuiteID,
            ConnectionInfo connection) throws IOException;

    /**
     * Adds connections for the suite being installed.
     *
     * <p><strong>Preconditin</strong>: <code>connection</code> MUST not be
     * already registered (the method doesn't check it)</p>
     *
     * @param midletSuiteID ID of <code>MIDlet suite</code> to register
     *   connection for
     *
     * @param connections Connections to register
     *
     * @throws IOException if the content store failed to perform operation
     */
    void addConnections(int midletSuiteID,
            ConnectionInfo [] connections) throws IOException;

    /**
     * Removes registered connection.
     *
     * <p><strong>Preconditin</strong>: <code>connection</code> MUST have been
     * already registered (the method doesn't check it)</p>
     *
     * @param midletSuiteID ID of <code>MIDlet suite</code> to remove
     *   connection for
     *
     * @param connection Connection to remove
     *
     * @throws IOException if the content store failed to perform operation
     */
    void removeConnection(int midletSuiteID,
            ConnectionInfo connection) throws IOException;

    /**
     * Removes all registered connections.
     *
     * <p><strong>Preconditin</strong>: <code>connection</code> MUST have been
     * already registered (the method doesn't check it)</p>
     *
     * @param midletSuiteID ID of <code>MIDlet suite</code> to remove
     *   connections for
     *
     * @throws IOException if the content store failed
     */
    void removeConnections(int midletSuiteID) throws IOException;

    /** Alarms consumer. */
    static interface AlarmsConsumer {
        /**
         * Consumes app suite alarms.
         *
         * @param midletSuiteID ID of <code>MIDlet suite</code>
         *     <code>alarms</code> are registered for
         * @param alarms alarms mappings from <code>MIDlet</code> class name
         *  to the scheduled time
         */
        void consume(int midletSuiteID, Map alarms);
    }

    /**
     * Lists all alarms.
     *
     * <p>
     * This method MUST NOT list <code>MIDlet suites</code>
     * without regsitered alarms.
     * </p>
     *
     * @param alarmsConsumer connection consumer
     */
    void listAlarms(AlarmsConsumer alarmsConsumer);

    /**
     * Adds an alarm.
     *
     * @param midletSuiteID <code>MIDlet suite</code> to add alarm for
     * @param midlet <code>MIDlet</code> class name
     * @param time alarm time
     *
     * @throws IOException if the content store failed
     */
    void addAlarm(int midletSuiteID, String midlet, long time)
            throws IOException;

    /**
     * Removes an alarm.
     *
     * @param midletSuiteID <code>MIDlet suite</code> to remove alarm for
     * @param midlet <code>MIDlet</code> class name
     *
     * @throws IOException if the content store failed
     */
    void removeAlarm(int midletSuiteID, String midlet) throws IOException;
}
