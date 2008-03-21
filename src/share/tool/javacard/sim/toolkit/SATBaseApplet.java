/*
 *   
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
 
package sim.toolkit;

import javacard.framework.*;
import sim.toolkit.*;

/**
 * This is a base class for all SAT applets. It provides shareable
 * interface, local APDU buffer creation, local instance of the Toolkit
 * Exception. The 'process' method should be never invoked and 
 * its overriding not needed.
 *
 * <p />Derived SAT applet can look like:
 * <pre>
 * public class SATApplet extends SATBaseApplet {
 *     private SATApplet() {
 *         ToolkitRegistry reg;
 *         register();
 *         reg = ToolkitRegistry.getEntry();
 *         reg.setEvent(EVENT_XXXXX);
 *     }
 *   
 *     public static void install(byte[] bArray, short bOffset, byte bLength) {
 *         new SATApplet();
 *     }
 *   
 *     public void processToolkit(byte event) {
 *         switch (event) {
 *             case EVENT_XXXXX:
 *                 break;
 *         }
 *     }
 * }
 * </pre>
 */
public abstract class SATBaseApplet extends Applet
    implements ToolkitInterface, ToolkitConstants {
                            
    /** APDU buffer */
    private byte[] apduBuffer = null;
    /** Shared instance of TookitException */
    private ToolkitException toolkitExceptionInstance = null;
    
    /**
     * Constructor
     */
    protected SATBaseApplet() {
    }

    /**
     * Returns shareable interface object to SAT Applet.
     *
     * @param clientAID - aid of SIM Framework
     * @param parameter
     * @return The current applet instance
     */
    public Shareable getShareableInterfaceObject(AID clientAID, 
                                                byte parameter) {
        return this;
    }
    
    /**
     * Returns the APDUBuffer. If it does not exist then allocates it.
     * @return apdu buffer
     * @throws ToolkitException if SATAccessor is unavailable
     */
    public byte[] getAPDUBuffer() throws ToolkitException {
        if (apduBuffer == null) {
            apduBuffer = new byte[ViewHandler.SATAccessor.getAPDUBufferMax()];
        }
        return apduBuffer;
    }
    
    /**
     * Returns the ToolkitException instance. 
     * If it does not exist yet then creates it.
     * @return ToolkitException instance
     */
    public ToolkitException getToolkitExceptionInstance() {
        if (toolkitExceptionInstance == null) {
            toolkitExceptionInstance = new ToolkitException((short)0);
        }
        return toolkitExceptionInstance;
    }

    
    /** 
     * Process method that is called by JCRE if this applet is selected.
     * @param apdu An APDU to process
     */
    public void process(APDU apdu) {
    }
    /**
     * Process method called by GSMApplet when an evelope is received.
     * @param event An event to be handled
     */
    public abstract void processToolkit(byte event);
}
