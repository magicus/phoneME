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

package javax.microedition.xml.rpc;

import javax.xml.namespace.QName;

/**
 * The <code>FaultDetailException</code> class
 * is used to return service specific exception detail values, and 
 * an associated <code>QName</code>, to a <code>Stub</code> instance.
 *
 * <p>This exception class is returned from the runtime implementation
 * as the <code>cause</code> of a <code>JAXRPCException</code> and 
 * retrieved via the <code>JAXRPCException.getLinkedCause</code> method.
 * 
 * @see javax.xml.rpc.JAXRPCException
 * @see javax.microedition.xml.rpc.FaultDetailHandler
 * @version 1.0
 */
public class FaultDetailException extends java.lang.Exception {

    private Object faultDetail;
    private QName faultDetailName;

    /**
     * Constructs a new exception with the specified fault detail
     * and associated fault detail <code>QName</code>.
     *
     * @param faultDetail Object array containing the values for 
     *        SOAP fault detail. The values are retrieved using
     *        the getFaultDetail method
     * @param faultDetailName the <code>QName</code> of the SOAP fault
     *        detail element
     *
     * @see javax.xml.namespace.QName
     */
    public FaultDetailException(QName faultDetailName, 
				Object faultDetail) {
	this.faultDetail = faultDetail;
	this.faultDetailName = faultDetailName;
    }

    /**
     * Returns the fault detail values
     *
     * @return the fault detail values for the service specific exception
     */
    public Object getFaultDetail() {
	return this.faultDetail;
    }

    /**
     * Returns the QName of the fault detail element associated
     * with this exception.
     *
     * @return the <code>QName</code> of the fault detail element
     * @see javax.xml.namespace.QName
     */
    public QName getFaultDetailName() {
	return this.faultDetailName;
    }

}

