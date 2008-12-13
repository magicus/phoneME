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

import javax.microedition.lcdui.Graphics;
import java.util.Hashtable;
import java.lang.ref.WeakReference;

/**
 * A class encapsulating an EGL surface.
 */
final class EGLSurfaceImpl extends EGLSurface {
    
    static final Hashtable byId = new Hashtable(); 
    private int nativeId;
    private int width, height;

    /**
     * A native pointer (cast to int) to the JSR239_Pixmap object
     * backing the surface, or 0 for null.
     */
    private int pixmapPointer = 0;
    
    /**
     * An LCDUI Graphics object referencing the surface.
     */
    private Graphics target;

    public EGLSurfaceImpl(int nativeId, int width, int height) {
        synchronized (byId) {
            this.nativeId = nativeId;
            this.width = width;
            this.height = height;
            byId.put(new Integer(nativeId), new WeakReference(this));
        }
    }

    private native void finalize();
    
    public int nativeId() {
	return nativeId;
    }

    public static EGLSurfaceImpl getInstance(int nativeId,
                                             int width, int height) {
        synchronized (byId) {
            WeakReference ref = (WeakReference)byId.get(new Integer(nativeId));
            EGLSurfaceImpl surface = ref != null ?
                    (EGLSurfaceImpl)ref.get() : null;
            if (surface == null) {
                return new EGLSurfaceImpl(nativeId, width, height);
            } else {
                return surface;
            }
        }
    }

    public static EGLSurfaceImpl getInstance(int nativeId) {
        return getInstance(nativeId, -1, -1);
    }

    public String toString() {
  	return "EGLSurfaceImpl[" + nativeId + "]";
    }

    public void setPixmapPointer(int pixmapPointer) {
	this.pixmapPointer = pixmapPointer;
    }

    public int getPixmapPointer() {
	return this.pixmapPointer;
    }

    public void setTarget(Graphics target) {
	this.target = target;
    }
    
    public Graphics getTarget() {
	return this.target;
    }

    public int getWidth() {
        return this.width;
    }

    public int getHeight() {
        return this.height;
    }

    public void dispose() {
	synchronized (byId) {
	    byId.remove(new Integer(nativeId));
	    this.nativeId = 0;
	}
    }
}
