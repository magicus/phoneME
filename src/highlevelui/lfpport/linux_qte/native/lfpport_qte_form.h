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
 * The custom item widget and its helper classes.
 */

#ifndef _LFPPORT_QTE_FORM_H_
#define _LFPPORT_QTE_FORM_H_

#include <qwidget.h>

/**
 * Form is a container for Items which are added later.
 * This container is added as the only child of the mscreen. 
 * Mscreen can scroll within form. Any scrolling by mscreen
 * is being notified to form through viewportChanged slot.
 */
class Form : public QWidget
{
  Q_OBJECT

 public :
    /* Constructor of the form's container */
   Form();

protected:
   /**
    * Finds a new widget to give the keyboard focus to, 
    * returns TRUE if is can find a new widget and FALSE if it can't
    *
    * Override to notify Java.
    */
  bool focusNextPrevChild ( bool next );


 public slots:
   /* 
    * This slot is called by Qt when there is a change
    * in the form's viewport scroll location.
    *
    * @param vpX the x coordinate of the new viewport scroll
    *            location
    * @param vpY the y coordinate of the new viewport scroll
    *            location
    */
   void viewportChanged(int vpX, int vpY);
};

#endif /* _LFPPORT_QTE_FORM_H_ */
