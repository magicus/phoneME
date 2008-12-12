/*
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
 */

package javax.microedition.khronos.egl;

import java.util.Hashtable;
import java.lang.ref.WeakReference;

/**
 * A class encapsulating an EGL display.  An <code>EGLDisplay</code>
 * instance may be obtained from <code>EGL.eglGetDisplay</code>.
 */
final class EGLDisplayImpl extends EGLDisplay {

    private static final Hashtable byId = new Hashtable(); 
    int nativeId;

    public EGLDisplayImpl(int nativeId) {
        synchronized (byId) {
            this.nativeId = nativeId;
            byId.put(new Integer(nativeId), new WeakReference(this));
        }
    }

    public int nativeId() {
	return nativeId;
    }

    public static EGLDisplayImpl getInstance(int nativeId) {
        synchronized (byId) {
            WeakReference ref = (WeakReference)byId.get(new Integer(nativeId));
            EGLDisplayImpl display = ref != null ?
                    (EGLDisplayImpl)ref.get() : null;
            if (display == null) {
                return new EGLDisplayImpl(nativeId);
            } else {
                return display;
            }
        }
    }

    public String toString() {
  	return "EGLDisplayImpl[" + nativeId + "]";
    }

    public void dispose() {
	synchronized (byId) {
	    byId.remove(new Integer(nativeId));
	    this.nativeId = 0;
	}
    }
}
