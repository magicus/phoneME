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

//-----------------------------------------------------------------------------
// PACKAGE DEFINITION
//-----------------------------------------------------------------------------
package sim.toolkit;

//-----------------------------------------------------------------------------
// IMPORTS
//-----------------------------------------------------------------------------
import javacard.framework.*;

/**
 *
 * This class extends the Throwable class and allows the classes of this package
 * to throw specific exceptions in case of problems. <p>
 *
 * @version 8.3.0
 */
public class ToolkitException extends CardRuntimeException {

    // ------------------------------- Constants ------------------------------
    /**
     * This reason code (= 1) is used to indicate that data are to
     * large than the storage space available in the handler.  
     */
    public static final short HANDLER_OVERFLOW          = (short)1;

    /**
     * This reason code (= 2) is used to indicate that the Handler is
     * not available (e.g. busy).  
     */
    public static final short HANDLER_NOT_AVAILABLE     = (short)2;

    /**
     * This reason code (= 3) is used to indicate that the element is
     * unavailable in the handler buffer.  
     */
    public static final short UNAVAILABLE_ELEMENT       = (short)3;

    /**
     * This reason code (= 4) is used to indicate that the requested
     * menu entry is not define for the corresponding applet.  
     */
    public static final short MENU_ENTRY_NOT_FOUND      = (short)4;

    /**
     * This reason code (= 5) is used to indicate an error in the
     * applet registry.
     */
    public static final short REGISTRY_ERROR            = (short)5;

    /**
     * This reason code (= 6) is used to indicate that the event code
     * is not supported by the toolkit framework 
     */
    public static final short EVENT_NOT_SUPPORTED       = (short)6;

    /**
     * This reason code (= 7) is used to indicate that the maximum
     * number of registered applet for this event is already reached
     * (e.g Call Control) 
     */
    public static final short EVENT_ALREADY_REGISTERED  = (short)7;

    /**
     * This reason code (= 8) is used to indictae that either the
     * offset, the length or both are out of current TLV boundaries.
     */
    public static final short OUT_OF_TLV_BOUNDARIES     = (short)8;

    /**
     * This reason code (= 9) is used to indicate that the Terminal
     * Profile data are not available
     */
    public static final short ME_PROFILE_NOT_AVAILABLE  = (short)9;

    /**
     * This reason code (=10) is used to indicate that the provided
     * menu entry string is bigger than the space alloacted space.
     */
    public static final short ALLOWED_LENGTH_EXCEEDED 	= (short)10;

    /**
     * This reason code (=11) is used to indicate that all the
     * available timers or the maximum number of timers have been
     * allocated to the applet.
     */
    public static final short NO_TIMER_AVAILABLE        = (short)11;

    /**
     * This reason code (=12) is used to indicate that the indicated
     * timer identifier is not allocated to this applet.  
     */
    public static final short INVALID_TIMER_ID 	        = (short)12;

    /**
     * This reason code (=13) is used to indicate that the
     * registration to an indicated event can not be changed by the
     * called method.  
     */
    public static final short EVENT_NOT_ALLOWED         = (short)13;

    /**
     * This reason code (=14) is used to indicate that an input
     * parameter of the method is not valid.
     */
    public static final short BAD_INPUT_PARAMETER       = (short)14;
    
    // ------------------------------- Constructors ---------------------------
    /**
     * Construct a ToolkitException instance with the specified reason.
     *
     * @param reason the reason for the exception
     */
    public ToolkitException(short reason) {
        super(reason);
    }


    // ------------------------------- Public methods -------------------------
    /**
     * Throws the Toolkit Interface's instance of the 
     * <code>ToolkitException</code> class with the specified reason.
     *
     * @param reason the reason for the exception.
     *
     * @exception ToolkitException always
     */
    public static void throwIt(short reason) throws ToolkitException {
        ToolkitException systemInstance = 
            ViewHandler.currentTI.getToolkitExceptionInstance();
        systemInstance.setReason(reason);
        throw systemInstance;
    }
}
