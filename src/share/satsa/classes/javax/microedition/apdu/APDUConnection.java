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

package javax.microedition.apdu;

import java.io.*;
import javax.microedition.io.*;

/**
 * <p>
 * This interface defines the APDU (Application Protocol Data Unit)
 * connection. J2ME applications can use this connection
 * to communicate with applications on a smart card using APDU protocol.
 * ISO 7816-4 defines the APDU protocol as an application-level
 * protocol between
 * a smart card and an application on the device. There are two types
 * of APDU messages:
 * command APDUs and response APDUs. Command APDUs are sent to the
 * smart card by J2ME applications.
 * Response APDUs are the messages received from the smart cards. For
 * more information
 * on APDU protocol see the ISO 7816-4 specifications.
 * </p>
 * <p>
 * Also J2ME applications can use {@link #getATR() getATR} method in this 
 * interface 
 * to obtain information regarding an Answer To Reset (ATR) returned 
 * by the smart card on card reset and use the 
 * {@link #enterPin(int pinID) enterPin} 
 * method to
 * ask the user to enter PIN which is sent to the smart card for 
 * verification.
 * </p>
 * <p>
 * The methods of APDUConnection are not synchronized. The only method 
 * that can be called safely in another thread is close. When close is 
 * invoked on a connection that is executing in another thread, any 
 * pending I/O method MUST throw an InterruptedIOException. If an 
 * application terminates without calling close on the open connection, 
 * the implementation SHOULD perform the close operation automatically in 
 * order to recover resources, such as the logical channel. 
 * </p>
 * <h4>
 * Creating an APDU Connection
 * </h4>
 * <p>
 * An APDU connection is created by passing a generic connection URI
 * string with a card
 * application identifier (AID) and optionally the slot in which the
 * card is inserted,
 * to the {@link javax.microedition.io.Connector#open Connector.open} method. 
 * For example, the
 * connection string:</p>
 * <pre>
 * apdu:0;target=A0.0.0.67.4.7.1F.3.2C.3
 * </pre>
 * <p>indicates that the connection is to be established with a target
 * application having 
 * AID A0.0.0.67.4.7.1F.3.2C.3, which resides in
 * the smart card
 * inserted in the default slot; that is, slot number 0. If the slot number
 * is not specified, then the
 * default slot is assumed.
 * </p>
 *
 * <p>
 * Each APDU connection has a logical channel reserved exclusively
 * for it. That is, the channel
 * is dedicated to the J2ME application and the selected smart card
 * application until the
 * connection is closed. 
 * A smart card supporting logical channels allows the host device 
 * to open multiple logical channels to communicate with on-card applications.
 * Logical channels other than the basic channel may be closed when 
 * the connection is closed. Basic channel or channel 0 has to remain
 * open while the card is powered on.
 * <p>
 * Since the basic channel or channel 0 cannot be closed, the 
 * terminal should maintain its availability status. 
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
 * Communicating With Smart Card Applications
 * </h4>
 * <p>
 * Once an APDU connection is created, the J2ME application can use the
 *  {@link #exchangeAPDU(byte[]) exchangeAPDU}
 * method to send command APDUs and receive response APDUs to and from
 * the card. 
 * J2ME applications cannot use <code>exchangeAPDU</code>
 * to send card
 * application selection APDUs or channel management APDUs. Card
 * application selection is allowed only by calling <code>Connector.open</code>
 * method with the URI
 * string described above and logical channels management is defined by 
 * API functionality.
 * </p>
 * 
 * <p>
 * There may be several APDU connections open at the same time using 
 * different logical channels with the same card. However, since APDU 
 * protocol is synchronous, there can be no interleaving of command and 
 * their response APDUs across logical channels. Between the receipt of 
 * the command APDU and the sending of the response APDU to that command,
 * only one logical channel is active. 
 * For T=0 protocol, for case 4 and case 2 command APDUs the card may 
 * respond with '61 XX' or '6C XX'.  These special cases MUST be handled by 
 * the implementation in the following manner:
 * <ul>
 * <li>
 * '61 XX': The implementation MUST send GET RESPONSE to 
 * the card to get the response data before any other command is sent.
 * </li>
 * <li>
 * '6C XX': The implementation MUST resend the command after setting Le equal 
 * to XX received from the card before any other command is sent.
 * </li>
 * </ul>
 * In both the cases discussed above, the implementation MUST make sure that 
 * between sending the command APDU, receiving status word '61 XX' or 
 * '6C XX', and sending GET RESPONSE or resending the command APDU with Le 
 * set to XX respectively, there MUST not be any other APDU exchange on any 
 * logical channel with the card. In case the status word '61 XX' is 
 * received multiple times successively from the card, the implementation 
 * must accumulate
 * all the response data received from the card before returning it to the
 * J2ME application. J2ME applications MUST remain oblivious of 
 * the exchanges mentioned above and should only get the response received 
 * as a result of the above operations.
 * </p>
 * <h4>
 * Communicating With Multiple Smart Card Applications
 * </h4>
 * <p>
 * A J2ME application may connect and communicate with multiple smart card
 * applications interchangeably or have multiple connections with the
 * same card application, if the card application is multi-selectable. To
 * achieve this the
 * J2ME application can repeat the procedure mentioned above to create
 * corresponding connection objects.
 * </p>
 * <h4>
 * Closing an APDUConnection
 * </h4>
 * <p>
 * J2ME application can call 
 * <code>javax.microedition.io.Connection.close()</code> on an APDU 
 * connection to close the
 * connection. If the connection was established on channel other than
 * channel 0, this action closes the channel consequently deselecting
 * the card application and making the channel available for use 
 * by other applications. In case of channel 0, the channel is marked
 * as available for use by other applications. The application selected
 * on channel 0 is not deselected at the time the connection is closed
 * but remains selected until a new connection is established on
 * channel 0. 
 * </p>
 * <h3>Exceptions that can be Thrown During Connection Establishment</h3>
 * A <code>ConnectionNotFoundException</code> is thrown in any of the following
 * situations:
 * <p>
 * <ul>
 *    <li>if the card slot does not exist</li>
 *    <li>if the card is not inserted or powered on</li>
 *    <li>if the card application selection fails because
 *        the card application with the specified application
 *        identifier does not exist or because the card application
 *        refused selection</li>
 * </ul>
 * </p>
 * <p>If the card application selection fails because the J2ME application
 * is not allowed to access the application with the specified application
 * identifier a <code>SecurityException</code> is thrown.</p>
 *
 * <p>In cases such as when there is no logical channel available to
 * establish a connection, an <code>IOException</code>
 * will be thrown and a connection object will not be returned to the 
 * J2ME application.</p>
 * <p>
 * If a card is removed after the connection is established and then 
 * re-inserted,
 * the J2ME application will have to re-establish the connection and get a new
 * connection object. Any attempts to exchange APDUs using the connection
 * object created before removal of the card will result in 
 * <code>InterruptedIOException</code> being thrown.
 * </p>
 *
 * <h3>
 * BNF Format for Connector.open() string
 * </h3>
 * <p>
 *
 * The URI MUST conform to the BNF syntax specified below.  If the URI
 * does not conform to this syntax, an <code>IllegalArgumentException</code>
 * is thrown.
 * </p>
 * <table BORDER="1">
 * <tr>
 * <td>&lt;<em>APDU_connection_string</em>&gt; </td>
 * <td>::= "<strong>apdu:</strong>"&lt;<em>targetAddress</em>&gt;</td>
 * </tr>
 *
 * <tr>
 * <td>&lt;<em>targetAddress</em>&gt; </td>
 * <td>::= <i>[slot];target </i> </td>
 * </tr>
 *
 * <tr>
 * <td>&lt;<em>slot</em>&gt; </td>
 * <td>::= <i>smart card slot number.</i> (optional. Hexadecimal number 
 * identifying the smart card slot. Default slot assumed if left empty)
 * </td>
 * </tr>
 * <tr>
 * <td>&lt;<em>target</em>&gt; </td>
 * <td>::= "target="<i>&lt;<em>AID</em>&gt;|"SAT"</i> </td>
 * </tr>
 * <tr>
 * <td>&lt;<em>AID</em>&gt; </td>
 * <td>::= <i>&lt; 5 - 16 bytes &gt; </i>
 * <br> An AID (Application Identifier) uniquely identifies a smart card 
 * application. It is represented by 5 to 16 hexadecimal bytes where each
 * byte value is seperated by a ".".</td>
 * </tr>
 * 
 * </table>
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
 * <h3>
 * Support for (U)SIM Application Toolkit
 * </h3>
 * <p>
 * If an implementation allows J2ME applications to communicate with (U)SAT, 
 * support for communication with (U)SAT should be implemented as specified 
 * below.</p>
 * <p>
 * The APDUConnection can be used to communicate with (U)SAT applications on
 * channel 0.
 * The following sections describe the constraints and methods 
 * in supporting communicating with (U)SAT applications.
 * </p>
 * <h4>
 * Technical Constraints
 * </h4>
 * <ul>
 * <li>The operator domain has full and exclusive access to this connection.
 * </li>
 * <li>Only ENVELOPE APDUs may be sent. For all other APDUs  
 * <code>IllegalArgumentException</code> is thrown.</li>
 * <li>The class byte MUST be set by the implementation which will be either
 * A0 or 80 depending on whether the phone is running GSM or UMTS. The class
 * byte supplied by the J2ME application will be ignored.</li>
 * <li>In the case when (U)SIM responds with status word '9E XX' or '9F XX',
 * the behavior of APDUConnection would be the same as when
 * status word '61 XX' is received from the card.</li>
 * <li>In the case when (U)SIM responds with status word '62 XX' or '63 XX' 
 * the implementation MUST send GET RESPONSE to the card with Le set to '00' 
 * before any other command is sent. The implementation MUST make 
 * sure that between sending the ENVELOPE APDU, receiving status word '62 XX'
 * or '63 XX', and sending GET RESPONSE APDU with Le set to '00', there MUST 
 * not be any other APDU exchange on any logical channel with the card.</li>
 * <li> When the J2ME application sends an ENVELOPE APDU to the (U)SIM, the 
 * native application may be performing a proactive session. In this case the 
 * (U)SIM MUST manage the synchronization issue. The (U)SIM may respond
 * with status word '93 00' (SIM Application Toolkit is busy) when the
 * (U)SIM is performing another proactive session.</li>
 * </ul>
 * <h4>
 * Open Connection with (U)SIM, invoke, Close Connection 
 * </h4>
 * To communicate using (U)SAT commands, a J2ME application performs these 
 * steps:
 * <ul>
 * <li>The J2ME application establishes a connection by using the 
 * <code>Connector.open</code> method. To open a connection with a 
 * smart card using (U)SAT commands, the URI string passed to the 
 * <code>Connector.open</code> method MUST be the following:<br>
 * <br>
 * "apdu:&lt;slot&gt;;target=SAT"<br>
 * <br>
 * Where "apdu" is the protocol and slot is the card slot having
 * the (U)SIM. The value "SAT" for parameter "target" indicates
 * that the connection is to be established with (U)SAT.<br>
 * <br>
 * The implementation MUST use logical channel 0 even if this channel is 
 * occupied by other applications, and MUST not send a 
 * <code>MANAGE CHANNEL</code> or a <code>SELECT</code> by DF NAME command
 * to the (U)SIM.</li>
 * <li>The J2ME application invokes the <code>exchangeAPDU</code> method on 
 * <code>javax.microedition.apdu.APDUConnection</code> interface to send the 
 * ENVELOPE commands. The J2ME application MUST not initiate a proactive 
 * session since if a proactive session is started, it could cause
 * synchronization problems with other entities talking to the (U)SAT, such as
 * other J2ME applications or native applications.
 * If a proactive session is started by the (U)SAT application in 
 * response to an envelope sent by the J2ME application, it is the 
 * responsibility of the J2ME application to deal with it accordingly.<br>
 * The implementation MUST check the INS byte of the APDU 
 * sent by the J2ME application. If the APDU is not an ENVELOPE
 * command, the implementation throws an 
 * <code>IllegalArgumentException</code>. The
 * implementation MUST set the CLA byte corresponding to whether the phone 
 * is running GSM or UMTS.</li>
 * <li>To close the connection, the J2ME application invokes the close method
 * on javax.microedition.apdu.APDUConnection. The implementation MUST not send
 * a CLOSE CHANNEL to the (U)SIM.</li>
 * </ul>
 * <h3>
 * APDUConnection Example
 * </h3>
 *
 * <p>
 * The following example shows how an <code>APDUConnection</code>
 * can be used to access a smart card application.
 * </p>
 * <pre>
 * APDUConnection acn = null;
 * try{
 *     // Create an APDUConnection object
 *     acn = (APDUConnection)
 *           Connector.open("apdu:0;target=A0.0.0.67.4.7.1F.3.2C.3");
 * 
 *     // Send a command APDU and receive response APDU
 *     responseAPDU = acn.exchangeAPDU(commandAPDU);
 *      ...
 * } catch (IOException e) {
 *   ...
 * } finally {
 *   ...
 *   if(acn != null) {
 *      // Close connection
 *      acn.close();
 *   }
 *   ...
 * }
 * ...
 *</pre>
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


public interface APDUConnection extends Connection {
    /**
     * Exchanges an APDU command with a smart card application.
     * This method will put the channel number of the associated channel 
     * in the CLA byte of the command APDU. 
     * 
     * Communication with a smart card device is synchronous.
     * This method blocks until the response has been received
     * from the smart card application, or is interrupted.
     * The interruption could be due to the card being removed from
     * the slot or operation timeout.
     *
     * @param commandAPDU a byte encoded command for the smart card
     * application
     * @return a byte encoded response to the requested operation
     * @throws IllegalArgumentException if: 
     * <ul>
     *    <li><code>commandAPDU</code> parameter is null</li>
     *    <li><code>commandAPDU</code> contains a card application 
     *    selection APDU</li>
     *    <li><code>commandAPDU</code> contains a MANAGE CHANNEL</code> 
     *    command APDU</li>
     *    <li>if the channel associated with the connection object is 
     *    non-zero and the CLA byte has a value other than 0x0X, 0x8X, 
     *    0x9X or 0xAX</li>
     *    <li><code>commandAPDU</code> parameter contains a 
     *    malformed APDU</li>
     * </ul>
     * @throws InterruptedIOException can be thrown in any of these situations:
     * <ul>
     *    <li>if this <code>Connection</code>
     *        object is closed during the exchange 
     *        operation</li> 
     *     <li>if the card is removed after connection is established and 
     *         then reinserted, and an APDU exchange is attempted 
     *         without re-establishing the connection</li>
     * </ul>
     * @exception IOException is thrown if the operation was not
     * successful because of problems communicating with the card, 
     * or if the connection was already closed
     * @throws SecurityException if <code>commandAPDU</code> byte array
     * contains an APDU that the J2ME application is not allowed to send to
     * the card application.
     
    */
    byte[] exchangeAPDU(byte[] commandAPDU) throws IOException;
    
    /**
     * Returns the <code>Answer To Reset</code> (ATR) message sent 
     * by the smart card in response to the reset operation. ATR is 
     * received from the card when the card is powered up or when it
     * is reset. The implementation MUST detect the presence of the
     * card when this method is called and return the value of ATR
     * if the card is present and <code>null</code> if the card has
     * been removed.
     * @return the ATR message if the card is inserted, or <code>null</code>
     * if the card has been removed or the connection has been closed.
     */
    byte[] getATR();
    
    /**
     * This is a high-level method that lets the J2ME application
     * ask for the user PIN value for PIN verification purposes and
     * send it to the card.
     * A call to <code>enterPin</code> method pops up a UI that requests 
     * the PIN from the user. The pinID field indicates which PIN must be
     * requested from the user. The user can either cancel the request
     * or continue. If the user enters the PIN and chooses to continue the 
     * implementation is responsible
     * for presenting the PIN value to the card for verification. 
     * If padding is required for the PIN, the implementation is responsible
     * for providing appropriate padding.
     * @param pinID the type of PIN the implementation is supposed to prompt
     * the user to enter.
     * @return result of PIN verification which is the status word 
     * received from the smart card in the form of a byte array. This method
     * would return null if the user cancels the request.
     * @exception IOException is thrown if the PIN could not be communicated
     * with the card because the connection was 
     * closed before this method was called or because
     * of communication problems.
     * @throws InterruptedIOException can be thrown in any of these situations:
     * <ul>
     *    <li>if this <code>Connection</code>
     *        object is closed during the exchange 
     *        operation</li>
     *     <li>if the card is removed after connection is established and 
     *         then reinserted, and PIN entry is attempted 
     *         without re-establishing the connection</li>
     * </ul>
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for PIN verification.
     * @exception UnsupportedOperationException is thrown if the 
     * implementation does not support this method. 
     */
    byte[] enterPin(int pinID) throws IOException;
    

    /**
     * This is a high-level method that lets the J2ME application
     * ask the user for PIN values for changing the PIN and sending these
     * values to the card.
     * A call to <code>changePin</code> method pops up a UI that requests 
     * the user for an old or existing PIN value and the new PIN value 
     * to change the value of the PIN. The pinID field indicates which PIN is
     * to be changed. The user can 
     * either cancel the request
     * or continue. If the user enters the PIN values and chooses to
     * continue the
     * implementation is responsible
     * for presenting the PIN value to the card. 
     * If padding is required for the PIN, the implementation is responsible
     * for providing appropriate padding.
     * @param pinID the type of PIN the implementation is supposed to prompt
     * the user to change.
     * @return result of changing the PIN value which is the status word 
     * received from the smart card in the form of a byte array. This method
     * would return null if the user cancels the request.
     * @exception IOException is thrown if the PIN could not be communicated
     * with the card because the connection was 
     * closed before this method was called or because
     * of communication problems.
     * @throws InterruptedIOException can be thrown in any of these situations:
     * <ul>
     *    <li>if this <code>Connection</code>
     *        object is closed during the exchange 
     *        operation</li>
     *     <li>if the card is removed after connection is established and 
     *         then reinserted, and attempt is made to change PIN
     *         without re-establishing the connection</li>
     * </ul>
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for changing the PIN value.
     * @exception UnsupportedOperationException is thrown if the 
     * implementation does not support this method. 
     */
    byte [] changePin(int pinID) throws IOException;
    
    /**
     * This is a high-level method that lets the J2ME application
     * ask for the user PIN value for the PIN that is to be 
     * disabled and send it to the card.
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
     * @return result of disabling the PIN value which is the status word 
     * received from the smart card in the form of a byte array. This method
     * would return null if the user cancels the request.
     * @exception IOException is thrown if the PIN could not be communicated
     * with the card because the connection was 
     * closed before this method was called or because
     * of communication problems.
     * @throws InterruptedIOException can be thrown in any of these situations:
     * <ul>
     *    <li>if this <code>Connection</code>
     *        object is closed during the exchange 
     *        operation</li>
     *     <li>if the card is removed after connection is established and 
     *         then reinserted, and attempt is made to disable PIN 
     *         without re-establishing the connection</li>
     * </ul>
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for disabling the PIN.
     * @exception UnsupportedOperationException is thrown if the 
     * implementation does not support this method. 
     */
    byte [] disablePin(int pinID) throws IOException;
    
    /**
     * This is a high-level method that lets the J2ME application
     * ask for the user to enter the value for the PIN that is to 
     * be enabled and send it to the card.
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
     * @return result of enabling the PIN value which is the status word 
     * received from the smart card in the form of a byte array. This method
     * would return null if the user cancels the request.
     * @exception IOException is thrown if the PIN could not be communicated
     * with the card because the connection was 
     * closed before this method was called or because
     * of communication problems.
     * @throws InterruptedIOException can be thrown in any of these situations:
     * <ul>
     *    <li>if this <code>Connection</code>
     *        object is closed during the exchange 
     *        operation</li>
     *     <li>if the card is removed after connection is established and 
     *         then reinserted, and attempt is made to enable PIN
     *         without re-establishing the connection</li>
     * </ul>
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for enabling the PIN.
     * @exception UnsupportedOperationException is thrown if the 
     * implementation does not support this method. 
     */    
    byte [] enablePin(int pinID) throws IOException;
    
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
     * @return result of unblocking the PIN value which is the status word 
     * received from the smart card in the form of a byte array. This method
     * would return null if the user cancels the request.
     * @exception IOException is thrown if the PIN could not be communicated
     * with the card because the connection was 
     * closed before this method was called or because
     * of communication problems.
     * @throws InterruptedIOException can be thrown in any of these situations:
     * <ul>
     *    <li>if this <code>Connection</code>
     *        object is closed during the exchange 
     *        operation</li>
     *     <li>if the card is removed after connection is established and 
     *         then reinserted, and attempt is made to unblock PIN
     *         without re-establishing the connection</li>
     * </ul>
     * @exception SecurityException is thrown if the J2ME application does
     * not have appropriate rights to ask for unblocking the PIN.
     * @exception UnsupportedOperationException is thrown if the 
     * implementation does not support this method. 
     */
    byte [] unblockPin(int blockedPinID, int unblockingPinID) 
    throws IOException;
    
}
