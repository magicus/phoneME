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
import javax.xml.rpc.JAXRPCException;
import java.util.Hashtable;

/**
 * The <code>javax.microedition.xml.rpc.Operation</code>
 * class corresponds to a wsdl:operation defined for a
 * target service endpoint.
 * 
 * @version 0.1
 */
public class Operation {

    protected Operation() {};

    /**
     * Standard property for SOAPAction. Indicates the SOAPAction
     * URI if the <code>javax.xml.rpc.soap.http.soapaction.use</code>
     * property is set to <code>true</code>.
     * <p>Type: <code>java.lang.String</code>
     **/
    public static final String SOAPACTION_URI_PROPERTY =
        "javax.xml.rpc.soap.http.soapaction.uri";

    /**
     * Creates an <code>Operation</code> corresponding to the
     * operation that is being performed.
     *
     * @param input the Element describing the input parameter to
     *              this operation.
     * @param output the Element describing the return result to this
     *               operation. NULL indicates there is no return
     *               value.
     * @return a new <code>Operation</code> with the given input
     *         and output Type characteristics
     */
    public static Operation newInstance(QName name,
                                        Element input,
                                        Element output)
    {
        return new com.sun.j2mews.xml.rpc.OperationImpl(name, input, output);
    }

    /**
     * Creates an <code>Operation</code> corresponding to the
     * operation that is being performed. The <code>faultDetailHandler</code>
     * parameter is passed to the runtime and used to map custom SOAP faults.
     *
     * @param input the Element describing the input parameter to
     *              this operation.
     * @param output the Element describing the return result to this
     *               operation. NULL indicates there is no return
     *               value.
     * @param faultDetailHandler the FaultDetailHandler to be called
     *               to handle custom faults thrown by this <code>
                     Operation</code>.
     * @return a new <code>Operation</code> with the given input
     *         and output Type characteristics
     */
    public static Operation newInstance(QName name,
                                        Element input,
                                        Element output,
                                        FaultDetailHandler faultDetailHandler)
    {
        return new com.sun.j2mews.xml.rpc.OperationImpl(name,
                                                        input,
                                                        output,
                                                        faultDetailHandler);
    }

    /**
     * Sets the property <code>name</code> to the value,
     * <code>value</code>.
     *
     * @param name the name of the property to be set
     * @param value the value the property is to be set
     *
     * @throws IllegalArgumentException
     * <UL>
     * <LI>if an error occurs setting the property
     * </UL>
     */
    public void setProperty(String name, String value)
        throws IllegalArgumentException
    {
    }

    /**
     * Invokes the wsdl:operation defined by this
     * <code>Operation</code> and returns the result.
     *
     * @param inParams an <code>Object</code> representing the input
     *                 parameter value(s) to this operation
     *
     * @return a <code>Object</code> representing the output
     *         value(s) for this operation. Can be <code>null</code>
     *         if this operation returns no value.
     *
     * @throws JAXRPCException
     * <UL>
     * <LI>if an error occurs while excuting the operation.
     * </UL>
     * @see javax.microedition.xml.rpc.Operation
     */
    public Object invoke(Object inParams) throws JAXRPCException
    {
        return null;
    }

}


