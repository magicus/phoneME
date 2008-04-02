/*
 *   
 *
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

package com.sun.midp.io.j2me.cbs;

import java.util.Date;

/**
 * Implements a CBS binary message for the CBS message connection.
 */
public class BinaryObject extends com.sun.midp.io.j2me.sms.BinaryObject {

    /** 
     * Constructs a message and initialize the  target address. 
     * @param toAddress The address of the recipient. May be <code>null</code>.
     */         
    public BinaryObject(String toAddress) {
        super(toAddress);
    }

    /**
     * Returns the time at which the message was sent.
     *
     * @return Always returns <code>null</code>, since CBS messages do not
     *     support a time stamp.
     */
    public Date getTimestamp() {
        return null;
    }

    /**
     * Sets the time at which the message was sent.
     *
     * @throws IllegalArgumentException Always thrown, since the time stamp is
     *     always <code>null</code>.
     */
    public void setTimeStamp() {
        throw new IllegalArgumentException("Cannot set time stamp.");
    }

}

