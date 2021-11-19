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

package com.sun.j2mews.xml.rpc;

import javax.microedition.io.Connector;
import javax.microedition.io.HttpConnection;
import javax.microedition.xml.rpc.*;
import javax.xml.namespace.QName;
import javax.xml.rpc.*;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.rmi.MarshalException;
import java.rmi.ServerException;

import com.sun.j2me.io.Base64;

/**
 * The <code>OperationImpl</code> class is an implementation
 * of the <code>javax.microedition.xml.rpc.Operation</code>
 * class, corresponding to a wsdl:operation defined for a
 * target service endpoint.
 *
 * @version 0.1
 */
public class OperationImpl extends Operation {

    /**
     * HTTP Sessions are implemented through the use of cookies.
     * Since a "session" would involve likely more than one Operation,
     * the set of cookies for all known sessions is made static across
     * all Operations. The cookies array is implemented like the properties
     * array - as a set of tuples. The first value being the endpoint
     * address which started the session, the second value being the
     * cookie for the session (i.e., cookie 0 is represented by the
     * endpoint at index 0, and the cookie string at index 1).
     */
    private static String[] cookies;

    /**
     * The index of the next available entry in the cookie set.
     * Since cookies are tuples, the algorithm (cookieIndex / 2)
     * will yield the number of properties stored in the set.
     */
    private static int cookieIndex;

    /**
     * The set of properties set by the <code>setProperty</code>
     * method. Each property is a tuple, represented by two sequential
     * entries in the array (i.e., property 0 is represented by the
     * key at index 0, and the value at index 1)
     */
    private String[] properties;

    /**
     * The index of the next available entry in the property set.
     * Since properties are tuples, the algorithm (propertyIndex / 2)
     * will yield the number of properties stored in the set.
     */
    private int propertyIndex;

    /**
     * The Encoder to use when encoding communications to
     * Web Services
     */
    private SOAPEncoder encoder;

    /**
     * The Decoder to use when decoding communications from
     * Web Services
     */
    private SOAPDecoder decoder;

    /**
     * The QName of this operation
     */
    private QName name;

    /**
     * The Type of the input parameter to this Operation
     */
    private Element inputType;

    /**
     * The Type of the output parameter to this Operation
     */
    private Element returnType;

    /**
     * The handler to provide the decoding information for
     * custom fault details
     */
    private FaultDetailHandler faultHandler;

    /**
     * A flag indicating the the object was moved (i.e. 3xx HTTP response
     * was received) and we have to retry the operation using a new location
     */
    private boolean resourceMoved = false;

    /**
     * Default constructor matches that of Operation
     *
     * @param name the QName of this Operation
     * @param input the input Type for this Operation
     * @param output the output Type for this Operation
     */
    public OperationImpl(QName name, Element input, Element output)
        throws IllegalArgumentException
    {
        this.name = name;
        this.inputType = input;
        this.returnType = output;

        this.encoder = new SOAPEncoder();
        this.decoder = new SOAPDecoder();
    }

    /**
     * Default constructor matches that of Operation
     *
     * @param name the QName of this Operation
     * @param input the input Type for this Operation
     * @param output the output Type for this Operation
     * @param faultDetailHandler the handler which will return the type
     *                           descriptor for a custom fault detail
     *                           this operation may encounter
     */
    public OperationImpl(QName name, Element input, Element output,
                         FaultDetailHandler faultDetailHandler)
        throws IllegalArgumentException
    {
        this.name = name;
        this.inputType = input;
        this.returnType = output;
        this.faultHandler = faultDetailHandler;

        this.encoder = new SOAPEncoder();
        this.decoder = new SOAPDecoder();
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
        // disallow any null key or value data
        if (name == null || value == null) {
            throw new IllegalArgumentException();
        } else if (
            !name.equals(Stub.ENDPOINT_ADDRESS_PROPERTY) &&
            !name.equals(Stub.PASSWORD_PROPERTY) &&
            !name.equals(Stub.USERNAME_PROPERTY) &&
            !name.equals(Stub.SESSION_MAINTAIN_PROPERTY) &&
            !name.equals(Operation.SOAPACTION_URI_PROPERTY))
        {
            throw new IllegalArgumentException();
        }

        // Before appending, check to see if we are modifying
        // a previous key/value pair
        if (properties != null) {
            for (int i = 0; i < propertyIndex; i += 2) {
                if (properties[i].equals(name)) {
                    properties[i + 1] = value;
                    return;
                }
            }
        }

        // If properties not instantiated yet, start out with '
        // room to hold 5 properties (5 * 2 entries = 10)
        if (properties == null) {
            properties = new String[10];

        // If properties is full, re-size with room for another
        // 5 properties
        } else if (propertyIndex == properties.length) {
            String[] newProps = new String[properties.length + 10];
            System.arraycopy(properties, 0, newProps, 0, properties.length);
            properties = null;
            properties = newProps;
        }

        properties[propertyIndex++] = name;
        properties[propertyIndex++] = value;

        // propertyIndex is left pointing at the next available
        // entry in the property list
    }

    /**
     * Invokes the wsdl:operation defined by this
     * <code>Operation</code> and returns the result.
     *
     * @param params a <code>ValueType</code> array representing the
     *               input parameters for this <code>Operation</code>.
     *               Can be <code>null</code> if this operation takes
     *               no parameters.
     * @return a <code>ValueType</code> array representing the output
     *         value(s) for this operation. Can be <code>null</code>
     *         if this operation returns no value.
     *
     * @throws JAXRPCException
     * <UL>
     * <LI>if an error occurs while executing the operation.
     * </UL>
     * @see javax.microedition.xml.rpc.Operation
     */
    public Object invoke(Object params)
        throws JAXRPCException
    {
        HttpConnection http = null;
        OutputStream ostream = null;
        InputStream istream = null;
        Object result = null;
        int attempts = 0;
        // Maximal number of "Object moved" http responses that we will handle
        final int maxAttempts = Constants.MAX_REDIRECT_ATTEMPTS;

        try {
            do {
                // This flag will be set to 'true' by setupResStream() method
                // if code 3xx is returned by the http connection.
                resourceMoved = false;

                // open stream to service endpoint
                http = (HttpConnection)Connector.open(
                			getProperty(Stub.ENDPOINT_ADDRESS_PROPERTY));

                ostream = setupReqStream(http);
                // IMPL NOTE: encoding should be either UTF-8 or UTF-16
                encoder.encode(params, inputType, ostream, null);
                if (ostream != null) {
                    ostream.close(); ostream = null;
                }

                istream = setupResStream(http);
                if (returnType != null && istream != null) {
                    result = decoder.decode(returnType,
                                            istream,
                                            http.getEncoding(),
                                            http.getLength());
                }

                if (istream != null) {
                    istream.close(); istream = null;
                }
                if (http != null) {
                    http.close(); http = null;
                }

            } while (resourceMoved && (attempts++ < maxAttempts));

            if (resourceMoved) {
                throw new JAXRPCException("Too many redirections");
            }

            return result;

        } catch (Exception t) {
            // Debug Line

            if (ostream != null) {
                try {
                    ostream.close();
                } catch (Exception t1) {}
                ostream = null;
            }
            if (istream != null) {
                try {
                    istream.close();
                } catch (Exception t1) {}
                istream = null;
            }
            if (http != null) {
                try {
                    http.close();
                } catch (Exception t1) {}
                http = null;
            }
            // Re-throw whatever error/exception occurs as a new
            // JAXRPCException
            if (t instanceof RuntimeException)
                throw (RuntimeException)t;
            if (t instanceof MarshalException ||
                    t instanceof ServerException ||
                        t instanceof FaultDetailException) {
                    throw new JAXRPCException(t);
            }
            throw new JAXRPCException(t.toString());
        }
    }

    /**
     * Method to configure the HTTP request stream.
     * This method will do whatever mechanics are necessary to
     * utilize the HTTP connection object to return an output
     * stream to be utilized to send the HTTP request.
     *
     * @param http the HttpConnection object, unopened, with no
     *        headers or any configuration yet set
     * @return an OutputStream which can be used to write the
     *         request data to this HTTP request.
     */
    protected OutputStream setupReqStream(HttpConnection http)
        throws IOException
    {
        http.setRequestMethod(HttpConnection.POST);
        http.setRequestProperty("User-Agent",
            "Profile/MIDP-1.0 Configuration/CLDC-1.0");
        http.setRequestProperty("Content-Language", "en-US");
        http.setRequestProperty("Content-Type", "text/xml");

        String soapAction = getProperty(Operation.SOAPACTION_URI_PROPERTY);
        if (soapAction == null || "".equals(soapAction)) {
            soapAction = "\"\"";
        } else {
            if (!soapAction.startsWith("\"")) {
                soapAction = "\"" + soapAction;
            }
            if (!soapAction.endsWith("\"")) {
                soapAction = soapAction + "\"";
            }
        }

        http.setRequestProperty("SOAPAction", soapAction);

        String useSession = getProperty(Stub.SESSION_MAINTAIN_PROPERTY);
        if (useSession != null && useSession.toLowerCase().equals("true")) {
            String cookie = getSessionCookie(
                getProperty(Stub.ENDPOINT_ADDRESS_PROPERTY));
            if (cookie != null) {
                http.setRequestProperty("Cookie", cookie);
            }
        }

        String s1 = getProperty(Stub.USERNAME_PROPERTY);
        String s2 = getProperty(Stub.PASSWORD_PROPERTY);
        if (s1 != null && s2 != null) {
            byte[] encodeData = (s1 + ":" + s2).getBytes();
            http.setRequestProperty("Authorization", "Basic " +
                Base64.encode(encodeData, 0, encodeData.length));
        }
        return http.openOutputStream();
    }

    /**
     * Method to configure the HTTP response stream.
     * This method will do whatever mechanics are necessary to
     * utilize the HTTP connection object to return an input
     * stream to the HTTP response.
     *
     * @param http the HttpConnection object, already opened and
     *        presumably with response data waiting
     * @return an InputStream corresponding to the response data
     *         from this HttpConnection or null if not available.
     */
    protected InputStream setupResStream(HttpConnection http)
        throws IOException
    {
        int response = http.getResponseCode();

        if (response == HttpConnection.HTTP_MOVED_PERM ||
                response == HttpConnection.HTTP_MOVED_TEMP) {
            // Resource was moved, get a new location and retry the operation
            String newLocation = http.getHeaderField("Location");
            setProperty(Stub.ENDPOINT_ADDRESS_PROPERTY, newLocation);
            resourceMoved = true;
            return null;
        }

        InputStream input = http.openInputStream();
        try {
            if (response == HttpConnection.HTTP_OK) {
    
                // Catch any session cookie if one was set
                String useSession = getProperty(Stub.SESSION_MAINTAIN_PROPERTY);
                if (useSession != null && useSession.toLowerCase().equals("true"))
                {
                    String cookie = http.getHeaderField("Set-Cookie");
                    if (cookie != null) {
                        addSessionCookie(
                            getProperty(Stub.ENDPOINT_ADDRESS_PROPERTY), cookie);
                    }
                }
    
                return input;
            }
            Object detail = decoder.decodeFault(faultHandler,
                                                input,
                                                http.getEncoding(),
                                                http.getLength());
            if (detail instanceof String) {
                if (((String)detail).indexOf("DataEncodingUnknown") != -1)
                    throw new MarshalException((String)detail);
                throw new ServerException((String)detail);
            }
            Object[] wrapper = (Object[])detail;
            String message = (String)wrapper[0];
            QName name = (QName)wrapper[1];
            detail = wrapper[2];
            throw new JAXRPCException(message,
                            new FaultDetailException(name, detail));
        } catch( IOException t ){
            input.close();
            throw t;
        } catch( RuntimeException t ){
            input.close();
            throw t;
        }
    }

    /**
     * Internal utility method to retrieve a property value
     * from the property set
     *
     * @param key the property identifier
     * @return the string value of the property
     */
    private String getProperty(String key) {
        if (properties != null) {
            for (int i = 0; i < (properties.length - 2); i += 2) {
                if (properties[i] == null) {
                    return null;
                }
                if (properties[i].equals(key)) {
                    return properties[i + 1];
                }
            }
        }
        return null;
    }

    /**
     * Adds a cookie to identify this session.
     * Please refer to the section 13.2 of the JAX-RPC 1.1 spec.
     *
     * @param endpoint an address to associate with the given cookie
     * @param cookie a cookie identifying the session
     */
    private static synchronized void addSessionCookie(String endpoint,
                                                      String cookie)
    {
        if (endpoint == null || cookie == null) {
            return;
        }

        // We strip off everything after the name=value info
        int i = cookie.indexOf(";");
        if (i > 0) {
            cookie = cookie.substring(0, i);
        }

        // Before appending, check to see if we are modifying
        // a previous key/value pair
        if (cookies != null) {
            for (i = 0; i < cookieIndex; i += 2) {
                if (cookies[i].equals(endpoint)) {
                    cookies[i + 1] = cookie;
                    return;
                }
            }
        }

        // If cookies not instantiated yet, start out with '
        // room to hold 5 cookies (5 * 2 entries = 10)
        if (cookies == null) {
            cookies = new String[10];

        // If cookies is full, re-size with room for another
        // 5 cookies
        } else if (cookieIndex == cookies.length) {
            String[] newCookies = new String[cookies.length + 10];
            System.arraycopy(cookies, 0, newCookies, 0, cookies.length);
            cookies = null;
            cookies = newCookies;
        }

        cookies[cookieIndex++] = endpoint;
        cookies[cookieIndex++] = cookie;

        // cookieIndex is left pointing at the next available
        // entry in the cookie list
    }

    /**
     * Look through the set of session cookies, and return the
     * one that matches the current endpoint address of this
     * Operation, if any.
     *
     * @param endpoint address of this Operation
     * @return the session cookie for this Operation's endpoint address,
     *         or null if there is no session cookie
     */
    private static synchronized String getSessionCookie(String endpoint) {
        if (cookies != null) {
            for (int i = 0; i < (cookies.length - 2); i += 2) {
                if (cookies[i] == null) {
                    return null;
                }
                if (cookies[i].equals(endpoint)) {
                    return cookies[i + 1];
                }
            }
        }
        return null;
    }
}
