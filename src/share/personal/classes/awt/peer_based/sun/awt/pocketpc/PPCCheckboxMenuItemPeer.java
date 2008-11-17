/*
 * @(#)PPCCheckboxMenuItemPeer.java	1.3 02/12/09
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
 */

package sun.awt.pocketpc;

import java.awt.*;
import sun.awt.peer.*;
import java.awt.event.ItemEvent;

/**
 *
 *
 * @author Nicholas Allen
 */

class PPCCheckboxMenuItemPeer extends PPCMenuItemPeer implements CheckboxMenuItemPeer {

    private static native void initIDs();
    static {
        initIDs();
    }

    // CheckboxMenuItemPeer implementation

    public native void setState(boolean t);

    // Toolkit & peer internals

    PPCCheckboxMenuItemPeer(CheckboxMenuItem target) {
      this.target = target;
      isCheckbox = true;
      PPCMenuPeer parent = (PPCMenuPeer) PPCToolkit.getMenuComponentPeer((MenuComponent)target.getParent());
      create(parent);
      setState(target.getState());

      MenuShortcut sc = ((CheckboxMenuItem)target).getShortcut();
      if (sc != null) {
          shortcutLabel = sc.toString();
      }
    }

    // native callbacks

    public void handleAction(boolean state, int modifiers) {
	CheckboxMenuItem target = (CheckboxMenuItem)this.target;
	target.setState(state);
        PPCToolkit.postEvent(
            new ItemEvent(target, ItemEvent.ITEM_STATE_CHANGED, target.getLabel(), 
                          state ? ItemEvent.SELECTED : ItemEvent.DESELECTED));
    }
}
