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

package com.sun.midp.io.j2me.push;

import java.io.IOException;

import javax.microedition.io.ConnectionNotFoundException;

import com.sun.j2me.security.AccessControlContext;

import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.security.SecurityToken;

/**
 * Stubbed implementation for CDC without JUMP variant of stack
 *
 * IMPL_NOTE: some (if not all) methods might go away as a result of
 *  sources refactoring
 */
public final class PushRegistryInternal {

    /**
     * Push option to only launch this suite when not other applications
     * are running.
     */
    public static final int PUSH_OPT_WHEN_ONLY_APP = 1;

    /**
     * Hides the default constructor.
     */
    private PushRegistryInternal() { }

    /**
     * Register a dynamic connection with the
     * application management software. Once registered,
     * the dynamic connection acts just like a
     * connection preallocated from the descriptor file.
     * The internal implementation includes the storage name
     * that uniquely identifies the <code>MIDlet</code>.
     * This method bypasses the class loader specific checks
     * needed by the <code>Installer</code>.
     * Method requires com.sun.midp.ams permission.
     *
     * @param context Access control context the suite
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
     * @param bypassChecks if true, bypass the permission checks,
     *         used by the installer when redo old connections during an
     *         aborted update
     *
     * @exception  IllegalArgumentException if the connection string is not
     *               valid
     * @exception ConnectionNotFoundException if the runtime system does not
     *              support push delivery for the requested
     *              connection protocol
     * @exception IOException if the connection is already
     *              registered or if there are insufficient resources
     *              to handle the registration request
     * @exception ClassNotFoundException if the <code>MIDlet</code> class
     *               name can not be found in the current
     *               <code>MIDlet</code> suite
     * @exception SecurityException if the <code>MIDlet</code> does not
     *              have permission to register a connection
     *
     * @see #unregisterConnection
     */
    public static void registerConnectionInternal(
            final AccessControlContext context,
            final MIDletSuite midletSuite,
            final String connection,
            final String midlet,
            final String filter,
            final boolean bypassChecks)
                throws ClassNotFoundException, IOException {
        throw new ConnectionNotFoundException("not supported");
    }

    /**
     * Initializes the security token for this class, so it can
     * perform actions that a normal MIDlet Suite cannot.
     *
     * @param token security token for this class.
     */
    public static void initSecurityToken(final SecurityToken token) {
    }

    /**
     * Return a list of registered connections for given
     * <code>MIDlet</code> suite.
     *
     * @param id identifies the specific <code>MIDlet</code>
     *               suite to be launched
     * @param available if <code>true</code>, only return the list of
     *      connections with input available
     *
     * @return string representing an array of connection strings,
     *       where each connection is represented by the generic connection
     *       <em>protocol</em>, <em>host</em> and <em>port number</em>
     *       identification
     */
    public static String listConnections(
            final int id,
            final boolean available) {
        return "";
    }

    /**
     * Unregister all the connections for a <code>MIDlet</code> suite.
     *
     * @param id identifies the specific <code>MIDlet</code>
     *               suite
     */
    public static void unregisterConnections(final int id) {
    }

    /**
     * Sets the flag which enables push launches to take place.
     *
     * @param enable set to <code>true</code> to enable launching
     *  of MIDlets based on alarms and connection notification
     *  events, otherwise set to <code>false</code> to disable
     *  launches
     */
    public static void enablePushLaunch(final boolean enable) {
    }
}
