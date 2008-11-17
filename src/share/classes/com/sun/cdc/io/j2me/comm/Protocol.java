/*
 * @(#)Protocol.java	1.16 06/10/13
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
 */

package com.sun.cdc.io.j2me.comm;

import java.io.*;
import java.util.Vector;

import javax.microedition.io.*;

import com.sun.cdc.io.GeneralBase;
import com.sun.cdc.io.BufferedConnectionAdapter;
import sun.security.action.GetPropertyAction;

/**
 * This implements the comm port protocol.
 *
 * @version 3.1 5/11/2001
 */
public class Protocol extends BufferedConnectionAdapter 
    implements CommConnection {

    static File file;
    static FileInputStream fis;    
    static FileOutputStream fos;

    /** Native handle to the serial port. */
    private int handle = -1;

    /** Size of the read ahead buffer, default is 256. */
    protected static int bufferSize = 256;

    /* List of successfully opened serial ports */
    private static Vector openedPorts = new Vector();

    /* This object's device name */
    private String thisDeviceName = null;

    /* This object's device mode  */
    private int deviceMode = Connector.READ;

    /**
     * Class initializer
     */
    static {
        /* See if a read ahead / write behind buffer size has been specified */
	///        String size = Configuration.getProperty(
        ///                  "com.sun.midp.io.j2me.comm.buffersize");
	
	        String size = (String) java.security.AccessController.doPrivileged(
                          new GetPropertyAction("com.sun.cdc.io.j2me.comm.buffersize"));
        if (size != null) {
            try {
                bufferSize = Integer.parseInt(size);
            } catch (NumberFormatException ex) {}
        }

    }

    // From SerialMgr.h of the Palm api

    /** Bit flag: 1 stop bits. */
    private final static int serSettingsFlagStopBits1     = 0x00000000;
    /** Bit flag: 2 stop bits. */
    private final static int serSettingsFlagStopBits2     = 0x00000001;
    /** Bit flag: parity on. */
    private final static int serSettingsFlagParityOddM    = 0x00000002;
    /** Bit flag: parity even. */
    private final static int serSettingsFlagParityEvenM   = 0x00000004;
    /** Bit flag: RTS rcv flow control. */
    private final static int serSettingsFlagRTSAutoM      = 0x00000010;
    /** Bit flag: CTS xmit flow control. */
    private final static int serSettingsFlagCTSAutoM      = 0x00000020;
    /** Bit flag: 7 bits/char. */
    private final static int serSettingsFlagBitsPerChar7  = 0x00000080;
    /** Bit flag: 8 bits/char. */
    private final static int serSettingsFlagBitsPerChar8  = 0x000000C0;
    /** Bit per char. */
    private int bbc      = serSettingsFlagBitsPerChar8;
    /** Stop bits. */
    private int stop     = serSettingsFlagStopBits1;
    /** Parity. */
    private int parity   = 0;
    /** RTS. */
    private int rts      = serSettingsFlagRTSAutoM;
    /** CTS. */
    private int cts      = serSettingsFlagCTSAutoM;
    /** Baud rate. */
    private int baud     = 19200;
    /** Blocking. */
    private boolean blocking = true;

    /** True if the permissions have been checked. */
    private boolean permissionChecked;

    /** Protocol to use when asking permissions. */
    private static final String protocol = "comm";

    /** Creates a buffered comm port connection. */
    public Protocol() {
        // use the default buffer size
        super(bufferSize);
    }

    /**
     * Check for the required permission.
     *
     * @param token security token of the calling class or null
     * @param name the URL string without the protocol for prompting
     *
     * @exception SecurityException if the permission is not available
     * @exception InterruptedIOException if I/O associated with permissions is interrupted
     */
    
    /**
     * Parse the next parameter out of a string.
     *
     * @param parm a string containing one or more parameters
     * @param start where in the string to start parsing
     * @param end where in the string to stop parsing
     *
     * @exception IllegalArgumentException if the next parameter is wrong
     */
    private void parseParameter(String parm, int start, int end) {
        parm = parm.substring(start, end);

        if (parm.equals("baudrate=110")) {
            baud = 110;
        } else if (parm.equals("baudrate=300")) {
            baud = 300;
        } else if (parm.equals("baudrate=600")) {
            baud = 600;
        } else if (parm.equals("baudrate=1200")) {
            baud = 1200;
        } else if (parm.equals("baudrate=2400")) {
            baud = 2400;
        } else if (parm.equals("baudrate=4800")) {
            baud = 4800;
        } else if (parm.equals("baudrate=9600")) {
            baud = 9600;
        } else if (parm.equals("baudrate=14400")) {
            baud = 14400;
        } else if (parm.equals("baudrate=19200")) {
            baud = 19200;
        } else if (parm.equals("baudrate=38400")) {
            baud = 38400;
        } else if (parm.equals("baudrate=56000")) {
            baud = 56000;
        } else if (parm.equals("baudrate=57600")) {
            baud = 57600;
        } else if (parm.equals("baudrate=115200")) {
            baud = 115200;
        } else if (parm.equals("baudrate=128000")) {
            baud = 128000;
        } else if (parm.equals("baudrate=256000")) {
            baud = 256000;
        } else if (parm.equals("bitsperchar=7")) {
            bbc   = serSettingsFlagBitsPerChar7;
        } else if (parm.equals("bitsperchar=8")) {
            bbc   = serSettingsFlagBitsPerChar8;
        } else if (parm.equals("stopbits=1")) {
            stop   = serSettingsFlagStopBits1;
        } else if (parm.equals("stopbits=2")) {
            stop   = serSettingsFlagStopBits2;
        } else if (parm.equals("parity=none")) {
            parity = 0;
        } else if (parm.equals("parity=odd")) {
            parity = serSettingsFlagParityOddM;
        } else if (parm.equals("parity=even")) {
            parity = serSettingsFlagParityEvenM;
        } else if (parm.equals("autorts=off")) {
            rts = 0;
        } else if (parm.equals("autorts=on")) {
            rts = serSettingsFlagRTSAutoM;
        } else if (parm.equals("autocts=off")) {
            cts = 0;
        } else if (parm.equals("autocts=on")) {
            cts = serSettingsFlagCTSAutoM;
        } else if (parm.equals("blocking=off")) {
            blocking = false;
        } else if (parm.equals("blocking=on")) {
            blocking = true;
        } else {
            throw new IllegalArgumentException("Bad parameter");
        }
    }

    /*
     * Throws SecurityException if permission check fails.
     * Will be overriden by MIDP version of the protocol
     * handler.
     */
    protected void checkPermission(String name) {
        java.lang.SecurityManager sm = System.getSecurityManager();
        if (sm != null) {
            if (deviceMode == Connector.READ) {
                sm.checkRead(name);
            } else {
                sm.checkWrite(name);
            }
        }
        return;
    }

    /*
     * Check permission when opening an OutputStream. MIDP
     * versions of the protocol handler should override this
     * with an empty method. Throw a SecurityException if
     * the connection is not allowed. Currently the comm
     * protocol handler does not make a permission check at
     * this point so this method is empty.
     */
    protected void outputStreamPermissionCheck() {
        return;
    }

    /*
     * Check permission when opening an InputStream. MIDP
     * versions of the protocol handler should override this
     * with an empty method. A SecurityException will be
     * raised if the connection is not allowed. Currently the
     * comm protocol handler does not make a permission
     * check at this point so this method is empty.
     */
    protected void inputStreamPermissionCheck() {
        return;
    }

    /**
     * Open a serial port connection.
     * 
     * Note: DTR line is always on. <br>
     * Hint: On Solaris opening by port number or /dev/term/* will block
     *       until the Data Set Ready line is On. To work around this open
     *       by device name using /dev/cua/* as root.
     *
     * @param name A URI with the type and parameters for the connection.
     * <pre>
     * The scheme must be: comm
     *
     * The first parameter must be a port ID: A device name or
     * a logical port number from 0 to 9.
     *
     * Any additional parameters must be separated by a ";" and
     * spaces are not allowed.
     *
     * The optional parameters are:
     *
     * baudrate:    The speed of the port, defaults to 19200.
     *              bitsperchar: The number bits that character is. 7 or 8.
     *              Defaults to 8.
     * stopbits:    The number of stop bits per char. 1 or 2.
     *              Defaults to 1.
     * parity:      The parity can be "odd", "even", or "none".
     *              Defaults to "none".
     * blocking:    If "on" wait for a full buffer when reading.
     *              Defaults to "on".
     * autocts:     If "on", wait for the CTS line to be on
     *              before writing. Defaults to "on".
     *              autorts:     If "on", turn on the RTS line when the
     *              input buffer is not full. If "off",
     *              the RTS line is always on.
     *              Defaults to "on".
     * </pre>
     * @param mode       A flag that is <code>true</code> if the caller expects
     *                   to write to the connection. This is ignored
     *                   in all connections that are read-write.
     * @param timeouts   A flag to indicate that the called wants
     *                   timeout exceptions. This is ignored.
     *
     * @exception  IOException  if an I/O error occurs, or
     *             IllegalArgumentException
     *             if the name string is has an error.
     */
    public void connect(String name, int mode, boolean timeouts)
            throws IOException {

        int portNumber = 0;
        String deviceName = null;
        int start = 0;
        int pos = 0;
	deviceMode = mode;

        if (name.length() == 0) {
             throw new IllegalArgumentException("Missing port ID");
        }

        if (Character.isDigit(name.charAt(0))) {
            portNumber = Integer.parseInt(name.substring(0, 1));
            pos++;
        } else {
            pos = name.indexOf(";");
            if (pos < 0) {
                deviceName = name;
                pos = name.length();
            } else {
                deviceName = name.substring(0, pos);
            }
        }

        while (name.length() > pos) {
            if (name.charAt(pos) != ';') {
                throw new IllegalArgumentException(
                    "missing parameter delimiter");
            }

            pos++;
            start = pos;
            while (true) {
                if (pos == name.length()) {
                    parseParameter(name, start, pos);
                    break;
                }

                if (name.charAt(pos) == ';') {
                    parseParameter(name, start, pos);
                    break;
                }
                pos++;
            }
        }

        // blocking is handled at the Java layer so other Java threads can run

        if (deviceName != null) {
            checkPermission(deviceName);
	    /* 6231661: before checking if port is already open, 
	       check if no open Streams (ensureNoStreamsOpen).  This is 
	       done to throw the correct exception: IOException when 
	       open Streams exist */
	    ensureNoStreamsOpen();

            /* 6227981: before opening, check to see if the port is already 
	       opened" */
            if (openedPorts.contains(deviceName)) {
                throw new IOException("Connection already open");
            }
	    handle = native_openByName(deviceName, baud,
				       bbc|stop|parity|rts|cts);
        } else {
            checkPermission("comm:" + portNumber);
	    handle = native_openByNumber(portNumber, baud,
					 bbc|stop|parity|rts|cts);
        }
        /* 6227970: if open fails throw an IOException */
        if (handle < 0) {
            throw new IOException("Could not open connection");
        }
        openedPorts.addElement(deviceName);
	thisDeviceName = new String(deviceName);
        registerCleanup();
    }

    /** 
     * Gets the baudrate for the serial port connection.
     * @return the baudrate of the connection
     * @see #setBaudRate
     */
    public int getBaudRate() {
	return baud;
    }

    /** 
     * Sets the baudrate for the serial port connection.
     * If the requested <code>baudrate</code> is not supported 
     * on the platform, then the system MAY use an alternate valid setting.
     * The alternate value can be accessed using the 
     * <code>getBaudRate</code> method.
     * @param baudrate the baudrate for the connection
     * @return the previous baudrate of the connection
     * @see #getBaudRate
     */
    public int setBaudRate(int baudrate) {
	int temp = baud;

	/*
	 * If the baudrate is not supported, select one
	 * that is allowed.
	 */
	if (baudrate < 299) {
            baudrate = 110;
        } else if (baudrate < 599) {
            baudrate = 300;
        } else if (baudrate < 1199) {
            baudrate = 600;
        } else if (baudrate < 2399) {
            baudrate = 1200;
        } else if (baudrate < 4799) {
            baudrate = 2400;
        } else if (baudrate < 9599) {
            baudrate = 4800;
        } else if (baudrate < 14399) {
            baudrate = 9600;
        } else if (baudrate < 19199) {
            baudrate = 14400;
        } else if (baudrate < 38399) {
            baudrate = 19200;
        } else if (baudrate < 55999) {
            baudrate = 38400;
        } else if (baudrate < 57599) {
            baudrate = 56000;
        } else if (baudrate < 115199) {
            baudrate = 57600;
        } else if (baudrate < 127999) {
            baudrate = 115200;
        } else if (baudrate < 255999) {
            baudrate = 128000;
        } else {
            baudrate = 256000;
	}
	try {
	    /* Set the new baudrate. */ 
	    ///*	    native_configurePort(handle, baudrate,
	    ///*				 bbc|stop|parity|rts|cts);
	    native_configurePort(handle, baudrate,
			       bbc|stop|parity|rts|cts);
	    /* If successful, update the local baud variable. */
	    baud = baudrate;
	} catch (IOException ioe) {
	    // NYI - could not set baudrate as requested.
	}
	return temp;
    }

    /**
     * Override close the GCF connection
     *
     *
     * @exception  IOException  if an I/O error occurs when closing the
     *                          connection.
     */
    public void close() throws IOException {
	if ((thisDeviceName != null) && 
	    (openedPorts.contains(thisDeviceName))) {
	    openedPorts.remove(thisDeviceName);
	}
	super.close();
    }
      
    /**
     * Close the native serial port.
     *
     * @exception  IOException  if an I/O error occurs.
     */
    protected void disconnect() throws IOException {
	try {
	    ///*	    native_close(handle);
	    native_close(handle);
	} finally {
	    /* Reset handle to prevent resgistered cleanup close. */
	    handle = -1;
	}
    }

    /**
     * Reads up to <code>len</code> bytes of data from the input stream into
     * an array of bytes, blocks until at least one byte is available,
     * if blocking is turned on.
     * Sets the <code>eof</code> field of the connection when the native read
     * returns -1.
     * <p>
     * Polling the native code is done here to avoid the need for
     * asynchronous native methods to be written. Not all implementations
     * work this way (they block in the native code) but the same
     * Java code works for both.
     *
     * @param      b     the buffer into which the data is read
     * @param      off   the start offset in array <code>b</code>
     *                   at which the data is written
     * @param      len   the maximum number of bytes to read
     * @return     the total number of bytes read into the buffer, or
     *             <code>-1</code> if there is no more data because the end of
     *             the stream has been reached
     * @exception  IOException  if an I/O error occurs
     */
    protected int nonBufferedRead(byte b[], int off, int len)
        throws IOException {

        int bytesRead = 0;

        try {
            if (b == null) {
                int chunk = 256;
                b = new byte[chunk];
                int end = off + len;
                int tmp = chunk;
                for (; off < end && tmp == chunk; off += chunk) {
                    if (off + chunk > end) {
                        chunk = end - off;
                    }
                    tmp = native_readBytes(handle, b, 0, chunk);
                    if (tmp > 0) {
                        bytesRead += tmp;
                    }
                }
                if (tmp < 0) {
                    eof = true;
                }
            } else {
                bytesRead = native_readBytes(handle, b, off, len);
            }
        } finally {
            if (iStreams == 0) {
                throw new InterruptedIOException("Stream closed");
            }
        }

        if (bytesRead == -1) {
            eof = true;
        }

        ///            GeneralBase.iowait(); 
        return(bytesRead);
    }

    /**
     * Reads up to <code>len</code> bytes of data from the input stream into
     * an array of bytes, but does not block if no bytes available.
     * Sets the <code>eof</code> flag if the end of stream is reached.
     * <p>
     * This is implemented so the <code>available</code> method of
     * <code>BufferedConnectionBaseAdapter</code> will return more than
     * zero if the buffer is empty.
     *
     * @param      b     the buffer into which the data is read
     * @param      off   the start offset in array <code>b</code>
     *                   at which the data is written
     * @param      len   the maximum number of bytes to read
     * @return     the total number of bytes read into the buffer, or
     *             <code>-1</code> if there is no more data because the end of
     *             the stream has been reached
     * @exception  IOException  if an I/O error occurs
     */
    protected int readBytesNonBlocking(byte b[], int off, int len)
            throws IOException {
        int bytesRead;

        try {
            // the native read does not block
	    ///*            bytesRead = native_readBytes(handle, b, off, len);
	    bytesRead = native_readBytes(handle, b, off, len);
        } finally {
            if (iStreams == 0) {
                throw new InterruptedIOException("Stream closed");
            }
        }

        if (bytesRead == -1) {
            eof = true;
        }

        return bytesRead;
    }

    /**
     * Writes <code>len</code> bytes from the specified byte array
     * starting at offset <code>off</code> to this output stream.
     * <p>
     * Polling the native code is in the stream object handed out by
     * our parent helper class. This done to avoid the need for
     * asynchronous native methods to be written. Not all implementations
     * work this way (they block in the native code) but the same
     * Java code works for both.
     *
     * @param      b     the data
     * @param      off   the start offset in the data
     * @param      len   the number of bytes to write
     *
     * @return     number of bytes written
     * @exception  IOException  if an I/O error occurs. In particular,
     *             an <code>IOException</code> is thrown if the output
     *             stream is closed.
     */
    public int writeBytes(byte b[], int off, int len) throws IOException {
	///* return native_writeBytes(handle, b, off, len);
        return native_writeBytes(handle, b, off, len);
    }

    /*
     * Real primitive methods
     */

    /**
     * Open a serial port by logical number.
     *
     * @param port logical number of the port 0 being the first
     * @param baud baud rate to set the port at
     * @param flags options for the serial port
     *
     * @return handle to a native serial port
     *
     * @exception  IOException  if an I/O error occurs.
     */
    private static native int native_openByNumber(int port, int baud,
						  int flags) 
	throws IOException;

    /**
     * Open a serial port by system dependent device name.
     *
     * @param name device name of the port
     * @param baud baud rate to set the port at
     * @param flags options for the serial port
     *
     * @return handle to a native serial port
     *
     * @exception  IOException  if an I/O error occurs.
     */
    private static native int native_openByName(String name, int baud,
        int flags) throws IOException;

    /**
     * Configure a serial port optional parameters.
     *
     * @param port device port returned from open
     * @param baud baud rate to set the port at
     * @param flags options for the serial port
     *
     * @exception  IOException  if an I/O error occurs
     */
    private static native void native_configurePort(int port, int baud,
						    int flags) 
	throws IOException;

    /**
     * Close a serial port.
     *
     * @param hPort handle to a native serial port
     *
     * @exception  IOException  if an I/O error occurs
     */
    private static native void native_close(int hPort) throws IOException;


    /** Register this object's native cleanup function. */
    private native void registerCleanup();

    /**
     * Read from a serial port without blocking.
     *
     * @param hPort handle to a native serial port
     * @param b I/O buffer
     * @param off starting offset for data
     * @param len length of data
     *
     * @return number of bytes read
     *
     * @exception  IOException  if an I/O error occurs
     */
    private static native int native_readBytes(int hPort, byte b[], int off,
        int len) throws IOException;

    /**
     * Write to a serial port without blocking.
     *
     * @param hPort handle to a native serial port
     * @param b I/O buffer
     * @param off starting offset for data
     * @param len length of data
     *
     * @return number of bytes that were written
     *
     * @exception  IOException  if an I/O error occurs.
     */
    private static native int native_writeBytes(int hPort, byte b[], int off, 
						int len) throws IOException;

}
