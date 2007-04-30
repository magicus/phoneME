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

package javax.wireless.messaging;

/**
 * This is the base interface for derived interfaces
 * that represent various types of messages. This package is
 * designed to work with <code>Message</code> objects that
 * may contain different elements depending on the underlying
 * messaging protocol. This is different from <code>Datagram</code>s that
 * are assumed always to be just blocks of binary data.
 * An adapter specification for a given messaging protocol
 * may define further interfaces derived from the <code>Message</code>
 * interfaces included in this generic specification.
 *
 * <p>
 * The wireless messaging protocols that are accessed
 * via this API are typically of store-and-forward nature,
 * unlike network layer datagrams. Thus, the messages will
 * usually reach the recipient, even if the recipient is not
 * connected at the time of sending the message. This may
 * happen significantly later if the recipient is
 * disconnected for a long time. Sending, and possibly also
 * receiving, these wireless messages typically involves
 * a financial cost to the end user that cannot be neglected. Therefore,
 * applications should not send many messages unnecessarily.
 * </p>
 * <p>This interface contains the functionality common
 * to all messages. Concrete object instances representing
 * a message will typically implement other (sub)interfaces
 * providing access to the content and other information in
 * the message which is dependent on the type of the message.
 * </p>
 * <p>Object instances implementing this interface are just
 * containers for the data that is passed in. The <code>setAddress()</code>
 * method just sets the value of the address in the
 * data container without any checking whether the value
 * is valid in any way.
 * </p>
 */

public interface Message {

    /**
     * Returns the address associated with this message.
     *
     * <p>If this is a message to be sent, then this address
     * is the recipient's address.
     * </p>
     * <p>If this is a message that has been received, then
     * this address is the sender's address.
     * </p>
     * <p>Returns <code>null</code>, if the address for the message
     * is not set.
     * </p>
     * <p><strong>Note</strong>: This design allows responses to be
     * sent to a received message by reusing the
     * same <code>Message</code> object and just replacing the
     * payload. The address field can normally be
     * kept untouched (unless the messaging protocol
     * requires some special handling of the address).
     * </p>
     * <p>The returned address uses the same URL string 
     * syntax that <code>Connector.open()</code> uses to 
     * obtain this <code>MessageConnection</code>.</p>
     * @return the address of this message, or <code>null</code>
     * if the address is not set
     * @see #setAddress(String)
     */
    public String getAddress();

    /**
     * Sets the address associated with this message,
     * that is, the address returned by the <code>getAddress</code> method.
     * The address may be set to <code>null</code>.
     * <p> The address MUST use the same URL string 
     * syntax that <code>Connector.open()</code> uses to obtain 
     * this <code>MessageConnection</code>. </p>
     * @param addr address for the message
     * @see #getAddress()
     */
    public void setAddress(String addr);

    /**
     * Returns the timestamp indicating when this message has been
     * sent.
     * @return <code>Date</code> indicating the timestamp in the message or
     *         <code>null</code> if the timestamp is not set or
     *         if the time information is not available in the underlying
     *         protocol message
     */
    public java.util.Date getTimestamp();

}
