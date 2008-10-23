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

package com.sun.jumpimpl.windowing;

import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageSender;
import com.sun.jump.message.JUMPMessageHandler;
import com.sun.jump.message.JUMPMessagingService;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPMessageDispatcherTypeException;
import com.sun.jump.command.JUMPExecutiveWindowRequest;
import com.sun.jump.command.JUMPIsolateWindowRequest;
import com.sun.jump.command.JUMPResponse;
import com.sun.jump.common.JUMPApplication;

import com.sun.jumpimpl.process.RequestSenderHelper;

import com.sun.me.gci.windowsystem.GCIDisplay;
import com.sun.me.gci.windowsystem.GCIScreenWidget;
import com.sun.me.gci.windowsystem.GCIDisplayListener;
import com.sun.me.gci.windowsystem.GCIGraphicsEnvironment;
import com.sun.me.gci.windowsystem.event.GCIFocusEvent;
import com.sun.me.gci.windowsystem.event.GCIFocusEventListener;

import java.awt.GraphicsEnvironment;

import java.util.Vector;
import java.io.IOException;


//
// Binds GCI windowing with JUMP windowing.
// WindowingClient sets relationship between GCIScreenWidget and JUMPWindow: 
// every single GCIScreenWidget instance is mapped into a single JUMPWindow 
// instance (strictly saying into JUMP window id).
// 
// WindowingClient is supposed to be instantiated in executive and isolate VMs.
//
// WindowingClient handles JUMPExecutiveWindowRequest requests from executive 
// translating them in appropriate GCI calls. 
// When some GCIScreenWidget changes its state such event in translated into 
// appropriate JUMPIsolateWindowRequest notification sent to the executive.
//
// GCIScreenWidget can be in three states:
//  - foreground:       GCIScreenWidget is raised foreground, handles user 
//                      input and updates screen;
//  - background:       GCIScreenWidget is lowered background, doesn't handle 
//                      user input and doesn't update screen;
//  - soft background:  GCIScreenWidget is lowered background, handles user 
//                      input and updates screen;
// "soft background" is useful for presentation app running in executive in 
// case the presentation app has its own screen area and applications share
// another screen area. In this case if presentation app's window is set 
// background with WindowingModule#setBackground call it can still receive 
// user input and control applications.
//
public class WindowingClient implements JUMPMessageHandler {

    private Vector              windows;
    private RequestSenderHelper requestSender;
    private ListenerImpl        listener;
    private int                 isolateId;
    private JUMPMessageSender   executive;
    private boolean             softBackgroundState;

    private static class Window {
        final static int STATE_FOREGROUND       = 1;
        final static int STATE_BACKGROUND       = 2;
        final static int STATE_SOFT_BACKGROUND  = 3;

        private GCIScreenWidget widget;

        public Window(GCIScreenWidget widget) {
            this.widget = widget;
        }

        public void
        setState(int state) {
            switch(state) {
            case STATE_FOREGROUND:
                widget.requestFocus();
                GCIGraphicsEnvironment.getInstance(
                    ).getEventManager().startEventLoop();
                widget.suspendRendering(false);
                break;

            case STATE_BACKGROUND:
                GCIGraphicsEnvironment.getInstance(
                    ).getEventManager().stopEventLoop();
                widget.suspendRendering(true);
                widget.yieldFocus();
                break;

            case STATE_SOFT_BACKGROUND:
                widget.yieldFocus();
                break;
            }
        }

        public GCIScreenWidget
        getWidget() {
            return widget;
        }
    }

    private class ListenerImpl implements GCIDisplayListener, GCIFocusEventListener {

        private GCIScreenWidget         selfContained;
        private GCIFocusEventListener   origListener;

        void
        setOrigListener(GCIFocusEventListener origListener) {
            this.origListener = origListener;
        }

        void
        setSelfContained(GCIScreenWidget selfContained) {
            this.selfContained = selfContained;
        }

        public void
        screenWidgetCreated(GCIDisplay source, GCIScreenWidget widget) {
            windows.add(new Window(widget));

            // By default widget is created in foreground state, so notify
            // executive rather then wait when GCI sends appropriate
            // GCIFocusEvent to indicate that widget is foreground.
            //
            // This is workaround for the problem when no foreground
            // notification is sent by GCI impl and thus windowing module
            // doesn't know window state. The problem is observed with
            // directfb when user starts application, then quickly switchs to
            // another terminal (Alt+F7).
            postRequest(
                widget, JUMPIsolateWindowRequest.ID_NOTIFY_WINDOW_FOREGROUND);
        }

        public boolean
        focusEventReceived(GCIFocusEvent event) {
            switch(event.getID()) {
            case GCIFocusEvent.FOCUS_GAINED:
                postRequest(
                    event.getScreenWidget(),
                    JUMPIsolateWindowRequest.ID_NOTIFY_WINDOW_FOREGROUND);
                break;

            case GCIFocusEvent.FOCUS_LOST:
                postRequest(
                    event.getScreenWidget(),
                    JUMPIsolateWindowRequest.ID_NOTIFY_WINDOW_BACKGROUND);
                break;
            }

            return
                (origListener != null)
                    ? origListener.focusEventReceived(event)
                    : true;
        }
    }

    private void
    postRequest(GCIScreenWidget widget, String requestId) {
        int winId = -1;
        synchronized(windows) {
            for(int i = 0, size = windows.size(); i != size; ++i) {
                if(((Window)windows.get(i)).getWidget() == widget) {
                    winId = i;
                    break;
                }
            }
        }

        if(winId != -1) {
            requestSender.sendRequestAsync(
                executive,
                new JUMPIsolateWindowRequest(
                    requestId, winId, isolateId));
        }
    }

    private void
    setState(Window w, boolean foreground) {
        try {
            listener.setSelfContained(w.getWidget());
            if(foreground) {
                w.setState(Window.STATE_FOREGROUND);
            } else if(softBackgroundState) {
                w.setState(Window.STATE_SOFT_BACKGROUND);
            } else {
                w.setState(Window.STATE_BACKGROUND);
            }
        }
        finally {
            listener.setSelfContained(null);
        }
    }

    public void
    init(String screenBounds) {
        if(screenBounds != null && screenBounds.length() != 0) {
            System.setProperty("PBP_SCREEN_BOUNDS", screenBounds);
        }

        // kick start GCI native library loading to avoid UnsatisfiedLinkError
        GraphicsEnvironment.getLocalGraphicsEnvironment();

        GCIGraphicsEnvironment gciEnv = GCIGraphicsEnvironment.getInstance();

        // register listener with all available displays and event manager
        for(int i = 0, count = gciEnv.getNumDisplays(); i != count; ++i) {
            gciEnv.getDisplay(i).addListener(listener);
        }

        listener.setOrigListener(gciEnv.getEventManager().getFocusListener());
        gciEnv.getEventManager().setFocusListener(
            listener, gciEnv.getEventManager().getSupportedFocusIDs());

        if(!gciEnv.getEventManager().supportsFocusEvents()) {
            System.err.println(
                "WARNING: focus events are not supported "
                + "by the running GCI impl!");
        }
    }

    public WindowingClient(
        JUMPMessageSender       executive,
        JUMPMessagingService    ms,
        int                     isolateId,
        boolean                 softBackgroundState) {

        this.windows                = new Vector();
        this.requestSender          = new RequestSenderHelper(ms);
        this.isolateId              = isolateId;
        this.executive              = executive;
        this.softBackgroundState    = softBackgroundState;

        try {
            ms.getMessageDispatcher().registerHandler(
                JUMPExecutiveWindowRequest.MESSAGE_TYPE, this);
        } catch (JUMPMessageDispatcherTypeException dte) {
            dte.printStackTrace();
            throw new IllegalStateException();
        } catch (IOException ex) {
            ex.printStackTrace();
            throw new IllegalStateException();
        }

        listener = new ListenerImpl();
    }

    public void
    handleMessage(JUMPMessage message) {
        if(JUMPExecutiveWindowRequest.MESSAGE_TYPE.equals(message.getType())) {
            JUMPExecutiveWindowRequest cmd =
                (JUMPExecutiveWindowRequest)
                    JUMPExecutiveWindowRequest.fromMessage(message);

            int     winId   = cmd.getWindowId();
            Window  win     = null;
            synchronized(windows) {
                if(winId < windows.size()) {
                    win = (Window)windows.get(winId);
                }
            }

            if(JUMPExecutiveWindowRequest.ID_FOREGROUND.equals(
                cmd.getCommandId())) {

                if(win == null) {
                    requestSender.sendBooleanResponse(message, false);
                } else {
                    setState(win, true);
                    requestSender.sendBooleanResponse(message, true);
                }
                return;
            }

            if(JUMPExecutiveWindowRequest.ID_BACKGROUND.equals(
                cmd.getCommandId())) {

                if(win == null) {
                    requestSender.sendBooleanResponse(message, false);
                } else {
                    setState(win, false);
                    requestSender.sendBooleanResponse(message, true);
                }
                return;
            }

            return;
        }
    }
}
