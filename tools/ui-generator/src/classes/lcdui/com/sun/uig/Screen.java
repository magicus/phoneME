/*
 *
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

package com.sun.uig;

import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.List;


public abstract class Screen extends BaseScreen {
    private Displayable displayable;

    protected abstract Displayable createDisplayable();

    protected Command getSelectItemCommand() {
        //  TODO: i18n for OK string - bug 6781618
        return new Command("OK", Command.OK, 1);
    }

    protected static Form
    createForm(String title) {
        return new Form(title);
    }

    protected static List
    createList(String title) {
        return new List(title, List.EXCLUSIVE | List.IMPLICIT);
    }

    public Screen(ScreenProperties props, Strings strings) {
        super(props, strings);
    }

    synchronized Displayable getDisplayable() {
        if (displayable == null) {
            displayable = createDisplayable();
        }
        return displayable;
    }
}
