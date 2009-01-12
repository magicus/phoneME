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

package com.sun.midp.installer;

import javax.microedition.io.ConnectionNotFoundException;

/**
 * Tunnel class to make Installer.getInstaller()
 * accessible outside its package
 */
public final class AutoGetInstallerTunnel {
    /**
     * Get an instance of an Installer. If the SecureInstaller class is
     * available then it will be returned else a basic Installer will be
     * returned.
     *
     * @param url url from where the new suite should be installed
     *
     * @return an Installer instance
     */
    static public Installer getInstaller(String url)
            throws ConnectionNotFoundException {
        // IMPL_NOTE: File/HttpInstaller() should be returned depending
        // on the url scheme.
        return new HttpInstaller();
    }

    /**
     * Private constructor
     */
    private AutoGetInstallerTunnel() {
    }
}
