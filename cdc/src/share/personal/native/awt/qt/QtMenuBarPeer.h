/*
 * @(#)QtMenuBarPeer.h	1.7 06/10/25
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
#ifndef _QTMENU_BAR_PEER_H_
#define _QTMENU_BAR_PEER_H_

#include "awt.h"
#include "QtMenuComponentPeer.h"

class QtMenuBarPeer : public QtMenuComponentPeer 
{
 public:
   QtMenuBarPeer(JNIEnv *env, jobject thisObj, QpWidget *menuBar);
   virtual ~QtMenuBarPeer(void);
   void propagateSettings(void); 
   void addToList(void *);
   void setPreferredHeight(int h) { preferredHeight = h; }
   int getPreferredHeight() { return preferredHeight; }

   static QtMenuBarPeer *getPeerFromJNI (JNIEnv *env, jobject peer);

 private:

  /* The list that stores the references to child popup menus so that  */
  /* changes to the menu bar can propagate down to the inserted items. */
  QList<void *> list;

  /*
   * The preferred height when QMenuBar is first constructed by Qt.
   */
  int preferredHeight;
}; 

#endif
