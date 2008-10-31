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

package com.sun.midp.automation;

import com.sun.midp.main.*;
import com.sun.cldc.isolate.*;

/**
 * Some info associated with AutoMIDletImpl.
 */
class AutoMIDletInfo {
    /** Suite ID */
    int suiteID;

    /** MIDlet's class name */
    String midletClassName;

    /** When MIDlet is running: corresponding MIDletProxy */
    MIDletProxy midletProxy;

    /** When MIDlet is running: corresponding Isolate */
    Isolate midletIsolate;

    /** When MIDlet is running: corresponding AutoMIDletImpl */
    AutoMIDletImpl midlet;

    /** Indicates if there was error starting MIDlet */
    boolean startError;

    /** Contains error code in case there was an error */
    int startErrorCode;

    /** Contains error details in case there was an error */
    String startErrorDetails;

    /**
     * Constructor.
     *
     * @param suiteID suite ID
     * @param midletClassName MIDlet's class name
     */
    AutoMIDletInfo(int suiteID, String midletClassName) {
        this.suiteID = suiteID;
        this.midletClassName = midletClassName;
        this.midletProxy = null;
        this.midletIsolate = null;
        this.midlet = null;
        this.startError = false;
        this.startErrorCode = 0;
        this.startErrorDetails = null;
    }
}

