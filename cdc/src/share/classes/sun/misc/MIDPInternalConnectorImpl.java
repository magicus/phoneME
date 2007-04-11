package sun.misc;

import com.sun.cdc.io.* ;

public class MIDPInternalConnectorImpl extends InternalConnectorImpl {
     
    protected String getClassRoot() {
        if (classRoot != null) {
            return classRoot;
        }
        try {
             /*
              * Check to see if there is a property override for the dynamic
              * building of class root.
              */
            classRoot = System.getProperty("javax.microedition.io.Connector.protocolpath");
        } catch (Throwable t) {
            // do nothing
        }
        if (classRoot == null) {
            classRoot = "com.sun.midp.io";
        }
        
         return classRoot;
    }
    
    protected ClassLoader getProtocolClassLoader() {
        if (protocolClassLoader != null) {
            return protocolClassLoader;
        }
        
        protocolClassLoader = MIDPConfig.getMIDPImplementationClassLoader();
        return protocolClassLoader;
    }
}
