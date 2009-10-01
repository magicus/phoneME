/*
 *  @(#)QpWidgetFactory.cc	1.13 06/10/25
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
#include "QpWidgetFactory.h"
#include <stdio.h>
#include <qframe.h>
#include <qmenubar.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qscrollview.h>
#include <qscrollbar.h>
#include <qpopupmenu.h>
#include "QxLineEdit.h"
#if (QT_VERSION >= 0x030000)
#include "QxTextEdit.h"
#else
#include "QxMultiLineEdit.h"
#endif /* QT_VERSION */
#include "QxFileDialog.h"


typedef struct {
    struct {
        QpWidget *parent;
        const char *name;
        int flags;
        void *x0;
    } in ;
    struct {
        QWidget *rvalue;
    } out;
} qtMargsCreate;

// 6176847
QpWidgetFactory *QpWidgetFactory::DefaultFactory = new QpWidgetFactory();

QpWidgetFactory *
QpWidgetFactory::instance() {
    return QpWidgetFactory::DefaultFactory; // 6176847
}

QWidget *
QpWidgetFactory::createFrame(QpWidget *parent, char *name, int flags){
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreateFrame, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createMenuBar(QpWidget *parent, char *name, int flags){
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreateMenuBar, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createPushButton(QpWidget *parent, char *name, int flags){
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreatePushButton, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createRadioButton(QpWidget *parent, char *name, int flags) {
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreateRadioButton, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createCheckBox(QpWidget *parent, char *name, int flags) {
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreateCheckBox, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createWidget(QpWidget *parent, char *name, int flags) {
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreateWidget, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createComboBox(bool rw, QpWidget *parent, 
                                char *name, int flags){
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = (void *)rw;
    invokeAndWait(QpWidgetFactory::CreateComboBox, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget  *
QpWidgetFactory::createLabel(QString text, QpWidget *parent, 
                             char *name, int flags) {
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = &text;
    invokeAndWait(QpWidgetFactory::CreateLabel, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createListBox(QpWidget *parent, char *name, int flags) {
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreateListBox, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createScrollView(QpWidget *parent, char *name, int flags) {
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreateScrollView, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createScrollBar(int type,QpWidget *parent, 
                                 char *name, int flags){
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = (void *)type;
    invokeAndWait(QpWidgetFactory::CreateScrollBar, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createPopupMenu(QpWidget *parent, char *name, int flags){
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreatePopupMenu, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createTextEdit(QpWidget *parent, char *name, int flags){
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreateTextEdit, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createMultiLineEdit(QpWidget *parent, char *name, int flags){
#if (QT_VERSION >= 0x030000)
    return NULL;
#else
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreateMultiLineEdit, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
#endif /*  QT_VERSION */
}

QWidget *
QpWidgetFactory::createLineEdit(QpWidget *parent, char *name, int flags){
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = NULL;
    invokeAndWait(QpWidgetFactory::CreateLineEdit, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidgetFactory::createFileDialog(QpWidget *parent, char *name, int flags){
    QT_METHOD_ARGS_ALLOC(qtMargsCreate, argp);
    argp->in.parent = parent;
    argp->in.name   = name;
    argp->in.flags  = flags;
    argp->in.x0     = (void *)((flags & Qt::WType_Modal) == Qt::WType_Modal);
    invokeAndWait(QpWidgetFactory::CreateFileDialog, argp);
    QWidget *rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

#ifdef QT_KEYPAD_MODE
class NoCloseOnBackQFrame : public QFrame {
public:
    NoCloseOnBackQFrame(QWidget *parent, const char *name, WFlags f)
        : QFrame(parent, name, f)
    {}
    ~NoCloseOnBackQFrame(void) {}

protected:
    void keyPressEvent(QKeyEvent* k) {
        if (this->isTopLevel() && !k->isAccepted() &&
            (k->key() == Key_Back || k->key() == Key_No) &&
            !this->testWFlags(WStyle_Dialog|WStyle_Customize|WType_Popup|WType_Desktop)) {
		// Accepts the key so the top level widget won't be closed
                // by the Qt library.
		k->accept();
                return;
        }
        QFrame::keyPressEvent(k);
    }
};
#endif

void
QpWidgetFactory::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpWidgetFactory, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpObject);

    qtMargsCreate *a = (qtMargsCreate *)args;
    QWidget *parent = (a->in.parent != NULL) ? a->in.parent->getQWidget()
                                             : NULL;

    switch ( mid ) {
    case CreateFrame:
#ifdef QT_KEYPAD_MODE
        a->out.rvalue = new NoCloseOnBackQFrame(parent, a->in.name, a->in.flags);
#else
        a->out.rvalue = new QFrame(parent, a->in.name, a->in.flags);
#endif
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus);
        break ;
    case CreateMenuBar:
        a->out.rvalue = new QMenuBar(parent, a->in.name);
        break ;
    case CreatePushButton:
        a->out.rvalue = new QPushButton(parent, a->in.name);
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus); // 6253667.
        break ;
    case CreateRadioButton:
        a->out.rvalue = new QRadioButton(parent, a->in.name);
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus); // 6253667.
        break ;
    case CreateCheckBox:
        a->out.rvalue = new QCheckBox(parent, a->in.name);
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus); // 6253667.
        break ;
    case CreateWidget:
        a->out.rvalue = new QWidget(parent, a->in.name, a->in.flags);
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus); // 6253667.
        break ;
    case CreateComboBox:
        a->out.rvalue = new QComboBox((bool)a->in.x0, parent, a->in.name);
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus); // 6253667.
        break ;
    case CreateLabel: {
        QString str = *((QString *)a->in.x0);
        a->out.rvalue = new QLabel(str,parent, a->in.name, a->in.flags);
// Bug 6203016 - don't want labels to get focus by mouse or tab
//        a->out.rvalue->setFocusPolicy(QWidget::StrongFocus);
    }
        break ;
    case CreateListBox:
        a->out.rvalue = new QListBox(parent, a->in.name, a->in.flags);
        ((QListBox*)(a->out.rvalue))->setVariableHeight(FALSE);
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus); // 6253667.
        break ;
    case CreateScrollView:
        a->out.rvalue = new QScrollView(parent, a->in.name, a->in.flags);
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus); // 6253667.
        break ;
    case CreateScrollBar:
        a->out.rvalue = new QScrollBar((Qt::Orientation)((int)a->in.x0), 
                                       parent, 
                                       a->in.name);
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus); // 6253667.
        break ;
    case CreatePopupMenu:
        a->out.rvalue = new QPopupMenu(parent, a->in.name);
        break ;
    case CreateTextEdit:
#if (QT_VERSION >= 0x030000)
        a->out.rvalue = new QxTextEdit(parent, a->in.name);
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus); // 6253667.
#else
        printf("WARNING : Qt 2.x Implementation should use QMultiLineEdit\n");
        a->out.rvalue = NULL;
#endif /* QT_VERSION */
        break ;
    case CreateMultiLineEdit:
#if (QT_VERSION < 0x030000)
        a->out.rvalue = new QxMultiLineEdit(parent, a->in.name, a->in.flags);
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus); // 6253667.
#else
        printf("WARNING : Qt 3.x Implementation should use QTextEdit\n");
        a->out.rvalue = NULL;
#endif /* QT_VERSION */
        break ;
    case CreateLineEdit:
        a->out.rvalue = new QxLineEdit(parent, a->in.name);
        a->out.rvalue->setFocusPolicy(QWidget::ClickFocus); // 6253667.
        break ;
    case CreateFileDialog:
          a->out.rvalue = new QxFileDialog(parent, 
                                           a->in.name, 
                                           (bool)a->in.x0,
                                           a->in.flags);
        break ;
    default :
        break;
    }
}
