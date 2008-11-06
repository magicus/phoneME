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
 * <code>JUMPStore</code> provides methods to access a persistant store. The
 * abstraction provided by the store is a hierarchical tree structure
 * (very similar to a file system), where nodes in the tree could contain
 * other nodes (like a directory in a file system) or contain data
 * (like files in a file system). Every node in the store is identified
 * using a unique URI. The root of the store is identified by ".".
 * <p>
 * The following code shows how the store can be used to save an
 * application's title through <code>JUMPStoreHandle</code>.
 * <pre>
 *     JUMPStoreHandle storeHandle;
 *     JUMPData titleData = new JUMPData("Sample App1");
 *     storeHandle.createNode("./apps/App1");  // cause JUMPStore.createNode
 *     storeHandle.createDataNode("./apps/App1/title", titleData);  // cause JUMPStore.createDataNode
 * </pre>
 * <p>
 * All access to the store is controlled by higher level entities like
 * repositories, through a subclass of <code>JUMPContentStore</code>.
 * <p>
 * The JUMP content store API limits the URI String 
 * to use the character set of [a--z] + [0--9] for the node name.  
 * The node string should appear in the unique format; the use of "." and ".." 
 * is prohibited except for "." as the first character representing the root.  
 * The String is treated as case insensitive, "/" as a path element separator, 
 * and the use of a trailing "/" character is not allowed for both the list 
 * and the data node (ex. "./a/b/" or "./"). 
 */
public abstract class JUMPStore implements JUMPModule {
    /**
     * Create a <code>JUMPNode.Data</code> identified by this url in the store.  
     * The node contains this <code>JUMPData</code>. The method should behave in an
     * atomic fashion, i.e either the data node is created completely
     * or nothing at all. All the elements of the <i>URI</i> except for the 
     * part that represents the leaf node must
     * exist within the store for this call to succeed.  This does not 
     * create any intermediate nodes specified in the URI, if they do not
     * exist.  If the node represented by the URI already exists, then the method
     * also fails.
     * 
     * @exception IOException if the node creation fails.
     * @exception IllegalArgumentException if the URI format is invalid.
     * @exception JUMPStoreRuntimeException if the node represented by the URI already exists.
     */

    protected abstract void createDataNode(String uri, JUMPData data)
        throws IOException;
    
    /**
     * Create a <code>JUMPNode.List</code> identified by this URI in the store.
     * If any of the intermediate nodes does not exist yet, then this 
     * method creates those intermediate nodes.
     * 
     * @exception IOException if the node creation fails.
     * @exception IllegalArgumentException if the URI format is invalid.
     * @exception JUMPStoreRuntimeException if the node represented by the URI already exists.
     */
    protected abstract  void createNode(String uri) throws IOException;
    
    /**
     * Returns the node that is bound to the URI or null if no such node
     * exists.
     *
     * @exception IOException if internal error occurs while retrieving the data.
     * @exception IllegalArgumentException if the URI format is invalid.
     */
    protected abstract JUMPNode getNode(String uri) throws IOException;
    
    /**
     * Delete the node pointed to by the URI. If there are child nodes under
     * this URI, then they are deleted as well.
     *
     * @exception IOException if the node deletion fails
     * @exception IllegalArgumentException if the URI format is invalid.
     * @exception JUMPStoreRuntimeException if the URI does not point to an existing node.
     */
    protected abstract void deleteNode(String uri) throws IOException;
    
    /**
     * Update the data associated with the URI. The method can fail in the 
     * following cases 
     * <ul>
     *  <li>URI does not exist</li>
     *  <li>URI does not point to a <Code>JUMPNode.Data</code></li>
     * </ul>
     * If the data cannot be updated, then the previous data in the node 
     * MUST be preserved.
     * 
     * @exception IOException if the node update fails.
     * @exception IllegalArgumentException if the URI format is invalid.
     * @exception JUMPStoreRuntimeException if the URI does not exist or is not pointing to <code>JUMPNode.Data</code>
     */
    protected abstract void updateDataNode(String uri, JUMPData data)
        throws IOException;
}
