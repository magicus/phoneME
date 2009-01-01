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
 *
 */

package sun.misc;

import com.sun.cdc.io.* ;

public class MIDPInternalConnectorImpl extends InternalConnectorImpl {
    /*
     * Override the Foundation Profile class root with "com.sun.midp.io"
     * or the value of the javax.microedition.io.Connector.protocolpath
     * property.
     *
     * @return class root
     */
    protected String getClassRoot() {
        if (classRoot != null) {
            return classRoot;
        }

        try {
             /*
              * Check to see if there is a property override for the dynamic
              * building of class root.
              */
            classRoot = System.getProperty(
                        "javax.microedition.io.Connector.protocolpath");
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
