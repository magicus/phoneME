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

package com.sun.midp.services;

import com.sun.midp.links.*;

/**
 * Incapsulates pair of Links: one for sending and other 
 * for receiving data.
 */
final class SystemServiceConnectionLinks {
    /** Send Link */
    protected Link send = null;

    /** Receive Link */
    protected Link receive = null;

    /**
     * Constructor.
     *
     * @param send send Link
     * @param receive receive Link
     */
    SystemServiceConnectionLinks(Link send, Link receive) {
        this.send = send;
        this.receive = receive;
    }

    /**
     * Copy constructor.
     *
     * @param links SystemServiceConnectionLinks object
     */   
    SystemServiceConnectionLinks(SystemServiceConnectionLinks links) {
        this.receive = links.receive;
        this.send = links.send;
    }

    /**
     * Gets send Link.
     *
     * @return send Link
     */
    Link getSendLink() {
        return send;
    }

    /**
     * Gets receive Link
     *
     * @return receive Link
     */
    Link getReceiveLink() {
        return receive;
    }

    /**
     * Closes both send and receive Links
     */
    void close() {
        send.close();
        receive.close();
    }
}
