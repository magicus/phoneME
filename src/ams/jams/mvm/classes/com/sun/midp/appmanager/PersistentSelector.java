/*
 * @(#)PersistentSelector.java	1.4 04/12/01 @(#)
 *
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package com.sun.midp.appmanager;

/**
 * This class is Selector that will not exit after launching the selected
 * MIDlet, so the user can pick another MIDlet after the selected MIDlet ends.
 */
public class PersistentSelector extends Selector {
    /**
     * Create and initialize a new Persistent Selector MIDlet.
     */
    public PersistentSelector() {
        super(false);
    }
}

