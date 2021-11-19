/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package javax.microedition.khronos.opengles;


/**
 * The <code>GL</code> interface is the parent interface for
 * the Java(TM) programming language bindings for OpenGL(R) ES 1.0,
 * 1.1, and extensions.
 *
 * <p> The documentation in this interface and its subinterfaces is
 * normative with respect to instance variable names and values,
 * method names and signatures, and exception behavior.  The remaining
 * documentation is placed here for convenience and does not replace
 * the normative documentation found in the OpenGL ES 1.0 and 1.1
 * specifications, relevant extension specifications, and the OpenGL
 * specification versions referenced by any of the preceding
 * specifications.
 *
 * <p> A GL object is obtained by calling
 * <code>EGLContext.getGL()</code>.  The returned object will
 * implement either <code>GL10</code> or <code>GL11</code>, plus any
 * available extension interfaces (such as <code>GL10Ext</code>,
 * <code>GL11Ext</code>, or <code>GL11ExtensionPack</code>).  The
 * returned object must be cast to the appropriate interface (possibly
 * following an <code>instanceof</code> check) in order to call
 * GL methods.
 *
 * <p> A common superinterface is used for OpenGL ES 1.0, OpenGL ES
 * 1.1, and Khronos-defined core extensions. In order to determine if
 * the implementation supports GL 1.1, call
 * <code>glGetString(GL.GL_VERSION)</code>.
 *
 * <p> Some methods defined in subinterfaces are available only on
 * OpenGL ES 1.1.  The descriptions of these functions are marked
 * "(1.1 only)."  Similarly, some methods behave slightly differently
 * across OpenGL ES versions.  The sections that differ are marked
 * "(1.0 only)" and "(1.1 only)" as appropriate.  Some methods have an
 * additional section marked "1.0 Notes" or "1.1 Notes" that applies
 * to the corresponding engine version.
 *
 * <p> Some extensions are defined as a core part of the OpenGL ES
 * specification (they are extensions relative to desktop OpenGL).
 * These functions are treated as normal portions of OpenGL ES,
 * although they may still be queried as extensions using the normal
 * OpenGL ES query mechanisms.
 *
 * <p> Extensions may allow some arguments to take on values other
 * than those listed in this specification.  Implementations that
 * provide a given extension may pass such values to the underlying
 * engine.
 *
 * <p> Optional profile extensions defined as of the creation of this
 * specification may be found in the <code>GL10Ext</code>,
 * <code>GL11Ext</code>, and <code>GL11ExtensionPack</code>
 * interfaces.
 *
 * <h3>Vertex Buffer Objects</h3>
 *
 * <p>VBOs are considered to be enabled if the most recent call to
 * <code>glBindBuffer</code> had a target of
 * <code>GL_ARRAY_BUFFER</code> and a non-zero <code>buffer</code>
 * parameter.  When VBOs are enabled, only the variant of the
 * <code>gl*Pointer</code> functions that take an integer offset
 * (found in the <code>GL11</code> interface) may be called.  When
 * VBOs are disabled, only the variant of the <code>gl*Pointer</code>
 * functions that take a <code>Buffer</code> may be called.
 *
 * <h3>Clamping</h3>
 * 
 * <p>When method specifies that a value <code>x</code> is clamped to
 * a range <code>[A, B]</code>, it means that the value
 * <code>min(max(x, A), B)</code> is used in place of the original
 * value.
 */
public interface GL {

    // public void dispose();
}

