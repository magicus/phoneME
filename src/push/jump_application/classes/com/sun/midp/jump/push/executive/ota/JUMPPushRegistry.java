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

package com.sun.midp.jump.push.executive.ota;

import com.sun.midp.jump.push.executive.JUMPConnectionInfo;

/**
 * PushRegsitry system interface for OTA system.
 *
 * <p>
 * There would be a way in OTA system to obtain a singleton
 * implementing this interface.  This singleton would
 * hide behind the scene all IPC (as OTA is supposed to run
 * in a separate process).
 * </p>
 */
public interface JUMPPushRegistry {
    /**
     * Install PushRegistry connections.
     *
     * <p>
     * Install connections for a <code>MIDlet suite</code> being installed on
     * the system.
     * </p>
     *
     * <p>
     * If connection installation succeeds, these connections are disabled (i.e.
     * events on these connections won't lead to <code>MIDlet suite</code>
     * activation.)  To enable connections one should use
     * <code>enableConnections</code> method.  If for any reason installation of
     * the suite fails, one MUST call <code>uninstallConnections</code> to
     * uninstall connections before invoking <code>enableConnections</code>.
     *
     * <p>
     * <strong>Precondition</strong>: <code>midletSuiteId</code> MUST
     * refer to a suite without any registered Push connections (this
     * precondition isn't checked and violation leads to undefined
     * behaviour.)
     * </p>
     *
     * <p>
     * <strong>Precondition</strong>: <code>connections</code> array MUST be
     * non-empty and not have duplicate connections (i.e. it should be non-empty
     * set).
     * </p>
     *
     * @param midletSuiteId ID of <code>MIDlet suite</code> to install
     *  connections for
     * @param connections Connections to install
     *
     * @return <code>true</code> if connection can be installed (i.e. there is
     *  no conflicts), <code>false</code> otherwise
     */
    boolean installConnections(int midletSuiteId,
            JUMPConnectionInfo[] connections);

    /**
     * Enable PushRegistry connections.
     *
     * <p>
     * Enable previously installed PushRegistry connections.  That means that
     * from now on <code>MIDlet suite</code> can be activated by PushRegistry
     * system.
     * </p>
     *
     * <p>
     * If for any reason connections cannot be enabled, <code>false</code> is
     * returned and connections are automatically <em>uninstalled</em>.
     *
     * <p>
     * <strong>Precondition</strong>: connections for this suite should have
     * been already installed with <code>installConnections</code> method.
     * </p>
     *
     * @param midletSuiteId ID of <code>MIDlet suite</code> to enable
     *  connections for
     *
     * @return <code>true</code> if connections can be enabled,
     *  <code>false</code> otherwise.
     */
    boolean enableConnections(int midletSuiteId);

    /**
     * Uninstall PushRegistry connections.
     *
     * <p>
     * Uninstall previously installed PushRegistry connections.
     * </p>
     *
     * <p>
     * <strong>Precondition</strong>: connections for this suite should have
     * been already installed with <code>installConnections</code> method.
     * </p>
     *
     * @param midletSuiteId ID of <code>MIDlet suite</code> to uninstall
     *  connections of
     *
     * @return <code>true</code> if connections has been uninstalled,
     *  <code>false</code> otherwise
     */
    boolean uninstallConnections(int midletSuiteId);
}
