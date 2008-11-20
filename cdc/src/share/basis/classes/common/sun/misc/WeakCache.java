/*
 * @(#)WeakCache.java	1.14 06/10/10
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


package sun.misc;

/**
 * This is a hash table that uses a sun.misc.Ref to store its values.
 * With each operation, this hash table will put a little
 * bit of work into checking for ref's that have been nulled out by the
 * GC.  When it detects that this has happened, it removes the corresponding
 * key from the hashtable.  This prevents the hashtable from leaking keys.
 */

public class WeakCache extends java.util.Hashtable {
    private java.util.Vector keyVector;
    private int cleanupIndex = 0;
    //
    // We use a weak ref to store the value.  sun.misc.Ref is abstract,
    // and we need to override reconstitute.  We don't want it to actually
    // do anything, though -- if a value disappears from the cache, it just
    // goes away.
    //
    private static class Value extends Ref {
        public Value(Object t) {
            super();
            setThing(t);
        }

        public Object reconstitute() {
            return null;
        }
    }
    public WeakCache() {
        super();
        keyVector = new java.util.Vector();
    }

    public WeakCache(int initialCapacity) {
        super(initialCapacity);
        keyVector = new java.util.Vector(initialCapacity);
    }

    public synchronized Object get(Object key) {
        Object returnVal = super.get(key);
        doSomeCleanup(1);
        if (returnVal != null) {
            return ((Value) returnVal).get();
        } else {
            return null;
        }
    }

    public synchronized Object put(Object key, Object value) {
        if (!(value instanceof Value)) {
            value = new Value(value);
        }
        Object returnVal = super.put(key, value);
        if (returnVal == null) {
            keyVector.addElement(key);
        }
        doSomeCleanup(2);
        return returnVal;
    }

    public synchronized Object remove(Object key) {
        Object returnVal = super.remove(key);
        keyVector.removeElement(key);
        doSomeCleanup(2);
        return returnVal;
    }

    protected synchronized void doSomeCleanup(int amountOfCleanup) {
        if (keyVector.size() == 0)
            return;
        for (; amountOfCleanup != 0; amountOfCleanup--) {
            // In the case where the cleanupIndex is at the end of the
            // Vector and then an element is removed, the cleanupIndex
            // will be out of bounds.  So, before the cleanupIndex is
            // used, make sure it is in bounds.
            if (cleanupIndex >= keyVector.size()) {
                cleanupIndex = 0;
                break;
            }
            Object key = keyVector.elementAt(cleanupIndex);
            // If the value of this key in the hashtable has been
            // garbage collected, remove the key.
            Object v = super.get(key);
            if (v instanceof sun.misc.Ref 
                && ((sun.misc.Ref) v).check() == null) {
                super.remove(key);
                keyVector.removeElement(key);
                if (keyVector.size() == 0) {
                    cleanupIndex = 0;
                    return;
                }
            } else {
                // Otherwise, move the pointer to the next
                // element in the vector
                cleanupIndex++;
            }
            // Finally, modulate the index to loop back around to
            // the start of the vector once you hit the end.
            cleanupIndex = cleanupIndex % keyVector.size();
        }
    }
}
