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

package com.sun.cdc.io.j2me.jcrmi;

import com.sun.j2me.app.AppPackage;
import com.sun.j2me.security.SatsaPermission;
import java.io.*;
import java.rmi.Remote;
import java.rmi.RemoteException;
import com.sun.j2me.crypto.NoSuchAlgorithmException;
import com.sun.j2me.crypto.DigestException;
import com.sun.j2me.crypto.MessageDigest;
import javax.microedition.io.Connection;
import javax.microedition.io.StreamConnection;
import javax.microedition.jcrmi.JavaCardRMIConnection;
import javax.microedition.jcrmi.RemoteStub;

import com.sun.j2me.io.ConnectionBaseInterface;
import com.sun.cdc.io.j2me.apdu.APDUManager;
import com.sun.cdc.io.j2me.apdu.Handle;
import com.sun.satsa.acl.ACLPermissions;
import com.sun.satsa.acl.AccessControlManager;
import com.sun.satsa.acl.JCRMIPermissions;
import com.sun.satsa.util.Utils;
import javacard.framework.*;
import javacard.framework.service.ServiceException;

import com.sun.j2me.main.Configuration;

import com.sun.j2me.security.TrustedClass;
import com.sun.j2me.security.Token;
import com.sun.satsa.security.SecurityInitializer;

/**
 * JCRMI connection to card application.
 */
public class Protocol
    implements JavaCardRMIConnection, ConnectionBaseInterface,
	       StreamConnection {

    /*
     * Inner class to request security token from SecurityTokenInitializer.
     * SecurityTokenInitializer should be able to check this inner class name.
     */
    static private class SecurityTrusted
        implements TrustedClass { };

    /** This class has a different security domain than the App suite */
    private static Token securityToken =
        SecurityInitializer.requestToken(new SecurityTrusted());

    /**
     * Size of APDU buffer.
     */
    private static final int APDUBufferSize = 255;

    /**
     * Stub object for initial remote reference.
     */
    private Remote initialReference;

    /**
     * Reference object for initial remote reference.
     */
    private Reference internalReference;

    /**
     * Remote reference uses this buffer to prepare INVOKE APDU command.
     */
    private byte[] APDUBuffer = new byte[APDUBufferSize];

    /**
     * Current offset in <code>APDUBuffer</code> buffer for
     * <code>write</code> methods.
     */
    private int offset;

    /**
     * Response APDU data.
     */
    private byte[] response;

    /**
     * Current offset in <code>response</code> buffer for
     * <code>read</code> methods.
     */
    private int r_offset;

    /**
     * SHA-1 message digest object used by this connection.
     */
    private MessageDigest SHA;

    /** The current APDU connection handle. */
    private Handle h;

    /**
     * A flag to indicate if connection is open or not.
     */
    private boolean connectionOpen;

    /**
     * This object verifies access rights of MIDlet.
     */
    private JCRMIPermissions verifier;

    /**
     * Connector uses this method to initialize the connection object.
     * This method establishes APDU connection with card application,
     * obtains FCI information and creates stub for initial remote
     * reference.
     * @param name the URL for the connection without protocol name
     * @param mode the access mode (Ignored)
     * @param timeouts a flag to indicate that the caller wants timeout
     *                  exceptions. Ignored
     * @return this connection
     * @throws IOException if the connection can not be initialized
     * @throws RemoteException if initial remote reference object can not be
     * created
     * @throws SecurityException if access is restricted by ACL
     */
    public Connection openPrim(String name, int mode, boolean timeouts)
            throws IOException {

        AppPackage appPackage = AppPackage.getInstance();
        try {
            appPackage.checkForPermission(SatsaPermission.JCRMI_CONNECTION);
        } catch (InterruptedException ie) {
            throw new InterruptedIOException(
                "Interrupted while trying to ask the user permission");
        }

        int slotInfo;
        // parse URL string for slot number and AID
        try {
            slotInfo = parseURL(name);
        } catch (NullPointerException npe) {
            throw new IllegalArgumentException("Invalid URL");
        } catch (IndexOutOfBoundsException iobe) {
            throw new IllegalArgumentException("Invalid URL");
        } catch (IllegalArgumentException iae) {
            throw new IllegalArgumentException("Invalid URL");
        }

        APDUManager.checkSlotNumber(slotInfo);
        // get card application selected
         APDUManager.initACL(slotInfo);
         verifier = AccessControlManager
                .getJCRMIPermissions(slotInfo,
                APDUBuffer,
                appPackage.getCA());

         h = APDUManager.selectApplication(APDUBuffer, slotInfo);

        connectionOpen = true;

        // parse FCI
        byte[] FCI = h.getFCI();

        byte invokeINS;

        try {
            // fci_tag or application_data_tag
            if (FCI[0] != 0x6f || FCI[2] != 0x6e) {
                throw new RemoteException("Incorrect FCI format");
            }

            int index = 4;

            // skip unnecessary tag/value pairs
            while (FCI[index] != 0x5E) {     // jc_rmi_data_tag
                index += 2 + (FCI[index] & 0xff);
            }

            index += 4;

            invokeINS = FCI[index++];

            if (FCI[index++] != (byte) 0x81) {       // normal_tag
                throw new RemoteException("Incorrect FCI format");
            }

            response = FCI;
            r_offset = index;

            initialReference = createStub();

            if (initialReference == null) {
                throw new RemoteException();
            }
        } catch (RemoteException e) {
            close();
            throw e;
        } catch (Throwable e) {
            close();
            throw new RemoteException("Can't create initial reference");
        }


        offset = 0;
        putByte(0x80 | h.channel);
        putByte(invokeINS);
        putShort(0x0202);

        try {
            SHA = new MessageDigest("SHA-1");
        } catch (NoSuchAlgorithmException e) {
            // Ignore this exception
        }

        return this;
    }

    /**
     * Closes the connection.
     * @throws IOException If an I/O error occurs
     */
    public void close() throws IOException {
        synchronized (APDUBuffer) {
            if (connectionOpen) {
                connectionOpen = false;
                APDUManager.closeConnection(h);
            }
        }
    }

    /**
     * Returns the stub object for an initial remote reference.
     * @return the initial remote reference
     */
    public java.rmi.Remote getInitialReference() {
        return initialReference;
    }

    /**
     * Parses the URL to get the slot number and AID.
     * Prepares SELECT APDU in APDUBuffer.
     * @param URL contains the URL from which
     * the slot information is to be extracted
     * @return slot number for this connection
     */
    private int parseURL(String URL) {

        int slotIndex = URL.indexOf(":") + 1;
        int AIDIndex = URL.indexOf(";AID=");

        int slotInfo = (AIDIndex == slotIndex) ? 0 :
               Integer.parseInt(URL.substring(slotIndex, AIDIndex), 16);

        // prepare selection APDU
        offset = 0;

        // IMPL_NOTE: For JavaCard 2.1 compliant cards jcrmi emulation is used.
        String jcVersion = Configuration.getProperty("com.sun.javacard.version");
        if ((jcVersion.equals("2.1"))||(jcVersion.startsWith("2.1."))) {
            putInt(0x00a40400);  // selection APDU header
        }
        else {
            putInt(0x00a40410);  // selection APDU header
        }
        offset++;           // length

        // parse for AID
        int AIDLength = APDUManager.parseDottedBytes(
                URL.substring(AIDIndex + 5), APDUBuffer, offset);

        if (AIDLength < 5 || AIDLength > 16)
            throw new IllegalArgumentException();

        APDUBuffer[4] = (byte) AIDLength;
        offset += AIDLength;
        putByte(255);

        return slotInfo;
    }


    /**
     * Writes <code>param</code> value into <code>APDUBuffer</code>.
     * @param param the value to be written
     */
    private void putByte(int param) {
        APDUBuffer[offset++] = (byte) param;
    }

    /**
     * Writes <code>param</code> value into <code>APDUBuffer</code>.
     * @param param the value to be written
     */
    private void putShort(int param) {
        putByte(param >> 8);
        putByte(param);
    }

    /**
     * Writes <code>param</code> value into <code>APDUBuffer</code>.
     * @param param the value to be written
     */
    private void putInt(int param) {
        putByte(param >> 24);
        putByte(param >> 16);
        putByte(param >> 8);
        putByte(param);
    }


    /**
     * Reads one byte from response APDU, zero-extends it to type
     * <code>int</code>, and returns the result.
     * @return the unsigned 8-bit value
     */
    private int getByte() {
    	return response[r_offset++] & 0xff;
    }

    /**
     * Reads <code>short</code> value from response APDU
     * @return <code>short</code> value
     */
    private short getShort() {
    	return (short) (getByte() << 8 | getByte());
    }

    /**
     * Reads <code>int</code> value from response APDU.
     * @return <code>int</code> value
     */
    private int getInt() {
        return (getByte() << 24) |
               (getByte() << 16) |
               (getByte() << 8) |
                getByte();
    }

    /**
     * Reads <code>String</code> value from response APDU.
     * @return <code>String</code> value
     */
    private String getString() throws RemoteException {

        int len = getByte();

        if (len == 0) {
            return null;
        }

        String S;
        try {
            S = new String(response, r_offset, len, Utils.utf8);
        } catch (UnsupportedEncodingException e) {
            throw new RemoteException("UTF-8 encoding is not supported");
        }
        r_offset += len;
        return S;
    }

    /**
     * Creates stub using remote reference descriptor in
     * <code>response</code> buffer at <code>r_offset </code> offset.
     * @return new stub object
     * @throws java.rmi.RemoteException if an error occurs
     */
    private Remote createStub() throws RemoteException {

        short objectID = getShort();

        if (objectID == (short) 0xffff) {
            return null;                    // null reference returned
        }

        // hash modifier
        String hashModifier = getString();

        // list of interfaces

        int count = getByte();

        Class stubClass = null;
        String className = null;
        String packageName = null;
        Class[] classes = new Class[count];

        for (int i = 0; i < count; i++) {

            String name = getString();
            if (name != null) {
                packageName = name.replace('/', '.');
            }

            name = packageName + "." + getString();

            try {
                classes[i] = Class.forName(name);
            } catch (ClassNotFoundException e) {
                throw new RemoteException("Class not found", e);
            }

            if (stubClass == null ||
                    stubClass.isAssignableFrom(classes[i])) {
                stubClass = classes[i];
                className = name;
                continue;
            }
        }

        for (int i = 0; i < count; i++) {
            if (! classes[i].isAssignableFrom(stubClass)) {
                throw new RemoteException(
                        "Incorrect hierarchy in descriptor");
            }
        }

        RemoteStub stub;
        try {
            stub = RemoteStubCreation.createStub(className + "_Stub");                    
        } catch (ClassNotFoundException cnfe) {
            throw new RemoteException("Can't find stub class", cnfe);
        } catch (IllegalAccessException iae) {
            throw new RemoteException("Access to stub class denied", iae);
        } catch (InstantiationException ie) {
            throw new RemoteException("Can't create stub object", ie);
        }
        Reference r = new Reference(this, objectID, hashModifier, className);
        if (internalReference == null) {
            internalReference = r;
        }
        stub.setRef(r);

        return (Remote) stub;
    }

    /**
     * Constant for JCRMI protocol.
     */
    private static final byte NormalTag = (byte) 0x81;
    /**
     * Constant for JCRMI protocol.
     */
    private static final byte ExactExceptionTag = (byte) 0x82;
    /**
     * Constant for JCRMI protocol.
     */
    private static final byte ExceptionSubclassTag = (byte) 0x83;
    /**
     * Constant for JCRMI protocol.
     */
    private static final byte ErrorTag = (byte) 0x99;

    /**
     * Invokes a remote method.
     *
     * The remote method invoked on the card can throw an exception to
     * signal that an unexpected condition has been detected.<p>
     *
     * If the exception thrown on the card is an exception defined in
     * the Java Card 2.2 API, then the same exception is thrown to the
     * stub method. The client can access the reason code associated
     * with Java Card-specific exceptions using the standard
     * <code>getReason()</code> method.<p>
     *
     * If the exception thrown on the card is a subclass of an exception
     * defined in the Java Card 2.2 API, then the closest exception defined
     * in the API (along with the reason code, if applicable) is
     * thrown to the stub method. The detail message string of the
     * exception object may indicate that exception subclass was thrown
     * on the card.<p>
     *
     * Apart from the exceptions thrown by the remote method itself,
     * errors during communication, marshalling, protocol handling,
     * unmarshalling, stub object instantiation, and so on, related
     * to the JCRMI method invocation, results in a
     * <code>RemoteException</code> being thrown to the stub method.
     *
     * @param ref handle for remote object
     * @param method simple (not fully qualified) name of the method
     *        followed by the method descriptor. Representation of a
     *        method descriptor is the same as that described in The
     *        Java Virtual Machine Specification (&#167 4.3.3)
     * @param params the parameter list
     * @return result of remote method invocation
     * @exception java.lang.Exception if any exception occurs during
     *            the remote method invocation
     */
    Object invoke(Reference ref, String method, Object[] params)
            throws Exception {

        if (! connectionOpen) {
            throw new RemoteException("Connection is closed.");
        }

        verifier.checkPermission(ref.getClassName(), method);

        synchronized (APDUBuffer) {

            try {
                marshal(ref, method, params);
            } catch (RuntimeException ai) {
                throw new RemoteException("Error marshalling parameters");
            }

            try {

                netmonSaveApdu(APDUBuffer);
                response = APDUManager.exchangeAPDU(h, APDUBuffer);
                netmonSaveResponse(response);
            } catch (IOException e) {
                throw new RemoteException("IO error", e);
            }

            r_offset = response.length - 2;

            if (getShort() !=  (short) 0x9000) {
                throw new RemoteException("Incorrect status word");
            }

            r_offset = 0;

            byte tag = (byte) getByte();

            Exception ex = null;
            Object result = null;

            try {
                if (tag == ExactExceptionTag || tag == ExceptionSubclassTag) {
                    ex = parseException(tag);
                } else
                if (tag == ErrorTag) {
                    ex = parseError();
                } else
                if (tag == NormalTag) {
                    result = parseResult(method);
                } else {
                    throw new RemoteException("Incorrect tag value: " + tag);
                }

                getShort();     // status word

            } catch (RuntimeException e) {
                throw new RemoteException("Incorrect response structure");
            }

            if (ex != null) {
                throw ex;
            }

            return result;
        }
    }

    /**
     * Prepares INVOKE APDU.
     * @param ref handle for remote object
     * @param method simple (not fully qualified) name of the method
     *        followed by the method descriptor. Representation of a
     *        method descriptor is the same as that described in The
     *        Java Virtual Machine Specification (&#167 4.3.3)
     * @param params the parameter list
     * @throws RemoteException if an error occurs
     */
    private void marshal(Reference ref, String method, Object[] params)
        throws RemoteException {

        offset = 5;

        putShort(ref.getObjectID());

        String hashString = method;
        String hashModifier = ref.getHashModifier();

        if (hashModifier != null) {
            hashString = hashModifier + hashString;
        }

        byte[] buf = Utils.stringToBytes(hashString);

        SHA.reset();
        SHA.update(buf, 0, buf.length);
        try {
            SHA.digest(APDUBuffer, offset, SHA.getDigestLength());
        } catch (DigestException e) {
            throw new RemoteException("SHA1 error");
        }        
        
        offset += 2;

        if (params != null) {
            for (int i = 0; i < params.length; i++) {

                Object obj = params[i];

                if (obj == null) {
                    putByte(0xff);
                    continue;
                }

                if (obj instanceof Byte) {
                    putByte(((Byte)obj).byteValue());
                    continue;
                }

                if (obj instanceof Boolean) {
                    putByte(((Boolean)obj).booleanValue() ? 1 : 0);
                    continue;
                }

                if (obj instanceof Short) {
                    putShort(((Short)obj).shortValue());
                    continue;
                }

                if (obj instanceof Integer) {
                    putInt(((Integer)obj).intValue());
                    continue;
                }

                if (obj instanceof byte[]) {
                    byte[] param = (byte[]) obj;
                    putByte(param.length);
                    for (int k = 0; k < param.length; k++) {
                        putByte(param[k]);
                    }
                    continue;
                }

                if (obj instanceof boolean[]) {
                    boolean[] param = (boolean[]) obj;
                    putByte(param.length);
                    for (int k = 0; k < param.length; k++) {
                        putByte(param[k] ? 1 : 0);
                    }
                    continue;
                }

                if (obj instanceof short[]) {
                    short[] param = (short[]) obj;
                    putByte(param.length);
                    for (int k = 0; k < param.length; k++) {
                        putShort(param[k]);
                    }
                    continue;
                }

                if (obj instanceof int[]) {
                    int[] param = (int[]) obj;
                    putByte(param.length);
                    for (int k = 0; k < param.length; k++) {
                        putInt(param[k]);
                    }
                    continue;
                }

                throw new RemoteException("Incorrect parameter type");
            }
        }
        APDUBuffer[4] = (byte) (offset - 5);

        putByte(255);
    }


    /**
     * Parses response containing data about exception.
     * @param tag type tag of exception
     * @return the exception to be thrown to stub
     * @throws RemoteException  if an error occurs during parsing
     */
    private Exception parseException(byte tag) throws RemoteException {


        String message = (tag == ExactExceptionTag) ?
                            "Exception is thrown on card" :
                            "Exception subclass is thrown on card";

        byte type = (byte) getByte();
        short reason = getShort();

        switch (type) {
            case 0x00:
                return new Exception(message + ": java.lang.Throwable");
            case 0x01:
                return new ArithmeticException(message);
            case 0x02:
                return new ArrayIndexOutOfBoundsException(message);
            case 0x03:
                return new ArrayStoreException(message);
            case 0x04:
                return new ClassCastException(message);
            case 0x05:
                return new Exception(message);
            case 0x06:
                return new IndexOutOfBoundsException(message);
            case 0x07:
                return new NegativeArraySizeException(message);
            case 0x08:
                return new NullPointerException(message);
            case 0x09:
                return new RuntimeException(message);
            case 0x0A:
                return new SecurityException(message);
            case 0x0B:
                return new IOException(message);
            case 0x0C:
                return new RemoteException(message);
            case 0x20:
                return new APDUException(reason);
            case 0x21:
                return new CardException(reason);
            case 0x22:
                return new CardRuntimeException(reason);
            case 0x23:
                return new ISOException(reason);
            case 0x24:
                return new PINException(reason);
            case 0x25:
                return new SystemException(reason);
            case 0x26:
                return new TransactionException(reason);
            case 0x27:
                return new UserException(reason);
            case 0x30:
                return new javacard.security.CryptoException(reason);
            case 0x40:
                return new ServiceException(reason);
            default:
                throw new RemoteException(
                        "Unknown exception is thrown on card");
        }
    }

    /**
     * Parses response containing data about error.
     * @return the exception to be thrown to stub
     * @throws RemoteException if error code is unknown
     */
    private RemoteException parseError() throws RemoteException {

        short code = getShort();

        switch (code) {
            case 1: return new RemoteException("Object not exported");
            case 2: return new RemoteException("Method not found");
            case 3: return new RemoteException("Signature mismatch");
            case 4: return new RemoteException("Out of parameter resources");
            case 5: return new RemoteException("Out of response resources");
            case 6: return new RemoteException(
                        "Protocol error reported by the card");
            default:
                throw new RemoteException("Error reported by the card: " +
                                            code);
        }
    }

    /**
     * Parses the normal JCRMI response.
     * @param method method name and signature
     * @return the value returned by method
     * @throws RemoteException if an error occurs during parsing
     */
    private Object parseResult(String method) throws RemoteException {

        int index = method.indexOf(')') + 1;

        if (index != -1) {
            switch (method.charAt(index)) {
                case 'V': return null;
                case 'B': return new Byte((byte) getByte());
                case 'Z': return new Boolean(getByte() == 1 ? true : false);
                case 'S': return new Short(getShort());
                case 'I': return new Integer(getInt());
                case 'L': return createStub();
                case '[': {

                    int len = getByte();

                    if (len == 255)
                        return null;

                    switch (method.charAt(index + 1)) {
                        case 'B': {
                            byte[] data = new byte[len];

                            for (int i = 0; i < len; i++) {
                                data[i] = (byte) getByte();
                            }
                            return data;
                        }
                        case 'Z': {
                            boolean[] data = new boolean[len];

                            for (int i = 0; i < len; i++) {
                                data[i] = (getByte() == 1 ? true : false);
                            }
                            return data;
                        }
                        case 'S': {
                            short[] data = new short[len];

                            for (int i = 0; i < len; i++) {
                                data[i] = getShort();
                            }
                            return data;
                        }
                        case 'I': {
                            int[] data = new int[len];

                            for (int i = 0; i < len; i++) {
                                data[i] = getInt();
                            }
                            return data;
                        }
                    }
                }
            }
        }
        throw new RemoteException("Incorrect method signature");
    }

    /**
     * Returns the card session identifier for this connection.
     * @return the card session identifier
     */
    int getCardSessionId() {
        return h.cardSessionId;
    }

    /**
     * Returns the flag that indicates if connection is open.
     * @return true if the connection is open
     */
    boolean isOpened() {
        return connectionOpen;
    }

    /**
     * Open and return an input stream for a connection.
     * This method always throw
     * <code>IllegalArgumentException</code>.
     * @return An input stream
     * @exception IllegalArgumentException  is thrown for all requests
     */
    public InputStream openInputStream() {
        throw new IllegalArgumentException("Not supported");
    }

    /**
     * Open and return a data input stream for a connection.
     * This method always throw
     * <code>IllegalArgumentException</code>.
     * @return An input stream
     * @exception IllegalArgumentException  is thrown for all requests
     */
    public DataInputStream openDataInputStream() {
        throw new IllegalArgumentException("Not supported");
    }

    /**
     * Open and return an output stream for a connection.
     * This method always throw
     * <code>IllegalArgumentException</code>.
     * @return An output stream
     * @exception IllegalArgumentException  is thrown for all requests
     */
    public OutputStream openOutputStream() {
        throw new IllegalArgumentException("Not supported");
    }

    /**
     * Open and return a data output stream for a connection.
     * This method always throw
     * <code>IllegalArgumentException</code>.
     * @return An output stream
     * @exception IllegalArgumentException  is thrown for all requests
     */
    public DataOutputStream openDataOutputStream() {
        throw new IllegalArgumentException("Not supported");
    }

    /**
     * A call to enterPin method pops up a UI that requests the PIN
     * from the user. The pinID field indicates which PIN must be
     * requested from the user. The user can either cancel the request
     * or continue. If the user enters the PIN and chooses to continue,
     * The implementation is responsible for
     * presenting the PIN entered by the user to the card for verification.
     * @param pinID the type of PIN the implementation is suppose to prompt
     * the user to enter.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for PIN verification.
     */
    public short enterPin(int pinID) throws java.rmi.RemoteException {
        return doEnterPin(pinID, 0, ACLPermissions.CMD_VERIFY);
    }

    /**
     * A call to <code>changePin</code> method pops up a UI that requests
     * the user for an old or existing PIN value and the new PIN value to
     * to change the value of the PIN. The pinID field indicates which PIN is
     * to be changed. The user can either cancel the request
     * or continue. If the user enters the PIN values and chooses to continue
     * the implementation is responsible for presenting the
     * the old and new values of the PIN to the card.
     * @param pinID the type of PIN the implementation is suppose to prompt
     * the user to change.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for changing the PIN value.
     */
    public short changePin(int pinID) throws RemoteException {
        return doEnterPin(pinID, 0, ACLPermissions.CMD_CHANGE);
    }

    /**
     * A call to <code>disablePin</code> method pops up a UI that requests
     * the user to enter the value for the PIN that is to be disabled.
     * The pinID field
     * indicates which PIN is to be disabled. The user can
     * either cancel the request
     * or continue. If the user enters the PIN and chooses to continue the
     * implementation is responsible
     * for presenting the PIN value to the card to disable PIN.
     * @param pinID the type of PIN the implementation is required to prompt
     * the user to enter.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for disabling the PIN.
     */
    public short disablePin(int pinID) throws RemoteException {
        return doEnterPin(pinID, 0, ACLPermissions.CMD_DISABLE);
    }

    /**
     * A call to <code>enablePin</code> method pops up a UI that requests
     * the user to enter the value for the PIN that is to be enabled.
     * The pinID field
     * indicates which PIN is to be enabled. The user can
     * either cancel the request
     * or continue. If the user enters the PIN and chooses to continue the
     * implementation is responsible
     * for presenting the PIN value to the card for enabling the PIN.
     * @param pinID the type of PIN the implementation is required to prompt
     * the user to enter.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for enabling the PIN.
     */
    public short enablePin(int pinID) throws RemoteException {
        return doEnterPin(pinID, 0, ACLPermissions.CMD_ENABLE);
    }

    /**
     * This is a high-level method that lets the J2ME application
     * ask the user to enter the value for an unblocking PIN,
     * and the new value for the blocked PIN and send
     * these to the card.
     * A call to <code>unblockPin</code> method pops up a UI that requests
     * the user to enter the value for the unblocking PIN and the
     * new value for the blocked PIN.
     * The <code>unblockingPinID</code> field indicates which unblocking
     * PIN is to be
     * used to unblock the blocked PIN which is indicated by the field
     * <code>blockedPinId</code>.
     * The unblockingPinID field indicates which PIN is to be unblocked.
     * The user can either cancel the request
     * or continue. If the user enters the PIN values and chooses to continue,
     * the implementation is responsible
     * for presenting the PIN values to the card for unblocking the
     * blocked PIN.
     * If padding is required for either of the PIN values, the
     * implementation is responsible for providing appropriate padding.
     * @param blockedPinID the Id of PIN that is to be unblocked.
     * @param unblockingPinId the Id of unblocking PIN.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for unblocking the PIN.
     */
    public short unblockPin(int blockedPinID, int unblockingPinId)
    throws RemoteException {
        return doEnterPin(blockedPinID, unblockingPinId,
                ACLPermissions.CMD_UNBLOCK);
    }

    /**
     * Performs PIN entry operation.
     * @param pinID PIN identifier.
     * @param uPinID unblocking PIN identifier.
     * @param action PIN operation identifier.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for unblocking the PIN.
     */
    private short doEnterPin(int pinID, int uPinID,
                             int action) throws RemoteException {

        if (! connectionOpen) {
            throw new RemoteException("Connection is closed.");
        }

        String method = null;
        method = verifier.preparePIN(pinID, uPinID, action,
                                    internalReference.getClassName());
        Object[] pins = verifier.enterPIN(action, securityToken);

        if (pins == null) {
            return PINENTRY_CANCELLED;
        }

        try {
            Short r = (Short) invoke(internalReference, method, pins);
            return r.shortValue();
        } catch (Exception e) {
            throw new RemoteException("" + e);
        }
    }

    /*
     * Pass the APDU that will be sent to APDUManager.exchangeAPDU()
     * to the com.sun.kvem.io.j2me.apdu.Protocol
     */
    protected void netmonSaveApdu(byte[] APDUBuffer) {
        // empty. Overloaded in a derived class
    }

    /*
     * Pass the response that received from APDUManager.exchangeAPDU()
     * to the com.sun.kvem.io.j2me.apdu.Protocol
     */
    protected void netmonSaveResponse(byte[] response) {
        // empty. Overloaded in a derived class
    }
}
