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

import java.util.Iterator;

/**
 * <code>JUMPNode</code> abstracts the interface to access the data
 * stored in the store. Every node has a name and an URI that serves as the
 * unique identifier for the node that is part of the store.
 * <p>
 * The implementation of <code>JUMPNode</code> is not expected to be 
 * synchronized.  Therefore it is a calling application's responsibility to
 * ensure any synchronization in case of concurrent access.
 * The behaviour of <code>JUMPNode</code> implementation is undefined  
 * if a node is modified while there is another reference to this node. 
 */
public interface JUMPNode {
    /**
     * Returns the name of the node.
     */
    public String getName();
    
    /**
     * Returns the URI of the node.
     */
    public String getURI();
    
    /**
     * Indicates if the node contains data or not. If the node contains 
     * data, then it can be casted to <Code>JUMPDataNode</code> or else
     * it can be casted to <Code>JUMPContainerNode</code>
     * <p>
     * <pre>
     *    JUMPNode node;
     *    if ( node.containsData() ) {
     *         JUMPData data = ((JUMPNode.Data)node).getData();
     *    }
     *    else {
     *         Iterator nodeIterator = 
     *         ((JUMPNode.List)node).getChildren();
     *         // use the iterator to get the children nodes
     *    }
     * </pre>
     */
    public boolean containsData();
    
    /**
     * A node that can contain data.
     */
    interface Data extends JUMPNode {
        /**
         * Returns the data contained within the node.
         */
        public JUMPData getData();
    }
    
    /**
     * A node that can contain the list of other nodes.
     */
    interface List extends JUMPNode {
        /**
         * Returns the list of children contained within this node. The 
         * class of the object contained in the iterator is 
         * <Code>JUMPNode</code>
         */
        public Iterator getChildren();
    }
}
