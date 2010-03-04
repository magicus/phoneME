/*
 *  
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 * This source file is specific for Qt-based configurations.
 */

/**
 * @file
 * Patches to platform library.
 */

#ifndef _LFPPORT_QTE_PATCHED_H_
#define _LFPPORT_QTE_PATCHED_H_

#include <qpopupmenu.h>

/**
 * Patch a feature in QPopupMenu::clear and QPopupMenu::removeItem.
 */
class PatchedQPopupMenu : public QPopupMenu {

public:
    /**
     * Constructor.
     *
     * @param parent parent widget
     */
    PatchedQPopupMenu(QWidget *parent) : QPopupMenu(parent) { }

    /**
     * Patch default implementation by clearing current item index.
     */
    void clear() {
        QPopupMenu::clear();
        actItem = -1;
    }

    /**
     * Patch default implementation by updating current item index.
     */
    void removeItemAt(int index) {
	    QPopupMenu::removeItemAt(index);

        if (actItem >= (int)count()) {
            actItem = -1;
        }
    }
};

#endif
