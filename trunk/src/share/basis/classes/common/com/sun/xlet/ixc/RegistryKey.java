/*
 * @(#)RegistryKey.java	1.10 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 *
 */

package com.sun.xlet.ixc;

import java.rmi.Remote;

/**
 * Key for an object registry, held in a hashtable.  This is a weak key that
 * knows to clean up the hashtable it's in when it is collected.  It also
 * has an efficiency issue:  it's equivalent to the remote object it
 * refers to for hashing purposes.  That means you can search for the
 * referred-to object in the hashtable, without wrapping it in a
 * RegistryKey instance.
 * <p>
 * It's important to note that two keys are equivalent only if the Remote
 * object to which they refer are <i>identical</i>.  Two remote objects
 * that are != but are .equals() should <i>not</i> be treated as the same
 * object!
 */

final class RegistryKey extends java.lang.ref.WeakReference {
    private int hash;
    RegistryKey(Remote referent) {
        super(referent);
        hash = referent.hashCode();
    }

    public int hashCode() {
        return hash;
    }

    public boolean equals(Object other) {
        if (other == this) {
            return true;
            // This isn't just an optimization.  The operation of
            // ImportRegistryImpl.unregisterStub depends on it.  Once
            // the weak ref gets cleared by the GC, the identity test is
            // the only one that can succeed.
        }
        if (other instanceof Remote) {
            return other == get();
        } else if (other instanceof RegistryKey) {
            Object r = get();
            if (r == null) {
                return false;
            } else {
                return r == (((RegistryKey) other).get());
            }
        } else {
            return false;
        }
    }
}
