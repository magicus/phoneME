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
import javacard.framework.Shareable;


/**
 * This interface must be implemented by a Toolkit applet (which extends 
 * the <code>javacard.framework.Applet</code> class) so that it can be 
 * triggered by the Toolkit Handler according to the registration information.
 * The Toolkit applet will have to implement the processToolkit shared method
 * so that it can be notified of the following events :
 *
 * EVENT_PROFILE_DOWNLOAD 
 * EVENT_FORMATTED_SMS_PP_ENV 
 * EVENT_FORMATTED_SMS_PP_UPD 
 * EVENT_UNFORMATTED_SMS_PP_ENV 
 * EVENT_UNFORMATTED_SMS_PP_UPD 
 * EVENT_UNFORMATTED_SMS_CB 
 * EVENT_MENU_SELECTION 
 * EVENT_MENU_SELECTION_HELP_REQUEST 
 * EVENT_CALL_CONTROL_BY_SIM 
 * EVENT_MO_SHORT_MESSAGE_CONTROL_BY_SIM 
 * EVENT_TIMER_EXPIRATION 
 * EVENT_EVENT_DOWNLOAD_MT_CALL 
 * EVENT_EVENT_DOWNLOAD_CALL_CONNECTED 
 * EVENT_EVENT_DOWNLOAD_CALL_DISCONNECTED 
 * EVENT_EVENT_DOWNLOAD_LOCATION_STATUS 
 * EVENT_EVENT_DOWNLOAD_USER_ACTIVITY 
 * EVENT_EVENT_DOWNLOAD_IDLE_SCREEN_AVAILABLE
 * EVENT_EVENT_DOWNLOAD_CARD_READER_STATUS 
 * EVENT_STATUS_COMMAND 
 * EVENT_UNRECOGNIZED_ENVELOPE 
 * 
 *
 * Toolkit applet example :<pre><code>
 * import javacard.framework.*;
 * import sim.toolkit.*;
 * //
 * // The HelloWorld class is a simple Toolkit applet, which may be used as an
 * // example to show how such an applet will be installed and registered and
 * // how it will be triggered.
 * //
 * public class HelloWorld extends Applet implements 
 *                                        ToolkitInterface,ToolkitConstants {
 *     // data fields
 *     private static final byte CMD_QUALIFIER = (byte)0x80;
 *     private byte[] menuEntry = {'S','e','r','v','i','c','e','1'};
 *     private byte[] textBuf = {'H','e','l','l','o',' ',
 *                                'w','o','r','l','d',' ','!'};
 *     private ToolkitRegistry reg;
 *     //
 *     // Constructor of applet
 *     //
 *     public HelloWorld() {
 *         // get a Registry object...
 *         // ...and initialize it according to the applet characteristics
 *         reg.initMenuEntry(menuEntry, (short)0, (short)menuEntry.length, 
 *                           PRO_CMD_DISPLAY_TEXT, false, 0, 0);
 *     }
 *     //
 *     // Install method
 *     // *param bArray the array containing installation parameters
 *     // *param bOffset the starting offset in bArray
 *     // *param bLength the length in bytes of the parameter data in bArray
 *     //
 *     public static void install(byte bArray[], short bOffset, byte bLength) 
 *                                throws ISOException {
 *         // create and register applet
 *         HelloWorld HelloWorldApplet = new HelloWorld();
 *         HelloWorldApplet.register();
 *     }
 *     //
 *     // Process toolkit events
 *     // *param event the type of event to be processed
 *     // *exception ToolkitException
 *     //
 *     public void processToolkit(byte event) throws ToolkitException {
 *         // get the ProactiveHandler system instance
 *         ProactiveHandler proHdlr = ProactiveHandler.getTheHandler();
 *
 *         if (event == EVENT_MENU_SELECTION) {
 *             // prepare a Display Text command
 *             proHdlr.init((byte) PRO_CMD_DISPLAY_TEXT, 
 *                          (byte)CMD_QUALIFIER, (byte)0x02);
 *             proHdlr.appendTLV((byte)(TAG_TEXT_STRINGTAG_SET_CR), textBuf,
 *                              (short)0, (short)textBuf.length);
 *             proHdlr.send();
 *         }
 *     }
 * }
 * </code></pre>
 *
 * @version 8.3.0
 *
 * @see ToolkitRegistry
 * @see ToolkitException
 * @see ToolkitConstants
 */
public interface ToolkitInterface extends Shareable {


    // ------------------------------- Public methods -------------------------

    /** 
     * This method is the standard toolkit event handling method of a Toolkit 
     * applet and 
     * is called by the Toolkit Handler to process the current Toolkit event. 
     * This method is invoked for notification of registered events.
     * 
     * @param event the type of event to be processed. 
     *
     * @exception ToolkitException
     *
     * @see sim.toolkit.ToolkitRegistry#getEntry() 
     */ 
    public void processToolkit(byte event) throws ToolkitException; 
    
    /**
     * Returns the APDUBuffer.
     * @return apdu buffer
     * @throws ToolkitException if an error occures
     */
    public byte[] getAPDUBuffer() throws ToolkitException;
    
    /**
     * Returns the ToolkitException instance. 
     * @return ToolkitException instance
     */
    public ToolkitException getToolkitExceptionInstance();
}
