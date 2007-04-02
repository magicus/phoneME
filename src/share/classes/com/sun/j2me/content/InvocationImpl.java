/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.j2me.content;

import javax.microedition.content.Invocation;
import javax.microedition.content.ContentHandler;
import javax.microedition.content.ContentHandlerException;

import java.io.IOException;
import javax.microedition.io.Connection;

/**
 * Implementation of Invocation class.
 * <p>
 * This class MUST NOT have any public methods that are not also
 * public in Invocation (the superclass).  The sensistive methods
 * of the class MUST be package private.
 */
public final class InvocationImpl {
    /**
     * The Invocation delegating to this instance.
     * This field is public to Invocation can set it.
     * This allows the implementation to pass a InvocationImpl to
     * back to the Invocation class and it can wrap it in an Invocation
     * before passing it to the application.
     */
    private Invocation invocation;

    /**
     * The URL of the content; may be <code>null</code>.
     * URLs of up to and including 256 characters in length MUST be
     * supported. A URL with a length of zero is treated as
     * <code>null</code> and is ignored..
     */
    private String url;

    /** The content type; may be <code>null</code>. */
    private String type;

    /** The content handler ID; may be <code>null</code> */
    private String ID;

    /** The action to perform on the content; may be <code>null</code> */
    private String action;

    /** The array of arguments; may NOT be <code>null</code> */
    private String[] arguments = ZERO_STRINGS;

    /** The data array; may NOT be <code>null</code>. */
    private byte[] data = ZERO_BYTES;

    /**
     * Set to <code>true</code> if the invoker must be notified of
     * completion.
     */
    private boolean responseRequired = true;

    /** The username in case it is needed for authentication. */
    String username;

    /** The password in case it is needed for authentication. */
    char[] password;

    /** Transaction Identifier. */
    private int tid;

    /**
     * The status of the request; one of
     * {@link Invocation#ACTIVE},
     * {@link Invocation#WAITING},
     * {@link Invocation#ERROR},
     * {@link Invocation#OK}, or
     * {@link Invocation#CANCELLED}.
     */
    int status;

    /** The authority that authenticated this Invocation. */
    private String invokingAuthority;

    /** The ID that authenticated this Invocation. */
    private String invokingID;

    /** The application name of the invoking MIDlet suite. */
    private String invokingAppName;

    /** The previous invocation, if any. */
    InvocationImpl previous;

    /** A zero length array of strings to re-use when needed.  */
    private static final String[] ZERO_STRINGS = new String[0];

    /** A zero length array of bites to re-use when needed.  */
    private static final byte[] ZERO_BYTES = new byte[0];

    private InvocationImpl() {
        tid = -1;
    }

    /**
     * Create a fresh InvocationImpl.
     */
    InvocationImpl(int tid) {
        this.tid = tid;
    }

    /**
     * Create a fresh InvocationImpl that is being delegated to byte
     * an Invocation instance created by an application.
     * @param invocation the Invocation delegating to this implementation
     */
    public InvocationImpl(Invocation invocation) {
        if (invocation.getStatus() != -1) {
            throw new RuntimeException("Illegal constructor call");
        }
        this.invocation = invocation;
        status = Invocation.INIT;
    }

    /**
     * Sets the argument list to a new array of Strings.  The arguments
     * are used by the application to communicate to the content
     * handler and return results from the content handler.
     * The values of the arguments are not checked when they are set.
     * Instead, they are checked during
     * {@link Registry#invoke Registry.invoke} to
     * check that none of the values are <code>null</code>.
     * @param args the String array; may be <code>null</code>.
     * A <code>null</code>
     * argument is treated the same as a zero-length array
     * @see #getArgs
     */
    public void setArgs(String[] args) {
        this.arguments = (args == null) ? ZERO_STRINGS : args;
    }

    /**
     * Gets the argument list as an array of Strings. These values
     * are passed to the content handler and are returned from
     * the content handler.
     * The array is not copied; modifications to array elements
     * will be visible.
     * @return the arguments array, which MUST NOT be <code>null</code>
     * @see #setArgs
     */
    public String[] getArgs() {
        return arguments;
    }

    /**
     * Sets the data used for the Invocation.  The data
     * is used by the application to communicate to the content
     * handler and return data from the content handler.
     * @param data the byte data array; may be <code>null</code>.
     * A <code>null</code> is treated the same as a zero-length array
     * @see #getData
     */
    public void setData(byte[] data) {
        this.data = (data == null) ? ZERO_BYTES : data;
    }

    /**
     * Gets the data for the Invocation. The data
     * is passed to the content handler.
     * The content handler may modify and return the data
     * if it returns a response.
     * The array is not copied; modifications to array elements
     * will be visible.
     * @return the data array, which MUST NOT be <code>null</code>
     * @see #setData
     */
    public byte[] getData() {
        return data;
    }

    /**
     * Gets the URL for the invocation.
     * The URL must be equal to the value set with {@link #setURL setURL}.
     * @return the URL or <code>null</code> if it has not been set
     * @see #setURL
     */
    public String getURL() {
        return url;
    }

    /**
     * Sets the URL for the invocation.
     * @param url the URL to be set; may be <code>null</code>
     * @see #getURL
     */
    public void setURL(String url) {
        this.url = url;
    }


    /**
     * Gets the content type for the Invocation.
     * @return the content type or <code>null</code> if it has not been set
     * @see #setType
     * @see #findType
     */
    public String getType() {
        return type;
    }

    /**
     * Sets the type for the Invocation.
     * @param type the type to be set for the content; may be <code>null</code>
     * @see #getType
     */
    public void setType(String type) {
        this.type = type;
    }


    /**
     * Gets the action to be performed on the content.
     * @return the content action or <code>null</code> if it has not been set
     * @see #setAction
     */
    public String getAction() {
        return action;
    }

    /**
     * Sets the action to be performed on the content.
     * @param action the action to be performed on the content;
     *  may be <code>null</code>
     * @see #getAction
     */
    public void setAction(String action) {
        this.action = action;
    }


    /**
     * Gets the <code>responseRequired</code> mode for
     * this Invocation.
     * If <code>true</code>, then the invoking application requires a
     * response to the Invocation.
     * @return the current value of the <code>responseRequired</code>
     * mode. If
     * <code>true</code>, then a response must be returned to the
     * invoking application.
     * @see #setResponseRequired
     */
    public boolean getResponseRequired() {
        return responseRequired;
    }

    /**
     * Sets the <code>responseRequired</code> mode for
     * this Invocation.
     * If <code>true</code>, then the invoking application requires a
     * response to the Invocation.
     * The value in the request can be changed only if the status is
     * <code>INIT</code>.
     * @param responseRequired
     * <code>true</code> to require a response,
     * <code>false</code> otherwise
     * @exception IllegalStateException is thrown if the status is not
     *        <code>INIT</code>
     * @see #getResponseRequired
     */
    public void setResponseRequired(boolean responseRequired) {
        if (getStatus() != Invocation.INIT) {
            throw new IllegalStateException();
        }
        this.responseRequired = responseRequired;
    }

    /**
     * Gets the content handler ID for this Invocation.
     * @see Registry#forID
     * @return the ID of the ContentHandler; may be
     * <code>null</code>
     * @see #setID
     */
    public String getID() {
        return ID;
    }

    /**
     * Sets the ID of the content handler for this Invocation.
     * @param ID of the content handler; may be <code>null</code>
     * @see #getID
     */
    public void setID(String ID) {
        this.ID = ID;
    }

    /**
     * Creates and opens a Connection to the content accessable by
     * using the URL. This method is
     * equivalent to
     * {@link javax.microedition.io.Connector#open Connector.open}
     * with the URL provided.
     * The application should use this method to access the
     * content of the URL
     * so that any type or content information cached by the
     * implementation can be fully utilized. The content is opened
     * in read only mode.
     *
     * @param timeouts         a flag to indicate that the caller
     *                         wants timeout exceptions
     * @return                 a Connection object
     *
     * @exception ConnectionNotFoundException is thrown if:
     *   <ul>
     *      <li>the target URL cannot be found, or</li>
     *      <li>the requested protocol type is not supported</li>
     *   </ul>
     * @exception NullPointerException if the URL is null
     * @exception IllegalArgumentException if a parameter is invalid.
     * @exception IOException  if some other kind of I/O error occurs
     * @exception SecurityException is thrown if access to the
     *   protocol handler is prohibited
     */
    public Connection open(boolean timeouts) throws IOException {
        if (url == null) {
            throw new NullPointerException();
        }

        ContentReader reader = new ContentReader(url, username, password);
        return reader.open(timeouts);
    }

    /**
     * Provide the credentials needed to access the content.
     * @param username the username; may be <code>null</code>
     * @param password the password for the username;
     *   may be <code>null</code>
     */
    public void setCredentials(String username, char[] password) {
        this.username = username;
        this.password = password;

    }

    /**
     * Returns the status of this Invocation, which can be
     * <code>INIT</code>, <code>WAITING</code>, <code>HOLD</code>,
     * <code>ACTIVE</code>, <code>OK</code>,
     * <code>CANCELLED</code>, or <code>ERROR</code>.
     * The application uses the status to determine how
     * to process an Invocation returned from
     * <code>getInvocation</code>.
     *
     * @see javax.microedition.content.Registry#invoke
     *
     * @return the current status of this Invocation
     */
    public int getStatus() {
        return status;
    }

    /**
     * Set the status of this InvocationImpl.
     * If the invocation is still active in the native code
     * set the status in native also.
     * @param status the new status
     */
    void setStatus(int status) {
        this.status = status;
    }

    /**
     * Finds the type of the content in this Invocation.
     * If the <tt>getType</tt> method return value is
     * <code>non-null</code>, then the type is returned.
     * <p>
     * If the type is <code>null</code> and the URL is non-<code>null</code>,
     * then the content type will be found by accessing the content
     * through the URL.
     * When found, the type is set as if the <code>setType</code> method
     * was called;  subsequent calls to
     * {@link #getType getType} and {@link #findType findType}
     * will return the type.
     * If an exception is thrown, the <code>getType</code> method will
     * return <code>null</code>.
     * <p>
     * The calling thread blocks while the type is being determined.
     * If a network access is needed there may be an associated delay.
     *
     * @return the <code>non-null</code> content type
     * @exception IOException if access to the content fails
     *
     * @exception ContentHandlerException is thrown with a reason of
     * {@link ContentHandlerException#TYPE_UNKNOWN}
     *  if the type is <code>null</code> and cannot be found from the
     *  content either because the URL is <code>null</code> or the type is
     *  not available from the content
     * @exception IllegalArgumentException if the content is accessed via
     *  the URL and the URL is invalid
     * @exception SecurityException is thrown if access to the content
     *  is required and is not permitted
     */
    public String findType() throws IOException, ContentHandlerException {

        if (type != null) {
            return type;
        }

        if (url != null) {
            ContentReader reader = new ContentReader(url, username, password);
            String type = reader.findType();
                        if (type != null) {
                this.type = type;
                return type;
                        }
        } else if (data.length > 0) {
            // TODO: try to determine type by data signature
                    }

                    throw new ContentHandlerException(
                "Can not determine the content type",
                                ContentHandlerException.TYPE_UNKNOWN);
                }

    /**
     * Returns the previous Invocation linked to this
     * Invocation by this application's previous call to
     * {@link Registry#invoke(Invocation invoc, Invocation previous)}.
     *
     * @return the previous Invocation, if any, set when this
     *        Invocation was invoked;
     *        <code>null</code> is returned if the Invocation was not
     *  invoked with a previous Invocation.
     */
    public InvocationImpl getPrevious() {
        return previous;
    }

    /**
     * Gets the authority, if any, used to authenticate the
     * application that invoked this request.
     * This value MUST be <code>null</code> unless the device has been
     * able to authenticate this application.
     * If <code>non-null</code>, it is the string identifiying the
     * authority.  For example,
     * if the application was a signed MIDlet, then this is the
     * "subject" of the certificate used to sign the application.
     *
     * <p>The format of the authority for X.509 certificates is defined
     * by the MIDP Printable Representation of X.509 Distinguished
     * Names as defined in class
     *
     * <code>javax.microedition.pki.Certificate</code>. </p>
     * @return the authority used to authenticate this application
     * or <code>null</code> otherwise
     *
     * @see ContentHandler#getAuthority
     */
    public String getInvokingAuthority() {
        return invokingAuthority;
    }

    /**
     * Get the user-friendly name of the application that invoked
     * the content handler. This information is available only if the status is
     * <code>ACTIVE</code> or <code>HOLD</code>.
     *
     * This information has been authenticated only if
     * <code>getInvokingAuthority</code> is non-null.
     *
     * @return the application's name if status is <code>ACTIVE</code>
     * or <code>HOLD</code>; <code>null</code> otherwise
     *
     * @see ContentHandler#getID
     */
    public String getInvokingAppName() {
        return invokingAppName;
    }

    /**
     * Gets the ID of the application that invoked the content
     * handler. This information is available only if the status is
     * <code>ACTIVE</code> or <code>HOLD</code>.
     *
     * This information has been authenticated only if
     * <code>getInvokingAuthority</code> is non-null.
     *
     * @return the application's ID if status is <code>ACTIVE</code>
     * or <code>HOLD</code>; <code>null</code> otherwise
     *
     * @see ContentHandler#getID
     */
    public String getInvokingID() {
        return invokingID;
    }

    /**
     * Return a printable form of InvocationImpl.
     * Disabled if not logging
     * @return a String containing a printable form
     */
    public String toString() {
        if (AppProxy.LOG_INFO) {
            StringBuffer sb = new StringBuffer(200);
            sb.append("tid: ");         sb.append(tid);
            sb.append(" status: ");     sb.append(status);
            //        sb.append("  suiteId: ");   sb.append(suiteId);
            sb.append(", type: ");      sb.append(getType());
            sb.append(", url: ");       sb.append(getURL());
            sb.append(", respReq: ");   sb.append(getResponseRequired());
            //        sb.append(", args: ");      sb.append(getArgs());
            //        sb.append(", prevTid: ");   sb.append(previousTid);
            //        sb.append(", previous: ");
            //        sb.append((previous == null) ? "null" : "non-null");
            //        sb.append("_suiteId: ");    sb.append(invokingSuiteId);
            //        sb.append(", _authority: "); sb.append(invokingAuthority);
            //        sb.append(", _ID: ");       sb.append(invokingID);
            return sb.toString();
        } else {
            return super.toString();
        }
    }

    /**
     * Returns Invocation instance counterpart.
     */
    public Invocation getInvocation() {
        return invocation;
    }

    /**
     * Sets Invocation instance counterpart.
     */
    public void setInvocation(Invocation invoc) {
        if (invocation != null || invoc.getStatus() != -1) {
            throw new RuntimeException("Operation is not permitted");
        }
        invocation = invoc;
    }

}
