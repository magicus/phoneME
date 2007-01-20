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

/**
 * <code>JUMPContentStore</code> provides a base class for all content stores
 * in the JUMP system. Concrete content stores extend this class and provides
 * an instance of the <code>JUMPStore</code> to be used for this 
 * content store. 
 * <p>
 * This also implements the <Code>JUMPModule</code> to indicate that all
 * the subclasses of this class is a JUMP module.
 */
public abstract class JUMPContentStore implements JUMPModule {

    private static int exclusiveAccess = 0;
    private static int readAccess = 0;
    private static Object  lock = new Object();

    /**
     * Creates a new instance of JUMPContentStore
     */
    protected JUMPContentStore() {
    }
    
    /**
     * Returns a <code>JUMPStore</code> instance to be used by the
     * content store.  Typically this method is called from the <code>
     * JUMPContentStore.openStore(boolean) </code> when creating the StoreHandle. 
     * The subclass of JUMPContentStore can choose to either work with a single 
     * instance of JUMPStore, which this <code>getStore()</code> returns, 
     * or use multiple instances.
     */
    protected abstract JUMPStore getStore();
    
    /**
     * Open the store for read-only or exclusive access. In exclsuive
     * access mutable operations can be performed on the store, whereas
     * in read-only mode, only immutable operations can be performed. This
     * method ensures that there can only be a single component that can
     * be in exclusuive mode. The caller blocks till the store is accessible
     * with the requested access.
     */
    protected JUMPStoreHandle openStore(boolean accessExclusive) {
        if (accessExclusive) {
           synchronized (lock) {
              while (exclusiveAccess > 0 || readAccess > 0) {
                 try {
                    // Wait until closeStore is called... 
                    lock.wait();
                 } catch (InterruptedException e) {}
              }
              exclusiveAccess++;
              return new JUMPStoreHandle(getStore(), accessExclusive);
           }
        } else {  // read-only
           synchronized(lock) {
              while (exclusiveAccess > 0) {
                 try {
                    // Wait until closeStore is called... 
                    lock.wait();
                 } catch (InterruptedException e) {}
              }
              readAccess++;
              return new JUMPStoreHandle(getStore(), accessExclusive);
           }
        }
    }
    
    /**
     * Called to indicate that the content store does not access the store
     * anymore.
     */
    protected void closeStore(JUMPStoreHandle storeHandle) {
        synchronized (lock) {
           if (storeHandle.isExclusive()) {
              exclusiveAccess--;
           } else {
              readAccess--;
           }
           lock.notifyAll(); // Notify all threads waiting at openStore()
        }
    }
}
