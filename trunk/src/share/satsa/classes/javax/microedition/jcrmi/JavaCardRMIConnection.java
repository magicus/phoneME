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

package javax.microedition.jcrmi;

import javax.microedition.io.*;

/**
 * This interface defines the Java Card RMI connection which can be used
 * by J2ME applications to communicate with applications on a smart card
 * using Java Card RMI protocol.
 * <h4>
 * Creating a Java Card RMI Connection
 * </h4> <p>
 * Java Card RMI connection is created by passing a generic connection URI
 * string with a card
 * application identifier (AID) and optionally the slot in which the
 * card is inserted,
 * to the {@link javax.microedition.io.Connector#open Connector.open} method. 
 * For example, the
 * connection string:
 * <pre>
 * jcrmi:0;AID=A0.0.0.67.4.7.1F.3.2C.3
 * </pre>
 * indicates that the connection is to be established with
 * an application having the 
 * AID A0.0.0.67.4.7.1F.3.2C.3 and which resides in
 * the smart card
 * inserted in the default slot; that is, slot number 0. If the slot number
 * is not specified, then the
 * default slot is assumed.
 * </p>
 * <p>
 * Each Java Card RMI connection has a logical channel reserved
 * exclusively for it. That is, the channel
 * is dedicated to the J2ME application and the selected smart card
 * application until the
 * connection is closed. 
 * A smart card supporting logical channels allows the host device 
 * to open multiple logical channels to communicate with on-card applications.
 * Logical channel other than the basic channel must be closed when 
 * corresponding connection is closed. Basic channel or channel 0 must remain
 * open while the card is powered on.
 * <p>
 * Since the basic channel or channel 0 cannot be closed, the 
 * implementation should maintain its availability status. 
 * When a J2ME application asks for a new connection, the implementation 
 * would find out if channel 0 is in use by any application 
 * (native or J2ME application). If channel 0 is not in use, the 
 * implementation would acquire the channel 0 for communicating with the 
 * card application by setting the state of channel 0 to "IN USE". It would 
 * then select the desired application on channel 0. If the selection is 
 * successful, the newly created connection object would be returned to the 
 * J2ME application which can then use it to communicate with the card 
 * application.
 * If the card application selection fails or the J2ME application calls 
 * <code>close</code> on the connection object, the availability state of 
 * the basic channel will be set back to "AVAILABLE" meaning that the basic 
 * channel is available for use. 
 * </p>
 * <p>
 * When a J2ME application requests a connection to the card and channel 0 
 * is in use by some other application (native or J2ME application), the 
 * implementation sends a <code>MANAGE CHANNEL</code> command to the 
 * card requesting  a logical channel for the 
 * new connection. If there is a logical channel available, the card  
 * returns with the logical channel number which will be used by the new 
 * connection. The implementation would select the desired application on 
 * the newly allocated logical channel. If the selection is successful, 
 * the implementation returns the newly created connection object to 
 * the J2ME application which can then use it for communicating with the card 
 * application. If application selection fails or the J2ME application calls 
 * <code>close()</code> method to close the connection with 
 * the card application, the implementation will send a 
 * <code>MANAGE CHANNEL</code> command to
 * the card to close the channel. The channel will then be available for use by 
 * other applications.
 * </p>
 * <p>
 * In the case when there are no logical channels available or the
 * card does not
 * support logical channels, the card will return an error. An 
 * <code>IOException</code> will be thrown and no connection object will be 
 * returned to the J2ME application. 
 * </p>
 * <h4>
 * Invoking Methods on Remote Objects in Java Card
 * </h4>
 * <p>Once the Java Card RMI connection is created, the J2ME application
 * can obtain an initial remote reference object using
 * {@link #getInitialReference() getInitialReference}. Using this reference,
 * the application can invoke
 * methods of an initial remote object on the card and obtain other
 * references to remote objects.</p>
 *
 * <h4>
 * Communicating With Multiple Smart Card Applications
 * </h4>
 * <p>
 * A J2ME application may connect and communicate with multiple smart card
 * applications interchangeably. To achieve this the
 * J2ME application can repeat the procedure mentioned above to create
 * corresponding connection objects.
 * </p>
 * <h4>
 * Closing a JavaCardRMIConnection
 * </h4>
 * <p>A J2ME application can call 
 * <code>javax.microedition.io.Connection.close()</code> on the
 * connection object to terminate the connection and release the 
 * logical channel. The logical channel is free to be
 * used by other applications. If an application terminates without
 * closing an open connection, the implementation SHOULD perform the
 * close operation automatically.</p>
 *
 * <h4>
 * Exceptions that can be Thrown During Connection Establishment
 * </h4>
 * <p>If the card slot does not exist, or if the card
 * is not inserted or powered on, or if application selection is failed
 * a <code>ConnectionNotFoundException</code>
 * must be thrown.</p>
 * <p>If the J2ME application is not allowed to access the application
 * with the specified application identifier a
 * <code>SecurityException</code> is thrown.</p>
 * <p>If there is no logical channel available to
 * establish a connection, an <code>IOException</code>
 * must be thrown.</p>
 * <p>If initial remote reference object can not be created
 * a <code>RemoteException</code> must be thrown.</p>
 * <p>If a card is removed after the connection is established and then 
 * re-inserted,
 * the J2ME application must re-establish the connection and get a new
 * connection object. Any attempts to invoke remote method using the
 * connection object created before removal of the card will result in 
 * <code>RemoteException</code> being thrown.
 * </p>
 * <h3>
 * BNF Format for Connector.open() string
 * </h3>
 * <p>
 * The URI MUST conform to the BNF syntax specified below. If the URI
 * does not conform to this syntax, an <code>IllegalArgumentException</code>
 * is thrown.
 * </p>
 * <table BORDER="1">
 * <tr>
 * <td>&lt;<em>JCRMI_connection_string</em>&gt; </td>
 * <td>::= "<strong>jcrmi:</strong>"&lt;
 * <em>cardApplicationAddress</em>&gt; </td>
 * </tr>
 *
 * <tr>
 * <td>&lt;<em>cardApplicationAddress</em>&gt; </td>
 * <td>::= <i>[slot];&lt;<em>AID_string</em>&gt; </i> </td>
 * </tr>
 *
 * <tr>
 * <td>&lt;<em>slot</em>&gt; </td>
 * <td>::= <i>smart card slot.</i> (optional. Hexadecimal number 
 * identifying the smart card slot. Default slot assumed if left empty)
 * </td>
 * </tr>
 *
 * <tr>
 * <td>&lt;<em>AID_string</em>&gt; </td>
 * <td>::= "AID="<i>&lt;AID&gt;</i> </td>
 * </tr>
 *
 * <tr>
 * <td>&lt;<em>AID</em>&gt; </td>
 * <td>::= <i>&lt;5 - 16 bytes&gt; </i>
 * <br> An AID (Application Identifier) uniquely identifies a smart card 
 * application. It is represented by 5 to 16 hexadecimal bytes where each
 * byte value is seperated by a ".".</td>
 * </tr>
 * </table>
 * 
 * <h3>Smart Card Slot Discovery Mechanism</h3>
 * <p> J2ME devices may support a variable number of security elements
 * (usually smart card slots). Some security elements are permanently
 * attached to the device (e.g. a soldered chip), others are removable.
 * The removable security elements may be cold-swappable, requiring
 * the battery to be removed before the security element can be exchanged
 * (e.g. a MiniSIM slot hidden behind the battery). Other removable
 * security elements can be inserted and removed while the system is
 * running. (e.g. a hot-swappable smart card reader).    </p>
 * <p>
 * A system property is defined to indicate the names of the 
 * smart card slots. The property can be queried through the 
 * <code>System.getProperty()</code> method using the key 
 * <em>microedition.smartcardslots</em>. The value returned 
 * is a comma-separated list of the smart card slots which can
 * be used in the <code>Connector.open()</code> string to identify the 
 * specific smart card slot.
 * </p>
 * <p> 
 * If the platform includes a (U)SIM card, it MUST be in slot 0.    
 * </p>
 * <p> 
 * The logical slot names include the slot number and a descriptor 
 * indicating the type of the slot. For cold-swappable slots the 
 * letter 'C' is appended to the slot number. For hot-swappable slots
 * the letter 'H' is appended to the slot number. The slot descriptors (the
 * letter 'C' and 'H' appended to the slot number) cannot be passed as part
 * of the URI to open a connection to the smart card application. The J2ME
 * application MUST remove the descriptor from the logical slot name and
 * only use the slot number in URI to identify the specific smart card
 * slot. 
 * </p>
 * <p> A typical configuration for a cold-swappable SIM card and 
 * a hot-swappable removable card would be: </p>
 * <pre><code> <br> microedition.smartcardslots: 0C,1H<br></code></pre>
 * 
 * 
 *
 * <h3>
 * Example
 * </h3>
 *
 * <p>
 * The following example shows how a <code>JavaCardRMIConnection</code>
 * can be used to access a smart card application.
 * </p>
 * <pre>
 * try {
 *    JavaCardRMIConnection connection = (JavaCardRMIConnection)
 *       Connector.open("jcrmi:0;AID=A0.0.0.67.4.7.1F.3.2C.3");
 *
 *    Counter counter = (Counter) connection.getInitialReference();
 *    ...
 *    currentValue = counter.add(50);
 *    ...
 *    connection.close();
 * } catch (Exception e) {
 *    ...
 * }
 * </pre>
 * <h3>
 * Note regarding PIN-related methods
 * </h3>
 * <p>
 * A platform should implement the PIN entry UI in such a way that:
 * <ul>
 * <li>
 * the UI is distinguishable from a UI generated by external sources 
 * (for example J2ME applications)
 * </li>
 * <li>
 * external sources are not able to retrieve or insert PIN data
 * </li>
 * <li>
 * If the static access control mechanism is implemented, the PIN 
 * related methods MUST be implemented as specified in the static access
 * control mechanism. For further details see Appendix A (Recommended 
 * Security Element Access Control).
 * </li>
 * </ul>
 * </p>
 */

public interface JavaCardRMIConnection extends Connection {
    /**
     * This status is returned to the calling J2ME application
     * if the operation for PIN verification/change/disable/
     * enable/unblock was not successful because the user
     * cancelled the PIN entry request.
     */
    public static final short PINENTRY_CANCELLED = -1;

    /**
     * Returns the stub object for an initial remote reference.
     * @return the initial remote reference
     */
    java.rmi.Remote getInitialReference();
    

    /**
     * A call to <code>enterPin</code> method pops up a UI that requests 
     * the PIN
     * from the user. The pinID field indicates which PIN must be
     * requested from the user. The user can either cancel the request
     * or continue. If the user enters the PIN and chooses to continue,
     * The implementation is responsible for
     * presenting the PIN entered by the user to the card for verification. 
     * If padding is required for the PIN, the implementation is responsible
     * for providing appropriate padding.
     * @param pinID the type of PIN the implementation is suppose to prompt
     * the user to enter.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for PIN verification.
     * @exception UnsupportedOperationException is thrown if the 
     * implementation does not support this method. 
     */
    short enterPin(int pinID) throws java.rmi.RemoteException;
    

    /**
     * A call to <code>changePin</code> method pops up a UI that requests 
     * the user for an old or existing PIN value and the new PIN value to 
     * change the value of the PIN. The pinID field indicates which PIN is
     * to be changed. The user can either cancel the request
     * or continue. If the user enters the PIN values and chooses to continue
     * the implementation is responsible for presenting  
     * the old and new values of the PIN to the card.
     * If padding is required for the PIN, the implementation is responsible
     * for providing appropriate padding.
     * @param pinID the type of PIN the implementation is suppose to prompt
     * the user to change.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for changing the PIN value.
     * @exception UnsupportedOperationException is thrown if the 
     * implementation does not support this method. 
     */
    short changePin(int pinID) throws java.rmi.RemoteException;
    
    /**
     * A call to <code>disablePin</code> method pops up a UI that requests 
     * the user to enter the value for the PIN that is to be disabled. 
     * The pinID field 
     * indicates which PIN is to be disabled. The user can 
     * either cancel the request
     * or continue. If the user enters the PIN and chooses to continue the 
     * implementation is responsible
     * for presenting the PIN value to the card to disable PIN.
     * If padding is required for the PIN, the implementation is responsible
     * for providing appropriate padding.
     * @param pinID the type of PIN the implementation is required to prompt
     * the user to enter.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for disabling the PIN.
     * @exception UnsupportedOperationException is thrown if the 
     * implementation does not support this method. 
     */
    short disablePin(int pinID) throws java.rmi.RemoteException;
    
    /**
     * A call to <code>enablePin</code> method pops up a UI that requests 
     * the user to enter the value for the PIN that is to be enabled. 
     * The pinID field 
     * indicates which PIN is to be enabled. The user can 
     * either cancel the request
     * or continue. If the user enters the PIN and chooses to continue the 
     * implementation is responsible
     * for presenting the PIN value to the card for enabling the PIN.
     * If padding is required for the PIN, the implementation is responsible
     * for providing appropriate padding.
     * @param pinID the type of PIN the implementation is required to prompt
     * the user to enter.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for enabling the PIN.
     * @exception UnsupportedOperationException is thrown if the 
     * implementation does not support this method. 
     */    
    short enablePin(int pinID) throws java.rmi.RemoteException;
    
    
    /**
     * This is a high-level method that lets the J2ME application
     * ask the user to enter the value for an unblocking PIN,
     * and the new value for the blocked PIN and send
     * these to the card. 
     * A call to <code>unblockPin</code> method pops up a UI that requests 
     * the user to enter the value for the unblocking PIN and the 
     * new value for the blocked PIN.
     * The <code>unblockingPinID</code> field indicates which unblocking 
     * PIN is to be 
     * used to unblock the blocked PIN which is indicated by the field
     * <code>blockedPinID</code>.
     * The user can either cancel the request
     * or continue. If the user enters the PIN values and chooses to continue,
     * the implementation is responsible
     * for presenting the PIN values to the card for unblocking the 
     * blocked PIN.
     * If padding is required for either of the PIN values, the 
     * implementation is responsible for providing appropriate padding.
     * @param blockedPinID the ID of PIN that is to be unblocked.
     * @param unblockingPinID the ID of unblocking PIN.
     * @return PINENTRY_CANCELLED if the user cancelled the PIN entry
     * request or the value returned by the remote method.
     * @exception java.rmi.RemoteException is thrown if the PIN could
     * not be communicated to the card or an exception is thrown
     * by the card in response to the PIN entry.
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for unblocking the PIN.
     * @exception UnsupportedOperationException is thrown if the 
     * implementation does not support this method. 
     */
    short unblockPin(int blockedPinID, int unblockingPinID) 
    throws java.rmi.RemoteException;
    
}
