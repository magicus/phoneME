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

import com.sun.me.gci.windowsystem.GCIDisplay;
import com.sun.me.gci.windowsystem.GCIScreenWidget;
import com.sun.me.gci.windowsystem.GCIGraphicsEnvironment;
import com.sun.me.gci.windowsystem.event.GCIEventManager;
import com.sun.me.gci.surface.GCIDrawingSurface;

import java.lang.reflect.Field;


class GraphicsEnvironment extends GCIGraphicsEnvironment {
    private GCIGraphicsEnvironment impl;
    private GCIDisplay displays[];
    private Listener listener;

    private class Display extends GCIDisplay {
        private GCIDisplay impl;

        Display(GCIDisplay impl) {
            this.impl = impl;
        }

        public String
        toString() {
            return impl.toString();
        }

        public int
        getWidth() {
            return impl.getWidth();
        }

        public int
        getHeight() {
            return impl.getHeight();
        }

        public int
        getResolution() {
            return impl.getResolution();
        }

        public int
        getCurrentFormat() {
            return impl.getCurrentFormat();
        }

        public int[]
        getFormats() {
            return impl.getFormats();
        }

        public int[]
        getOffscreenFormats() {
            return impl.getOffscreenFormats();
        }

        public GCIDrawingSurface
        createOffscreenDrawingSurface(
            int width, int height, int format) {

            return
                impl.createOffscreenDrawingSurface(
                    width, height, format);
        }

        public GCIDrawingSurface
        createCompatibleOffscreenDrawingSurface(int width, int height) {
            return
                impl.createCompatibleOffscreenDrawingSurface(width, height);
        }

        public GCIScreenWidget
        createScreenWidget(
            int x, int y, int width, int height, Object eventTarget) {

            GCIScreenWidget widget =
                impl.createScreenWidget(x, y, width, height, eventTarget);

            return new ScreenWidget(widget);
        }
    }

    private class ScreenWidget extends GCIScreenWidget {
        private GCIScreenWidget impl;

        ScreenWidget(GCIScreenWidget impl) {
            this.impl = impl;

            if(listener != null) {
                listener.windowCreated(impl);
            }
        }

        public GCIDrawingSurface
        getDrawingSurface() {
            return impl.getDrawingSurface();
        }

        public void
        show() {
            impl.show();
        }

        public void
        hide() {
            impl.hide();
        }

        public void
        resize(int x, int y, int width, int height) {
            impl.resize(x, y, width, height);
        }

        public void
        suspendRendering(boolean flag) {
            if(listener != null
            && flag ?
                listener.canStopRendering()
                : listener.canStartRendering()) {

                impl.suspendRendering(flag);
            }
        }

        public boolean
        isRenderingSuspended() {
            return impl.isRenderingSuspended();
        }

        public void requestBackground() {
            impl.requestBackground();
        }

        public void requestForeground() {
            impl.requestForeground();
        }

        public Object getEventTarget() {
            return impl.getEventTarget();
        }

    }


    private GraphicsEnvironment(GCIGraphicsEnvironment impl) {
        this.impl       = impl;
        this.displays   = new GCIDisplay[impl.getNumDisplays()];
    }

    static GraphicsEnvironment
    getInstalledInstance() {
        return (GraphicsEnvironment)GCIGraphicsEnvironment.getInstance();
    }

    static void
    install() {
        try {
            Field GCIGraphicsEnvironment_instance =
                GCIGraphicsEnvironment.class.getDeclaredField("instance");
            GCIGraphicsEnvironment_instance.setAccessible(true);

            Object value = GCIGraphicsEnvironment_instance.get(null);
            if(value != null) {
                if(value instanceof GraphicsEnvironment) {
                    return;
                }

                // too late to modify field, so report error
                throw new IllegalStateException();
            }

            value =
                new GraphicsEnvironment(GCIGraphicsEnvironment.getInstance());

            GCIGraphicsEnvironment_instance.set(null, value);
        } catch(RuntimeException e) {
            throw e;
        } catch(Exception e) {
            e.printStackTrace();
            throw new IllegalStateException();
        }
    }


    interface Listener {
        public void windowCreated(GCIScreenWidget widget);
        public boolean canStartEventLoop();
        public boolean canStopEventLoop();
        public boolean canStartRendering();
        public boolean canStopRendering();
    }

    void
    setListener(Listener listener) {
        this.listener = listener;
    }

    public void
    startEventLoop() {
        if(listener != null && listener.canStartEventLoop()) {
            impl.getEventManager().startEventLoop();
        }
    }

    public void
    stopEventLoop() {
        if(listener != null && listener.canStopEventLoop()) {
            impl.getEventManager().stopEventLoop();
        }
    }

    public int
    getNumDisplays() {
        return impl.getNumDisplays();
    }

    public void
    beep() {
        impl.beep();
    }

    public GCIDisplay
    getDisplay(int displayNum) {
        synchronized(displays) {
            if(displays[displayNum] == null) {
                displays[displayNum]
                    = new Display(impl.getDisplay(displayNum));
            }

            return displays[displayNum];
        }
    }


    public int
    getDisplayNumber(Object surface) {
        return impl.getDisplayNumber(surface);
    }

    public GCIEventManager getEventManager() {
        return impl.getEventManager();
    }
}
