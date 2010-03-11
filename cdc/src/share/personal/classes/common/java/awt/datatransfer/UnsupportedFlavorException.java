/*
 * @(#)UnsupportedFlavorException.java	1.14 06/10/10
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

package java.awt.datatransfer;

/**
 * Signals that the requested data is not supported in this flavor.
 * @see Transferable#getTransferData
 *
 * @version 	1.12, 02/09/01
 * @author	Amy Fowler
 */
public class UnsupportedFlavorException extends Exception {
    /*
     * JDK 1.1 serialVersionUID 
     */

    private static final long serialVersionUID = 5383814944251665601L;
    /**
     * Constructs an UnsupportedFlavorException.
     * @param flavor the flavor object which caused the exception
     * @throws NullPointerException if flavor is <code>null</code>
     */  
    public UnsupportedFlavorException(DataFlavor flavor) {
        // JCK Test UnsupportedFlavorException0002: if 'flavor' is null, throw
        // NPE
        //super(flavor.getHumanPresentableName());
        super((flavor != null) ? flavor.getHumanPresentableName() : null);
    }
}
