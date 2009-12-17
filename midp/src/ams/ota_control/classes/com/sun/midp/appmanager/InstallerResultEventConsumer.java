/*
 * Copyright 2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.appmanager;

/**
 * Installation result event cosumer.
 */
public interface InstallerResultEventConsumer {
    /** Installation result not set yet */
    public static final int INSTALLER_RESULT_UNKNOWN = -1;
    /** Installation result: suite successfully installed. */
    public static final int INSTALLER_RESULT_SUCCESS = 0;
    /** Installation result: suite installation failed. */
    public static final int INSTALLER_RESULT_FAILURE = 1;
    /** Installation result: suite installation canceled by user. */
    public static final int INSTALLER_RESULT_CANCELED = 2;

    /**
     * Processes MIDP_ODD_INSTALATION_DONE_EVENT.
     *
     * @param result One of INSTALLER_RESULT_* values.
     * @param suiteId Id of installed suite, UNUSED_SUITE_ID in case of
     *                unsuccessful installation
     * @param exception Name of the thrown exception if any
     * @param message Message of the thrown exception if any
     * @param reason Reason of the failure if applicable on the thrown exception
     * @param extraData Extra data if applicable on the thrown exception
     */
    public void handleODDInstallationDoneEvent(int result, int suiteId,
            String exception, String message, int reason, String extraData);
}
