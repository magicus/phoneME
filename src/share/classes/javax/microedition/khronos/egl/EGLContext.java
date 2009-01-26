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

import javax.microedition.khronos.opengles.*;

/**
 * A class encapsulating the EGL context. The class also contains
 * static methods used to obtain instances of the <code>EGL</code>
 * and <code>GL</code> interfaces.
 *
 * <p> An <code>EGLContext</code> is obtained from EGL using the
 * <code>eglCreateContext</code> function, and destroyed using the
 * <code>eglDestroyContext</code> function.  Its main role is as a
 * parameter to <code>eglMakeCurrent</code>.
 */
public abstract class EGLContext {

    EGLContext() {
    }

    /**
     * Returns an <code>EGL</code> object.
     *
     * @return an <code>EGL</code> instance.
     */
    public static synchronized EGL getEGL() {
        return EGL10Impl.getInstance();
    }

    /**
     * Returns a <code>GL</code> object associated with this EGL
     * context.
     *
     * <p>To call extension methods, cast the returned object to the
     * interface type that contains the methods of the extension.
     *
     * @return a <code>GL</code> instance that implements all
     * available extension interfaces.
     */
    public abstract GL getGL();
}
