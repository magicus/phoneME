/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.cdc.io;

import javax.microedition.io.*;

import com.sun.cdc.io.*;
import java.io.IOException;

/**
 * This class provides access to the internal only connection
 * types needed by the implementations of the supported subsystems.
 * If bypasses the javax.microedition.
 */
public class InternalConnector {
    /*
     * Implementation note. the open parameter is used to form a class name in the form:
     * <p>
     * <code>com.sun.midp.io.{platform}.{protocol}.Protocol</code>  or
     * <code>com.sun.cdc.io.{platform}.{protocol}.Protocol</code>
     * <p>
     * The platform name is derived from the system by looking for the system property "j2me.platform".
     * If this property key is not found or the associated class is not present then one of two default
     * directories are used. These are called "j2me" and "j2se". If the property "j2me.configuration"
     * is non-null then "j2me" is used, otherwise "j2se" is assumed.
     * <p>
     * The system property "microedition.platform" can be used to change the root of the class space
     * used to lookup the classes.
     * <p>
     * The protocol name is derived from the parameter string describing the target of the
     * connection. This takes the from:
     * <p>
     * <code> {protocol}:[{target}][ {parms}] </code>
     * <p>
     * The protocol name is used to find the appropriate class and stripped from the target
     * name before being passed to open() method of a new installation of the class.
     */

    /**
     * The platform name
     */
    private static String platform;

    /**
     * True if we are running on a J2ME system
     */
    private static boolean j2me = true;

    /**
     * The roots of the Protocol classes.
     * Each is tried in turn by open*.
     */
    private static String midpRoot;
    private static String cdcRoot;

    /*
     * If enableAllProtocols is true, then open(...)
     * will allow applications to use non-MIDP protocols.
     */
    private static boolean enableAllProtocols;

    /**
     * Class initializer
     */
    static { 
        try {
            /* Work out if we are running on a J2ME system */
            if(System.getProperty("microedition.configuration") != null) {
               j2me = true;
            }

            /* Setup the platform name */
            platform = System.getProperty("microedition.platform");
            if (platform == null) {
                platform = j2me ? "j2me" : "j2se";
            }

            /* Setup the MIDP and CDC root prefixes. */
            midpRoot = "com.sun.midp.io." + platform;
            cdcRoot = "com.sun.cdc.io." + platform;
            
            /* Initialize enableAllProtocols from System property. */
            enableAllProtocols = (System.getProperty("ENABLE_CDC_PROTOCOLS") != null);
            // System.out.println("enableAllProtocols = " + enableAllProtocols);
        } catch ( java.security.AccessControlException e) {
             // If running with SecurityManager, set these defaults
             j2me = true;
             platform = "j2me";
             cdcRoot = "com.sun.cdc.io." + platform;
        }
    }

    /**
     * Prevent instantiation
     */
    private InternalConnector(){}

    /**
     * Create and open a Connection. This method is internal to
     * the MIPD implementation to allow
     * any of the MIDP and CDC protocols can be instantiated.
     *
     * @param string           The URL for the connection
     * @param mode             The access mode
     * @param timeouts         A flag to indicate that the called wants timeout exceptions
     * @return                 A new Connection object
     *
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException if the requested connection
     * cannot be make, or the protocol type does not exist.
     * @exception IOException  If some other kind of I/O error occurs.
     */
    public static Connection openInternal(String name, int mode, boolean timeouts) throws IOException {
	/* Lookup the protocol using the MIDP root. */
        try {
            return openPrim(name, mode, timeouts, midpRoot);
        } catch(ClassNotFoundException x ) {
        }

        /* Drop back to the default classes of CDC root */
        try {
            return openPrim(name, mode, timeouts, cdcRoot);
        } catch(ClassNotFoundException x ) {
        }

        throw new ConnectionNotFoundException("The requested protocol does not exist "+name);
    }

    /**
     * Create and open a Connection
     *
     * @param string           The URL for the connection
     * @param mode             The access mode
     * @param timeouts         A flag to indicate that the called wants timeout exceptions
     * @return                 A new Connection object
     *
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException if the requested connection
     * cannot be make, or the protocol type does not exist.
     * @exception IOException  If some other kind of I/O error occurs.
     */
    public static Connection open(String name, int mode, boolean timeouts) throws IOException {
	/* Lookup the protocol using the MIDP root. */
        try {
            return openPrim(name, mode, timeouts, midpRoot);
        } catch(ClassNotFoundException x ) {
        }

	if (enableAllProtocols) {
	    /* Drop back to the default classes of CDC root */
	    try {
		return openPrim(name, mode, timeouts, cdcRoot);
	    } catch(ClassNotFoundException x ) {
	    }
	}

        throw new ConnectionNotFoundException("The requested protocol does not exist "+name);
    }

    /**
     * Create and open a Connection
     *
     * @param string           The URL for the connection
     * @param mode             The access mode
     * @param timeouts         A flag to indicate that the called wants timeout exceptions
     * @return                 A new Connection object
     *
     * @exception ClassNotFoundException  If the protocol cannot be found.
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException If the connection cannot be found.
     * @exception IOException If some other kind of I/O error occurs.

     * @exception IOException  If an I/O error occurs

     * @exception IllegalArgumentException If a parameter is invalid
     */
    private static Connection openPrim(String name, int mode, boolean timeouts, String root)
                                     throws IOException, ClassNotFoundException {
        /* Test for null argument */
        if(name == null) {
            throw new IllegalArgumentException("Null URL");
        }

        /* Look for : as in "http:", "file:", or whatever */
        int colon = name.indexOf(':');

        /* Test for null argument */
        if(colon < 1) {
            throw new IllegalArgumentException("no ':' in URL");
        }

        try {
            String protocol;

            /* Strip off the protocol name */
            protocol = name.substring(0, colon);

            /* Strip the protocol name from the rest of the string */
            name = name.substring(colon+1);

            /* "file" is always an alias for "storage" */
            if(protocol.equals("storage")) {
                protocol = "file";
            }

            /* Using the platform and protocol names lookup a class to implement the connection */
	    // System.out.println("openPrim: " + root + "." + protocol + ".Protocol");
            Class clazz = Class.forName(root+"."+protocol+".Protocol");

            /* Construct a new instance */
            ConnectionBaseInterface uc = (ConnectionBaseInterface)clazz.newInstance();

            /* Open the connection, and return it */
            return uc.openPrim(name, mode, timeouts);

        } catch(InstantiationException x) {
            throw new IOException(x.toString());
        } catch(IllegalAccessException x) {
            throw new IOException(x.toString());
        } catch(ClassCastException x) {
            throw new IOException(x.toString());
        }
    }
}
