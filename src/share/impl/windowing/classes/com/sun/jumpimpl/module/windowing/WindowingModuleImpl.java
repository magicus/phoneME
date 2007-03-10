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

import java.io.IOException;
import java.util.Map;
import java.util.Vector;
import java.util.NoSuchElementException;

import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageSender;
import com.sun.jump.message.JUMPMessageHandler;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPMessageDispatcherTypeException;
import com.sun.jump.command.JUMPIsolateLifecycleRequest;
import com.sun.jump.command.JUMPExecutiveWindowRequest;
import com.sun.jump.command.JUMPIsolateWindowRequest;
import com.sun.jump.command.JUMPResponse;
import com.sun.jump.module.windowing.JUMPWindowingModule;
import com.sun.jump.module.JUMPModule;
import com.sun.jump.common.JUMPIsolate;
import com.sun.jump.common.JUMPWindow;

import com.sun.jumpimpl.process.RequestSenderHelper;
import com.sun.jumpimpl.process.JUMPProcessProxyImpl;


class WindowingModuleImpl implements JUMPWindowingModule, JUMPMessageHandler {

    private final Object        lock = new Object();
    private Vector              windows;
    private RequestSenderHelper requestSender;
    private JUMPMessageSender   executiveMessageSender;


    private JUMPMessageSender
    getMessageSender(JUMPWindow window) {
        JUMPIsolate isolate = window.getIsolate();    
        if(isolate != null) {
            return (JUMPMessageSender)isolate;
        }
    
        return executiveMessageSender;
    }

    private void
    setForeground(JUMPWindow window, boolean notifyIsolate) {
        if(window == null) {
            throw new IllegalArgumentException();
        }

        WindowImpl oldFgWindow = (WindowImpl)getForeground();
        if(oldFgWindow != window && oldFgWindow != null) {
// FIXME: comment away by now. reason: directfb window manager takes care about
//        windows switching. The bad thing about this comment is that without
//        explicit setting window background GCI continues to eat VM's 
//        resources trying to handle user input and update screen
//            setBackground(oldFgWindow, notifyIsolate);
        }

        if(notifyIsolate) {
            JUMPResponse response =
                requestSender.sendRequest(
                    getMessageSender(window),
                    new JUMPExecutiveWindowRequest(
                        JUMPExecutiveWindowRequest.ID_FOREGROUND, window));
            if(!requestSender.handleBooleanResponse(response)) {
                return;
            }
        }

        synchronized(lock) {
            if(windows.contains(window)) {
                ((WindowImpl)window).setState(JUMPWindow.FOREGROUND_STATE);
                windows.remove(window);
                windows.add(window);
            }
        }
    }

    private void
    setBackground(JUMPWindow window, boolean notifyIsolate) {
        if(window == null) {
            throw new IllegalArgumentException();
        }

        if(notifyIsolate) {
            JUMPResponse response =
                requestSender.sendRequest(
                    getMessageSender(window),
                    new JUMPExecutiveWindowRequest(
                        JUMPExecutiveWindowRequest.ID_BACKGROUND, window));
            if(!requestSender.handleBooleanResponse(response)) {
                return;
            }
        }

        synchronized(lock) {
            if(windows.contains(window) && (windows.size() > 1)) {
                ((WindowImpl)window).setState(JUMPWindow.BACKGROUND_STATE);
            }
        }
    }


    WindowingModuleImpl() {
        // instantiate stuff that will track JUMPWindow-s 
        // created by presentation if any
        new WindowingExecutiveClient();

        JUMPExecutive   executive = JUMPExecutive.getInstance();
        int             isolateId = executive.getProcessId();

        windows                 = new Vector();
        requestSender           = new RequestSenderHelper(executive);
        executiveMessageSender  = 
            JUMPProcessProxyImpl.createProcessProxyImpl(
                executive.getProcessId());

        try {
            JUMPExecutive e = JUMPExecutive.getInstance();
            JUMPMessageDispatcher md = e.getMessageDispatcher();
            md.registerHandler(JUMPIsolateWindowRequest.MESSAGE_TYPE, this);
            md.registerHandler(JUMPIsolateLifecycleRequest.MESSAGE_TYPE, this);
        } catch (JUMPMessageDispatcherTypeException dte) {
            dte.printStackTrace();
            // FIXME: someone else listeneing -- what to do?
        } catch (IOException ex) {
            ex.printStackTrace();
            // FIXME: can't register, what to do?
        }
    }


    public void
    handleMessage(JUMPMessage message) {
        if(JUMPIsolateWindowRequest.MESSAGE_TYPE.equals(message.getType())) {
            JUMPIsolateWindowRequest cmd =
                (JUMPIsolateWindowRequest)
                    JUMPIsolateWindowRequest.fromMessage(message);

            WindowImpl window =
                WindowImpl.getWindow(cmd.getIsolateId(), cmd.getWindowId());

            synchronized(lock) {
                if(!windows.contains(window)) {
                    window.setState(JUMPWindow.BACKGROUND_STATE);
                    windows.add(0, window);
                }
            }

            if(JUMPIsolateWindowRequest.ID_NOTIFY_WINDOW_FOREGROUND.equals(
                cmd.getCommandId())) {

                setForeground(window, false);
                return;
            }
            
            if(JUMPIsolateWindowRequest.ID_NOTIFY_WINDOW_BACKGROUND.equals(
                cmd.getCommandId())) {

                setBackground(window, false);
                return;
            }
            return;
            
        }
        
        if(JUMPIsolateLifecycleRequest.MESSAGE_TYPE.equals(
            message.getType())) {

            JUMPIsolateLifecycleRequest cmd =
                (JUMPIsolateLifecycleRequest)
                    JUMPIsolateLifecycleRequest.fromMessage(message);

            if(JUMPIsolateLifecycleRequest.ID_ISOLATE_DESTROYED.equals(
                cmd.getCommandId())) {

                int isolateId = cmd.getIsolateId();

                synchronized(lock) {
                    // remove JUMPWindow-s hosted by the destroyed isolate
                    // from the list
                    int idx = 0;
                    while(idx != windows.size()) {
                        JUMPIsolate isolate = 
                            ((JUMPWindow)windows.elementAt(idx)).getIsolate();
                        if(isolate != null 
                        && isolate.getIsolateId() == isolateId) {
                            windows.remove(idx);
                            continue;
                        }
                        ++idx;
                    }

                    // enshure there is one foreground window
                    if(getForeground() == null) {
                        nextWindow();
                    }
                }
            }
        }
    }

    public void
    load(Map config) {
    }

    public void
    unload() {
    }

    public JUMPWindow[]
    getWindows() {
        synchronized(lock) {
            JUMPWindow[] res = new JUMPWindow[windows.size()];
            for(int i = 0; i != res.length; ++i) {
                res[i] = (JUMPWindow)windows.elementAt(i);
            }
            return res;
        }
    }

    public JUMPWindow
    getForeground() {
        try {
            synchronized(lock) {
                JUMPWindow res = (JUMPWindow)windows.lastElement();
                return
                    WindowImpl.FOREGROUND_STATE.equals(res.getState()) ?
                        res
                        : null;
            }
        } catch(NoSuchElementException e) {
            return null;
        }
    }

    public void
    setForeground(JUMPWindow window) {
        setForeground(window, true);
    }

    public void
    setBackground(JUMPWindow window) {
        setBackground(window, true);
    }

    public JUMPWindow
    nextWindow() {
        synchronized(lock) {
            JUMPWindow fg = null;
            for(int i = windows.size() - 2; i >= 0; --i) {
                fg = (JUMPWindow)windows.elementAt(i);
                setForeground(fg);
                if(fg == getForeground()) {
                    break;
                }
            }
            return fg;
        }
    }

    public JUMPWindow
    idToWindow(long id) {
        JUMPWindow res = null;
        synchronized(lock) {
            for(int i = 0, count = windows.size(); i != count; ++i) {
                if(((JUMPWindow)windows.elementAt(i)).getId() == id) {
                    break;
                }
            }
        }
        return res;
    }
}
