/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.appmanager.presentation;

/**
 * <p>This is a rudimentary interface to link a presentation layer
 * with the startup code. An implementation of {@link
 * com.sun.appmanager.AppManager} gets started, and in turn invokes
 * methods in Presentation to get the presentation layer kick-started.  */
public interface Presentation {

    /**
     * Initialize presentation mode in preparation for display to the user.
     */
    void initialize();

    /**
     * Load any application descriptors that might be displayed to the user
     */
    void loadApps();

    /**
     * Run any applications that the presentation layer feels is appropriate
     * to always display to the user.
     */
    void runStartupApps();

    /**
     * Show the presentation mode and start interacting with the user.
     */
    void startAppManager();
}
