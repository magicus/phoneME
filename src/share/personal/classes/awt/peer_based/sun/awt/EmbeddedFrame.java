/*
 * @(#)EmbeddedFrame.java	1.15 06/10/10
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
 *
 */

package sun.awt;

import java.awt.*;
import sun.awt.peer.ComponentPeer;

/**
 * A generic container used for embedding Java components, usually applets.  
 * An EmbeddedFrame has two related uses:  
 * 
 * . Within a Java-based application, an EmbeddedFrame serves as a sort of
 *   firewall, preventing the contained components or applets from using
 *   getParent() to find parent components, such as menubars.
 *
 * . Within a C-based application, an EmbeddedFrame contains a window handle
 *   which was created by the application, which serves as the top-level
 *   Java window.  EmbeddedFrames created for this purpose are passed-in a
 *   handle of an existing window created by the application.  The window
 *   handle should be of the appropriate native type for a specific 
 *   platform, as stored in the pData field of the ComponentPeer.
 *
 * @version 	1.10, 08/19/02
 * @author 	Thomas Ball
 */
public abstract class EmbeddedFrame extends Frame {
    private static final String base = "embeddedFrame";
    private boolean isCursorAllowed = false;
    private static final long serialVersionUID = 2967042741780317130L;
    protected EmbeddedFrame() {}

    protected EmbeddedFrame(int handle) {}

    /**
     * Block introspection of a parent window by this child.
     */
    public Container getParent() {
        return null;
    }

    /**
     * Block modifying any frame attributes, since they aren't applicable
     * for EmbeddedFrames.
     */
    public void setTitle(String title) {}

    public void setIconImage(Image image) {}

    public void setMenuBar(MenuBar mb) {}

    public void setResizable(boolean resizable) {}

    public void remove(MenuComponent m) {}

    public boolean isResizable() {
        return false;
    }

    public void addNotify() {
        int ncomponents = this.getComponentCount();
        for (int i = 0; i < ncomponents; i++) {
            getComponent(i).addNotify();
        }
    }

    // Need a native call to circumvent Component.peer not being public.
    protected native void setPeer(ComponentPeer p);

    /**
     * Sets whether or not Frame.setCursor works for the embedded frame
     * (currently only used in Win32)
     */
    public void setCursorAllowed(boolean isAllowed) {
        isCursorAllowed = isAllowed;
    }

    /**
     * Is the Frame.setCursor allowed for the embedded frame
     * (currently only used in Win32)
     */
    public boolean isCursorAllowed() {
        return isCursorAllowed;
    }
}
