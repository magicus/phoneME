/*
 * 
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

package javax.microedition.media.control;


/**
 * <code>GUIControl</code> extends <code>Control</code> and is defined
 * for controls that provide GUI functionalities.
 * <p>
 * <code>Control</code>s that support a GUI component
 * should implement this interface.
 */
public interface GUIControl extends javax.microedition.media.Control {

/* JAVADOC ELIDED */
    int USE_GUI_PRIMITIVE = 0;

    /**
     * Initialize the mode on how the GUI is displayed.
     *
     * @param mode The mode that determines how the GUI is
     * displayed.  <code>GUIControl</code> defines only
     * one mode:
     * <a href="#USE_GUI_PRIMITIVE"><code>USE_GUI_PRIMITIVE</code></a>.
     * Subclasses of this may introduce more modes.
     *
     * @param arg The exact semantics of this argument is
     * specified in the respective mode definitions.
     *
     * @return The exact semantics and type of the object returned
     * are specified in the respective mode definitions.
     * 
     * @exception IllegalStateException Thrown if 
     * <code>initDisplayMode</code> is called again after it has 
     * previously been called successfully.
     *
     * @exception IllegalArgumentException Thrown if
     * the <code>mode</code> or <code>arg</code> 
     * argument is invalid.   <code>mode</code> must be
     * defined by GUIControl or its subclasses; or a custom mode
     * supported by this implementation.
     * <code>arg</code> must conform to the 
     * constraints defined by the 
     * respective mode definitions.
     * Refer to the mode definitions for the required type
     * of <code>arg</code>.
     */
    Object initDisplayMode(int mode, Object arg);
}
