/*
 * %W% %E%
 *
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

package com.sun.jump.module.contentstore;

/**
 * <code>JUMPStoreHandle</code> is a handle to perform operations on
 * a <code>JUMPStore</code>. Instances of handles are acquired by calling
 * <code>JUMPContentStore.openStore()</code> method.
 */
public class JUMPStoreHandle {
    private boolean exclusive;
    private JUMPStore store;
    
    /**
     * Creates a new instance of JUMPStoreHandle
     */
    JUMPStoreHandle(JUMPStore store, boolean exclusive) {
        this.exclusive = exclusive;
        this.store = store;
    }
    
    public boolean isExclusive() {
        return this.exclusive;
    }
    
    public boolean isReadOnly() {
        return !this.exclusive;
    }
    
    /**
     * Create a node that can contain some data.
     */
    public void createDataNode(String uri, JUMPData data) {
        this.store.createDataNode(uri, data);
    }
    
    /**
     * Create a node that can contain other nodes.
     */
    public void createNode(String uri) {
        this.store.createNode(uri);
    }
    
    /**
     * Returns the node that is bound to the URI or null if no such node
     * exists.
     */
    public JUMPNode getNode(String uri) {
        return this.store.getNode(uri);
    }
    
    /**
     * Delete the node pointed to by the URI. If there are child nodes under
     * this uri, then they are deleted as well.
     */
    public void deleteNode(String uri) {
        this.store.deleteNode(uri);
    }
}
