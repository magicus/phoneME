/*
 * %W% %E%
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

package com.sun.jump.executive;

import com.sun.jump.common.JUMPProcess;
import com.sun.jump.messagequeue.JUMPIncomingQueue;
import com.sun.jump.messagequeue.JUMPMessage;
import com.sun.jump.messagequeue.JUMPOutgoingQueue;
import java.util.HashMap;
import java.util.Map;

/**
 * <code>JUMPExecutive</code> hosts all the JUMP executive services. 
 * The services are declratively specified 
 * in a file, which allows the executive to be extended with new 
 * services. 
 * 
 * @see com.sun.jump.module.JUMPModule
 */
public class JUMPExecutive implements JUMPProcess {
    private static JUMPExecutive INSTANCE = null;
    
    protected Map moduleFactoryMap = null;
    
    /**
     * Returns the singleton executive instance. If an alternate implementaion
     * should be returned, then a system property containing the classname of
     * the subclass of <code>JUMPExecutive</code> must be set. If such a
     * property is not set, then a default implementation will be provided.
     */ 
    public static synchronized JUMPExecutive getInstance() {
        if ( INSTANCE == null ) {
            // get the concrete executive classname from the environment
            // and instantiate that.
            String className = null;
            if ( className != null ) {
                try {
                    INSTANCE = (JUMPExecutive)
                        Class.forName(className).newInstance();
                    
                } catch (ClassNotFoundException ex) {
                    ex.printStackTrace();
                } catch (InstantiationException ex) {
                    ex.printStackTrace();
                } catch (IllegalAccessException ex) {
                    ex.printStackTrace();
                }
            }
            if ( INSTANCE == null ){
                INSTANCE = new JUMPExecutive();
            }
            INSTANCE.initialize();
        }
        return INSTANCE;
    }
    
    /**
     * Creates a new instance of JUMPExecutive
     */
    protected  JUMPExecutive() {
        this.moduleFactoryMap = new HashMap();
    }
    
    /**
     * Initialize the executive. This method initializes the list of 
     * modules, by reading the module definitions from a property file. This
     * method can be overriden to load the modules using some other 
     * mechanism, even possibly hardcoding the list of modules.
     */
    protected void initialize() {
    }

    public int getProcessId() {
        return 0;
    }

    public JUMPIncomingQueue getIncomingQueue() {
        return null;
    }

    public JUMPOutgoingQueue getOutgoingQueue() {
        return null;
    }

    public JUMPMessage newMessage(String mesgType, Object data) {
        return null;
    }

    public JUMPMessage newMessage(JUMPMessage requestMessage, 
        String mesgType, Object data) {
        return null;
    }
}
