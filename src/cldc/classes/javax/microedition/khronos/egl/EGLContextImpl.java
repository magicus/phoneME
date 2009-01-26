/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

import javax.microedition.khronos.opengles.GL;
import java.util.Hashtable;
import java.lang.ref.WeakReference;

import com.sun.jsr239.ContextAccess;
import com.sun.jsr239.GLConfiguration;
import com.sun.jsr239.GL10Impl;
import com.sun.jsr239.GL11Impl;

final class EGLContextImpl extends EGLContext implements ContextAccess {
    
    private static final Hashtable byId = new Hashtable(); 

    private GL gl = null; 
    private int nativeId;

    private Thread boundThread = null;
    private EGLDisplayImpl display = null;
    private EGLSurface drawSurface = null;
    private EGLSurface readSurface = null;
    private boolean destroyed = false;

    public EGLContextImpl(int nativeId) {
        synchronized (byId) {
            this.nativeId = nativeId;
            byId.put(new Integer(nativeId), new WeakReference(this));
        }
    }

    public int nativeId() {
	return nativeId;
    }

    public Thread getBoundThread() {
        return boundThread;
    }

    public void setBoundThread(Thread boundThread) {
        this.boundThread = boundThread;
    }

    public EGLDisplay getDisplay() {
        return display;
    }

    public void setDisplay(EGLDisplayImpl display) {
        this.display = display;
    }

    public EGLSurface getDrawSurface() {
        return drawSurface;
    }

    public void setDrawSurface(EGLSurfaceImpl drawSurface) {
        this.drawSurface = drawSurface;
    }

    public EGLSurface getReadSurface() {
        return readSurface;
    }

    EGLSurfaceImpl getDrawSurfaceImpl() {
        return (EGLSurfaceImpl)getDrawSurface();
    }

    public void setReadSurface(EGLSurfaceImpl readSurface) {
        this.readSurface = readSurface;
    }

    public void setDestroyed(boolean destroyed) {
        this.destroyed = destroyed;
    }

    public boolean isDestroyed() {
        return destroyed;
    }

    public static EGLContextImpl getInstance(int nativeId) {
        synchronized (byId) {
            WeakReference ref = (WeakReference)byId.get(new Integer(nativeId));
            EGLContextImpl context = ref != null ?
                    (EGLContextImpl)ref.get() : null;
            if (context == null) {
                return new EGLContextImpl(nativeId);
            } else {
                return context;
            }
        }
    }

    public GL getGL() {
	synchronized (this) {
	    if (gl == null) {
                if (!GLConfiguration.supportsGL11) {
                    gl = new GL10Impl(this);
                } else {
                    gl = new GL11Impl(this);
                }
	    }
	    return gl;
	}
    }

    /**
     * For debugging purposes, prints the native context ID.
     */
    public String toString() {
  	return "EGLContextImpl[" + nativeId + "]";
    }

    public void dispose() {
	synchronized (byId) {
	    byId.remove(new Integer(nativeId));
	    this.nativeId = 0;
	}
    }
}

