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

import com.sun.jump.module.JUMPModule;

import java.io.IOException;

/**
 * <code>JUMPStore</code> provides an interface to a persistant store. The
 * abstraction provided by the store is a hierarchical tree structure
 * (very similar to a file system), where nodes in the tree could contain
 * other nodes (like a directory in a file system) or contain data
 * (like files in a file system). Every node in the store is identified
 * using a unique URI. The root of the store is identified by ".".
 * <p>
 * The following code shows how the store can be used to save an
 * application's title.
 * <pre>
 *     JUMPStore store;
 *     JUMPData titleData = new JUMPData("Sample App1");
 *     store.createNode("./apps/App1");
 *     store.createDataNode("./apps/App1/title", titleData);
 * </pre>
 * <p>
 * All access to the store is controlled by higher level entities like
 * repositories.
 */
public interface JUMPStore extends JUMPModule {
    /**
     * Create a <code>JUMPNode.Data</code> identified by this url in the store.  
     * The node contains this JUMPData.
     * 
     * @throws <code>IOException</code> if the node creation fails.
     */
    public void createDataNode(String uri, JUMPData data);
    
    /**
     * Create a <code>JUMPNode.List</code> identified by this uri in the store.
     * 
     * 
     * @throws <code>IOException</code> if the node creation fails.
     */
    public void createNode(String uri);
    
    /**
     * Returns the node that is bound to the URI or null if no such node
     * exists.
     */
    public JUMPNode getNode(String uri);
    
    /**
     * Delete the node pointed to by the URI. If there are child nodes under
     * this uri, then they are deleted as well.
     *
     * @return true if the node deletion succeeds; false otherwise.
     */
    public boolean deleteNode(String uri);
}
