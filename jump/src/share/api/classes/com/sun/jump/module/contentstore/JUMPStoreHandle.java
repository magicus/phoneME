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

import java.io.IOException;

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
     * See {@link JUMPStore#createDataNode(String, JUMPData)}. This method
     * would throw a runtime exception if it is called on a non-exclusive
     * handle.
     *
     *
     * @exception IOException if the underlying <code>JUMPStore.createDataNode(String, JUMPData)</code> causes <code>IOException</code>
     * @exception JUMPStoreRuntimeException if the handle is in non-executive mode.
     */
    public void createDataNode(String uri, JUMPData data) throws IOException {
        if (isExclusive())
           this.store.createDataNode(uri, data);
        else
            throw new JUMPStoreRuntimeException("Access Denied");
    }
    
    /**
     * See {@link JUMPStore#createNode(String)}. This method
     * would throw a runtime exception if it is called on a non-exclusive
     * handle.
     *
     * @exception IOException if the underlying <code>JUMPStore.createNode(String) </code>causes <code>IOException</code>
     * @exception JUMPStoreRuntimeException if the handle is in non-executive mode.
     */
    public void createNode(String uri) throws IOException {
        if (isExclusive())
           this.store.createNode(uri);
        else
            throw new JUMPStoreRuntimeException("Access Denied");
    }
    
    /**
     * See {@link JUMPStore#getNode(String)}.
     *
     * @exception IOException if the underlying <code>JUMPStore.getNode(String)</code> causes <code>IOException</code>
     */
    public JUMPNode getNode(String uri) throws IOException {
        return this.store.getNode(uri);
    }
    
    /**
     * See {@link JUMPStore#deleteNode(String)}. This method
     * would throw a runtime exception if it is called on a non-exclusive
     * handle.
     *
     * @exception IOException if the underlying <code>JUMPStore.deleteNode(String)</code> causes <code>IOException</code>
     * @exception JUMPStoreRuntimeException if the handle is in non-executive mode.
     */
    public void deleteNode(String uri) throws IOException {
        if (isExclusive())
            this.store.deleteNode(uri);
        else 
            throw new JUMPStoreRuntimeException("Access Denied");
    }
    
    /**
     * See {@link JUMPStore#updateDataNode(String, JUMPData)}. This method
     * would throw a runtime exception if it is called on a non-exclusive
     * handle.
     *
     * @exception IOException if the underlying <code>JUMPStore.updateDataNode(String, JUMPData)</code> causes <code>IOException</code>
     * @exception JUMPStoreRuntimeException if the handle is in non-executive mode.
     */
    public void updateDataNode(String uri, JUMPData data) 
       throws IOException {
        if (isExclusive())
           this.store.updateDataNode(uri, data);
        else
            throw new JUMPStoreRuntimeException("Access Denied");
    }
}
