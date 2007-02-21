/*
 * %W% %E%
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jump.command;

import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageReader;
import com.sun.jump.message.JUMPOutgoingMessage;
import com.sun.jump.command.JUMPResponse;
import com.sun.jump.common.JUMPApplication;

/**
 * <code>JUMPResponseApplication</code> encapsulates 
 * <code>JUMPApplication</code> in the response command.
 */
public class JUMPResponseApplication extends JUMPResponse {
    private JUMPApplication app;
    
    public JUMPApplication getApp() {
        return app;
    }

    /**
     * Creates a new instance of <code>JUMPResponseApplication</code> by
     * deserializing the data from the <code>JUMPMessage</code>
     */
    public static JUMPResponse fromMessage(JUMPMessage message) {
        return (JUMPResponse)
        JUMPCommand.fromMessage(message, JUMPResponseApplication.class);
    }

    //
    // To be filled in when de-serializing
    //
    protected JUMPResponseApplication() {
        super();
    }

    public JUMPResponseApplication(String messageType) {
        this(messageType, null);
    }

    public JUMPResponseApplication(String messageType, JUMPApplication app) {
        super(messageType, app != null ? ID_SUCCESS : ID_FAILURE, null);
        this.app = app;
    }

    protected void deserializeFrom(JUMPMessageReader message) {
        // First deserialize any shared fields
        super.deserializeFrom(message);
        // And now JUMPResponseApplication specific fields
        byte[] bytes = message.getByteArray();
        if(bytes != null) {
            this.app = JUMPApplication.fromByteArray(bytes);
        } else {
            this.app = null;
        }
    }

    protected void serializeInto(JUMPOutgoingMessage message) {
        // First deserialize any shared fields
        super.serializeInto(message);
        // And now JUMPResponseApplication specific fields
        if(app != null) {
            message.addByteArray(app.toByteArray());
        } else {
            message.addByteArray(null);
        }
    }
}
