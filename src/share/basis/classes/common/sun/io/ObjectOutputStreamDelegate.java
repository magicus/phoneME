/*
 * @(#)ObjectOutputStreamDelegate.java	1.10 06/10/10
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

package sun.io;

/**
 * ObjectOutputStreamDelegate is an interface meant to be implemented
 * only by subclasses of ObjectOutputStream for the purpose of overriding
 * the final methods "writeObject()", "defaultWriteObject()", and
 * "enableReplaceObject()". Trusted subclasses of ObjectOutputStream
 * that implement this interface will have their delegate methods
 * "writeObjectDelegate()", "defaultWriteObjectDelegate()", and
 * "enableReplaceObjectDelegate()" called in place of the
 * corresponding final methods in ObjectOutputStream.
 *
 * @author  Ken Urquhart
 * @version 1.0, 02/02/98
 * @see java.io.ObjectOutputStream
 * @see java.io.ObjectInputStream
 * @since   JDK1.1.6
 */
public interface ObjectOutputStreamDelegate {
    /**
     * Override the actions of the final method "writeObject()"
     * in ObjectOutputStream.
     * @since     JDK1.1.6
     */
    void writeObjectDelegate(Object obj);
    /**
     * Override the actions of the final method "defaultWriteObject()"
     * in ObjectOutputStream.
     * @since     JDK1.1.6
     */
    void defaultWriteObjectDelegate();
    /**
     * Override the actions of the final method "enableReplaceObject()"
     * in ObjectOutputStream.
     * @since     JDK1.1.6
     */
    boolean enableReplaceObjectDelegate(boolean enable);     
}
