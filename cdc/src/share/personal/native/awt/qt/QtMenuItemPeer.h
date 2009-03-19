/*
 * @(#)QtMenuItemPeer.h	1.8 06/10/25
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
#ifndef _QTMENU_ITEM_PEER_H_
#define _QTMENU_ITEM_PEER_H_

#include "awt.h"
#include "QtMenuComponentPeer.h"

class QtMenuItemPeer : public QtMenuComponentPeer
{
  Q_OBJECT
 public:
    enum ItemType {
        Separator = 0 ,
        Label     = 1 ,
        CheckBox  = 2 ,
        Menu      = 3 ,
        Widget    = 4
    };

  QtMenuItemPeer(JNIEnv *env, jobject thisObj, ItemType type = Label,
                  QpWidget *menuItemWidget=NULL);
   ~QtMenuItemPeer(void);
  
   bool isShortcutSet();
   void setShortcutSet(bool);
   int getId();
   ItemType getType();

   virtual void propagateSettings(void);
   virtual void addToContainer(QpWidget *menuContainer);
   virtual void removeFromContainer(void);
   virtual void setContent(QString text, bool setAccelKey, int accelKey);
   virtual void setEnabled(bool enabled);
   virtual void setFont(QFont font);
   QpWidget *getContainer() ;

   static QtMenuItemPeer *getPeerFromJNI (JNIEnv *env, jobject peer);
private slots:
    void menuItemSelected();

 private:

 /* In Java, a MenuItem can enable itself. In Qt, this isn't possible.
    In Qt, the menu enables the menu item. This is done by saving the id
    when the item is added to the menu, then using it in 
   setItemEnabled(int id, boolean state) */

 protected:   
   ItemType type ; 
   int id;
   QpWidget *menuContainer ;
   bool hasShortcutBeenSet;
};

#endif
