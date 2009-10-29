/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.suite;

/**
 * Provides a LifeCycle notifier to classes that want notifications of MIDlet lifecycle events.
 */
public class LifeCycleNotifierProvider {
    /**
     * Singleton notifier
     */
    private static LifeCycleNotifier lifecycleNotifier;

    /**
     * A private LifeCycleNotifier constructor which enforces the singletion
     * pattern.
     */
    private LifeCycleNotifierProvider() {
	    // empty
    }

    /**
     * Returns an instance of a LifeCycleNotifier, or throws
     * IllegalArgumentException if there is no LifeCycleNotifier set.
     * 
     * @return an instance of a LifeCycleNotifier, or throws
     * IllegalArgumentException if there is no LifeCycleNotifier set.
     */
    public static LifeCycleNotifier getInstance() {
        if (lifecycleNotifier == null) {
            throw new IllegalArgumentException("Notifier not set");
        }

        return lifecycleNotifier;
    }

    /**
     * Initializes the LifeCycleNotifierProvider with a LifeCycleNotifier
     * 
     * @param notifier a LifeCycleNotifier.
     */
    public static void init(LifeCycleNotifier notifier) {
        if (notifier != null) {
            lifecycleNotifier = notifier;
        }
    }
}

