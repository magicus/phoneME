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

import com.sun.jump.common.JUMPWindow;
import com.sun.jump.module.windowing.JUMPWindowingModuleFactory;
import com.sun.jump.module.windowing.JUMPWindowingModule;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageReader;
import com.sun.jump.message.JUMPOutgoingMessage;

/**
 * <code>JUMPExecutiveWindowRequest</code> defines all the windowing
 * related requests that originate from the <Code>JUMPExecutive</code>
 */
public class JUMPExecutiveWindowRequest extends JUMPRequest {
    private int windowId;

    /**
     * Type of <code>JUMPMessage</code> corresponding
     * to <code>JUMPExecutiveWindowRequest</code> request.
     */
    public static final String MESSAGE_TYPE = "executive/window";

    /**
     * Command to bring the Isolate's window to the foreground.
     * <ol>
     *   <li>args[0] - Window Id</li>
     * </ol>
     * Synchronous request. expects a
     * {@link com.sun.jump.command.JUMPResponse#ID_SUCCESS} or
     * {@link com.sun.jump.command.JUMPResponse#ID_FAILURE}
     */
    public static final String ID_FOREGROUND= "Foreground";

    /**
     * Command to bring the Isolate's window to the background.
     * <ol>
     *   <li>args[0] - Window Id</li>
     * </ol>
     * Synchronous request. expects a
     * {@link com.sun.jump.command.JUMPResponse#ID_SUCCESS} or
     * {@link com.sun.jump.command.JUMPResponse#ID_FAILURE}
     */
    public static final String ID_BACKGROUND = "Background";

    private JUMPExecutiveWindowRequest(String id, String[] args) {
	super(MESSAGE_TYPE, id, args);
    }

    private JUMPExecutiveWindowRequest(String id) {
	super(MESSAGE_TYPE, id, null);
    }

    JUMPExecutiveWindowRequest() {
	super(MESSAGE_TYPE, null, null);
    }

    public JUMPExecutiveWindowRequest(String id, JUMPWindow w) {
	this(id);
	this.windowId = w.getId();
    }

    /**
     * For subclasses to use to initialize any fields
     * using <code>JUMPMessage.get*</code> methods.
     */
    protected void deserializeFrom(JUMPMessageReader message) {
	// First deserialize any shared fields
	super.deserializeFrom(message);
	// And now window specific fields
	this.windowId = message.getInt();
    }

    /**
     * For subclasses to use to put data in a message
     * using <code>JUMPOutgoingMessage.add*</code> methods.
     */
    protected void serializeInto(JUMPOutgoingMessage message) {
	super.serializeInto(message);
	// And now window specific fields
	message.addInt(this.windowId);
    }

    public static JUMPCommand fromMessage(JUMPMessage message) {
	return JUMPCommand.fromMessage(message,
				       JUMPExecutiveWindowRequest.class);
    }
    
    public int getWindowId() {
        return windowId;
    }
}
