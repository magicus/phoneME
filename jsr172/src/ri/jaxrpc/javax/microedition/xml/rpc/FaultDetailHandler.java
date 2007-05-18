/*
 *   
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

package javax.microedition.xml.rpc;

import javax.xml.namespace.QName;

/**
 * The <code>javax.microedition.xml.rpc.FaultDetailHandler</code>
 * interface is implemented by stubs that handle custom faults.
 * 
 * @version 0.1
 */
public interface FaultDetailHandler {

    /**
     * Returns the type description, as an <code>Element</code> for the 
     * given SOAP fault <code>faultDetailName</code>, <code>null</code> if there
     * is no mapping. 
     *
     * @param faultDetailName the <code>QName</code> of the SOAP fault
     *        detail element
     * @return an <code>Element</code> object describing the type to which
     *        <code>faultDetailName</code> is to be mapped, or, <code>null</code>
     *        if there is no mapping.
     */
    public Element handleFault(QName faultDetailName);
}
