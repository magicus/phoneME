package com.sun.cdc.io;

import java.io.*;
import javax.microedition.io.*;

public class InternalConnectorImpl implements InternalConnector {
    protected ClassLoader protocolClassLoader;
    protected String classRoot;
    
    protected String getClassRoot() {
        if (classRoot != null) {
            return classRoot;
        }
        
        String profileTemp = null;
        
        try {
            /*
             * Check to see if there is a property override for the dynamic
             * building of class root.
             */
            classRoot =
                System.getProperty("javax.microedition.io.Connector.protocolpath");
        } catch (Throwable t) {
            // do nothing
        }
        
        if (classRoot == null) {
            classRoot = "com.sun.cdc.io";
        }
        
        return classRoot;
    }
    
    protected ClassLoader getProtocolClassLoader() {
        if (protocolClassLoader != null) {
            return protocolClassLoader;
        }
        
        protocolClassLoader = this.getClass().getClassLoader();
        return protocolClassLoader;
    }

  /**
     * Create and open a Connection.
     *
     * @param string           The URL for the connection
     * @param mode             The access mode
     * @param timeouts         A flag to indicate that the caller
     *                         wants timeout exceptions
     * @param platform         Platform name
     * @return                 A new Connection object
     *
     * @exception ClassNotFoundException  If the protocol cannot be found.
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException If the target of the
     *   name cannot be found, or if the requested protocol type
     *   is not supported.
     * @exception IOException If some other kind of I/O error occurs.
     * @exception IllegalArgumentException If a parameter is invalid.
     */

    public Connection open(String name, int mode, boolean timeouts) throws IOException {
        /* Test for null argument */
        if (name == null) {
            throw new IllegalArgumentException("Null URL");
        }
        
        /* Look for : as in "http:", "file:", or whatever */
        int colon = name.indexOf(':');

        /* Test for null argument */
        if (colon < 1) {
            throw new IllegalArgumentException("no ':' in URL");
        }
        try {
            String protocol;

            /* Strip off the protocol name */
            protocol = name.substring(0, colon);

            /* Strip off the rest of the string */
            name = name.substring(colon+1);

            /*
             * Convert all the '-' characters in the protocol
             * name to '_' characters (dashes are not allowed
             * in class names).  This operation creates garbage
             * only if the protocol name actually contains dashes
             */
            protocol = protocol.replace('-', '_');
            
            /*
             * Use the platform and protocol names to look up
                 * a class to implement the connection
                 */
            String className = getClassRoot() + "." + "j2me"+ "." + protocol +
                ".Protocol";
            Class clazz = Class.forName(className, true, getProtocolClassLoader());
            
            /* Construct a new instance of the protocol */
            ConnectionBaseInterface uc =
                (ConnectionBaseInterface)clazz.newInstance();
            
            /* Open the connection, and return it */
            return uc.openPrim(name, mode, timeouts);
        } catch (InstantiationException x) {
            throw new IOException(x.toString());
        } catch (IllegalAccessException x) {
            throw new IOException(x.toString());
        } catch (ClassCastException x) {
            throw new IOException(x.toString());
        } catch (ClassNotFoundException x) {
            throw new ConnectionNotFoundException(
                "The requested protocol does not exist " + name);
        }
    }
}

    
   
      
