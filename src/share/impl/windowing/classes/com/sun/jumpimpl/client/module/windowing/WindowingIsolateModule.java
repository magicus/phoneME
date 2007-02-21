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

package com.sun.jumpimpl.client.module.windowing;

import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageSender;
import com.sun.jump.message.JUMPMessageHandler;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPMessageDispatcherTypeException;
import com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess;
import com.sun.jump.command.JUMPExecutiveWindowRequest;
import com.sun.jump.command.JUMPExecutiveLifecycleRequest;
import com.sun.jump.command.JUMPIsolateWindowRequest;
import com.sun.jump.command.JUMPResponseApplication;
import com.sun.jump.command.JUMPResponse;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPAppModel;

import com.sun.jumpimpl.process.RequestSenderHelper;

import com.sun.me.gci.windowsystem.GCIDisplay;
import com.sun.me.gci.windowsystem.GCIScreenWidget;
import com.sun.me.gci.windowsystem.GCIDisplayListener;
import com.sun.me.gci.windowsystem.GCIGraphicsEnvironment;
import com.sun.me.gci.windowsystem.event.GCIFocusEvent;
import com.sun.me.gci.windowsystem.event.GCIFocusEventListener;

import java.awt.GraphicsEnvironment;

import java.util.Vector;


public class WindowingIsolateModule implements JUMPMessageHandler {

    private Vector              windows;
    private RequestSenderHelper requestSender;
    private ListenerImpl        listener;
    private int                 isolateId;
    private JUMPMessageSender   executive;

    private static class Window {
        private JUMPApplication app;
        private GCIScreenWidget widget;

        public Window(GCIScreenWidget widget, JUMPApplication app) {
            this.app     = app;
            this.widget  = widget;
        }

        public void
        setState(boolean foreground, boolean request) {
            if(foreground) {
                GCIGraphicsEnvironment.getInstance(
                    ).getEventManager().startEventLoop();
                if(request) {
                    widget.requestForeground();
                }
                widget.suspendRendering(false);
            } else {
                GCIGraphicsEnvironment.getInstance(
                    ).getEventManager().stopEventLoop();
                if(request) {
                    widget.requestBackground();
                }
                widget.suspendRendering(true);
            }
        }

        public GCIScreenWidget
        getWidget() {
            return widget;
        }

        public JUMPApplication
        getApp() {
            return app;
        }
    }

    private class ListenerImpl implements GCIDisplayListener, GCIFocusEventListener {

        private GCIScreenWidget         selfContained;
        private GCIFocusEventListener   origListener;
        private JUMPApplication         curApp;

        void
        setCurApp(JUMPApplication app) {
            curApp = app;
        }

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
            windows.add(new Window(widget, curApp));
        }

        public boolean
        focusEventReceived(GCIFocusEvent event) {
            switch(event.getID()) {
            case GCIFocusEvent.FOCUS_GOT_FOREGROUND:
                postRequest(
                    event.getScreenWidget(),
                    JUMPIsolateWindowRequest.ID_REQUEST_FOREGROUND);
                break;

            case GCIFocusEvent.FOCUS_GOT_BACKGROUND:
                postRequest(
                    event.getScreenWidget(),
                    JUMPIsolateWindowRequest.ID_REQUEST_BACKGROUND);
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
    setState(Window w, boolean foreground, boolean request) {
        try {
            listener.setSelfContained(w.getWidget());
            w.setState(foreground, request);
        }
        finally {
            listener.setSelfContained(null);
        }
    }

    private void
    setState(JUMPExecutiveLifecycleRequest cmd, boolean foreground) {
        byte[]          barr    = cmd.getAppBytes();
	JUMPApplication app     = JUMPApplication.fromByteArray(barr);
        
        if(app == null) {
            return;
        }

        // find all windows owned by the app and notify them
        Object apps[] = windows.toArray();
        for(int i = 0; i < apps.length; ++i) {
            Window w = (Window)apps[i];
            if(app.equals(w.getApp())) {
                setState(w, foreground, false);
            }
        }
    }

    private void
    init() {
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

    private static void
    overrideScreenBounds(String value) {        
        if(value != null && value.length() != 0) {
            System.setProperty("PBP_SCREEN_BOUNDS", value);
        }
    }

    public WindowingIsolateModule() {
        JUMPIsolateProcess  host = JUMPIsolateProcess.getInstance();

        windows         = new Vector();
        requestSender   = new RequestSenderHelper(host);
        isolateId       = host.getIsolateId();
        executive       = host.getExecutiveProcess();

        try {
            JUMPMessageDispatcher md = host.getMessageDispatcher();

            md.registerHandler(JUMPExecutiveWindowRequest.MESSAGE_TYPE, this);
            md.registerHandler(
                JUMPExecutiveLifecycleRequest.MESSAGE_TYPE, this);
        } catch (JUMPMessageDispatcherTypeException dte) {
            dte.printStackTrace();
            throw new IllegalStateException();
        }

        listener = new ListenerImpl();

        String screenBounds = 
            (String)host.getConfig().get("isolate-screen-bounds");
        overrideScreenBounds(screenBounds);
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
                    setState(win, true, true);
                    requestSender.sendBooleanResponse(message, true);
                }
                return;
            }

            if(JUMPExecutiveWindowRequest.ID_BACKGROUND.equals(
                cmd.getCommandId())) {

                if(win == null) {
                    requestSender.sendBooleanResponse(message, false);                 
                } else {
                    setState(win, false, true);
                    requestSender.sendBooleanResponse(message, true);
                }
                return;
            }

            if(JUMPExecutiveWindowRequest.ID_GET_APPLICATION.equals(
                cmd.getCommandId())) {

                JUMPApplication app;
                if(win != null) {
                    app = win.getApp();
                } else {
                    app = null;
                }
                
                requestSender.sendResponse(
                    message, 
                    new JUMPResponseApplication(message.getType(), app));
            }

            return;
        }

        if(JUMPExecutiveLifecycleRequest.MESSAGE_TYPE.equals(
            message.getType())) {

            JUMPExecutiveLifecycleRequest cmd =
                (JUMPExecutiveLifecycleRequest)
                    JUMPExecutiveLifecycleRequest.fromMessage(message);

            if(JUMPExecutiveLifecycleRequest.ID_START_APP.equals(
                cmd.getCommandId())) {

                JUMPApplication app = 
                    JUMPApplication.fromByteArray(cmd.getAppBytes());
                if(app == null) {
                    return;
                }

	        listener.setCurApp(app);

                overrideScreenBounds(
                    app.getProperty(JUMPApplication.ID_SCREEN_BOUNDS));                

                init();
                return;
            }

            if(JUMPExecutiveLifecycleRequest.ID_RESUME_APP.equals(
                cmd.getCommandId())) {

                setState(cmd, true);
            }

            if(JUMPExecutiveLifecycleRequest.ID_PAUSE_APP.equals(
                cmd.getCommandId())) {

                setState(cmd, false);
            }
        }
    }
}
