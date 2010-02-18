/*
 * @(#)QtMenuPeer.h	1.8 06/10/25
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
#ifndef _QTMENU_PEER_H_
#define _QTMENU_PEER_H_

#include "QtMenuItemPeer.h"
#include <qlist.h>

class QtMenuPeer : public QtMenuItemPeer 
{
  Q_OBJECT
  public:

    QtMenuPeer(JNIEnv *env, jobject thisObj, QpWidget *menuWidget);
    ~QtMenuPeer(void);

    void addToList(void *);
    virtual void propagateSettings(void);
    virtual void addToContainer(QpWidget *menuContainer);
    virtual void removeFromContainer(void);
    virtual void setContent(QString text, bool setAccelKey, int accelKey);
    virtual void setEnabled(bool enabled);
  private:
  /* The list that stores the references to child popup menus so that  */
  /* changes to the menu bar can propagate down to the inserted items. */
    QList<void *> list;
};

#endif
