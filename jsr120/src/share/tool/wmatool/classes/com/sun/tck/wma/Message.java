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

package com.sun.tck.wma;

/** 
 * This is the base interface for derived interfaces
 * representing various types of messages.
 */
public interface Message {

    /**
     * Returns the address that this message is associated with.
     * 
     * <p>If this object represents a message to be sent, this address
     * is the recipient's address.
     * </p>
     * <p>If this object represents a message has been received, this address
     * is the address of the sender.
     * </p>
     * <p>If the address for the message isn't set, <code>null</code>
     * is returned.
     * </p>
     * <p><strong>Note</strong>: Sending responses to a received message
     * is allowed easily by this design through reusing the same
     * <code>Message</code> object with the replaced payload.
     * Unless a special processing of the address is required by the mesaging
     * protocol that is used, the address field can be left untouched.
     * </p>
     * @return <code>null</code> if the address isn't set, or this message's
     *         address. 
     * @see #setAddress(String)
     */
    public String getAddress();

    /**
     * Sets the address that this message is associated with,
     * i.e., the address that <code>getAddress</code> method returns.
     * <code>null</code> is allowed as an address.
     * @param addr address for the message
     * @see #getAddress()
     */
    public void setAddress(String addr);

    /**
     * Returns the timestamp that indicates when this message has been
     * sent. 
     * @return  <code>null</code> either if the timestamp isn't set or
     *         it isn't available in the underlying protocol message,
     *         or <code>Date</code> that indicates the timestamp in the message.
     */
    public java.util.Date getTimestamp();

}




