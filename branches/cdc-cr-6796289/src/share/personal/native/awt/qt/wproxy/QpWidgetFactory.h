/*
 *  @(#)QpWidgetFactory.h	1.9 06/10/25
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
#ifndef _QpWIDGET_FACTORY_H_
#define _QpWIDGET_FACTORY_H_

#include "QpObject.h"
#include "QpWidget.h"

class QpWidgetFactory : public QpObject {
public :
    static QpWidgetFactory *instance() ;

    QWidget *createFrame(QpWidget *parent, char *name, int flags=0);
    QWidget *createMenuBar(QpWidget *parent, char *name, int flags=0);
    QWidget *createPushButton(QpWidget *parent,
                              char *name, int flags=0);
    QWidget *createRadioButton(QpWidget *parent,
                               char *name, int flags=0);
    QWidget *createCheckBox(QpWidget *parent,
                            char *name, int flags=0);
    QWidget *createWidget(QpWidget *parent, char *name, int flags=0);
    QWidget *createComboBox(bool rw, QpWidget *parent, 
                            char *name, int flags=0);
    QWidget  *createLabel(QString text, QpWidget *parent, 
                          char *name, int flags=0);
    QWidget *createListBox(QpWidget *parent, char *name, int flags=0);
    QWidget *createScrollView(QpWidget *parent, char *name, int flags=0);
    QWidget *createScrollBar(int type,QpWidget *parent, 
                             char *name, int flags=0);
    QWidget *createPopupMenu(QpWidget *parent, char *name, int flags=0);
    QWidget *createTextEdit(QpWidget *parent, char *name, int flags=0);
    QWidget *createMultiLineEdit(QpWidget *parent, char *name, int flags=0);
    QWidget *createLineEdit(QpWidget *parent, char *name, int flags=0);
    QWidget *createFileDialog(QpWidget *parent, char *name, int flags=0);
protected :
    enum MethodId {
        SOM = QpObject::EOM,
        CreateFrame = QpWidgetFactory::SOM,
        CreateMenuBar,
        CreatePushButton,
        CreateRadioButton,
        CreateCheckBox,
        CreateWidget,
        CreateComboBox,
        CreateLabel,
        CreateListBox,
        CreateScrollView,
        CreateScrollBar,
        CreatePopupMenu,
        CreateTextEdit,
        CreateMultiLineEdit,
        CreateLineEdit,
        CreateFileDialog,
        EOM
    };

    virtual void execute(int method, void *args);
private :
    static QpWidgetFactory *DefaultFactory; // 6176847
};

#endif /* _QpWIDGET_FACTORY_H_ */
