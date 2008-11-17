/*
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
 */

package javax.microedition.io;

import java.io.*;
import com.sun.cdc.io.*;

/**
 * This class is a factory for creating new Connection objects.
 * <p>
 * The creation of Connections is performed dynamically by looking
 * up a protocol implementation class whose name is formed from the
 * <!-- platform name (read from a system property) and the --> protocol name
 * of the requested connection. <!-- (extracted from the parameter string
 * supplied by the application programmer.) -->
 *
 * The parameter string that describes the target should conform
 * to the URL format as described in RFC 2396.
 * This takes the general form:
 * <p>
 * <code>{scheme}:[{target}][{params}]</code>
 * <p>
 * where <code>{scheme}</code> is the name of a protocol such as
 * <i>http</i>.
 * <p>
 * The <code>{target}</code> is normally some kind of network
 * address.
 * <p>
 * Any <code>{params}</code> are formed as a series of equates
 * of the form ";x=y".  Example: ";type=a".
 * <p>
 * An optional second parameter may be specified to the open
 * function. This is a mode flag that indicates to the protocol
 * handler the intentions of the calling code. The options here
 * specify if the connection is going to be read (READ), written
 * (WRITE), or both (READ_WRITE). The validity of these flag
 * settings is protocol dependent. For instance, a connection
 * for a printer would not allow read access, and would throw
 * an IllegalArgumentException. If the mode parameter is not
 * specified, READ_WRITE is used by default.
 * <p>
 * An optional third parameter is a boolean flag that indicates
 * if the calling code can handle timeout exceptions. If this
 * flag is set, the protocol implementation may throw an
 * InterruptedIOException when it detects a timeout condition.
 * This flag is only a hint to the protocol handler, and it
 * does not guarantee that such exceptions will actually be thrown.
 * If this parameter is not set, no timeout exceptions will be
 * thrown.
 * <p>
 * Because connections are frequently opened just to gain access
 * to a specific input or output stream, four convenience
 * functions are provided for this purpose.
 *
 * See also: {@link DatagramConnection DatagramConnection}
 * for information relating to datagram addressing
 *
 * @version 12/17/01 (CLDC 1.1)
 * @since   CLDC 1.0
 */

public class Connector {
    /**
     * Access mode READ.
     */
    public final static int READ  = 1;

    /**
     * Access mode WRITE.
     */
    public final static int WRITE = 2;

    /**
     * Access mode READ_WRITE.
     */
    public final static int READ_WRITE = (READ|WRITE);

    /* Caches the MIDP connector. */
    private static InternalConnector midpConnector;

    /* Caches the Foundation connector. */
    private static InternalConnector foundationConnector;

    /**
     * Prevent instantiation of this class.
     */
    private Connector() { }

    /**
     * Create and open a Connection. In case of <code>file</code> protocol,
     * the file gets overwritten.
     *
     * @param name             The URL for the connection.
     * @return                 A new Connection object.
     *
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException If the target of the
     *   name cannot be found, or if the requested protocol type
     *   is not supported.
     * @exception IOException  If some other kind of I/O error occurs.
     * @exception SecurityException  May be thrown if access to the
     *   protocol handler is prohibited.
     */
    public static Connection open(String name) throws IOException {
        return open(name, READ_WRITE);
    }

    /**
     * Create and open a Connection. In case of <code>file</code> protocol,
     * the file gets overwritten for mode WRITE or READ_WRITE.
     *
     * @param name             The URL for the connection.
     * @param mode             The access mode.
     * @return                 A new Connection object.
     *
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException If the target of the
     *   name cannot be found, or if the requested protocol type
     *   is not supported.
     * @exception IOException  If some other kind of I/O error occurs.
     * @exception SecurityException  May be thrown if access to the
     *   protocol handler is prohibited.
     */
    public static Connection open(String name, int mode) throws IOException {
        return open(name, mode, false);
    }

    /**
     * Create and open a Connection. In case of <code>file</code> protocol,
     * the file gets overwritten for mode WRITE or READ_WRITE.
     *
     * @param name             The URL for the connection
     * @param mode             The access mode
     * @param timeouts         A flag to indicate that the caller
     *                         wants timeout exceptions
     * @return                 A new Connection object
     *
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException If the target of the
     *   name cannot be found, or if the requested protocol type
     *   is not supported.
     * @exception IOException  If some other kind of I/O error occurs.
     * @exception SecurityException  May be thrown if access to the
     *   protocol handler is prohibited.
     */
    public static Connection open(String name, int mode, boolean timeouts)
            throws IOException {
        InternalConnector ic = null;

        if (mode != READ && mode != WRITE && mode != READ_WRITE) {
          throw new IllegalArgumentException("illegal access mode: "+mode);
        }
        
        /* Test for null argument */
        if (name == null) {
            throw new IllegalArgumentException("Null URL");
        }

        if (name.equals("")) {
            throw new IllegalArgumentException("empty URL");
        }

        try {
            /*
             * This search is on every open so this class can used in VM's
             * running multiple apps of different types.
             */
            if (sun.misc.CVM.isMIDPContext()) {
                ic = getMidpConnector();
            } 
        } catch (Throwable t) {
            // Fall back to the Foundation profile Connector.
        } finally {
            if (ic == null) {
                ic = getFoundationConnector();
            }
        } 

        return ic.open(name, mode, timeouts);
    }

    /**
     * Create and open a connection input stream.
     *
     * @param  name            The URL for the connection.  Some possible URL
     *			       prefixes that can be used include: "comm:", 
     *			       "http:", "socket:", and "ssl:" when available
     *			       by the implementation and following the
     *			       respective connection specification.
     * @return                 A DataInputStream.
     *
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException If the target of the
     *   name cannot be found, or if the requested protocol type
     *   is not supported.
     * @exception IOException  If some other kind of I/O error occurs.
     * @exception SecurityException  May be thrown if access to the
     *   protocol handler is prohibited.
     */
    public static DataInputStream openDataInputStream(String name)
        throws IOException {

        InputConnection con = null;
        try {
            con = (InputConnection)Connector.open(name, Connector.READ);
        } catch (ClassCastException e) {
            throw new IllegalArgumentException(name);
        }

        try {
            return con.openDataInputStream();
        } finally {
            con.close();
        }
    }

    /**
     * Create and open a connection output stream.
     *
     * @param  name            The URL for the connection.  Some possible URL
     *			       prefixes that can be used include: "comm:", 
     *			       "http:", "socket:", and "ssl:" when available
     *			       by the implementation and following the
     *			       respective connection specification.
     * @return                 A DataOutputStream.
     *
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException If the target of the
     *   name cannot be found, or if the requested protocol type
     *   is not supported.
     * @exception IOException  If some other kind of I/O error occurs.
     * @exception SecurityException  May be thrown if access to the
     *   protocol handler is prohibited.
     */
    public static DataOutputStream openDataOutputStream(String name)
        throws IOException {

        OutputConnection con = null;
        try {
            con = (OutputConnection)Connector.open(name, Connector.WRITE);
        } catch (ClassCastException e) {
            throw new IllegalArgumentException(name);
        }

        try {
            return con.openDataOutputStream();
        } finally {
            con.close();
        }
    }

    /**
     * Create and open a connection input stream.
     *
     * @param  name            The URL for the connection.  Some possible URL
     *			       prefixes that can be used include: "comm:", 
     *			       "http:", "socket:", and "ssl:" when available
     *			       by the implementation and following the
     *			       respective connection specification.
     * @return                 An InputStream.
     *
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException If the target of the
     *   name cannot be found, or if the requested protocol type
     *   is not supported.
     * @exception IOException  If some other kind of I/O error occurs.
     * @exception SecurityException  May be thrown if access to the
     *   protocol handler is prohibited.
     */
    public static InputStream openInputStream(String name)
        throws IOException {

        return openDataInputStream(name);
    }

    /**
     * Create and open a connection output stream. In case of <code>file</code> protocol,
     * the file gets overwritten.
     *
     * @param  name            The URL for the connection.  Some possible URL
     *			       prefixes that can be used include: "comm:", 
     *			       "http:", "socket:", and "ssl:" when available
     *			       by the implementation and following the
     *			       respective connection specification.
     * @return                 An OutputStream.
     *
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException If the target of the
     *   name cannot be found, or if the requested protocol type
     *   is not supported.
     * @exception IOException  If some other kind of I/O error occurs.
     * @exception SecurityException  May be thrown if access to the
     *   protocol handler is prohibited.
     */
    public static OutputStream openOutputStream(String name)
        throws IOException {

        return openDataOutputStream(name);
    }

    private static InternalConnector getMidpConnector() throws Exception {
        if (midpConnector == null) {
            Class connectorClass =
                Class.forName("sun.misc.MIDPInternalConnectorImpl");

            midpConnector = (InternalConnector)connectorClass.newInstance();
        }

        return midpConnector;
    }

    private static InternalConnector getFoundationConnector() {
        if (foundationConnector == null) {
            foundationConnector = new InternalConnectorImpl();
        }

        return foundationConnector;
    }
}

