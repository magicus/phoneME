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
 */

package com.sun.midp.io.nci.server;

import com.sun.tck.wma.*;
import java.util.*;

public class NCIMessage implements Message {
    private String url;
    private Date timestamp = new Date();
    private String address;
    
    /**
     * Returns the timestamp indicating when this message has been
     * sent. 
     * @return <code>Date</code> indicating the timestamp in the message or
     * <code>null</code> if the timestamp is not set or
     * if the time information is not available in the underlying
     * protocol message.
     */
    public Date getTimestamp() {
        return timestamp;
    }

    /**
     * Sets the address associated with this message,
     * that is, the address returned by the <code>getAddress</code> method.
     * The address may be set to <code>null</code>.
     * @param address address for the message
     * @see #getAddress()
     */
    public void setAddress(String address) {
        this.address = address;
    }

    /**
     * Returns the address associated with this message.
     * 
     * <p>If this is a message to be sent, this address
     * is the address of the recipient.
     * </p>
     * <p>If this is a message that has been received,
     * this address is the sender's address.
     * </p>
     * <p>Returns <code>null</code>, if the address for the message 
     * is not set.
     * </p>
     * <p><strong>Note</strong>: This design allows sending responses
     * to a received message easily by reusing the 
     * same <code>Message</code> object and just replacing the 
     * payload. The address field can normally be
     * kept untouched (unless the used messaging protocol
     * requires some special handling of the address).
     * </p>
     * @return the address of this message, or <code>null</code>
     * if the address is not set
     * @see #setAddress(String)
     */
    public String getAddress() {
        return address;
    }
    
}
