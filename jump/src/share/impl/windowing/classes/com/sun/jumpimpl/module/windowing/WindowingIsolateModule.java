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

package com.sun.jumpimpl.module.windowing;

import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageHandler;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPMessageDispatcherTypeException;
import com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess;
import com.sun.jump.command.JUMPExecutiveWindowRequest;
import com.sun.jump.command.JUMPIsolateWindowRequest;
import com.sun.jump.command.JUMPResponse;

import com.sun.jumpimpl.process.RequestSenderHelper;

import com.sun.me.gci.windowsystem.GCIScreenWidget;


public class WindowingIsolateModule implements JUMPMessageHandler {

    private RequestSenderHelper requestSender;


    WindowingIsolateModule(JUMPIsolateProcess host) {
        requestSender = new RequestSenderHelper(host);

        try {
            JUMPMessageDispatcher md = host.getMessageDispatcher();
            md.registerHandler(JUMPIsolateWindowRequest.MESSAGE_TYPE, this);
        } catch (JUMPMessageDispatcherTypeException dte) {
            dte.printStackTrace();
            throw new IllegalStateException();
        }

        GraphicsEnvironment.install();
        GraphicsEnvironment.getInstalledInstance().setListener(
            new GraphicsEnvironment.Listener() {

                public void
                windowCreated(GCIScreenWidget widget) {
                }

                public boolean
                canStartEventLoop() {
                    return true;
                }

                public boolean
                canStopEventLoop() {
                    return true;
                }

                public boolean
                canStartRendering() {
                    return true;
                }

                public boolean
                canStopRendering() {
                    return true;
                }
            });
    }

    public void
    handleMessage(JUMPMessage message) {
    }
}
